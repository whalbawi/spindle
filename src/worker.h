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

    // Sort in ascending order of execution time.
    bool operator>(const Task& other) const;

    friend Worker;

  private:

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
    std::priority_queue<Task, std::vector<Task>, std::greater<>> work{};
    std::mutex m;
    std::condition_variable cv;
    clock::time_point deadline;
    bool terminated{};
    bool draining{};
    Latch drain_latch{};

    bool do_schedule(const Task& task);
};

template <class T>
bool Worker::schedule(const std::function<void()>& func, T delay, bool periodic) {
    Task task{func, delay, periodic, clock::now() + delay};
    std::lock_guard<std::mutex> lk{m};
        if (do_schedule(task)) {
            cv.notify_one();
            return true;
        }

    return false;
}

} // namespace spindle

#endif // SPINDLE_WORKER_H_
