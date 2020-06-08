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

typedef binary_collection::posting_type const* iterator_type;
const uint32_t num_jobs = 1 << 24;

void save_if(char const* output_filename, std::vector<uint8_t> const& output) {
    if (output_filename) {
        logger() << "writing encoded data..." << std::endl;
        std::ofstream output_file(output_filename);
        output_file.write(reinterpret_cast<char const*>(output.data()),
                          output.size() * sizeof(output[0]));
        output_file.close();
        logger() << "DONE" << std::endl;
    }
}

void print_statistics(std::string type, char const* collection_name,
                      std::vector<uint8_t> const& output,
                      uint64_t num_total_ints, uint64_t num_processed_lists) {
    double GiB_space = output.size() * 1.0 / constants::GiB;
    double bpi_space = output.size() * sizeof(output[0]) * 8.0 / num_total_ints;

    logger() << "encoded " << num_processed_lists << " lists" << std::endl;
    logger() << "encoded " << num_total_ints << " integers" << std::endl;
    logger() << GiB_space << " [GiB]" << std::endl;
    logger() << "bits x integer: " << bpi_space << std::endl;

    // stats to std output
    std::cout << "{";
    std::cout << "\"filename\": \"" << collection_name << "\", ";
    std::cout << "\"num_sequences\": \"" << num_processed_lists << "\", ";
    std::cout << "\"num_integers\": \"" << num_total_ints << "\", ";
    std::cout << "\"type\": \"" << type << "\", ";
    std::cout << "\"GiB\": \"" << GiB_space << "\", ";
    std::cout << "\"bpi\": \"" << bpi_space << "\"";
    std::cout << "}" << std::endl;
}

template <typename Encoder, typename Dictionary>
void encode_dint(std::string const& type, char const* coll_file,
                 char const* encoded_file, char const* dict_filename) {
    binary_collection input(coll_file);
    auto it = input.begin();
    uint64_t num_processed_lists = 0;
    uint64_t num_total_ints = 0;

    std::ifstream dict_file(dict_filename);
    typedef typename single_dictionary_rectangular_type::builder Builder;
    Builder dict_builder;
    dict_builder.load(dict_file);
    dict_file.close();
    logger() << "preparing for encoding..." << std::endl;
    dict_builder.prepare_for_encoding();
    dict_builder.print_usage();

    uint64_t total_progress = input.num_postings();
    bool docs = true;
    boost::filesystem::path collection_path(coll_file);
    if (collection_path.extension() == ".freqs") {
        docs = false;
        logger() << "encoding freqs..." << std::endl;
    } else if (collection_path.extension() == ".docs") {
        // skip first singleton sequence, containing num. of docs
        ++it;
        total_progress -= 2;
        logger() << "encoding docs..." << std::endl;
    } else {
        throw std::runtime_error("unsupported file format");
    }

    std::vector<uint8_t> output;
    uint64_t bytes = 5 * constants::GiB;
    output.reserve(bytes);

    std::vector<uint32_t> buf;
    boost::progress_display progress(total_progress);
    semiasync_queue jobs_queue(num_jobs);

    for (; it != input.end(); ++it) {
        auto const& list = *it;
        uint32_t n = list.size();
        std::shared_ptr<dint_sequence_adder<iterator_type, Encoder, Builder>>
            ptr(new dint_sequence_adder<iterator_type, Encoder, Builder>(
                list.begin(), n, dict_builder, progress, output, docs,
                num_processed_lists, num_total_ints));
        jobs_queue.add_job(ptr, n);
    }

    jobs_queue.complete();
    print_statistics(type, coll_file, output, num_total_ints,
                     num_processed_lists);
    save_if(encoded_file, output);
}

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

    // dump_stats(coll, s_r_type, plog.postings);
    // dump_index_specific_stats(coll, s_r_type);

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

    encode_dint<single_opt_dint, single_dictionary_rectangular_type>(
        s_r_type, freq_coll_file, encoded_file, freq_dict_file);
    encode_dint<single_greedy_dint, single_dictionary_rectangular_type>(
        s_r_type, freq_coll_file, encoded_file, freq_dict_file);
}

int main(int argc, const char** argv) {
    single_rect_dint();
}
