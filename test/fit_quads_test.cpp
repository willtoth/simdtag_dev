#include "fit_quads.h"

#include <fmt/format.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <opencv2/opencv.hpp>
#include <vector>

#include "gradient_point.h"

using namespace simdtag;

namespace {

struct pt {
    int x, y;
    double slope;
};

bool operator==(const pt& lhs, const pt& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

}  // namespace

TEST(FitQuads, QuadCenterAndSort) {
    std::srand(0);

    constexpr pt data[] = {
            {.x = 4, .y = 17},  {.x = 5, .y = 12},  {.x = 5, .y = 10},  {.x = 4, .y = 8},
            {.x = 6, .y = 6},   {.x = 8, .y = 6},   {.x = 9, .y = 9},   {.x = 9, .y = 14},
            {.x = 11, .y = 12}, {.x = 11, .y = 16}, {.x = 6, .y = 18},  {.x = 5, .y = 18},
            {.x = 5, .y = 17},  {.x = 5, .y = 16},  {.x = 5, .y = 5},   {.x = 5, .y = 6},
            {.x = 5, .y = 7},   {.x = 12, .y = 5},  {.x = 10, .y = 5},  {.x = 8, .y = 6},
            {.x = 12, .y = 18}, {.x = 11, .y = 17}, {.x = 10, .y = 18}, {.x = 9, .y = 16},
    };
    constexpr int size = sizeof(data) / sizeof(pt);

    std::array<pt, size> expected;
    std::copy(std::begin(data), std::end(data), expected.begin());

    std::vector<uint32_t> points;

    for (int i = 0; i < size; i++) {
        GradientPoint gp;
        gp.SetX(data[i].x);
        gp.SetY(data[i].y);

        points.push_back(gp.RawValue());
    }

    auto center = HWY_NAMESPACE::__FindCenterPoint(points);

    EXPECT_NEAR(center.first, 16.612, 0.5);
    EXPECT_NEAR(center.second, 22.314, 0.5);

    for (int i = 0; i < size; i++) {
        expected[i].slope = std::atan2((double)expected[i].y - (center.second / 2.0),
                                       (double)expected[i].x - (center.first / 2.0));
    }
    std::sort(expected.begin(), expected.end(),
              [](pt point1, pt point2) { return point1.slope < point2.slope; });

    for (auto it = expected.begin(); it != expected.end(); it++) {
        fmt::println("{{{},{},{}}}", it->x, it->y, it->slope);
    }

    HWY_NAMESPACE::__SortBySlope(points, center);

    std::vector<pt> result;

    for (auto it = points.begin(); it != points.end(); it++) {
        GradientPoint gp(*it);

        float x = gp.GetX();
        float y = gp.GetY();

        fmt::println("{{{},{}}}", x, y);

        result.push_back(pt(x, y));
    }

    bool found = false;
    for (int i = 0; i < result.size(); i++) {
        if (result[0] == expected[0]) {
            found = true;
            break;
        }
        std::rotate(expected.begin(), expected.begin() + 1, expected.end());
    }

    EXPECT_TRUE(found);

    bool equal = std::equal(expected.begin(), expected.end(), result.begin());
    EXPECT_TRUE(equal);

    if (!equal) {
        for (int i = 0; i < expected.size(); i++) {
            fmt::println("Expected: {{{},{}}} == Actual: {{{},{}}}", expected[i].x, expected[i].y,
                         result[i].x, result[i].y);
        }
    }

    // for (auto point : result)

    // uint64_t gradient_buffer[260];

    // // Create a few clusters
    // {
    //     uint64_t hash = 0x1234 << 32;

    //     for (int i = 0; i < 50; i++) {
    //         GradientPoint p();
    //     }
    // }
}
