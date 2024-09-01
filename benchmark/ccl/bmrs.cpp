#include <benchmark/benchmark.h>

#include "ccl/bmrs.h"
#include <string>

#include <opencv2/opencv.hpp>

static void BM_bmrs(benchmark::State& state) {
  cv::Mat1b thresholdedOutput = cv::imread("../benchmark/ccl/testimage.png", cv::IMREAD_GRAYSCALE);

  for (auto _ : state) {
    state.PauseTiming();
    cv::Mat1i labels;
    cv::Mat1b testImage = thresholdedOutput;
    BMRS<1446, 902> ccl{testImage, labels};
    state.ResumeTiming();

    ccl.PerformLabeling();
  }
}

BENCHMARK(BM_bmrs);

BENCHMARK_MAIN();
