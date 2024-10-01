#include "gradient_clusters.h"

#include <gtest/gtest.h>
#include <stdint.h>

#include <cstdlib>

#include "hwy/highway.h"
#include "simdtag/highway_utils.h"

namespace hw = hwy::HWY_NAMESPACE;

using namespace simdtag;

// HWY_BEFORE_NAMESPACE();
// namespace HWY_NAMESPACE {}  // namespace HWY_NAMESPACE
// HWY_AFTER_NAMESPACE();

TEST(GradientClusters, SimdHash) {
    constexpr hw::ScalableTag<uint32_t> d;
    constexpr int N = hw::Lanes(d);

    alignas(64) uint32_t labels[] = {0xFAFAFAFA, 0xABABABAB, 0x12345678, 0x00000000, 0xFFFFFFFF,
                                     0x10101010, 0xAAAAAAAA, 0xBBBBBBBB, 0x0000FFFF, 0xFFFF0000,
                                     0x11001100, 0x23232323, 0x01230123, 0x11111111, 0x22222222,
                                     0x33333333, 0x44444444, 0x55555555};

    alignas(64) uint32_t expected[] = {0xdcafce5b, 0x60efa868, 0xb403f43,  0x9e3779b0, 0xa45a24c0,
                                       0x6f9cfc30, 0xb536e5fa, 0x4f8012bf, 0x79b061c8, 0xe4b9c79,
                                       0x85cc0df0, 0x42f9a123, 0x37ce8f83, 0x1b9d4c61, 0x2cae5d72,
                                       0x3dbf6e83, 0x4ed07f94};

    alignas(64) uint32_t result[sizeof(labels)];

    const auto vresult = HWY_NAMESPACE::__CalculateHashes(labels, labels + 1);
    hw::Store(vresult, d, result);

    for (int i = 0; i < N; i++) {
        EXPECT_EQ(expected[i], result[i]);
    }
}

// TODO: Make a nicer class for this in the library
class GradientPoint {
   public:
    GradientPoint(uint32_t value) : value_(value) {
    }

    float GetX() {
        return static_cast<float>(value_ >> 20) / 2.0f;
    }

    float GetY() {
        return static_cast<float>((value_ & 0x000FFF00u) >> 8) / 2.0f;
    }

    int GetDx() {
        switch (GradientValue()) {
            case 0:
            case 1:
                return 1;
            case 3:
                return -1;
            default:
                return 0;
        }
    }

    int GetDy() {
        return GradientValue() != 0;
    }

    bool GetBlackToWhite() {
        return value_ & 1;
    }

    int GradientValue() {
        return static_cast<int>((value_ & 0x6u) >> 1);
    }

   private:
    uint32_t value_;
};

int GetGradientValue(int DX, int DY) {
    if (DY == 1) {
        switch (DX) {
            case -1:
                return 3;
            case 0:
                return 2;
            default:
                return 1;
        }
    } else {
        return 0;
    }
}

template <int DX, int DY>
void RunSimdValueCalculations() {
    // Test by 32bit
    constexpr hw::ScalableTag<uint32_t> d;
    constexpr int N = hw::Lanes(d);

    alignas(64) uint8_t image[64];

    std::srand(0);
    for (int i = 0; i < 64; i++) {
        image[i] = (std::rand() % 10) > 7 ? 0xFF : 0x00;
    }

    alignas(64) uint32_t result[sizeof(image)];

    const auto vresult = HWY_NAMESPACE::__CalculateValue<DX, DY>(image, image + 1, 0, 0);
    hw::Store(vresult, d, result);

    int x = 0, y = 0;
    for (int i = 0; i < N; i++) {
        // Edge case has gibberish here!! (by design, responsibility is the caller's)
        if (DX == -1 && i == 0) {
            x += std::abs(DX);
            y += DY;
            continue;
        }

        GradientPoint p(result[i]);
        EXPECT_EQ(x + 0.5f * DX, p.GetX());
        EXPECT_EQ(y + 0.5f * DY, p.GetY());
        EXPECT_EQ(GetGradientValue(DX, DY), p.GradientValue());
        EXPECT_EQ(DX, p.GetDx());
        EXPECT_EQ(DY, p.GetDy());
        x += std::abs(DX);
        y += DY;
    }

    // Make sure the next load doesn't crash or anything (e.g. alignment)
    const auto vresult2 = HWY_NAMESPACE::__CalculateValue<1, 0>(image + N, image + N + 1, 0, 0);
}

TEST(GradientClusters, SimdValueCalculations) {
    RunSimdValueCalculations<-1, 1>();
    RunSimdValueCalculations<0, 1>();
    RunSimdValueCalculations<1, 1>();
    RunSimdValueCalculations<1, 0>();
}
