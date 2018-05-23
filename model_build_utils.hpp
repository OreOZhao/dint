#pragma

namespace ds2i {
    namespace util {

        double cost(uint32_t block_size, uint32_t block_frequency) {
            return block_frequency * (48.0 * block_size - 16.0);
        }

        static double bpi(uint32_t block_size, uint32_t block_frequency, uint64_t total_integers) {
            return cost(block_size, block_frequency) / total_integers;
        };

    }
}