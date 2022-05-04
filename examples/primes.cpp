#include <chrono>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <vector>

#include "spindle/thread_pool.h"

// Look for primes in the range [`search_range_min`, `search_range_max`]
constexpr int search_range_min = 1;
constexpr int search_range_max = 8'000'000;
constexpr int range_size = search_range_max - search_range_min + 1;

// Size of the per-task sub-range that is searched for primes.
constexpr int chunk_sz = 1'000'000;
constexpr uint32_t num_chunks = (range_size - 1 + chunk_sz) / chunk_sz;

static bool is_prime(int i) {
    for (int j = 2; j * j <= i; j++) {
        if (i % j == 0) {
            return false;
        }
    }
    return true;
}

namespace {
using clock = std::chrono::high_resolution_clock;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " <pool_size>"
                  << "\n";
        return -1;
    }

    uint32_t pool_size = std::stoul(argv[1]);

    std::cout << "Range: [" << search_range_min << ", " << search_range_max << "]\n";
    std::cout << "Number of chunks: " << num_chunks << "\n";
    std::cout << "Thread pool size: " << pool_size << "\n";
    std::cout << "Finding primes...\n";

    spindle::ThreadPool thread_pool{pool_size};
    std::vector<short> primes(search_range_max);
    std::vector<long> durations(num_chunks);

    auto start = clock::now();
    auto fn = [&primes, &durations](int range_min, uint32_t idx) {
        auto start = clock::now();
        for (int num = range_min; num < range_min + chunk_sz; num++) {
            primes[num] = is_prime(num) ? 1 : 0;
        }
        clock::duration duration = clock::now() - start;
        long duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        durations[idx] = duration_ms;
    };

    // Iterate over the range [search_range_min, search_range_max] in steps of chunk_sz.
    int idx = 0;
    for (int range_min = search_range_min; range_min <= search_range_max; range_min += chunk_sz) {
        thread_pool.execute([&fn, range_min, idx] { fn(range_min, idx); });
        idx++;
    }

    thread_pool.drain();
    clock::duration duration = clock::now() - start;

    std::cout << "Duration (ms)\n";
    int i = 0;
    for (auto&& task_duration : durations) {
        std::cout << std::setw(5) << i++ << ": " << std::setw(3) << task_duration << "\n";
    }

    std::cout << "----------\n";
    long duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    std::cout << "Total: " << duration_ms << "\n";

    int num_primes = std::accumulate(primes.begin(), primes.end(), 0);
    std::cout << "Number of primes: " << num_primes << "\n";

    return 0;
}
