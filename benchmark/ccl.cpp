#include <benchmark/benchmark.h>

#include <opencv2/opencv.hpp>
#include <sstream>
#include <string>

#include "apriltag.h"
#include "ccl/bmrs.h"
#include "common/image_u8.h"
#include "common/pjpeg.h"
#include "common/unionfind.h"
#include "common/workerpool.h"
#include "simdtag/atomic_stack.h"
#include "simdtag/memory_pool.h"
#include "simdtag/vision_utils.h"
#include "third_party/yacclab/bmrs.h"
#include "third_party/yacclab/labels_solver.h"
#include "third_party/yacclab/spaghetti.h"
#include "third_party/yacclab/spaghetti_dual.h"

// TODO: Work on april tag specific inputs (i.e. a thresholded image)

unsigned* UF::P_;
unsigned UF::length_;

#define IMAGE_PATH CMAKE_PROJECT_SOURCE_DIR "/assets/yacclab/test_threshold.png"
#define IMAGE_PATH2 CMAKE_PROJECT_SOURCE_DIR "/assets/yacclab/testimage.jpg"

static void BM_Bmrs(benchmark::State& state) {
    cv::Mat1b thresholdedOutput = cv::imread(IMAGE_PATH, cv::IMREAD_GRAYSCALE);
    simdtag::BMRS ccl{thresholdedOutput.size()};

    for (auto _ : state) {
        cv::Mat1i labels = cv::Mat1i{thresholdedOutput.size(), 0};
        ccl.PerformLabeling(thresholdedOutput, labels);
    }
}

static void BM_BmrsThreadSwap(benchmark::State& state) {
    cv::Mat1b thresholdedOutput = cv::imread(IMAGE_PATH, cv::IMREAD_GRAYSCALE);
    simdtag::AutoZerodMatPool<4> matPool(thresholdedOutput.size());
    simdtag::BMRS ccl{thresholdedOutput.size()};

    for (auto _ : state) {
        auto tmp = matPool.Aquire();
        ccl.PerformLabeling(thresholdedOutput, *tmp);
        matPool.Release(std::move(tmp));
    }
}

static void BM_BmrsDual(benchmark::State& state) {
    cv::Mat1b thresholdedOutput = cv::imread(IMAGE_PATH, cv::IMREAD_GRAYSCALE);
    simdtag::BMRS ccl{thresholdedOutput.size()};

    for (auto _ : state) {
        cv::Mat1i labels = cv::Mat1i{thresholdedOutput.size(), 0};
        ccl.PerformLabelingDual(thresholdedOutput, labels);
    }
}

static void BM_BmrsDualThreadSwap(benchmark::State& state) {
    cv::Mat1b thresholdedOutput = cv::imread(IMAGE_PATH, cv::IMREAD_GRAYSCALE);
    simdtag::AutoZerodMatPool<4> matPool(thresholdedOutput.size());
    simdtag::BMRS ccl{thresholdedOutput.size()};

    for (auto _ : state) {
        auto tmp = matPool.Aquire();
        ccl.PerformLabelingDual(thresholdedOutput, *tmp);
        matPool.Release(std::move(tmp));
    }
}

static void BM_YacclabBmrs(benchmark::State& state) {
    cv::Mat1b thresholdedOutput = cv::imread(IMAGE_PATH, cv::IMREAD_GRAYSCALE);
    cv::Mat1i labels;

    for (auto _ : state) {
        BMRS<UF> ccl{thresholdedOutput, labels};
        ccl.PerformYLLabeling();
    }
}

static void BM_YacclabSpaghetti(benchmark::State& state) {
    cv::Mat1b thresholdedOutput = cv::imread(IMAGE_PATH, cv::IMREAD_GRAYSCALE);
    cv::Mat1i labels;

    for (auto _ : state) {
        Spaghetti<UFPC> ccl{thresholdedOutput, labels};
        ccl.PerformSPLabeling();
    }
}

static void BM_YacclabSpaghettiDual(benchmark::State& state) {
    cv::Mat1b thresholdedOutput = cv::imread(IMAGE_PATH, cv::IMREAD_GRAYSCALE);
    cv::Mat1i labels;

    for (auto _ : state) {
        SpaghettiDual<UFPC> ccl{thresholdedOutput, labels};
        ccl.PerformSPLabeling();
    }
}

static void PrintOutAllImages(benchmark::State& state) {
    //// Output the file
    {
        cv::Mat1b thresholdedOutput = cv::imread(IMAGE_PATH, cv::IMREAD_GRAYSCALE);
        cv::Mat1i labels = cv::Mat1i{thresholdedOutput.size(), 0};
        SpaghettiDual<UFPC> ccl{thresholdedOutput, labels};
        ccl.PerformSPLabeling();

        std::stringstream filename;
        filename << CMAKE_PROJECT_BUILD_DIR << "/" << "YACCLAB_Spaghetti_Dual" << ".jpg";

        cv::imwrite(filename.str(), simdtag::CreateLabeledImage(labels, ccl.n_labels_));
    }
    ////

    //// Output the file
    {
        cv::Mat1b thresholdedOutput = cv::imread(IMAGE_PATH, cv::IMREAD_GRAYSCALE);
        cv::Mat1i labels = cv::Mat1i{thresholdedOutput.size(), 0};
        Spaghetti<UFPC> ccl{thresholdedOutput, labels};
        ccl.PerformSPLabeling();

        std::stringstream filename;
        filename << CMAKE_PROJECT_BUILD_DIR << "/" << "YACCLAB_Spaghetti" << ".jpg";

        cv::imwrite(filename.str(), simdtag::CreateLabeledImage(labels, ccl.n_labels_));
    }
    ////

    //// Output the file
    {
        cv::Mat1b thresholdedOutput = cv::imread(IMAGE_PATH, cv::IMREAD_GRAYSCALE);
        cv::Mat1i labels = cv::Mat1i{thresholdedOutput.size(), 0};
        BMRS<UF> ccl{thresholdedOutput, labels};
        ccl.PerformYLLabeling();

        std::stringstream filename;
        filename << CMAKE_PROJECT_BUILD_DIR << "/" << "YACCLAB_BMRS" << ".jpg";

        cv::imwrite(filename.str(), simdtag::CreateLabeledImage(labels, ccl.n_labels_));
    }
    ////

    //// Output the file
    {
        cv::Mat1b thresholdedOutput = cv::imread(IMAGE_PATH, cv::IMREAD_GRAYSCALE);
        cv::Mat1i labels = cv::Mat1i{thresholdedOutput.size(), 0};
        simdtag::BMRS ccl{thresholdedOutput.size()};
        ccl.PerformLabelingDual(thresholdedOutput, labels);

        std::stringstream filename;
        filename << CMAKE_PROJECT_BUILD_DIR << "/" << "BMRSDual" << ".jpg";

        cv::imwrite(filename.str(), simdtag::CreateLabeledImage(labels, ccl.LabelCount()));
    }
    ////

    //// Output the file
    {
        cv::Mat1b thresholdedOutput = cv::imread(IMAGE_PATH, cv::IMREAD_GRAYSCALE);
        cv::Mat1i labels = cv::Mat1i{thresholdedOutput.size(), 0};
        simdtag::BMRS ccl{thresholdedOutput.size()};
        ccl.PerformLabeling(thresholdedOutput, labels);

        std::stringstream filename;
        filename << CMAKE_PROJECT_BUILD_DIR << "/" << "BMRSThreadSwap" << ".jpg";

        cv::imwrite(filename.str(), simdtag::CreateLabeledImage(labels, ccl.LabelCount()));
    }
    ////

    //// Output the file
    {
        cv::Mat1b thresholdedOutput = cv::imread(IMAGE_PATH, cv::IMREAD_GRAYSCALE);
        cv::Mat1i labels = cv::Mat1i{thresholdedOutput.size(), 0};
        simdtag::BMRS ccl{thresholdedOutput.size()};
        ccl.PerformLabeling(thresholdedOutput, labels);

        std::stringstream filename;
        filename << CMAKE_PROJECT_BUILD_DIR << "/" << "BMRS" << ".jpg";

        cv::imwrite(filename.str(), simdtag::CreateLabeledImage(labels, ccl.LabelCount()));
    }
    ////

    // Without this, benchmark runs the whole routine
    for (auto _ : state) {
    }
}

extern "C" {
extern image_u8_t* threshold(apriltag_detector_t* td, image_u8_t* im);
extern unionfind_t* connected_components(apriltag_detector_t* td, image_u8_t* threshim, int w,
                                         int h, int ts);
}

static void BM_AprilTagUnionFind(benchmark::State& state) {
    apriltag_detector_t* td = apriltag_detector_create();
    td->quad_decimate = 1.0;
    td->quad_sigma = 0.0;
    td->nthreads = 1.0;
    td->debug = 0;
    td->refine_edges = 1;
    td->wp = workerpool_create(td->nthreads);

    int err = 0;
    const char* path = IMAGE_PATH2;
    pjpeg_t* pjpeg = pjpeg_create_from_file(path, 0, &err);
    if (pjpeg == NULL) {
        printf("pjpeg failed to load: %s, error %d\n", path, err);
        return;
    }

    image_u8_t* im = pjpeg_to_u8_baseline(pjpeg);

    int w = im->width, h = im->height;

    image_u8_t* threshim = threshold(td, im);
    // image_u8_write_pnm(threshim, "debug_threshold.pnm");
    int ts = threshim->stride;

    for (auto _ : state) {
        unionfind_t* uf = connected_components(td, threshim, w, h, ts);
    }
}

BENCHMARK(BM_YacclabBmrs);
BENCHMARK(BM_Bmrs);
BENCHMARK(BM_BmrsThreadSwap);
BENCHMARK(BM_BmrsDual);
BENCHMARK(BM_BmrsDualThreadSwap);
BENCHMARK(BM_YacclabSpaghetti);
BENCHMARK(BM_YacclabSpaghettiDual);
BENCHMARK(BM_AprilTagUnionFind);
// BENCHMARK(PrintOutAllImages);

BENCHMARK_MAIN();
