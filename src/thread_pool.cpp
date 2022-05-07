#include "spindle/thread_pool.h"

#include <sstream>
#include <stdexcept>

#include "worker.h"

namespace spindle {

ThreadPool::ThreadPool() : ThreadPool(std::thread::hardware_concurrency()) {}

ThreadPool::ThreadPool(uint32_t num_threads) : next_worker(0) {
    if (num_threads <= 0) {
        std::stringstream s;
        s << "Thread pool thread count must be positive: " << num_threads;
        throw std::runtime_error{s.str()};
    }
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
    workers[idx]->schedule(task);
}

void ThreadPool::drain() {
    for (auto&& worker : workers) {
        worker->drain();
    }

    for (auto&& thread : worker_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
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
