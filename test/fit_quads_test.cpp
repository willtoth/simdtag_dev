#include "fit_quads.h"

#include <fmt/format.h>
#include <gtest/gtest.h>

#include <cstdlib>
#include <opencv2/opencv.hpp>
#include <vector>

#include "gradient_point.h"

using namespace simdtag;

namespace {

struct pt {
    int x, y;
};

}  // namespace

TEST(FitQuads, QuadCenter) {
    std::srand(0);

    pt data[] = {
            {.x = 4, .y = 17},  {.x = 5, .y = 12},  {.x = 5, .y = 10},  {.x = 4, .y = 8},
            {.x = 6, .y = 6},   {.x = 8, .y = 6},   {.x = 9, .y = 9},   {.x = 9, .y = 14},
            {.x = 11, .y = 12}, {.x = 11, .y = 16}, {.x = 6, .y = 18},  {.x = 5, .y = 18},
            {.x = 5, .y = 17},  {.x = 5, .y = 16},  {.x = 5, .y = 5},   {.x = 5, .y = 6},
            {.x = 5, .y = 7},   {.x = 12, .y = 5},  {.x = 10, .y = 5},  {.x = 8, .y = 6},
            {.x = 12, .y = 18}, {.x = 11, .y = 17}, {.x = 10, .y = 18}, {.x = 9, .y = 16},
    };

    int size = sizeof(data) / sizeof(pt);

    std::vector<uint32_t> points;

    for (int i = 0; i < size; i++) {
        GradientPoint gp;
        gp.SetX(data[i].x);
        gp.SetY(data[i].y);

        points.push_back(gp.RawValue());
    }

    auto [cx, cy] = HWY_NAMESPACE::__FindCenterPoint(points);

    EXPECT_NEAR(cx, 16.0512, 0.25);
    EXPECT_NEAR(cy, 22.9714, 0.25);

    fmt::println("X: {}, Y: {}", cx, cy);

    // uint64_t gradient_buffer[260];

    // // Create a few clusters
    // {
    //     uint64_t hash = 0x1234 << 32;

    //     for (int i = 0; i < 50; i++) {
    //         GradientPoint p();
    //     }
    // }
}
