#include "fit_quads.h"

#include <fmt/format.h>
#include <gtest/gtest.h>

#include <cstdlib>
#include <opencv2/opencv.hpp>

#include "gradient_point.h"

using namespace simdtag;

TEST(FitQuads, QuadBounds) {
    // Test the first stage of the pipeline which should do the following:
    // 1) Take a sorted list of GradientPoints
    // 2) Split them into individual clusters - drop ones that don't match some criteria
    // 3) Provide min/max bounding box
    // rand(0);

    // uint64_t gradient_buffer[260];

    // // Create a few clusters
    // {
    //     uint64_t hash = 0x1234 << 32;

    //     for (int i = 0; i < 50; i++) {
    //         GradientPoint p();
    //     }
    // }
}
