#ifndef SPINDLE_WORKER_H_
#define SPINDLE_WORKER_H_

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <utility>

namespace spindle {

using clock = std::chrono::high_resolution_clock;

using sched_work_t = std::pair<clock::time_point, std::function<void()>>; // (deadline, task)

// Sort in ascending order of execution time.
static auto cmp = [](const sched_work_t& x, const sched_work_t& y) { return x.first > y.first; };

// `Worker` continuously executes tasks in a loop, until terminated.
class Worker {
  public:
    // Continuously executes enqueued tasks until terminated.
    void run();
    // Enqueues a task for execution.
    bool enqueue(const std::function<void()>& task);
    // Schedules a task for deferred execution.
    template <class T> bool schedule(const std::function<void()>& task, T delay);
    // Terminates the `Worker`. From this point onwards, the `Worker` will not reject new tasks
    // but will continue executing any inflight task.
    void terminate();

  private:
    std::queue<std::function<void()>> work;
    std::priority_queue<sched_work_t, std::vector<sched_work_t>, decltype(cmp)> sched_work{cmp};
    std::mutex m;
    std::condition_variable cv;
    clock::time_point deadline = clock::time_point::max();
    bool terminated{};
};

template <class T> bool Worker::schedule(const std::function<void()>& task, T delay) {
    std::lock_guard<std::mutex> lk{m};
    if (terminated) return false;
    clock::time_point task_deadline = clock::now() + delay;
    sched_work.emplace(task_deadline, task);
    deadline = std::min(deadline, task_deadline);
    cv.notify_one();

    return true;
}

} // namespace spindle

#endif // SPINDLE_WORKER_H_
