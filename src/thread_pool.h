#ifndef SPINDLE_THREAD_POOL_H_
#define SPINDLE_THREAD_POOL_H_

#include <atomic>
#include <functional>
#include <thread>
#include <vector>

namespace spindle {

class Worker;

// `ThreadPool` is a collection of threads on which work can be scheduled for execution. The threads
// correspond to operating system threads and are therefore subject to its scheduling policy.
class ThreadPool {
  public:
    // Creates a thread pool with a number of threads equal to the system's hardware concurrency.
    ThreadPool();
    // Creates a thread pool with the specified number of threads.
    ThreadPool(uint32_t num_threads);

    // Terminates all worker threads. Any inflight tasks continue to execute but no new tasks
    // will be enqueued or executed.
    ~ThreadPool();

    // Schedules a task for execution on one of the threads in this `ThreadPool`. Calling this
    // method concurrently with `ThreadPool::tear_down` does not guarantee execution of the task.
    void execute(const std::function<void()>& task);

    // Identical to the destructor.
    void tear_down();

  private:
    std::vector<std::unique_ptr<Worker>> workers;
    std::vector<std::thread> worker_threads;
    std::atomic_int next_worker{};
};

} // namespace spindle

#endif // SPINDLE_THREAD_POOL_H_
