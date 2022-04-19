#include "worker.h"

namespace spindle {

void Worker::run() {
    for (;;) {
        std::function<void()> task;
        std::unique_lock<std::mutex> lk(m);

        cv.wait(lk, [this] { return terminated || !work.empty(); });
        if (terminated) return;

        task = work.front();
        work.pop();
        lk.unlock();

        task();
    }
}

bool Worker::enqueue(const std::function<void()>& task) {
    std::lock_guard<std::mutex> lk(m);
    if (terminated) return false;
    work.emplace(task);
    cv.notify_one();

    return true;
}

void Worker::terminate() {
    std::lock_guard<std::mutex> lk(m);
    if (terminated) return;
    terminated = true;
    cv.notify_one();
}

} // namespace spindle
