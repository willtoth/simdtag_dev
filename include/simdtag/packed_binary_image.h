#pragma once

#include <fmt/format.h>
#include <hwy/highway.h>

#include <algorithm>
#include <bit>
#include <cstdint>
#include <new>
#include <opencv2/core.hpp>

namespace hw = hwy::HWY_NAMESPACE;

namespace simdtag {

HWY_BEFORE_NAMESPACE();
namespace HWY_NAMESPACE {

inline void __ToBinaryAlignedPadded(uint64_t* __restrict dst, const uint8_t* __restrict src,
                                    size_t len_bytes) {
    constexpr hw::ScalableTag<uint8_t> d;
    constexpr int N = hw::Lanes(d);

    uint8_t* ptr = (uint8_t*)dst;

    for (auto i = 0; i < len_bytes; i += N) {
        const auto va = hw::LoadU(d, src + i);  // Can't assume source is aligned
        ptr += hw::StoreMaskBits(d, va != hw::Zero(d), ptr);
    }
}

template <uint8_t COMPARE>
inline void __ToBinaryAlignedPaddedMasked(uint64_t* __restrict dst, const uint8_t* __restrict src,
                                          size_t len_bytes) {
    constexpr hw::ScalableTag<uint8_t> d;
    constexpr int N = hw::Lanes(d);
    static const auto vcompare = hw::Set(d, COMPARE);

    uint8_t* ptr = (uint8_t*)dst;

    for (auto i = 0; i < len_bytes; i += N) {
        // cv::Mat is allocated aligned
        const auto va = hw::Load(d, src + i);
        ptr += hw::StoreMaskBits(d, va == vcompare, ptr);
    }
}

// NOLINTNEXTLINE(google-readability-namespace-comments)
}  // namespace HWY_NAMESPACE
HWY_AFTER_NAMESPACE();

class PackedBinaryImage {
   public:
    PackedBinaryImage(int height, int width) {
        constexpr hw::ScalableTag<uint64_t> d;
        constexpr int N = std::max(hw::Lanes(d), static_cast<size_t>(2));

        height_ = std::max(1, height);
        width_ = std::max(1, width);

        double_word_width_ = width_ / 64 + 1;

        // Make sure to have at least the SIMD width and each row is aligned
        size_t padding = N - (double_word_width_ % N);
        double_word_stride_ = double_word_width_ + padding;

        // Add an extra row if there is an odd number of rows
        int alloc_height = height_ + (height_ & 1);

        bits_ = new (std::align_val_t(64)) uint64_t[alloc_height * double_word_stride_];
    }

    PackedBinaryImage(cv::Mat1b const& image)
        : PackedBinaryImage(image, HWY_NAMESPACE::__ToBinaryAlignedPadded) {
    }

    template <size_t MASK>
    static PackedBinaryImage CreateFromMask(cv::Mat1b const& image) {
        return PackedBinaryImage{image, HWY_NAMESPACE::__ToBinaryAlignedPaddedMasked<MASK>};
    }

    ~PackedBinaryImage() {
        if (bits_) delete[] bits_;
    }

    PackedBinaryImage(const PackedBinaryImage& other) {
        *this = other;
    }

    PackedBinaryImage(PackedBinaryImage&& other) noexcept {
        swap(*this, other);
    }

    PackedBinaryImage& operator=(PackedBinaryImage other) {
        swap(*this, other);
        return *this;
    }

    friend void swap(PackedBinaryImage& first, PackedBinaryImage& second) {
        using std::swap;
        swap(first.bits_, second.bits_);
        swap(first.height_, second.height_);
        swap(first.width_, second.width_);
        swap(first.double_word_stride_, second.double_word_stride_);
        swap(first.double_word_width_, second.double_word_width_);
    }

    void Print() {
        for (int i = 0; i < height_; i++) {
            for (int j = 0; j < double_word_width_; j++) {
                fmt::print("{:064b}", *(Row(i) + j));
            }
            fmt::println("");
        }
    }

    size_t Height() {
        return height_;
    }

    size_t Width() {
        return width_;
    }

    size_t WidthBits() {
        return width_;
    }

    size_t DoubleWordWidth() {
        return double_word_width_;
    }

    size_t DoubleWordStride() {
        return double_word_stride_;
    }

    uint64_t* operator[](uint64_t row) {
        return bits_ + double_word_stride_ * row;
    }

    uint64_t* Row(uint64_t row) {
        return bits_ + double_word_stride_ * row;
    }

    bool Index(size_t row, size_t col) {
        uint64_t mask = 1 << col;
        return (*(Row(row) + (col / 64)) & mask) != 0;
    }

    cv::Mat ToMat() {
        cv::Mat1b result{cv::Size{static_cast<int>(width_), static_cast<int>(height_)}};

        for (int i = 0; i < height_; i++) {
            uint8_t* output_row = result.ptr<uint8_t>(i);
            uint64_t* input_row = Row(i);
            uint64_t mask = 1;
            for (int j = 0; j < width_; j++) {
                *output_row++ = ((*input_row) & mask) == 0 ? 0 : 255;
                mask = std::rotl(mask, 1);
                if (mask == 1) {
                    input_row++;
                }
            }
        }

        return result;
    }

   private:
    template <typename FCN>
    PackedBinaryImage(cv::Mat1b const& image, FCN&& fcn)
        : PackedBinaryImage(image.rows, image.cols) {
        assert(image.isContinuous());
        uint64_t mask = 0xFFFFFFFFFFFFFFFF >> (64 - (width_ % 64));
        for (int i = 0; i < height_; i++) {
            uint64_t* dst = bits_ + double_word_stride_ * i;
            fcn(dst, image.ptr<uint8_t>(i), image.cols);

            // Bits that overhang the 64-bit boundary should have the remaining bits set to 0
            // e.g. a 32px wide image should have the top 32 bits set to 0
            dst[double_word_width_ - 1] &= mask;
        }

        if (height_ & 1) {
            std::memset(bits_ + double_word_stride_ * height_, 0, double_word_width_ * 8);
        }
    }

    PackedBinaryImage() = delete;
    uint64_t* bits_;
    size_t height_;
    size_t width_;

    // Actual length of row in memory referenced to uint64_t (includes padding)
    size_t double_word_stride_;

    // Number of uint64_t a
    size_t double_word_width_;
};

}  // namespace simdtag
