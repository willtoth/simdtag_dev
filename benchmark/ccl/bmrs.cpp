#include "ccl/bmrs.h"

#include <benchmark/benchmark.h>

#include <opencv2/opencv.hpp>
#include <string>

#include "yacclab/bmrs.h"
#include "yacclab/labels_solver.h"

unsigned* UF::P_;
unsigned UF::length_;

std::string& GetImageFilename() {
    static std::string filename =
            std::string(CMAKE_PROJECT_SOURCE_DIR) + std::string("/assets/testimage.png");
    return filename;
}

static void BM_Bmrs(benchmark::State& state) {
    cv::Mat1b thresholdedOutput = cv::imread(GetImageFilename(), cv::IMREAD_GRAYSCALE);

    for (auto _ : state) {
        state.PauseTiming();
        cv::Mat1i labels;
        cv::Mat1b testImage = thresholdedOutput;
        apriltag::BMRS<1446, 902> ccl{testImage, labels};
        state.ResumeTiming();

        ccl.LocalPerformLabeling();
    }
}

static void BM_YacclabBmrs(benchmark::State& state) {
    cv::Mat1b thresholdedOutput = cv::imread(GetImageFilename(), cv::IMREAD_GRAYSCALE);

    for (auto _ : state) {
        state.PauseTiming();
        cv::Mat1i labels;
        cv::Mat1b testImage = thresholdedOutput;
        BMRS<UF> ccl{testImage, labels};
        state.ResumeTiming();

        ccl.YLPerformLabeling();
    }
}

BENCHMARK(BM_Bmrs);
BENCHMARK(BM_YacclabBmrs);

BENCHMARK_MAIN();
