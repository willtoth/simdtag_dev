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

// Generate a single row of pixels with simd 8x64
// Values are 45% 0xFF 45% 0x00, 10% 127
void GenerateThresholdImage8x64(uint8_t image[64], int seed = 0) {
    std::srand(seed);
    for (int i = 0; i < 64; i++) {
        image[i] = std::rand() % 100;

        if (image[i] < 45) {
            image[i] = 0xFF;
        } else if (image[i] < 90) {
            image[i] = 0;
        } else {
            image[i] = 127;
        }
    }
}

int GenerateLabelsFromThreshold(uint8_t image[64], uint32_t labels[64], int start_label) {
    for (int i = 0; i < 64; i++) {
        if (image[i] == 127) {
            labels[i] = 0;
        } else {
            labels[i] = start_label++;
        }
    }
    return start_label;
}

template <int DX, int DY>
void RunSimdValueCalculations() {
    // Test by 32bit
    constexpr hw::ScalableTag<uint32_t> d;
    constexpr int N = hw::Lanes(d);

    alignas(64) uint8_t image[64];
    GenerateThresholdImage8x64(image);

    alignas(64) uint32_t result[sizeof(image)];

    const auto vresult = HWY_NAMESPACE::__CalculateValue<DX, DY>(image, image + 1, 0, 0);
    hw::Store(vresult, d, result);

    int x = 0;
    for (int i = 0; i < N; i++) {
        // Edge case has gibberish here!! (by design, responsibility is the caller's)
        if (DX == -1 && i == 0) {
            x++;
            continue;
        }

        GradientPoint p(result[i]);
        EXPECT_EQ(x + 0.5f * DX, p.GetX());
        EXPECT_EQ(DY * 0.5, p.GetY());
        EXPECT_EQ(GetGradientValue(DX, DY), p.GradientValue());
        EXPECT_EQ(DX, p.GetDx());
        EXPECT_EQ(DY, p.GetDy());
        x++;
    }

    // Make sure the next load doesn't crash or anything (e.g. alignment)
    const auto vresult2 = HWY_NAMESPACE::__CalculateValue<DX, DY>(image + N, image + N + 1, 0, 0);
}

TEST(GradientClusters, SimdValueCalculations) {
    RunSimdValueCalculations<-1, 1>();
    RunSimdValueCalculations<0, 1>();
    RunSimdValueCalculations<1, 1>();
    RunSimdValueCalculations<1, 0>();
}

TEST(GradientClusters, SimdMaskCalculation) {
    // Test by 32bit
    constexpr hw::ScalableTag<uint32_t> d;
    constexpr int N = hw::Lanes(d);

    alignas(64) uint8_t image_A[64], image_B[64];
    GenerateThresholdImage8x64(image_A);
    GenerateThresholdImage8x64(image_B, 123);

    alignas(64) uint64_t result;
    const auto mask = HWY_NAMESPACE::__CalculateMask(image_A, image_B, nullptr, nullptr, 100);
    hw::StoreMaskBits(d, mask, (uint8_t*)&result);

    // Make sure each case is _actually_ hit in the test data...
    int cnt_127 = 0, cnt_255 = 0;
    for (int i = 0; i < N; i++) {
        cnt_127 += (image_A[i] != 127);
        cnt_255 += (image_A[i] + image_B[i]) == 255;
        bool expected = (image_A[i] != 127) && (image_A[i] + image_B[i]) == 255;
        bool actual = result & (0x1 << i);
        // fmt::println("{}, {} --> expected: {}, Actual: {}", image_A[i], image_B[i], expected,
        //              actual);
        EXPECT_EQ(expected, actual);
    }

    EXPECT_TRUE(cnt_127 > 0);
    EXPECT_TRUE(cnt_255 > 0);

    // For now this is only
    // v0 != 127
    // v0 + v1 == 255
}

TEST(GradientClusters, SimdGradientClustersCalculations) {
    // Test by 32bit
    constexpr hw::ScalableTag<uint32_t> d;
    constexpr int N = hw::Lanes(d);

    uint8_t image_A[64], image_B[64];
    uint32_t labels_A[64], labels_B[64];
    GenerateThresholdImage8x64(image_A);
    GenerateThresholdImage8x64(image_B, 123);
    GenerateLabelsFromThreshold(image_A, labels_A, 100);
    GenerateLabelsFromThreshold(image_B, labels_B, 500);

    uint64_t result[sizeof(image_A) * 2];

    int written = HWY_NAMESPACE::__CalculateAndStoreGradientVector<0, 1>(
            image_A, image_B, labels_A, labels_B, 0, 0, 100, result);

    int idx = 0;
    for (int i = 0; i < N; i++) {
        if ((image_A[i] != 127) && (image_A[i] + image_B[i]) == 255) {
            uint64_t value = result[idx];
            uint64_t repl = std::max(labels_B[i], labels_A[i]);
            uint64_t reph = std::min(labels_B[i], labels_A[i]);
            uint32_t rep = (((repl | reph << 32) * 2654435761ull) >> 32);
            EXPECT_EQ(rep, value >> 32);
            idx++;
        }
    }

    EXPECT_EQ(written, idx);
}
