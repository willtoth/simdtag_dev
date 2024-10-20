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
}

static void BM_SimdTag(benchmark::State& state) {
    cv::Mat1b input = cv::imread(IMAGE_PATH, cv::IMREAD_GRAYSCALE);
    cv::Mat1b threshold = cv::Mat1b{input.size(), 0};
    cv::Mat1i labels = cv::Mat1i{input.size(), 0};
    simdtag::BMRS ccl{input.size()};
    simdtag::GradientClusters gc{input.size()};
    simdtag::GradientClusterHash hash{100};

    for (auto _ : state) {
        simdtag::AdaptiveThreshold(input, threshold);
        ccl.PerformLabelingDual(threshold, labels);
        gc.Perform(threshold, labels, hash);
    }
}

static void BM_AprilTag(benchmark::State& state) {
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

    for (auto _ : state) {
        image_u8_t* threshim = threshold(td, im);
        int ts = threshim->stride;
        unionfind_t* uf = connected_components(td, threshim, w, h, ts);
        zarray_t* out = gradient_clusters(td, threshim, w, h, ts, uf);
    }
}

BENCHMARK(BM_SimdTag);
BENCHMARK(BM_AprilTag);

BENCHMARK_MAIN();
