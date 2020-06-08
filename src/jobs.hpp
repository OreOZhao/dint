#pragma once

#include "semiasync_queue.hpp"

namespace ds2i {

// for generic block-codec
template <typename Iterator, typename Encoder>
struct enc_sequence_adder : semiasync_queue::job {
    enc_sequence_adder(Iterator begin, uint64_t n,
                       boost::progress_display& progress,
                       std::vector<uint8_t>& output, bool docs,
                       uint64_t& num_processed_lists, uint64_t& num_total_ints)
        : begin(begin)
        , n(n)
        , universe(0)
        , progress(progress)
        , output(output)
        , docs(docs)
        , num_processed_lists(num_processed_lists)
        , num_total_ints(num_total_ints) {}

    virtual void prepare() {
        std::vector<uint32_t> buf;
        buf.reserve(n);
        uint32_t prev = docs ? -1 : 0;
        for (uint64_t i = 0; i != n; ++i, ++begin) {
            buf.push_back(*begin - prev - 1);
            if (docs)
                prev = *begin;
            universe += buf.back();
        }
        assert(buf.size() == n);

        Encoder::encode(buf.data(), universe, buf.size(), tmp);
    }

    virtual void commit() {
        header::write(n, universe, output);
        output.insert(output.end(), tmp.begin(), tmp.end());
        progress += n + 1;
        ++num_processed_lists;
        num_total_ints += n;
    }

    Iterator begin;
    uint64_t n;
    uint32_t universe;
    boost::progress_display& progress;
    std::vector<uint8_t> tmp;
    std::vector<uint8_t>& output;
    bool docs;
    uint64_t& num_processed_lists;
    uint64_t& num_total_ints;
};

// for DINT
template <typename Iterator, typename Encoder, typename Builder>
struct dint_sequence_adder : semiasync_queue::job {
    dint_sequence_adder(Iterator begin, uint64_t n, Builder& builder,
                        boost::progress_display& progress,
                        std::vector<uint8_t>& output, bool docs,
                        uint64_t& num_processed_lists, uint64_t& num_total_ints)
        : begin(begin)
        , n(n)
        , universe(0)
        , builder(builder)
        , progress(progress)
        , output(output)
        , docs(docs)
        , num_processed_lists(num_processed_lists)
        , num_total_ints(num_total_ints) {}

    virtual void prepare() {
        std::vector<uint32_t> buf;
        buf.reserve(n);
        uint32_t prev = docs ? -1 : 0;
        for (uint64_t i = 0; i != n; ++i, ++begin) {
            buf.push_back(*begin - prev - 1);
            if (docs)
                prev = *begin;
            universe += buf.back();
        }
        assert(buf.size() == n);

        Encoder::encode(builder, buf.data(), universe, buf.size(), tmp);
    }

    virtual void commit() {
        header::write(n, universe, output);
        output.insert(output.end(), tmp.begin(), tmp.end());
        progress += n + 1;
        ++num_processed_lists;
        num_total_ints += n;
    }

    Iterator begin;
    uint64_t n;
    uint32_t universe;
    Builder& builder;
    boost::progress_display& progress;
    std::vector<uint8_t> tmp;
    std::vector<uint8_t>& output;
    bool docs;
    uint64_t& num_processed_lists;
    uint64_t& num_total_ints;
};


}  // namespace ds2i
