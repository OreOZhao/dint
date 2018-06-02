#pragma once

#include <boost/progress.hpp>
#include <boost/filesystem.hpp>

#include <unordered_map>
#include <queue>

#include "dint_configuration.hpp"
#include "statistics_collectors.hpp"
#include "binary_blocks_collection.hpp"
#include "dictionary.hpp"
#include "heap.hpp"
#include "hash_utils.hpp"
#include "util.hpp"
#include "model_build_utils.hpp"

namespace ds2i {

    struct freq_sorter {
        bool operator()(block_type const& l, block_type const& r) {
            return l.freq > r.freq;
        }
    };

    struct length_freq_sorter {
        bool operator()(block_type const& l, block_type const& r) {
            if (l.data.size() == r.data.size()) {
                return l.freq > r.freq;
            }
            return l.data.size() > r.data.size();
        }
    };

    struct freq_length_sorter {
        bool operator()(block_type const& l, block_type const& r) {
            if (l.freq == r.freq) {
                return l.data.size() > r.data.size();
            }
            return l.freq > r.freq;
        }
    };

    // struct no_filter {
    //     bool operator()(block_type const& /*block*/, uint64_t /*total_integers*/) {
    //         return true;
    //     }
    // };

    struct no_filter {



        bool operator()(block_type const& block, uint64_t total_integers) const {
            return compute_saving(block.data.size(),
                                  block.freq,
                                  total_integers) > constants::eps / 1000;
        }
    };

    struct cost_filter
    {
        cost_filter(double threshold = constants::eps)
            : m_threshold(threshold)
        {}

        bool operator()(block_type const& block, uint64_t total_integers) const {
            return compute_saving(block.data.size(),
                                  block.freq,
                                  total_integers) > m_threshold;
        }

    private:
        double m_threshold;
    };


    template<typename Dictionary, typename Statistics>
    struct decreasing_static_frequencies
    {
        typedef Dictionary dictionary_type;
        typedef Statistics statistics_type;

        static std::string type() {
            return "DSF-" + std::to_string(dictionary_type::num_entries) +
                      "-" + std::to_string(dictionary_type::max_entry_size);
        }

        static auto sorter() {
            freq_sorter sorter;
            return sorter;
        }

        static auto filter() {
            cost_filter filter;
            return filter;
        }

        static void build(typename dictionary_type::builder& builder,
                          statistics_type& stats)
        {
            logger() << "building " << type() << " dictionary for " << stats.total_integers << std::endl;
            builder.init(stats.total_integers);

            logger() << "(1) finding the most covering blocks";
            freq_sorter sorter;
            std::priority_queue<block_type,
                                std::vector<block_type>,
                                freq_sorter> pq(sorter);
            {
                boost::progress_display progress(stats.blocks.size());
                for (auto const& block : stats.blocks) {
                    ++progress;
                    if (pq.size() < dictionary_type::num_entries) {
                        pq.push(block);
                    } else {
                        if (sorter(block, pq.top())) {
                            pq.pop();
                            pq.push(block);
                        }
                    }
                }
            }

            logger() << "(2) adding entries to the dictionary";
            std::vector<block_type> blocks;
            while (!pq.empty()) {
                auto& block = pq.top();
                blocks.push_back(std::move(block));
                pq.pop();
            }

            {
                freq_length_sorter sorter;
                std::sort(blocks.begin(), blocks.end(), sorter);
            }

            for (auto const& block: blocks) {
                builder.append(block.data.data(),
                               block.data.size(),
                               block.freq);
            }
            logger() << "DONE" << std::endl;
        }
    };

    template<typename Dictionary, typename Statistics>
    struct prefix_discounted_frequencies
    {
        typedef Dictionary dictionary_type;
        typedef Statistics statistics_type;
        typedef std::unordered_map<uint64_t, uint64_t> hash_map;

        static std::string type() {
            return "PDF-" + std::to_string(dictionary_type::num_entries) +
                      "-" + std::to_string(dictionary_type::max_entry_size);
        }

        static auto sorter() {
            freq_sorter sorter;
            return sorter;
        }

        static auto filter() {
            cost_filter filter(constants::eps / 1000);
            return filter;
        }

        static void build(typename dictionary_type::builder& builder,
                          statistics_type& stats)
        {
            logger() << "building " << type() << " dictionary for " << stats.total_integers << std::endl;
            builder.init(stats.total_integers);

            logger() << "(1) preparing initial estimates" << std::endl;
            std::vector<int64_t> freedom(stats.blocks.size());
            std::vector<uint8_t> dictionary(stats.blocks.size());
            using pqdata_t = std::pair<int64_t, size_t>;
            auto cmp = [](pqdata_t const& l, pqdata_t const& r) {
                return l.first < r.first;
            };
            std::priority_queue<pqdata_t, std::vector<pqdata_t>, decltype(cmp) > pq(cmp);
            hash_map hash_id_map;
            hash_id_map.max_load_factor(0.05);
            hash_id_map.reserve(stats.blocks.size());
            for (size_t i = 0; i < stats.blocks.size(); ++i) {
                auto const& block = stats.blocks[i];
                if (block.data.size() < dictionary_type::max_entry_size) {
                    hash_id_map[block.hash()] = i;
                }
                freedom[i] = block.freq;
                pq.emplace(freedom[i], i);
            }

            logger() << pq.size() << " blocks in the priority_queue" << std::endl;

            logger() << "(2) finding the most covering blocks" << std::endl;
            size_t needed = dictionary_type::num_entries;
            std::vector<int64_t> prefix_ids(dictionary_type::max_entry_size);
            while (needed != 0) {

                if (pq.empty()) break;

                // (a) get top item
                auto item = pq.top(); pq.pop();
                auto cur_max_id = item.second;

                if (freedom[cur_max_id] != item.first or dictionary[cur_max_id] == 1) {
                    // is the item 'dirty?'
                    continue;
                }

                // (b) add to dict and adjust freedom of top item
                auto adjust = freedom[cur_max_id];
                dictionary[cur_max_id] = 1;
                auto& block = stats.blocks[cur_max_id];
                // logger() << "\tADD TO DICT with freedom = " << adjust << " - "
                //     << block_stats.block_string(cur_max_id);

                // (c) add freedom of prefixes
                auto num_prefixes = compute_prefix_ids(hash_id_map, block, prefix_ids);
                for (size_t p = num_prefixes; p != 0; --p) {
                    auto p_id = prefix_ids[p-1];
                    adjust = adjust * 2;
                    auto padjust = freedom[p_id];
                    // logger() << "\t\tadjust freedom " << freedom[p_id] << " -> "
                    //     << freedom[p_id] - adjust << " - "
                    //     << block_stats.block_string(p_id);
                    freedom[p_id] = freedom[p_id] - adjust;
                    if (dictionary[p_id] == 1) {
                        // logger() << "\t\tremove from dict " << p_id;
                        dictionary[p_id] = 0;
                        adjust = adjust - padjust;
                        needed = needed + 1;
                    }
                    pq.emplace(freedom[p_id], p_id);
                }

                --needed;
            }

            logger() << "(3) adding entries to the dictionary" << std::endl;

            std::vector<block_type> blocks;
            for (size_t i = 0; i < dictionary.size(); ++i) {
                if (dictionary[i] == 1) {
                    auto& old_block = stats.blocks[i];
                    block_type block;
                    block.freq = freedom[i];
                    block.data.swap(old_block.data);
                    blocks.push_back(std::move(block));
                }
            }

            {
                freq_length_sorter sorter;
                std::sort(blocks.begin(), blocks.end(), sorter);
            }

            for (auto const& block: blocks) {
                builder.append(block.data.data(),
                               block.data.size(),
                               block.freq);
            }
        }

    private:
        static size_t compute_prefix_ids(hash_map const& hash_id_map,
                                         block_type const& block,
                                         std::vector<int64_t>& prefix_ids) {
            size_t num_prefixes = 0;
            for (size_t size_u32 = 1; size_u32 < block.data.size(); size_u32 *= 2) {
                auto hash = hash_bytes64(block.data.data(), size_u32);
                auto it = hash_id_map.find(hash);
                if (it != hash_id_map.end()) {
                    prefix_ids[num_prefixes++] = it->second;
                }
            }
            return num_prefixes;
        }
    };

    template<typename Dictionary, typename Statistics>
    struct longest_to_shortest_sweep
    {
        typedef Dictionary dictionary_type;
        typedef Statistics statistics_type;

        static std::string type() {
            return "LSS-" + std::to_string(dictionary_type::num_entries) +
                      "-" + std::to_string(dictionary_type::max_entry_size);
        }

        static auto sorter() {
            length_freq_sorter sorter;
            return sorter;
        }

        static auto filter() {
            cost_filter filter;
            return filter;
        }

        static void build(typename dictionary_type::builder& builder,
                          statistics_type& stats)
        {
            logger() << "building " << type() << " dictionary for " << stats.total_integers << std::endl;
            builder.init(stats.total_integers);

            // <hash of block, unique_id>
            std::unordered_map<uint64_t,
                               uint32_t // assume less than 2^32 candidates
                               > map;

            std::vector<uint64_t> id_lowerbounds(constants::max_fractal_steps + 1, 0);
            uint32_t id = 0;
            uint32_t curr_block_size = dictionary_type::max_entry_size;
            uint32_t k = 0;

            std::vector<uint32_t> candidates_ids;
            candidates_ids.reserve(stats.blocks.size());
            cost_filter cf;
            for (auto const& block: stats.blocks)
            {
                if (cf(block, stats.total_integers)) {
                    if (block.data.size() == curr_block_size / 2) {
                        id_lowerbounds[++k] = id;
                        curr_block_size /= 2;
                    }
                    map[block.hash()] = id;
                }
                ++id;
            }

            assert(id == stats.blocks.size());
            id_lowerbounds[constants::max_fractal_steps] = id;

            curr_block_size = dictionary_type::max_entry_size;
            k = 0;
            for (uint32_t i = 0; i < candidates_ids.size(); ++i)
            {
                if (builder.full()) break;

                auto const& block = stats.blocks[candidates_ids[i]];
                size_t size = block.data.size();
                builder.append(block.data.data(), size, block.freq);

                // decrease frequencies of smaller blocks (if any)
                for (uint32_t block_size = size / 2; block_size != 0; block_size /= 2) {
                    for (uint32_t begin = 0; begin < size; begin += block_size) {
                        uint8_t const* b = reinterpret_cast<uint8_t const*>(&(block.data[begin]));
                        uint8_t const* e = b + std::min<uint64_t>(block_size, size - begin) * sizeof(uint32_t);
                        assert(std::min<uint64_t>(block_size, size - begin) == block_size);
                        uint64_t hash = hash_bytes64(byte_range(b, e));
                        auto it = map.find(hash);
                        if (it != map.end()) {
                            uint32_t id = map[hash];
                            if (stats.blocks[id].freq >= block.freq) {
                                stats.blocks[id].freq -= block.freq;
                            }
                        }
                    }
                }

                if (size == curr_block_size / 2) {
                    logger() << "covering " << builder.coverage() << "% of integers "
                             << "with entries of size " << size << std::endl;
                    curr_block_size /= 2;
                    ++k;
                    // sort sub-blocks after decreasing of frequencies
                    length_freq_sorter sorter;
                    std::sort(
                        stats.blocks.begin() + id_lowerbounds[k],
                        stats.blocks.begin() + id_lowerbounds[k + 1],
                        sorter
                    );
                }
            }
        }
    };
}
