#include "spindle/thread_pool.h"

#include "spindle/latch.h"

#include "benchmark/benchmark.h"

namespace {
using clock = std::chrono::high_resolution_clock;
}

// Look for primes in the range [`search_range_min`, `search_range_max`]
constexpr int search_range_min = 1;
constexpr int search_range_max = 8'000'000;
constexpr int range_size = search_range_max - search_range_min + 1;

// Size of the per-task sub-range that is searched for primes.
constexpr int chunk_sz = 1'000'000;
constexpr uint32_t num_chunks = (range_size - 1 + chunk_sz) / chunk_sz;

class PrimeSieve : public benchmark::Fixture {
  public:
    void SetUp(const benchmark::State& state) override {
        uint32_t pool_size = state.range(0);
        thread_pool = std::make_unique<spindle::ThreadPool>(pool_size);
    }

    void TearDown(const benchmark::State& state) override {
        thread_pool->tear_down();
    }

  protected:
    static bool is_prime(int i) {
        for (int j = 2; j * j <= i; j++) {
            if (i % j == 0) {
                return false;
            }
        }
        return true;
    }

    static void find_primes(int range_min, std::vector<short>& primes) {
        for (int num = range_min; num < range_min + chunk_sz; ++num) {
            primes[num] = is_prime(num) ? 1 : 0;
        }
    }

    std::unique_ptr<spindle::ThreadPool> thread_pool;
};

BENCHMARK_DEFINE_F(PrimeSieve, FindInRange)(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<short> primes(search_range_max);
        spindle::Latch latch{num_chunks};
        int i = 0;
        for (int range_min = search_range_min; range_min < search_range_max;
             range_min += chunk_sz) {
            i++;
            thread_pool->execute([range_min, &primes, &latch] {
                find_primes(range_min, primes);
                latch.decrement();
            });
        }
        latch.wait();
    }
}

BENCHMARK_REGISTER_F(PrimeSieve, FindInRange)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(10)
    ->RangeMultiplier(2)
    ->Range(1, 8);
