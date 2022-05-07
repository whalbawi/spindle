#include "worker.h"

namespace spindle {

Worker::Worker() : work{Task::cmp}, deadline{clock::time_point::max()} {}

void Worker::run() {
    for (;;) {
        std::unique_lock<std::recursive_mutex> lk{m};

        // Wait until:
        // - Terminated
        // - Drained
        // - There is work due
        // - `cv` times out

        // Note: `cv` will never time out with the predicate evaluating to false. This is because
        //       the deadline is set if and only if and only if work was scheduled.
        cv.wait_until(lk, deadline, [this] {
            bool drained = draining && work.empty();
            bool work_due = !work.empty() && (clock::now() > work.top().deadline);
            return terminated || drained || work_due;
        });

        if (terminated) return;

        if (draining && work.empty()) {
            drain_latch.decrement();
            return;
        }

        Task task = work.top();
        work.pop();
        deadline = work.empty() ? clock::time_point::max() : work.top().deadline;
        if (task.periodic) {
            task.deadline += task.delay;
            do_schedule(task);
        }

        lk.unlock();
        task.func();
    }
}

bool Worker::do_schedule(const Task& task) {
    std::lock_guard<std::recursive_mutex> lk{m};
    if (terminated || draining) return false;

    work.push(task);
    deadline = std::min(deadline, work.top().deadline);
    cv.notify_one();

    return true;
}

void Worker::drain() {
    {
        std::lock_guard<std::recursive_mutex> lk{m};
        if (draining) return;
        draining = true;
        cv.notify_one();
    }
    drain_latch.wait();
}

void Worker::terminate() {
    std::lock_guard<std::recursive_mutex> lk{m};
    if (terminated) return;
    terminated = true;
    cv.notify_one();
}

// Sort in ascending order of execution time.
Task::Cmp Task::cmp = [](const Task& x, const Task& y) { return x.deadline > y.deadline; };

Task::Task(std::function<void()> func,
           clock::duration delay,
           bool periodic,
           clock::time_point deadline)
    : func(std::move(func)), delay(delay), periodic(periodic), deadline(deadline) {}

} // namespace spindle
