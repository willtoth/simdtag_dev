#include <fmt/format.h>
#include <hwy/cache_control.h>
#include <hwy/highway.h>
#include <hwy/print-inl.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>

namespace hw = hwy::HWY_NAMESPACE;

constexpr int IMG_W = 1200;
constexpr int IMG_H = 1600;
uint8_t IMG[IMG_H][IMG_W];
uint8_t img_compressed[IMG_H / 2][IMG_W];

void ImgInit() {
    srand(0);
    for (int i = 0; i < IMG_H; i++) {
        for (int j = 0; j < IMG_W; j++) {
            IMG[i][j] = (rand() % 100) > 60;
        }
    }
}

HWY_BEFORE_NAMESPACE();
namespace project {
namespace HWY_NAMESPACE {

template <size_t W>
void test(int row) {
    constexpr hw::ScalableTag<uint8_t> d;
    constexpr int N = hw::Lanes(d);
    constexpr size_t count = W / N;
    int idx = 0;
    fmt::println("Lanes: {}", N);

    for (size_t i = 0; i < count; i += N) {
        const auto va = hw::Load(d, IMG[row] + i);
        const auto vb = hw::Load(d, IMG[row + 1] + i);

        hw::Store(va | vb, d, img_compressed[row / 2] + idx);
        idx += N;
    }
}

template <size_t N>
static constexpr std::array<uint8_t, N> getIncrementingArray() {
    std::array<uint8_t, N> result{};
    for (auto i = 0; i < result.size(); i++) {
        result[i] = i;
    }
    return result;
}

void compress_test() {
    constexpr hw::ScalableTag<uint8_t> d;
    constexpr int N = hw::Lanes(d);
    constexpr auto incrementingValues = getIncrementingArray<N>();

    const auto va = hw::Load(d, img_compressed[0]);
    const auto vb = hw::Load(d, img_compressed[0]);
    const auto vIncrementing = hw::Load(d, incrementingValues.data());
    const auto vresult = hw::Compress(vIncrementing, (vb ^ va) != hw::Zero(d));
    hw::Print(d, "Compressed: ", (va));
    hw::Print(d, "Compressed: ", (vb));
}

void or_test() {
    constexpr hw::ScalableTag<uint16_t> d;
    constexpr uint16_t a[] = {1, 2, 3, 4, 5, 6, 7, 8};

    const auto va = hw::Load(d, a);
    const auto vb = hw::Set(d, 1);

    const auto result = hw::Or(va, vb);
    hw::Print(d, "A: ", (va));
    hw::Print(d, "B: ", (vb));
    hw::Print(d, "Or", result);
}

template <size_t LEN>
void ArrayToBinary(uint64_t* __restrict dst, const uint8_t* __restrict src) {
    constexpr hw::ScalableTag<uint8_t> d;
    constexpr int N = hw::Lanes(d);
    static_assert(LEN % 64 == 0);

    uint8_t* workingPtr = (uint8_t*)dst;

    for (auto i = 0; i < LEN; i += N) {
        const auto va = hw::Load(d, src + i);
        workingPtr += hw::StoreMaskBits(d, va != hw::Zero(d), workingPtr);
    }
}

void TestSlide1Down() {
    constexpr hw::ScalableTag<uint64_t> d;
    constexpr int N = hw::Lanes(d);
    uint64_t values[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    const auto va = hw::Load(d, values);
    hw::Print(d, "Before: ", va);
    hw::Print(d, "After: ", hw::Slide1Down(d, va));
}

void TestUpperLower() {
    constexpr hw::ScalableTag<uint32_t> d;
    constexpr int N = hw::Lanes(d);
    constexpr hw::FixedTag<uint32_t, N / 2> d_half;
    constexpr hw::ScalableTag<uint64_t> d_64;
    constexpr int N_64 = hw::Lanes(d_64);

    uint32_t values[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    const auto va = hw::Load(d, values);
    const auto vresize = hw::PromoteLowerTo(d_64, va);
    const auto vuresize = hw::PromoteUpperTo(d_64, va);
    hw::Print(d, "Before: ", va, 0, N);
    hw::Print(d_64, "After (Resize Lower): ", vresize, 0, N_64);
    hw::Print(d_64, "After (Resize Upper): ", vuresize, 0, N_64);
    //  hw::Print(d, "After (Upper 32): ", hw::UpperHalf(d, va));
    //  hw::Print(d_64, "After (Upper 64): ", va2);

    const auto cast = hw::OrderedTruncate2To(d, vresize, vuresize);
    hw::Print(d, "OrderedTruncate2To: ", cast, 0, N);
}

void TestU8ToU32() {
    constexpr hw::ScalableTag<uint32_t> d;
    constexpr int N = hw::Lanes(d);
    constexpr hw::FixedTag<uint8_t, N> d8;

    uint8_t values[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
    const auto va = hw::Load(d8, values);

    hw::Print(d8, "Before: ", va, 0, N);

    const auto cast = hw::PromoteTo(d, va);
    hw::Print(d, "Promoted: ", cast, 0, N);
}

void CompressStore() {
    constexpr hw::ScalableTag<uint64_t> d;
    constexpr int N = hw::Lanes(d);
    alignas(64) uint64_t maskbuffer[N] = {255, 255, 0, 255, 0, 0, 0, 255};
    alignas(64) uint64_t inbuffer[N] = {21, 22, 23, 24, 25, 26, 27, 28};
    alignas(64) uint64_t outbuffer[N + 1];
    for (int i = 0; i < N + 1; i++) {
        outbuffer[i] = 1;
    }

    const auto va = hw::LoadU(d, inbuffer);
    const auto mask = hw::LoadU(d, maskbuffer) > hw::Zero(d);
    const auto vcompress = hw::Compress(va, mask);
    hw::Stream(vcompress, d, outbuffer);
    // hwy::FlushStream();

    hw::Print(d, "va: ", va, 0, N);
    hw::Print(d, "vcompress: ", vcompress, 0, N);

    for (int i = 0; i < N + 1; i++) {
        fmt::println("{}", outbuffer[i]);
    }
}

// NOLINTNEXTLINE(google-readability-namespace-comments)
}  // namespace HWY_NAMESPACE
}  // namespace project
HWY_AFTER_NAMESPACE();

int main() {
    fmt::println("Hello");

    project::HWY_NAMESPACE::CompressStore();

    return 0;

    ImgInit();

    for (int i = 0; i < 32; i++) {
        fmt::print("{}", IMG[0][i]);
    }
    fmt::println("");

    for (int i = 0; i < 32; i++) {
        fmt::print("{}", IMG[1][i]);
    }
    fmt::println("");

    project::HWY_NAMESPACE::test<IMG_W>(0);

    for (int i = 0; i < 32; i++) {
        fmt::print("{}", img_compressed[0][i]);
    }

    fmt::println("");
    project::HWY_NAMESPACE::compress_test();

    fmt::println("BinaryImage:");

    uint64_t binaryImage[IMG_H][IMG_W / 8];
    project::HWY_NAMESPACE::ArrayToBinary<IMG_H * IMG_W>(&binaryImage[0][0], &IMG[0][0]);
    fmt::println("{:64b}", binaryImage[0][0]);

    project::HWY_NAMESPACE::or_test();

    return 0;
}