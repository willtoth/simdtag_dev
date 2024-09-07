#include "ccl/bmrs.h"

#include <benchmark/benchmark.h>

#include <iostream>
#include <semaphore>
#include <stack>
#include <string>
#include <thread>

#include "apriltag/atomic_stack.h"
#include "apriltag/memory_pool.h"
#include "third_party/yacclab/bmrs.h"
#include "third_party/yacclab/labels_solver.h"
#include "third_party/yacclab/spaghetti.h"

unsigned* UF::P_;
unsigned UF::length_;

std::string& GetImageFilename() {
    static std::string filename =
            std::string(CMAKE_PROJECT_SOURCE_DIR) + std::string("/assets/yacclab/testimage.png");
    return filename;
}

static void BM_Bmrs(benchmark::State& state) {
    cv::Mat1b thresholdedOutput = cv::imread(GetImageFilename(), cv::IMREAD_GRAYSCALE);
    apriltag::BMRS ccl{thresholdedOutput};

    for (auto _ : state) {
        cv::Mat1i labels = cv::Mat1i{thresholdedOutput.size(), 0};
        ccl.PerformLabeling(thresholdedOutput, labels);
    }
}

template <size_t N>
class AutoZerodMatPool {
    using T = cv::Mat1i;

   public:
    AutoZerodMatPool(cv::Size size) : run_thread_(true) {
        for (int i = 0; i < N; i++) {
            clean_pool_.push(std::make_unique<cv::Mat1i>(size));
        }

        thread_ = std::thread{[this]() { this->ThreadRunner(); }};
    }

    ~AutoZerodMatPool() {
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
            *value = 0;
            clean_pool_.push(std::move(value));
        }
    }

    apriltag::FixedSizeAtomicStack<std::unique_ptr<T>, N> dirty_pool_;
    apriltag::FixedSizeAtomicStack<std::unique_ptr<T>, N> clean_pool_;

    std::thread thread_;
    std::atomic_bool run_thread_;
};

static void BM_BmrsThreadSwap(benchmark::State& state) {
    cv::Mat1b thresholdedOutput = cv::imread(GetImageFilename(), cv::IMREAD_GRAYSCALE);
    apriltag::AutoZerodMatPool<4> matPool(thresholdedOutput.size());
    apriltag::BMRS ccl{thresholdedOutput};

    for (auto _ : state) {
        auto tmp = matPool.Aquire();
        ccl.PerformLabeling(thresholdedOutput, *tmp);
        matPool.Release(std::move(tmp));
    }
}

static void BM_YacclabBmrs(benchmark::State& state) {
    cv::Mat1b thresholdedOutput = cv::imread(GetImageFilename(), cv::IMREAD_GRAYSCALE);
    cv::Mat1i labels;

    for (auto _ : state) {
        BMRS<UF> ccl{thresholdedOutput, labels};
        ccl.PerformYLLabeling();
    }
}

static void BM_YacclabSpaghetti(benchmark::State& state) {
    cv::Mat1b thresholdedOutput = cv::imread(GetImageFilename(), cv::IMREAD_GRAYSCALE);
    cv::Mat1i labels;

    for (auto _ : state) {
        Spaghetti<UFPC> ccl{thresholdedOutput, labels};
        ccl.PerformSPLabeling();
    }
}

BENCHMARK(BM_YacclabBmrs);
BENCHMARK(BM_Bmrs);
BENCHMARK(BM_BmrsThreadSwap);
//  BENCHMARK(BM_YacclabSpaghetti);
//  BENCHMARK(BM_YacclabBmrs);
//  BENCHMARK(BM_Bmrs);
//  BENCHMARK(BM_YacclabSpaghetti);

BENCHMARK_MAIN();
