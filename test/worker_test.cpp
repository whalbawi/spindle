#include "worker.h"

#include "gtest/gtest.h"

#ifndef SPINDLE_WORKER_DEFERRED_TASK_TESTS_SKIP
#define SPINDLE_WORKER_DEFERRED_TASK_TESTS_SKIP 0
#endif

class WorkerSingleThreadedTest : public ::testing::Test {
  protected:
    WorkerSingleThreadedTest() : terminator(worker) {}

    bool enqueue(const std::function<void()>& task) {
        terminator.add_task();
        // TODO (whalbawi): Figure out why we need to capture `task` by value rather than reference.
        return worker.enqueue([task, this] {
            task();
            terminator();
        });
    }

    template <class T> bool schedule(const std::function<void()>& task, T delay) {
        terminator.add_task();
        // TODO (whalbawi): Figure out why we need to capture `task` by value rather than reference.
        return worker.schedule(
            [task, this] {
                task();
                terminator();
            },
            delay);
    }

    template <class S> static std::chrono::milliseconds duration_since(S s) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(spindle::clock::now() - s);
    }

    class Terminator {
      public:
        Terminator(spindle::Worker& worker) : worker(worker) {}
        void add_task() {
            num_tasks++;
        }

        void operator()() {
            if (--num_tasks == 0) worker.terminate();
        }

      private:
        spindle::Worker& worker;
        int num_tasks{};
    };

    spindle::Worker worker;
    Terminator terminator;
    static long delay_tol_pct;
};

long WorkerSingleThreadedTest::delay_tol_pct = 10; // Being with 10% and tweak as we improve.

TEST_F(WorkerSingleThreadedTest, EnqueueAndTerminate) {
    int x = 0;
    auto task = [&] { x = 1; };

    ASSERT_EQ(enqueue(task), true);
    ASSERT_EQ(x, 0); // Enqueuing does not run the task

    worker.run();
    ASSERT_EQ(x, 1);

    ASSERT_EQ(enqueue(task), false);
}

TEST_F(WorkerSingleThreadedTest, EnqueueMultipleTasks) {
    int x = 0;
    int y = 0;
    int z = 0;
    auto task1 = [&] { x = 1; };
    auto task2 = [&] { y = 2; };
    auto task3 = [&] { z = 3; };

    ASSERT_EQ(enqueue(task1), true);
    ASSERT_EQ(enqueue(task2), true);
    ASSERT_EQ(enqueue(task3), true);
    // Enqueuing does not run the task
    ASSERT_EQ(x, 0);
    ASSERT_EQ(y, 0);
    ASSERT_EQ(z, 0);

    worker.run();
    ASSERT_EQ(x, 1);
    ASSERT_EQ(y, 2);
    ASSERT_EQ(z, 3);

    ASSERT_EQ(enqueue(task1), false);
}

TEST_F(WorkerSingleThreadedTest, EnqueueFromEnqueuedTask) {
    int x = 0;
    auto inner_task = [&] { x = 1; };
    auto outer_task = [&] { enqueue(inner_task); };

    ASSERT_EQ(enqueue(outer_task), true);
    // Enqueuing does not run the task
    ASSERT_EQ(x, 0);

    worker.run();
    ASSERT_EQ(x, 1);

    ASSERT_EQ(enqueue(outer_task), false);
}

TEST_F(WorkerSingleThreadedTest, DeferredTask) {
#if SPINDLE_WORKER_DEFERRED_TASK_TESTS_SKIP
    GTEST_SKIP();
#endif
    std::chrono::milliseconds delay_ms{100};
    std::chrono::milliseconds delay;

    spindle::clock::time_point start = spindle::clock::now();

    schedule([&] { delay = duration_since(start); }, delay_ms);

    worker.run();

    long tol = delay_ms.count() * delay_tol_pct / 100;
    ASSERT_NEAR(delay.count(), delay_ms.count(), tol);
}

TEST_F(WorkerSingleThreadedTest, ImmediateAndDeferredTask) {
#if SPINDLE_WORKER_DEFERRED_TASK_TESTS_SKIP
    GTEST_SKIP();
#endif
    int x = 0;
    std::chrono::milliseconds delay_ms{100};
    std::chrono::milliseconds delay;

    spindle::clock::time_point start = spindle::clock::now();

    schedule(
        [&] {
            x++;
            delay = duration_since(start);
            ASSERT_EQ(x, 2);

            long tol = delay_ms.count() * delay_tol_pct / 100;
            ASSERT_NEAR(delay.count(), delay_ms.count(), tol);
        },
        delay_ms);

    enqueue([&] {
        x++;
        // Even though we enqueued this task second, it should execute first.
        ASSERT_EQ(x, 1);
    });

    worker.run();
}

TEST_F(WorkerSingleThreadedTest, MultipleDeferredTasks) {
#if SPINDLE_WORKER_DEFERRED_TASK_TESTS_SKIP
    GTEST_SKIP();
#endif

    int x = 0;
    std::chrono::milliseconds delay_ms{100};
    std::chrono::milliseconds delay;

    spindle::clock::time_point start = spindle::clock::now();

    schedule(
        [&] {
            x += 1;
            delay = duration_since(start);
            ASSERT_EQ(x, 2);
            long tol = delay_ms.count() * delay_tol_pct / 100;
            ASSERT_NEAR(delay.count(), delay_ms.count(), tol);
        },
        delay_ms);

    enqueue([&] {
        x += 1;
        // Even though we enqueued this task second, it should execute first.
        ASSERT_EQ(x, 1);
    });

    worker.run();
}

TEST_F(WorkerSingleThreadedTest, MultipleDeferredAndImmediateTasks) {
#if SPINDLE_WORKER_DEFERRED_TASK_TESTS_SKIP
    GTEST_SKIP();
#endif

    int x = 0;
    std::chrono::milliseconds delay_1_ms{200};
    std::chrono::milliseconds delay_2_ms{150};
    std::chrono::milliseconds delay_3_ms{100};

    // ASSERT_FALSE(true);
    spindle::clock::time_point start = spindle::clock::now();
    // Position 1
    enqueue([&] {
        x++;
        ASSERT_EQ(x, 1);
    });
    // Position 6
    schedule(
        [&] {
            std::chrono::milliseconds delay = duration_since(start);
            x++;
            ASSERT_EQ(x, 6);
            long tol = delay_1_ms.count() * delay_tol_pct / 100;
            ASSERT_NEAR(delay.count(), delay_1_ms.count(), tol);
        },
        delay_1_ms);
    // Position 2
    enqueue([&] {
        x++;
        ASSERT_EQ(x, 2);
    });
    // Position 5
    schedule(
        [&] {
            std::chrono::milliseconds delay = duration_since(start);
            x++;
            ASSERT_EQ(x, 5);
            long tol = delay_2_ms.count() * delay_tol_pct / 100;
            ASSERT_NEAR(delay.count(), delay_2_ms.count(), tol);
        },
        delay_2_ms);
    // Position 3
    enqueue([&] {
        x++;
        ASSERT_EQ(x, 3);
    });
    // Position 4
    schedule(
        [&] {
            std::chrono::milliseconds delay = duration_since(start);
            x++;
            ASSERT_EQ(x, 4);
            long tol = delay_3_ms.count() * delay_tol_pct / 100;
            ASSERT_NEAR(delay.count(), delay_3_ms.count(), tol);
        },
        delay_3_ms);
    worker.run();
}

TEST_F(WorkerSingleThreadedTest, RecursiveDeferredTasks) {
#if SPINDLE_WORKER_DEFERRED_TASK_TESTS_SKIP
    GTEST_SKIP();
#endif

    int x = 0;
    std::chrono::milliseconds delay_1_ms{200};
    std::chrono::milliseconds delay_2_ms{150};

    spindle::clock::time_point start = spindle::clock::now();
    enqueue([&] {
        x++;
        ASSERT_EQ(x, 1);
    });
    schedule(
        [&x, &start, delay_1_ms, delay_2_ms, this] {
            std::chrono::milliseconds delay = duration_since(start);
            x++;
            ASSERT_EQ(x, 2);
            long tol = delay_1_ms.count() * delay_tol_pct / 100;
            ASSERT_NEAR(delay.count(), delay_1_ms.count(), tol);
            start = spindle::clock::now();
            schedule(
                [&x, start, delay_2_ms] {
                    std::chrono::milliseconds delay = duration_since(start);
                    x++;
                    ASSERT_EQ(x, 3);
                    long tol = delay_2_ms.count() * delay_tol_pct / 100;
                    ASSERT_NEAR(delay.count(), delay_2_ms.count(), tol);
                },
                delay_2_ms);
        },
        delay_1_ms);
    worker.run();
}
