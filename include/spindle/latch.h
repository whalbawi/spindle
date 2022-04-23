#ifndef SPINDLE_LATCH_H_
#define SPINDLE_LATCH_H_

#include <condition_variable>
#include <mutex>

namespace spindle {

// `Latch` is a synchronization primitive that blocks the execution of one or more threads until
// some condition is met. The weight of a `Latch` signifies a quantity that is involved in
// establishing this condition, which is usually a number of threads that must complete an
// operation. Each time such operation is completed successfully, the weight of the `Latch` is
// decremented by one, and the waiting threads are unblocked once the weight reaches zero. Unlike a
// semaphore, a `Latch` cannot be reused to block threads once its weight reaches zero.
class Latch {
  public:
    // Instantiates a `Latch` of weight one.
    Latch();
    // Instantiates a `Latch` of the given positive weight.
    Latch(uint32_t weight);

    // Decrements the weight of the `Latch` by one.
    // Calling this method when the weight is already zero is a no-op.
    void decrement();
    // Blocks the calling thread until the weight of the `Latch` reaches zero.
    // Calling this method when the weight is already zero is a no-op.
    void wait();

  private:
    std::condition_variable cv;
    std::mutex m;
    uint32_t weight{};
};

} // namespace spindle

#endif // SPINDLE_LATCH_H_
