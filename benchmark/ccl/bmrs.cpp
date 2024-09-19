#include "ccl/bmrs.h"

#include <benchmark/benchmark.h>

#include <iostream>
#include <semaphore>
#include <stack>
#include <string>
#include <thread>

#include "simdtag/atomic_stack.h"
#include "simdtag/memory_pool.h"
#include "third_party/yacclab/bmrs.h"
#include "third_party/yacclab/labels_solver.h"
#include "third_party/yacclab/spaghetti.h"
#include "third_party/yacclab/spaghetti_dual.h"

unsigned* UF::P_;
unsigned UF::length_;

std::string& GetImageFilename() {
    static std::string filename =
            std::string(CMAKE_PROJECT_SOURCE_DIR) + std::string("/assets/yacclab/testimage.png");
    return filename;
}

static void BM_Bmrs(benchmark::State& state) {
    cv::Mat1b thresholdedOutput = cv::imread(GetImageFilename(), cv::IMREAD_GRAYSCALE);
    simdtag::BMRS ccl{thresholdedOutput};

    for (auto _ : state) {
        cv::Mat1i labels = cv::Mat1i{thresholdedOutput.size(), 0};
        ccl.PerformLabeling(thresholdedOutput, labels);
    }
}

static void BM_BmrsThreadSwap(benchmark::State& state) {
    cv::Mat1b thresholdedOutput = cv::imread(GetImageFilename(), cv::IMREAD_GRAYSCALE);
    simdtag::AutoZerodMatPool<4> matPool(thresholdedOutput.size());
    simdtag::BMRS ccl{thresholdedOutput};

    for (auto _ : state) {
        auto tmp = matPool.Aquire();
        ccl.PerformLabeling(thresholdedOutput, *tmp);
        matPool.Release(std::move(tmp));
    }
}

static void BM_BmrsDual(benchmark::State& state) {
    cv::Mat1b thresholdedOutput = cv::imread(GetImageFilename(), cv::IMREAD_GRAYSCALE);
    simdtag::BMRS ccl{thresholdedOutput};

    for (auto _ : state) {
        cv::Mat1i labels = cv::Mat1i{thresholdedOutput.size(), 0};
        ccl.PerformLabelingDual(thresholdedOutput, labels);
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

static void BM_YacclabSpaghettiDual(benchmark::State& state) {
    cv::Mat1b thresholdedOutput = cv::imread(GetImageFilename(), cv::IMREAD_GRAYSCALE);
    cv::Mat1i labels;

    for (auto _ : state) {
        SpaghettiDual<UFPC> ccl{thresholdedOutput, labels};
        ccl.PerformSPLabeling();
    }
}

BENCHMARK(BM_YacclabBmrs);
BENCHMARK(BM_Bmrs);
BENCHMARK(BM_BmrsThreadSwap);
BENCHMARK(BM_BmrsDual);
BENCHMARK(BM_YacclabSpaghetti);
BENCHMARK(BM_YacclabSpaghettiDual);
//  BENCHMARK(BM_YacclabBmrs);
//  BENCHMARK(BM_Bmrs);
//  BENCHMARK(BM_YacclabSpaghetti);

BENCHMARK_MAIN();
