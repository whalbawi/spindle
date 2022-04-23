#include "spindle/latch.h"

#include <atomic>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

TEST(Latch, InvalidWeight) {
    EXPECT_THROW(spindle::Latch{0}, std::runtime_error);
}

TEST(Latch, WeightOne) {
    spindle::Latch latch{};
    latch.decrement();
    latch.wait();
}

TEST(Latch, WeightMoreThanOne) {
    uint32_t weight = 3;
    spindle::Latch latch{weight};
    for (int i = 0; i < weight; ++i) {
        latch.decrement();
    }

    latch.wait();
}

TEST(Latch, DecrementMoreThanWeight) {
    uint32_t weight = 16;
    spindle::Latch latch{weight};
    for (int i = 0; i < 2 * weight; ++i) {
        latch.decrement();
    }

    latch.wait();
}

TEST(Latch, ManyThreads) {
    uint32_t weight = 16;
    std::vector<std::thread> threads(weight);
    std::atomic_int v{};
    spindle::Latch latch{weight};

    for (int i = 0; i < weight; ++i) {
        threads.emplace_back([&] {
            v++;
            latch.decrement();
        });
    }

    latch.wait();
    EXPECT_EQ(v, weight);

    for (auto&& thread : threads) {
        if (thread.joinable()) thread.join();
    }
}
