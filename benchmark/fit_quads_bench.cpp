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
#include "fit_quads.h"
#include "gradient_clusters.h"
#include "halide/bm_only_halide_gradient_clusters.h"
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

static void BM_FitQuads(benchmark::State& state) {
    cv::Mat1b input = cv::imread(IMAGE_PATH, cv::IMREAD_GRAYSCALE);
    cv::Mat1b threshold = cv::Mat1b{input.size(), 0};
    cv::Mat1i labels = cv::Mat1i{input.size(), 0};
    simdtag::BMRS ccl{input.size()};
    simdtag::GradientClusters gc{input.size()};
    simdtag::GradientClusterHash hash{100};

    simdtag::AdaptiveThreshold(input, threshold);
    ccl.PerformLabelingDual(threshold, labels);
    gc.Perform(threshold, labels, hash);

    for (auto _ : state) {
        simdtag::FitQuads::Perform(hash, input.size());
    }
}

static void fit_quads_epoch1(apriltag_detector_t* td, image_u8_t* im, zarray_t* cluster,
                             int tag_width) {
    int res = 0;

    int sz = zarray_size(cluster);
    if (sz < 24)  // Synchronize with later check.
        return;

    /////////////////////////////////////////////////////////////
    // Step 1. Sort points so they wrap around the center of the
    // quad. We will constrain our quad fit to simply partition this
    // ordered set into 4 groups.

    // compute a bounding box so that we can order the points
    // according to their angle WRT the center.
    struct pt* p1;
    zarray_get_volatile(cluster, 0, &p1);
    uint16_t xmax = p1->x;
    uint16_t xmin = p1->x;
    uint16_t ymax = p1->y;
    uint16_t ymin = p1->y;
    for (int pidx = 1; pidx < zarray_size(cluster); pidx++) {
        struct pt* p;
        zarray_get_volatile(cluster, pidx, &p);

        if (p->x > xmax) {
            xmax = p->x;
        } else if (p->x < xmin) {
            xmin = p->x;
        }

        if (p->y > ymax) {
            ymax = p->y;
        } else if (p->y < ymin) {
            ymin = p->y;
        }
    }

    if ((xmax - xmin) * (ymax - ymin) < tag_width) {
        return;
    }

    // add some noise to (cx,cy) so that pixels get a more diverse set
    // of theta estimates. This will help us remove more points.
    // (Only helps a small amount. The actual noise values here don't
    // matter much at all, but we want them [-1, 1]. (XXX with
    // fixed-point, should range be bigger?)
    float cx = (xmin + xmax) * 0.5 + 0.05118;
    float cy = (ymin + ymax) * 0.5 + -0.028581;

    float dot = 0;

    float quadrants[2][2] = {{-1 * (2 << 15), 0}, {2 * (2 << 15), 2 << 15}};

    for (int pidx = 0; pidx < zarray_size(cluster); pidx++) {
        struct pt* p;
        zarray_get_volatile(cluster, pidx, &p);

        float dx = p->x - cx;
        float dy = p->y - cy;

        dot += dx * p->gx + dy * p->gy;

        float quadrant = quadrants[dy > 0][dx > 0];
        if (dy < 0) {
            dy = -dy;
            dx = -dx;
        }

        if (dx < 0) {
            float tmp = dx;
            dx = dy;
            dy = -tmp;
        }
        p->slope = quadrant + dy / dx;
    }
}

static void BM_AprilTagFitQuads(benchmark::State& state) {
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
    zarray_t* clusters = gradient_clusters(td, threshim, w, h, ts, uf);

    for (auto _ : state) {
        for (int i = 0; i < zarray_size(clusters); i++) {
            zarray_t** cluster;
            zarray_get_volatile(clusters, i, &cluster);
            fit_quads_epoch1(td, im, *cluster, 1);
        }
    }
}

BENCHMARK(BM_FitQuads);
BENCHMARK(BM_AprilTagFitQuads);

BENCHMARK_MAIN();
