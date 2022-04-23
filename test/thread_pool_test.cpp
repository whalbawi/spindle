#include "thread_pool.h"

#include <thread>
#include <vector>

#include "latch.h"

#include "gtest/gtest.h"

class ThreadPoolTest : public testing::Test {
  protected:
    spindle::ThreadPool thread_pool{};
};

TEST_F(ThreadPoolTest, ZeroThreads) {
    EXPECT_THROW(spindle::ThreadPool{0}, std::runtime_error);
}

TEST_F(ThreadPoolTest, OneTask) {
    int x = 0;

    thread_pool.execute([&] {
        x = 1;
    });

    thread_pool.drain();
    ASSERT_EQ(x, 1);
}

TEST_F(ThreadPoolTest, SingleThreadManyTasks) {
    uint32_t task_count = 1024;
    std::vector<uint32_t> x(task_count);

    for (int i = 0; i < task_count; ++i) {
        thread_pool.execute([&, i] {
            x[i] = i;
        });
    }

    thread_pool.drain();

    for (int i = 0; i < task_count; ++i) {
        ASSERT_EQ(x[i], i);
    }
}

TEST_F(ThreadPoolTest, MultipleThreadsManyTasks) {
    uint32_t thread_count = 16;
    uint32_t tasks_per_thread = 2048;
    uint32_t task_count = tasks_per_thread * thread_count;
    std::vector<std::thread> threads(thread_count);
    spindle::Latch latch{thread_count};
    std::vector<uint32_t> x(task_count);

    auto task = [&](uint32_t pos) {
        x[pos] = pos;
    };

    auto thread_work = [&, tasks_per_thread](int off) {
        for (int i = 0; i < tasks_per_thread; ++i) {
            thread_pool.execute([&, off, i] {
                uint32_t pos = tasks_per_thread * off + i;
                task(pos);
            });
        }
    };

    for (int i = 0; i < thread_count; ++i) {
        threads[i] = std::thread([&, i] {
            thread_work(i);
            latch.decrement();
        });
    }

    latch.wait();
    thread_pool.drain();

    for (int i = 0; i < task_count; ++i) {
        ASSERT_EQ(x[i], i);
    }

    for (auto&& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

TEST_F(ThreadPoolTest, MultipleThreadsManyTasksRecursiveSchedule) {
    uint32_t thread_count = 16;
    uint32_t tasks_per_thread = 2048;
    uint32_t task_count = tasks_per_thread * thread_count;
    std::vector<std::thread> threads(thread_count);
    spindle::Latch latch{task_count};
    std::vector<uint32_t> x(task_count);
    std::vector<uint32_t> y(task_count);

    auto inner_task = [&](uint32_t pos) {
        thread_pool.execute([&, pos] {
            y[pos] = 2 * pos;
        });
        latch.decrement();
    };
    auto task = [&](uint32_t pos) {
        x[pos] = pos;
        inner_task(pos);
    };

    auto thread_work = [&, tasks_per_thread](int off) {
        for (int i = 0; i < tasks_per_thread; ++i) {
            thread_pool.execute([&, off, i] {
                uint32_t pos = tasks_per_thread * off + i;
                task(pos);
            });
        }
    };

    for (int i = 0; i < thread_count; ++i) {
        threads[i] = std::thread([&, i] { thread_work(i); });
    }

    latch.wait();
    thread_pool.drain();

    for (int i = 0; i < task_count; ++i) {
        ASSERT_EQ(x[i], i);
        ASSERT_EQ(y[i], 2 * i);
    }

    for (auto&& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}
