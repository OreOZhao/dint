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

template <typename Encoder, typename Dictionary>
void encode_dint(std::string const& type, char const* coll_file,
                 char const* encoded_file, char const* dict_filename) {}

template <typename Decoder, typename Dictionary>
void decode_dint(std::string const& type, char const* encoded_data_filename,
                 char const* dictionary_filename) {}

void single_rect_dint() {
    // single_rect_dint
    std::string s_r_type = "single_rect_dint";
    logger() << "Start DINT Type: " << s_r_type << std::endl;
    std::string input_basename = "../test/test_data/test_collection";
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

    const char* freq_coll_file = "../test/test_data/test_collection.freqs";
    const char* freq_dict_file =
        "dict.test_collection.freqs.single_rect.DSF-65536-16";
    const char* encoded_file = "encode.freqs.single_rect.bin";
    std::string block_stats_filename =
        "test_collection.freqs.block_statistics-16-adjusted";
    data_type dt = data_type::freqs;
}
int main(int argc, const char** argv) {
    single_rect_dint();
}
