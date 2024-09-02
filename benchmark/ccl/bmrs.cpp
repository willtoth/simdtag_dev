#include "ccl/bmrs.h"

#include <benchmark/benchmark.h>

#include <opencv2/opencv.hpp>
#include <string>

#include "yacclab/bmrs.h"
#include "yacclab/labels_solver.h"
#include "yacclab/spaghetti.h"

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
        ccl.PerformLabeling();
    }
}

static void BM_YacclabBmrs(benchmark::State& state) {
    cv::Mat1b thresholdedOutput = cv::imread(GetImageFilename(), cv::IMREAD_GRAYSCALE);
    cv::Mat1i labels;
    BMRS<UF> ccl{thresholdedOutput, labels};

    for (auto _ : state) {
        ccl.PerformYLLabeling();
    }
}

static void BM_YacclabSpaghetti(benchmark::State& state) {
    cv::Mat1b thresholdedOutput = cv::imread(GetImageFilename(), cv::IMREAD_GRAYSCALE);
    cv::Mat1i labels;
    Spaghetti<UFPC> ccl{thresholdedOutput, labels};

    for (auto _ : state) {
        ccl.PerformSPLabeling();
    }
}

BENCHMARK(BM_YacclabBmrs);
BENCHMARK(BM_Bmrs);
BENCHMARK(BM_YacclabSpaghetti);
BENCHMARK(BM_YacclabBmrs);
BENCHMARK(BM_Bmrs);
BENCHMARK(BM_YacclabSpaghetti);

BENCHMARK_MAIN();
