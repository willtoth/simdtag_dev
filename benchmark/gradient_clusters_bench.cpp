#include <benchmark/benchmark.h>
#include <fmt/format.h>

#include <opencv2/opencv.hpp>
#include <sstream>
#include <string>

#include "apriltag.h"
#include "apriltag_helper.h"
#include "ccl/bmrs.h"
#include "common/image_u8.h"
#include "common/pjpeg.h"
#include "common/unionfind.h"
#include "common/workerpool.h"
#include "gradient_clusters.h"
#include "simdtag/vision_utils.h"
#include "threshold.h"

#define IMAGE_PATH CMAKE_PROJECT_SOURCE_DIR "/assets/apriltag/tags_3_desk.jpg"
#define IMAGE_PATH2 CMAKE_PROJECT_SOURCE_DIR "/assets/apriltag/shapes.png"

extern "C" {
extern image_u8_t* threshold(apriltag_detector_t* td, image_u8_t* im);
extern unionfind_t* connected_components(apriltag_detector_t* td, image_u8_t* threshim, int w,
                                         int h, int ts);
extern zarray_t* gradient_clusters(apriltag_detector_t* td, image_u8_t* threshim, int w, int h,
                                   int ts, unionfind_t* uf);

int32_t __TEST123(int32_t x, int32_t y) {
    static int32_t i = 1;
    // fmt::print("{},{} - {}  ", x, y, i);
    return i++;
}
}

static void BM_TryGc() {  //(benchmark::State& state) {
    cv::Mat1b input = cv::imread(IMAGE_PATH2, cv::IMREAD_GRAYSCALE);
    cv::Mat1i labels = cv::Mat1i{input.size(), 0};
    simdtag::BMRS ccl{input.size()};
    ccl.PerformLabelingDual(input, labels);
    simdtag::GradientClusters gc{input.size()};
    gc.Perform(input, labels, ccl);
}

static void BM_GenerateTestData() {  //(benchmark::State& state) {
    apriltag_detector_t* td = DefaultApriltagDetector();

    cv::Mat1b gray = cv::imread(IMAGE_PATH2, cv::IMREAD_GRAYSCALE);
    image_u8_t im = {gray.cols, gray.rows, gray.cols, gray.data};
    int w = im.width, h = im.height;

    // image_u8_t* threshim = threshold(td, &im);
    int ts = im.stride;
    unionfind_t* uf = connected_components(td, &im, w, h, ts);

    WriteConnectedComponents(td, w, h, uf);

    zarray_t* out = gradient_clusters(td, &im, w, h, ts, uf);

    PrintGradientClusters(out);
    WriteGradientClusters(w, h, out);

    // for (auto _ : state) {
    // }
}

static void BM_HalideGradientClusters(benchmark::State& state) {
    cv::Mat1b input = cv::imread(IMAGE_PATH, cv::IMREAD_GRAYSCALE);
    cv::Mat1b threshold = cv::Mat1b{input.size(), 0};
    cv::Mat1i labels = cv::Mat1i{input.size(), 0};
    simdtag::BMRS ccl{input.size()};
    simdtag::GradientClusters gc{input.size()};

    simdtag::AdaptiveThreshold(input, threshold);
    ccl.PerformLabelingDual(threshold, labels);

    cv::imwrite(CMAKE_PROJECT_BUILD_DIR "/BM_HalideGradientClustersThreshold.png", threshold);
    cv::imwrite(CMAKE_PROJECT_BUILD_DIR "/BM_HalideGradientClustersLabeled.png",
                simdtag::CreateLabeledImage(labels, ccl.LabelCount()));

    for (auto _ : state) {
        gc.Perform(threshold, labels, ccl);
    }

    gc.Perform(threshold, labels, ccl);
}

static void BM_AprilTagGradientClusters(benchmark::State& state) {
    apriltag_detector_t* td = DefaultApriltagDetector();

    int err = 0;
    const char* path = IMAGE_PATH;
    pjpeg_t* pjpeg = pjpeg_create_from_file(path, 0, &err);
    if (pjpeg == NULL) {
        printf("pjpeg failed to load: %s, error %d\n", path, err);
        return;
    }

    image_u8_t* im = pjpeg_to_u8_baseline(pjpeg);

    int w = im->width, h = im->height;

    image_u8_t* threshim = threshold(td, im);
    int ts = threshim->stride;

    unionfind_t* uf = connected_components(td, threshim, w, h, ts);

    for (auto _ : state) {
        zarray_t* out = gradient_clusters(td, threshim, w, h, ts, uf);
    }
}

#if 1
BENCHMARK(BM_HalideGradientClusters);
BENCHMARK(BM_AprilTagGradientClusters);

BENCHMARK_MAIN();

#else
int main() {
    // BM_GenerateTestData();
    BM_TryGc();
    return 0;
}
#endif
