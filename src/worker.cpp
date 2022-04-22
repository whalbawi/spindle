#include "worker.h"

namespace spindle {

Worker::Worker() : deferred_work{DeferredTask::cmp}, deadline{clock::time_point::max()} {}

void Worker::run() {
    for (;;) {
        std::function<void()> task;
        std::unique_lock<std::recursive_mutex> lk{m};

        bool deferred_task_due = false;
        // Wait until:
        // - Terminated
        // - There is a deferred task due to run
        // - There is a task to run
        // - `cv` times out

        // Note: `cv` will never time out with the predicate evaluating to false. This is because
        //       the deadline is set if and only if there is a deferred task due to run.
        cv.wait_until(lk, deadline, [this, &deferred_task_due] {
            clock::time_point now = clock::now();
            deferred_task_due = !deferred_work.empty() && (now > deferred_work.top().deadline);
            return terminated || deferred_task_due || !work.empty();
        });

        if (terminated) return;

        if (deferred_task_due) {
            // Enqueue the task for execution and reset `deadline` if there's deferred work
            // remaining
            DeferredTask deferred_task = deferred_work.top();
            deferred_work.pop();
            work.push(deferred_task.task);
            deadline =
                deferred_work.empty() ? clock::time_point::max() : deferred_work.top().deadline;
            // Re-schedule the task if it is periodic
            if (deferred_task.periodic) {
                schedule(deferred_task.task, deferred_task.delay, deferred_task.periodic);
            }
            // and go on to check if there's a task to execute.
        }

        if (work.empty()) continue;
        task = work.front();
        work.pop();
        lk.unlock();

        task();
    }
}

bool Worker::enqueue(const std::function<void()>& task) {
    std::lock_guard<std::recursive_mutex> lk{m};
    if (terminated) return false;
    work.emplace(task);
    cv.notify_one();

    return true;
}

void Worker::terminate() {
    std::lock_guard<std::recursive_mutex> lk{m};
    if (terminated) return;
    terminated = true;
    cv.notify_one();
}

// Sort in ascending order of execution time.
DeferredWorkCmp DeferredTask::cmp = [](const DeferredTask& x, const DeferredTask& y) {
    return x.deadline > y.deadline;
};

DeferredTask::DeferredTask(std::function<void()> task,
                           clock::time_point deadline,
                           bool periodic,
                           clock::duration delay)
    : task(std::move(task)), deadline(deadline), periodic(periodic), delay(delay) {}

} // namespace spindle
