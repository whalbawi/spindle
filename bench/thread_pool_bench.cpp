#include "thread_pool.h"

#include "latch.h"

#include "benchmark/benchmark.h"

class ThreadPoolFixture : public benchmark::Fixture {
  public:
    void SetUp(const benchmark::State& state) override {
        if (state.thread_index() != 0) return;
        uint32_t pool_size = state.range(0);
        thread_pool = std::make_unique<spindle::ThreadPool>(pool_size);
    }

    void TearDown(const benchmark::State& state) override {
        if (state.thread_index() != 0) return;
        thread_pool->tear_down();
    }

  protected:
    static uint32_t work(uint32_t seed) {
        uint32_t x = seed;
        for (int i = 0; i < 16 * 1024; ++i) {
            x = (x << 16) | x;
            x |= 0xBADDECAF;
            x = (x >> 4) & seed;
        }
        return x;
    }

    std::unique_ptr<spindle::ThreadPool> thread_pool;
};

BENCHMARK_DEFINE_F(ThreadPoolFixture, ProcessingTest)(benchmark::State& state) {
    uint32_t x = 0;
    for (auto _ : state) {
        uint32_t num_tasks = state.range(1);
        spindle::Latch latch{num_tasks};
        for (int i = 0; i < num_tasks; ++i) {
            thread_pool->execute([x, &latch] {
                benchmark::DoNotOptimize(work(123 * x + 19));
                latch.decrement();
            });
        }
        latch.wait();
    }
}

BENCHMARK_REGISTER_F(ThreadPoolFixture, ProcessingTest)
    ->ThreadRange(1, 16)
    ->Ranges({
        {1, 32},            // pool size
        {1 << 10, 32 << 10} // number of tasks
    });
