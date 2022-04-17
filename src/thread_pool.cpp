#include "thread_pool.h"

#include "worker.h"

namespace spindle {

ThreadPool::ThreadPool() : ThreadPool(std::thread::hardware_concurrency()) {}

ThreadPool::ThreadPool(uint32_t num_threads) {
    for (int i = 0; i < num_threads; ++i) {
        std::unique_ptr<Worker> worker = std::make_unique<Worker>();
        workers.push_back(std::move(worker));
        worker_threads.emplace_back(&Worker::run, workers[i].get());
    }
}

ThreadPool::~ThreadPool() {
    tear_down();
}

void ThreadPool::execute(const std::function<void()>& task) {
    uint32_t idx = next_worker++ % workers.size(); // No harm in overflowing.
    workers[idx]->enqueue(task);
}

void ThreadPool::tear_down() {
    for (auto&& worker : workers) {
        worker->terminate();
    }

    for (auto&& thread : worker_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

} // namespace spindle
