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

////////////////////////////////////////////////////
//////////// Direct from apriltag code /////////////
////////////////////////////////////////////////////

float pt_compare_angle(struct pt* a, struct pt* b) {
    return a->slope - b->slope;
}

static inline void ptsort(struct pt* pts, int sz) {
#define MAYBE_SWAP(arr, apos, bpos)                         \
    if (pt_compare_angle(&(arr[apos]), &(arr[bpos])) > 0) { \
        tmp = arr[apos];                                    \
        arr[apos] = arr[bpos];                              \
        arr[bpos] = tmp;                                    \
    };

    if (sz <= 1) return;

    if (sz == 2) {
        struct pt tmp;
        MAYBE_SWAP(pts, 0, 1);
        return;
    }

    // NB: Using less-branch-intensive sorting networks here on the
    // hunch that it's better for performance.
    if (sz == 3) {  // 3 element bubble sort is optimal
        struct pt tmp;
        MAYBE_SWAP(pts, 0, 1);
        MAYBE_SWAP(pts, 1, 2);
        MAYBE_SWAP(pts, 0, 1);
        return;
    }

    if (sz == 4) {  // 4 element optimal sorting network.
        struct pt tmp;
        MAYBE_SWAP(pts, 0, 1);  // sort each half, like a merge sort
        MAYBE_SWAP(pts, 2, 3);
        MAYBE_SWAP(pts, 0, 2);  // minimum value is now at 0.
        MAYBE_SWAP(pts, 1, 3);  // maximum value is now at end.
        MAYBE_SWAP(pts, 1, 2);  // that only leaves the middle two.
        return;
    }
    if (sz == 5) {
        // this 9-step swap is optimal for a sorting network, but two
        // steps slower than a generic sort.
        struct pt tmp;
        MAYBE_SWAP(pts, 0, 1);  // sort each half (3+2), like a merge sort
        MAYBE_SWAP(pts, 3, 4);
        MAYBE_SWAP(pts, 1, 2);
        MAYBE_SWAP(pts, 0, 1);
        MAYBE_SWAP(pts, 0, 3);  // minimum element now at 0
        MAYBE_SWAP(pts, 2, 4);  // maximum element now at end
        MAYBE_SWAP(pts, 1, 2);  // now resort the three elements 1-3.
        MAYBE_SWAP(pts, 2, 3);
        MAYBE_SWAP(pts, 1, 2);
        return;
    }

#undef MAYBE_SWAP

    // a merge sort with temp storage.

    struct pt* tmp = (struct pt*)malloc(sizeof(struct pt) * sz);

    memcpy(tmp, pts, sizeof(struct pt) * sz);

    int asz = sz / 2;
    int bsz = sz - asz;

    struct pt* as = &tmp[0];
    struct pt* bs = &tmp[asz];

    ptsort(as, asz);
    ptsort(bs, bsz);

#define MERGE(apos, bpos)                               \
    if (pt_compare_angle(&(as[apos]), &(bs[bpos])) < 0) \
        pts[outpos++] = as[apos++];                     \
    else                                                \
        pts[outpos++] = bs[bpos++];

    int apos = 0, bpos = 0, outpos = 0;
    while (apos + 8 < asz && bpos + 8 < bsz) {
        MERGE(apos, bpos);
        MERGE(apos, bpos);
        MERGE(apos, bpos);
        MERGE(apos, bpos);
        MERGE(apos, bpos);
        MERGE(apos, bpos);
        MERGE(apos, bpos);
        MERGE(apos, bpos);
    }

    while (apos < asz && bpos < bsz) {
        MERGE(apos, bpos);
    }

    if (apos < asz) memcpy(&pts[outpos], &as[apos], (asz - apos) * sizeof(struct pt));
    if (bpos < bsz) memcpy(&pts[outpos], &bs[bpos], (bsz - bpos) * sizeof(struct pt));

    free(tmp);

#undef MERGE
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
    ptsort((struct pt*)cluster->data, zarray_size(cluster));
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
