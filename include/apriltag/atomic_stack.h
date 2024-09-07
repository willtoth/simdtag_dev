#pragma once

#include <mutex>
#include <semaphore>
#include <stack>

namespace apriltag {

template <typename T, size_t N>
class FixedSizeAtomicStack {
   public:
    FixedSizeAtomicStack() : sem_(0) {
    }

    T pop() {
        sem_.acquire();

        std::scoped_lock lock{mutex_};
        T result = std::move(stack_.top());
        stack_.pop();

        return result;
    }

    void push(const T& value) {
        {
            std::scoped_lock lock{mutex_};
            stack_.push(value);
        }
        sem_.release();
    }

    void push(T&& value) {
        {
            std::scoped_lock lock{mutex_};
            stack_.push(std::move(value));
        }
        sem_.release();
    }

   private:
    std::stack<T> stack_;
    std::counting_semaphore<N> sem_;
    std::mutex mutex_;
};

}  // namespace apriltag
