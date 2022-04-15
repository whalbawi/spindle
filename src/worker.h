#ifndef SPINDLE_WORKER_H_
#define SPINDLE_WORKER_H_

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>

namespace spindle {

// `Worker` continuously executes tasks in a loop, until terminated.
class Worker {
  public:
    // Continuously executes enqueued tasks until terminated.
    void run();
    // Enqueue a task for execution.
    bool enqueue(const std::function<void()>& task);
    // Terminate the `Worker`. From this point onwards, the `Worker` will not reject new tasks
    // but will continue executing any inflight task.
    void terminate();

  private:
    std::queue<std::function<void()>> work{};
    std::mutex m{};
    std::condition_variable cv{};
    bool terminated{};
};

} // namespace spindle

#endif // SPINDLE_WORKER_H_
