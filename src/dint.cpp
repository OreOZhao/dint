#include <fstream>
#include <iostream>
#include <algorithm>
#include <thread>
#include <numeric>

#include <succinct/mapper.hpp>

#include "configuration.hpp"
#include "index_types.hpp"
#include "util.hpp"
#include "verify_collection.hpp"
#include "index_build_utils.hpp"
#include "dict_freq_index.hpp"
#include "hash_utils.hpp"
#include "binary_collection.hpp"
#include "dint_codecs.hpp"

using namespace ds2i;

struct dint_stats {
    double secs;
    double docs_bpi;
    double freqs_bpi;
    dint_stats(double s, double d, double f)
        : secs(s), docs_bpi(d), freqs_bpi(f) {}
};

template <typename Collection>
dint_stats get_stats(double secs, Collection& coll, std::string const& type,
                     uint64_t postings) {
    uint64_t docs_size = 0, freqs_size = 0;
    size_t total_index_size = get_size_stats(coll, docs_size, freqs_size);

    double bits_per_doc = docs_size * 8.0 / postings;
    double bits_per_freq = freqs_size * 8.0 / postings;
    return dint_stats(secs, bits_per_doc, bits_per_freq);
}

template <typename Collection>
void dump_index_specific_stats(Collection const&, std::string const&) {}

void dump_index_specific_stats(uniform_index const& coll,
                               /* Uniform freq index collection */
                               std::string const& type) {
    stats_line()("type", type)("log_partition_size",
                               int(coll.params().log_partition_size));
    // Output type, log_partition_size
}
//
void dump_index_specific_stats(opt_index const& coll,
                               /* Optimal freq index collection */
                               std::string const& type) {
    auto const& conf = configuration::get();

    double long_postings = 0;
    double docs_partitions = 0;
    double freqs_partitions = 0;

    for (size_t s = 0; s < coll.size(); ++s) {
        // Traverse every sequence in collection
        // to calculate long_postings, docs and freqs' partitions
        auto const& list = coll[s];
        if (list.size() > constants::min_size) {
            long_postings += list.size();
            docs_partitions += list.docs_enum().num_partitions();
            freqs_partitions += list.freqs_enum().base().num_partitions();
        }
    }

    stats_line()("type", type)("eps1", conf.eps1)("eps2", conf.eps2)(
        "fix_cost", conf.fix_cost)("docs_avg_part",
                                   long_postings / docs_partitions)(
        "freqs_avg_part", long_postings / freqs_partitions);
    // Output type, conf's info. Calculate docs & freqs average parg
}

dint_stats single_rect_dint(std::string input_basename) {
    // single_rect_dint
    std::string s_r_type = "single_rect_dint";
    logger() << std::endl
             << std::endl
             << "Start DINT Type: " << s_r_type << std::endl;
    // std::string input_basename = "../test/test_data/test_collection";
    const char* output_filename = "single_rect_dint.bin";
    bool check = false;
    ds2i::global_parameters params;
    params.log_partition_size = configuration::get().log_partition_size;
    binary_freq_collection input(input_basename.c_str());
    size_t num_docs = input.num_docs();

    double tick = get_time_usecs();
    double user_tick = get_user_time_usecs();
    // create single rect dint freq index collection
    single_rect_dint_index::builder builder(num_docs, params);
    builder.build_model(input_basename);
    logger() << "Processing " << input.num_docs() << " documents..."
             << std::endl;
    progress_logger plog("Encoded");

    boost::progress_display progress(input.num_postings());

    for (auto const& plist : input) {
        uint64_t n = plist.docs.size();
        if (n > constants::min_size) {
            uint64_t freqs_sum = std::accumulate(
                plist.freqs.begin(), plist.freqs.end(), uint64_t(0));
            builder.add_posting_list(n, plist.docs.begin(), plist.freqs.begin(),
                                     freqs_sum);
            plog.done_sequence(n);
            progress += n + plist.freqs.size() + 2;
        }
    }
    single_rect_dint_index coll;
    builder.build(coll);
    double elapsed_secs = (get_time_usecs() - tick) / 1000000;
    double user_elapsed_secs = (get_user_time_usecs() - user_tick) / 1000000;
    logger() << s_r_type << " collection built in " << elapsed_secs
             << " seconds" << std::endl;

    stats_line()("type", s_r_type)("worker_threads",
                                   configuration::get().worker_threads)(
        "construction_time", elapsed_secs)("construction_user_time",
                                           user_elapsed_secs);

    dump_stats(coll, s_r_type, plog.postings);
    dump_index_specific_stats(coll, s_r_type);

    if (output_filename) {
        succinct::mapper::freeze(coll, output_filename);
    }
    return get_stats(elapsed_secs, coll, s_r_type, plog.postings);
}

dint_stats single_packed_dint(std::string input_basename) {
    // single_packed_dint
    std::string s_p_type = "single_packed_dint";
    logger() << std::endl
             << std::endl
             << "Start DINT Type: " << s_p_type << std::endl;
    // std::string input_basename = "../test/test_data/test_collection";
    const char* output_filename = "single_packed_dint.bin";
    bool check = false;
    ds2i::global_parameters params;
    params.log_partition_size = configuration::get().log_partition_size;
    binary_freq_collection input(input_basename.c_str());
    size_t num_docs = input.num_docs();

    double tick = get_time_usecs();
    double user_tick = get_user_time_usecs();
    // create single rect dint freq index collection
    single_packed_dint_index::builder builder(num_docs, params);
    builder.build_model(input_basename);
    logger() << "Processing " << input.num_docs() << " documents..."
             << std::endl;
    progress_logger plog("Encoded");

    boost::progress_display progress(input.num_postings());

    for (auto const& plist : input) {
        uint64_t n = plist.docs.size();
        if (n > constants::min_size) {
            uint64_t freqs_sum = std::accumulate(
                plist.freqs.begin(), plist.freqs.end(), uint64_t(0));
            builder.add_posting_list(n, plist.docs.begin(), plist.freqs.begin(),
                                     freqs_sum);
            plog.done_sequence(n);
            progress += n + plist.freqs.size() + 2;
        }
    }
    single_packed_dint_index coll;
    builder.build(coll);
    double elapsed_secs = (get_time_usecs() - tick) / 1000000;
    double user_elapsed_secs = (get_user_time_usecs() - user_tick) / 1000000;
    logger() << s_p_type << " collection built in " << elapsed_secs
             << " seconds" << std::endl;

    stats_line()("type", s_p_type)("worker_threads",
                                   configuration::get().worker_threads)(
        "construction_time", elapsed_secs)("construction_user_time",
                                           user_elapsed_secs);

    dump_stats(coll, s_p_type, plog.postings);
    dump_index_specific_stats(coll, s_p_type);

    if (output_filename) {
        succinct::mapper::freeze(coll, output_filename);
    }
    return get_stats(elapsed_secs, coll, s_p_type, plog.postings);
}

dint_stats multi_packed_dint(std::string input_basename) {
    // multi_packed_dint
    std::string m_p_type = "multi_packed_dint";
    logger() << std::endl
             << std::endl
             << "Start DINT Type: " << m_p_type << std::endl;
    // std::string input_basename = "../test/test_data/test_collection";
    const char* output_filename = "multi_packed_dint.bin";
    bool check = false;
    ds2i::global_parameters params;
    params.log_partition_size = configuration::get().log_partition_size;
    binary_freq_collection input(input_basename.c_str());
    size_t num_docs = input.num_docs();

    double tick = get_time_usecs();
    double user_tick = get_user_time_usecs();
    // create single rect dint freq index collection
    multi_packed_dint_index::builder builder(num_docs, params);
    builder.build_model(input_basename);
    logger() << "Processing " << input.num_docs() << " documents..."
             << std::endl;
    progress_logger plog("Encoded");

    boost::progress_display progress(input.num_postings());

    for (auto const& plist : input) {
        uint64_t n = plist.docs.size();
        if (n > constants::min_size) {
            uint64_t freqs_sum = std::accumulate(
                plist.freqs.begin(), plist.freqs.end(), uint64_t(0));
            builder.add_posting_list(n, plist.docs.begin(), plist.freqs.begin(),
                                     freqs_sum);
            plog.done_sequence(n);
            progress += n + plist.freqs.size() + 2;
        }
    }
    multi_packed_dint_index coll;
    builder.build(coll);
    double elapsed_secs = (get_time_usecs() - tick) / 1000000;
    double user_elapsed_secs = (get_user_time_usecs() - user_tick) / 1000000;
    logger() << m_p_type << " collection built in " << elapsed_secs
             << " seconds" << std::endl;

    stats_line()("type", m_p_type)("worker_threads",
                                   configuration::get().worker_threads)(
        "construction_time", elapsed_secs)("construction_user_time",
                                           user_elapsed_secs);

    dump_stats(coll, m_p_type, plog.postings);
    dump_index_specific_stats(coll, m_p_type);

    if (output_filename) {
        succinct::mapper::freeze(coll, output_filename);
    }
    return get_stats(elapsed_secs, coll, m_p_type, plog.postings);
}

dint_stats single_overlapped_dint(std::string input_basename) {
    std::string s_o_type = "single_overlapped_dint";
    logger() << std::endl
             << std::endl
             << "Start DINT Type: " << s_o_type << std::endl;
    // std::string input_basename = "../test/test_data/test_collection";
    const char* output_filename = "single_overlapped_dint.bin";
    bool check = false;
    ds2i::global_parameters params;
    params.log_partition_size = configuration::get().log_partition_size;
    binary_freq_collection input(input_basename.c_str());
    size_t num_docs = input.num_docs();

    double tick = get_time_usecs();
    double user_tick = get_user_time_usecs();
    // create single overlapped dint freq index collection
    single_overlapped_dint_index::builder builder(num_docs, params);
    builder.build_model(input_basename);
    logger() << "Processing " << input.num_docs() << " documents..."
             << std::endl;
    progress_logger plog("Encoded");

    boost::progress_display progress(input.num_postings());

    for (auto const& plist : input) {
        uint64_t n = plist.docs.size();
        if (n > constants::min_size) {
            uint64_t freqs_sum = std::accumulate(
                plist.freqs.begin(), plist.freqs.end(), uint64_t(0));
            builder.add_posting_list(n, plist.docs.begin(), plist.freqs.begin(),
                                     freqs_sum);
            plog.done_sequence(n);
            progress += n + plist.freqs.size() + 2;
        }
    }
    single_overlapped_dint_index coll;
    builder.build(coll);
    double elapsed_secs = (get_time_usecs() - tick) / 1000000;
    double user_elapsed_secs = (get_user_time_usecs() - user_tick) / 1000000;
    logger() << s_o_type << " collection built in " << elapsed_secs
             << " seconds" << std::endl;

    stats_line()("type", s_o_type)("worker_threads",
                                   configuration::get().worker_threads)(
        "construction_time", elapsed_secs)("construction_user_time",
                                           user_elapsed_secs);

    dump_stats(coll, s_o_type, plog.postings);
    dump_index_specific_stats(coll, s_o_type);

    if (output_filename) {
        succinct::mapper::freeze(coll, output_filename);
    }
    return get_stats(elapsed_secs, coll, s_o_type, plog.postings);
}

dint_stats multi_overlapped_dint(std::string input_basename) {
    std::string m_o_type = "multi_overlapped_dint";
    logger() << std::endl
             << std::endl
             << "Start DINT Type: " << m_o_type << std::endl;
    // std::string input_basename = "../test/test_data/test_collection";
    const char* output_filename = "multi_overlapped_dint.bin";
    bool check = false;
    ds2i::global_parameters params;
    params.log_partition_size = configuration::get().log_partition_size;
    binary_freq_collection input(input_basename.c_str());
    size_t num_docs = input.num_docs();

    double tick = get_time_usecs();
    double user_tick = get_user_time_usecs();
    // create single overlapped dint freq index collection
    multi_overlapped_dint_index::builder builder(num_docs, params);
    builder.build_model(input_basename);
    logger() << "Processing " << input.num_docs() << " documents..."
             << std::endl;
    progress_logger plog("Encoded");

    boost::progress_display progress(input.num_postings());

    for (auto const& plist : input) {
        uint64_t n = plist.docs.size();
        if (n > constants::min_size) {
            uint64_t freqs_sum = std::accumulate(
                plist.freqs.begin(), plist.freqs.end(), uint64_t(0));
            builder.add_posting_list(n, plist.docs.begin(), plist.freqs.begin(),
                                     freqs_sum);
            plog.done_sequence(n);
            progress += n + plist.freqs.size() + 2;
        }
    }
    multi_overlapped_dint_index coll;
    builder.build(coll);
    double elapsed_secs = (get_time_usecs() - tick) / 1000000;
    double user_elapsed_secs = (get_user_time_usecs() - user_tick) / 1000000;
    logger() << m_o_type << " collection built in " << elapsed_secs
             << " seconds" << std::endl;

    stats_line()("type", m_o_type)("worker_threads",
                                   configuration::get().worker_threads)(
        "construction_time", elapsed_secs)("construction_user_time",
                                           user_elapsed_secs);

    dump_stats(coll, m_o_type, plog.postings);
    dump_index_specific_stats(coll, m_o_type);

    if (output_filename) {
        succinct::mapper::freeze(coll, output_filename);
    }
    return get_stats(elapsed_secs, coll, m_o_type, plog.postings);
}

int main(int argc, const char** argv) {
    std::string basename = "../new/Gov2";
    // std::string basename = "/mnt/OPTANE/zhaoyu/Gov2/Gov2";
    dint_stats s_r_stats = single_rect_dint(basename);
    dint_stats s_p_stats = single_packed_dint(basename);
    dint_stats m_p_stats = multi_packed_dint(basename);
    // dint_stats s_o_stats = single_overlapped_dint(basename);
    // dint_stats m_o_stats = multi_overlapped_dint(basename);
    /*
    logger() << std::endl << "Dint Stats: " << std::endl;
    std::cout << "index\t\t\t\t"
              << "secs\t\t"
              << "docs[bpi]\t\t"
              << "freqs[bpi]" << std::endl;
    std::cout << "single_rect_dint\t\t" << s_r_stats.secs << "\t\t"
              << s_r_stats.docs_bpi << "\t\t" << s_r_stats.freqs_bpi << "\t\t"
              << std::endl;
    std::cout << "single_packed_dint\t\t" << s_p_stats.secs << "\t\t"
              << s_p_stats.docs_bpi << "\t\t" << s_p_stats.freqs_bpi << "\t\t"
              << std::endl;
    std::cout << "multi_packed_dint\t\t" << m_p_stats.secs << "\t\t"
              << m_p_stats.docs_bpi << "\t\t" << m_p_stats.freqs_bpi << "\t\t"
              << std::endl;
              */
}
