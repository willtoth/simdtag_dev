#pragma once

#include <atomic>
#include <memory>
#include <opencv2/opencv.hpp>

#include "atomic_stack.h"

namespace apriltag {

template <typename T, size_t N>
class PreallocMemPool {
    using PreallocMemPoolFunction = std::function<void(T*)>;

   public:
    PreallocMemPool(PreallocMemPoolFunction fcn, auto&&... args)
        : run_thread_(true), prealloc_function_(fcn) {
        for (int i = 0; i < N; i++) {
            clean_pool_.push(std::make_unique<T>(std::forward<decltype(args)>(args)...));
        }

        thread_ = std::thread{[this]() { this->ThreadRunner(); }};
    }

    ~PreallocMemPool() {
        run_thread_ = false;
        thread_.join();
    }

    std::unique_ptr<T> Aquire() {
        return std::move(clean_pool_.pop());
    }

    void Release(std::unique_ptr<T>&& ptr) {
        dirty_pool_.push(std::move(ptr));
    }

   private:
    void ThreadRunner() {
        while (run_thread_) {
            std::unique_ptr<T> value = std::move(dirty_pool_.pop());
            prealloc_function_(&(*value));
            clean_pool_.push(std::move(value));
        }
    }

    apriltag::FixedSizeAtomicStack<std::unique_ptr<T>, N> dirty_pool_;
    apriltag::FixedSizeAtomicStack<std::unique_ptr<T>, N> clean_pool_;

    std::thread thread_;
    std::atomic_bool run_thread_;

    PreallocMemPoolFunction prealloc_function_;
};

template <size_t N>
class AutoZerodMatPool : public PreallocMemPool<cv::Mat1i, N> {
   public:
    AutoZerodMatPool(cv::Size size)
        : PreallocMemPool<cv::Mat1i, N>(
                  [](cv::Mat1i* mat) { *mat = cv::Mat1i::zeros(mat->size()); }, size) {
    }
};

}  // namespace apriltag
