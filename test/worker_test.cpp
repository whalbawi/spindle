#include "worker.h"

#include "gtest/gtest.h"

class WorkerSingleThreadedTest : public ::testing::Test {
  protected:
    spindle::Worker worker;
};

TEST_F(WorkerSingleThreadedTest, EnqueueAndTerminate) {
    int x = 0;
    auto task = [&] {
        x = 1;
        worker.terminate();
    };

    ASSERT_EQ(worker.enqueue(task), true);
    ASSERT_EQ(x, 0); // Enqueuing does not run the task

    worker.run();
    ASSERT_EQ(x, 1);

    ASSERT_EQ(worker.enqueue(task), false);
}

TEST_F(WorkerSingleThreadedTest, EnqueueMultipleTasks) {
    int x = 0;
    int y = 0;
    int z = 0;
    auto task1 = [&] { x = 1; };
    auto task2 = [&] { y = 2; };
    auto task3 = [&] {
        z = 3;
        worker.terminate();
    };

    ASSERT_EQ(worker.enqueue(task1), true);
    ASSERT_EQ(worker.enqueue(task2), true);
    ASSERT_EQ(worker.enqueue(task3), true);
    // Enqueuing does not run the task
    ASSERT_EQ(x, 0);
    ASSERT_EQ(y, 0);
    ASSERT_EQ(z, 0);

    worker.run();
    ASSERT_EQ(x, 1);
    ASSERT_EQ(y, 2);
    ASSERT_EQ(z, 3);

    ASSERT_EQ(worker.enqueue(task1), false);
}

TEST_F(WorkerSingleThreadedTest, EnqueueFromEnqueuedTask) {
    int x = 0;
    auto inner_task = [&] {
        x = 1;
        worker.terminate();
    };
    auto outer_task = [&] { worker.enqueue(inner_task); };

    ASSERT_EQ(worker.enqueue(outer_task), true);
    // Enqueuing does not run the task
    ASSERT_EQ(x, 0);

    worker.run();
    ASSERT_EQ(x, 1);

    ASSERT_EQ(worker.enqueue(outer_task), false);
}
