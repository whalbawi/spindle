#ifndef SPINDLE_WORKER_H_
#define SPINDLE_WORKER_H_

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>

#include "spindle/latch.h"

namespace spindle {

using clock = std::chrono::high_resolution_clock;

class Worker;

class Task {
  public:
    Task(std::function<void()> func,
         clock::duration delay,
         bool periodic,
         clock::time_point deadline);

    friend Worker;

  private:
    using Cmp = std::function<bool(const Task& x, const Task& y)>;
    static Cmp cmp;
    std::function<void()> func;
    clock::time_point deadline;
    clock::duration delay;
    bool periodic;
};

// `Worker` continuously executes tasks in a loop, until terminated.
class Worker {
  public:
    Worker();
    // Continuously executes enqueued tasks until terminated.
    void run();
    // Schedules a task for execution.
    template <class T = clock::duration>
    bool schedule(const std::function<void()>& func, T delay = {}, bool periodic = false);
    // Drains the `Worker`. From this point onwards, the `Worker` will reject new tasks but will
    // continue executing the inflight task and the any tasks remaining in the work queue.
    void drain();
    // Terminates the `Worker`. From this point onwards, the `Worker` will reject new tasks but will
    // continue executing any inflight task.
    void terminate();

  private:
    std::priority_queue<Task, std::vector<Task>, Task::Cmp> work;
    // Need a re-entrant mutex because `Worker::run` might call `Worker::do_schedule`.
    // TODO (whalbawi): Can we refactor `Worker::schedule` to avoid this?
    std::recursive_mutex m;
    std::condition_variable_any cv;
    clock::time_point deadline;
    bool terminated{};
    bool draining{};
    Latch drain_latch{};

    bool do_schedule(const Task& task);
};

template <class T>
bool Worker::schedule(const std::function<void()>& func, T delay, bool periodic) {
    Task task{func, delay, periodic, clock::now() + delay};
    return do_schedule(task);
}

} // namespace spindle

#endif // SPINDLE_WORKER_H_
