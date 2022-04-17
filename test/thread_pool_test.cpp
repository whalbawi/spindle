#include "thread_pool.h"

#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "latch.h"

#include "gtest/gtest.h"

class ThreadPoolTest : public testing::Test {
  protected:
    spindle::ThreadPool thread_pool{};
};

TEST_F(ThreadPoolTest, OneTask) {
    int x = 0;
    spindle::Latch latch{};

    thread_pool.execute([&] {
        x = 1;
        latch.decrement();
    });

    latch.wait();
    ASSERT_EQ(x, 1);
}

TEST_F(ThreadPoolTest, SingleThreadManyTasks) {
    uint32_t task_count = 1024;
    std::vector<uint32_t> x(task_count);
    spindle::Latch latch{task_count};

    for (int i = 0; i < task_count; ++i) {
        thread_pool.execute([&, i] {
            x[i] = i;
            latch.decrement();
        });
    }

    latch.wait();

    for (int i = 0; i < task_count; ++i) {
        ASSERT_EQ(x[i], i);
    }
}

TEST_F(ThreadPoolTest, MultipleThreadsManyTasks) {
    uint32_t thread_count = 16;
    uint32_t tasks_per_thread = 2048;
    uint32_t task_count = tasks_per_thread * thread_count;
    std::vector<std::thread> threads(thread_count);
    std::vector<uint32_t> x(task_count);
    spindle::Latch latch{task_count};

    auto task = [&](uint32_t pos) {
        x[pos] = pos;
        latch.decrement();
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
    std::vector<uint32_t> x(task_count);
    std::vector<uint32_t> y(task_count);
    spindle::Latch latch{2 * task_count};

    auto inner_task = [&](uint32_t pos) {
        thread_pool.execute([&, pos] {
            y[pos] = 2 * pos;
            latch.decrement();
        });
    };
    auto task = [&](uint32_t pos) {
        x[pos] = pos;
        inner_task(pos);
        latch.decrement();
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
