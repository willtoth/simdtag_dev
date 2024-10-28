#include <opencv2/opencv.hpp>

#include "apriltag.h"
#include "apriltag_helper.h"
#include "ccl/bmrs.h"
#include "common/image_u8.h"
#include "common/pjpeg.h"
#include "common/unionfind.h"
#include "common/workerpool.h"
#include "common/zarray.h"
#include "fit_quads.h"
#include "fmt/format.h"
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

static void Perf_GradientClusters() {
    cv::Mat1b input = cv::imread(IMAGE_PATH, cv::IMREAD_GRAYSCALE);
    cv::Mat1b threshold = cv::Mat1b{input.size(), 0};
    cv::Mat1i labels = cv::Mat1i{input.size(), 0};
    simdtag::BMRS ccl{input.size()};
    simdtag::GradientClusters gc{input.size()};
    simdtag::GradientClusterHash hash{100};

    simdtag::AdaptiveThreshold(input, threshold);
    ccl.PerformLabelingDual(threshold, labels);

    for (int i = 0; i < PERF_ITERATION; i++) {
        gc.Perform(threshold, labels, hash);
    }

    simdtag::FitQuads::Perform(hash, input.size());

    // gc.Print(buffer);
    cv::Mat1b result = gc.Draw(hash);

    std::stringstream filename;
    filename << CMAKE_PROJECT_BUILD_DIR << "/" << "GradientClustersOutput" << ".jpg";

    cv::imwrite(filename.str(), result);
}

static void Perf_AprilTagGradientClusters() {
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

    for (int i = 0; i < PERF_ITERATION; i++) {
        zarray_t* out = gradient_clusters(td, threshim, w, h, ts, uf);
    }

    zarray_t* clusters = gradient_clusters(td, threshim, w, h, ts, uf);

    std::stringstream filename;
    filename << CMAKE_PROJECT_BUILD_DIR << "/" << "GradientClustersApriltagOutput" << ".jpg";

    cv::imwrite(filename.str(), DrawGradientClustersBlackAndWhite(w, h, clusters));

#if 0
    int sz = zarray_size(clusters);
    for (int i = 0; i < sz; i++) {
        zarray_t* cluster;
        zarray_get(clusters, i, &cluster);
        int cluster_size = zarray_size(cluster);
        fmt::print("{{");
        for (int j = 0; j < cluster_size; j++) {
            struct pt* p;
            zarray_get_volatile(cluster, j, &p);
            int gx = p->gx > 0 ? 1 : p->gx < 0 ? -1 : 0;
            int gy = p->gy > 0 ? 1 : p->gy < 0 ? -1 : 0;
            fmt::print("{{{},{},{},{}}},", p->x, p->y, gx, gy);
        }
        fmt::print("}}");
    }
#endif
}

int main() {
#if defined(HALIDE_GRADIENT_CLUSTERS)
    Perf_GradientClusters();
#elif defined(APRILTAG_GRADIENT_CLUSTERS)
    Perf_AprilTagGradientClusters();
#else
#error "Must define a test to run
#endif
    return 0;
}
