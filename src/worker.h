#ifndef SPINDLE_WORKER_H_
#define SPINDLE_WORKER_H_

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>

namespace spindle {

using clock = std::chrono::high_resolution_clock;

class DeferredTask;

using DeferredWorkCmp = std::function<bool(const DeferredTask& x, const DeferredTask& y)>;

// `Worker` continuously executes tasks in a loop, until terminated.
class Worker {
  public:
    Worker();
    // Continuously executes enqueued tasks until terminated.
    void run();
    // Enqueues a task for execution.
    bool enqueue(const std::function<void()>& task);
    // Schedules a task for deferred execution.
    template <class T>
    bool schedule(const std::function<void()>& task, T delay, bool periodic = false);
    // Terminates the `Worker`. From this point onwards, the `Worker` will not reject new tasks
    // but will continue executing any inflight task.
    void terminate();

  private:
    std::queue<std::function<void()>> work;
    std::priority_queue<DeferredTask, std::vector<DeferredTask>, DeferredWorkCmp> deferred_work;
    // Need a re-entrant mutex because `Worker::run` might call `Worker::schedule`.
    // TODO (whalbawi): Can we refactor `Worker::schedule` to avoid this?
    std::recursive_mutex m;
    std::condition_variable_any cv;
    clock::time_point deadline;
    bool terminated{};
};

class DeferredTask {
  public:
    DeferredTask(std::function<void()> task,
                 clock::time_point deadline,
                 bool periodic,
                 clock::duration delay);

    friend Worker;

  private:
    static std::function<bool(const DeferredTask& x, const DeferredTask& y)> cmp;
    std::function<void()> task;
    clock::time_point deadline;
    bool periodic;
    clock::duration delay;
};

template <class T>
bool Worker::schedule(const std::function<void()>& task, T delay, bool periodic) {
    std::lock_guard<std::recursive_mutex> lk{m};
    if (terminated) return false;
    clock::time_point task_deadline = clock::now() + delay;
    deferred_work.emplace(task, task_deadline, periodic, delay);
    deadline = std::min(deadline, task_deadline);
    cv.notify_one();

    return true;
}

} // namespace spindle

#endif // SPINDLE_WORKER_H_
