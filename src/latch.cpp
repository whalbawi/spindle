#include "latch.h"

#include <sstream>
#include <stdexcept>

namespace spindle {

Latch::Latch() : Latch(1) {}

Latch::Latch(uint32_t weight) : weight(weight) {
    if (weight <= 0) {
        std::stringstream s;
        s << "Latch weight must be positive: " << weight;
        throw std::runtime_error{s.str()};
    }
}

void Latch::decrement() {
    std::lock_guard<std::mutex> lk{m};
    // Prevent potential underflow of `weight`.
    if (weight == 0 || --weight > 0) return;
    cv.notify_all();
}

void Latch::wait() {
    std::unique_lock<std::mutex> lk{m};
    cv.wait(lk, [&] { return weight == 0; });
}

} // namespace spindle
