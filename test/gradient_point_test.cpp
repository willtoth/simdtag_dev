#include "gradient_point.h"

#include <gtest/gtest.h>
#include <hwy/highway.h>

#include <cstdint>
#include <cstdlib>

#include "fmt/format.h"

using namespace simdtag;

TEST(GradientPoint, GetSet) {
    GradientPoint pt;
    constexpr int MAX_IMG_SIZE = 2047;
    pt.SetX(MAX_IMG_SIZE);
    pt.SetY(MAX_IMG_SIZE);
    pt.SetDxDy(-1, 1);
    pt.SetBlackToWhite(255, 0);

    EXPECT_EQ(MAX_IMG_SIZE, pt.GetX());
    EXPECT_EQ(MAX_IMG_SIZE, pt.GetY());
    EXPECT_EQ(-1, pt.GetDx());
    EXPECT_EQ(1, pt.GetDy());
    EXPECT_EQ(0, pt.GetBlackToWhite());

    pt.SetY(123);
    EXPECT_EQ(123, pt.GetY());
    EXPECT_EQ(MAX_IMG_SIZE, pt.GetX());

    pt.SetX(123);
    EXPECT_EQ(123, pt.GetX());
    EXPECT_EQ(123, pt.GetY());

    pt.SetDxDy(0, 0);
    pt.SetBlackToWhite(0, 255);
    EXPECT_EQ(0, pt.GetDx());
    EXPECT_EQ(0, pt.GetDy());
    EXPECT_EQ(1, pt.GetBlackToWhite());
}
