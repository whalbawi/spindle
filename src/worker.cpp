#include "worker.h"

namespace spindle {

void Worker::run() {
    for (;;) {
        std::function<void()> task;
        std::unique_lock<std::mutex> lk{m};

        bool sched_task_due{};
        // Wait until:
        // - Terminated
        // - There is a scheduled task due to run
        // - There is a task to run
        // - `cv` times out

        // Note: `cv` will never time out with the predicate evaluating to false. This is because
        //       The deadline is set if and only if there is scheduled work due.
        cv.wait_until(lk, deadline, [this, &sched_task_due] {
            clock::time_point now = clock::now();
            sched_task_due = !sched_work.empty() && (now > sched_work.top().first);
            return terminated || sched_task_due || !work.empty();
        });

        if (terminated) return;

        if (sched_task_due) {
            // Enqueue the task for execution and reset `deadline` if there's scheduled work
            // remaining
            work.push(sched_work.top().second);
            sched_work.pop();
            deadline = sched_work.empty() ? clock::time_point::max() : sched_work.top().first;
            sched_task_due = false;
            continue;
        }

        task = work.front();
        work.pop();
        lk.unlock();

        task();
    }
}

bool Worker::enqueue(const std::function<void()>& task) {
    std::lock_guard<std::mutex> lk{m};
    if (terminated) return false;
    work.emplace(task);
    cv.notify_one();

    return true;
}

void Worker::terminate() {
    std::lock_guard<std::mutex> lk{m};
    if (terminated) return;
    terminated = true;
    cv.notify_one();
}

} // namespace spindle
