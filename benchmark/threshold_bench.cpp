#include <benchmark/benchmark.h>

#include <opencv2/opencv.hpp>
#include <sstream>
#include <string>

#include "apriltag.h"
#include "common/image_u8.h"
#include "common/pjpeg.h"
#include "common/unionfind.h"
#include "common/workerpool.h"
#include "threshold.h"

#define IMAGE_PATH CMAKE_PROJECT_SOURCE_DIR "/assets/apriltag/tags_3_desk.jpg"

extern "C" {
extern image_u8_t* threshold(apriltag_detector_t* td, image_u8_t* im);
}

static void BM_Halide(benchmark::State& state) {
    cv::Mat1b input = cv::imread(IMAGE_PATH, cv::IMREAD_GRAYSCALE);
    cv::Mat1b output{input.size()};

    for (auto _ : state) {
        simdtag::AdaptiveThreshold(input, output);
    }

    simdtag::AdaptiveThreshold(input, output);

    std::stringstream filename;
    filename << CMAKE_PROJECT_BUILD_DIR << "/" << "AdaptiveThreholdOut" << ".jpg";

    cv::imwrite(filename.str(), output);
}

static void BM_Apriltag(benchmark::State& state) {
    apriltag_detector_t* td = apriltag_detector_create();
    td->quad_decimate = 1.0;
    td->quad_sigma = 0.0;
    td->nthreads = 1.0;
    td->debug = 0;
    td->refine_edges = 1;
    td->wp = workerpool_create(td->nthreads);

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
    }
}

BENCHMARK(BM_Halide);
BENCHMARK(BM_Apriltag);

BENCHMARK_MAIN();
