#pragma once

#include <fmt/format.h>
#include <hwy/aligned_allocator.h>
#include <hwy/contrib/algo/copy-inl.h>
#include <hwy/highway.h>

#include <algorithm>
#include <cstdint>
#include <new>
#include <opencv2/core.hpp>

namespace hw = hwy::HWY_NAMESPACE;

namespace apriltag {

HWY_BEFORE_NAMESPACE();
namespace HWY_NAMESPACE {

inline void __ToBinaryAlignedPadded(uint64_t* __restrict dst, const uint8_t* __restrict src,
                                    size_t src_len) {
    constexpr hw::ScalableTag<uint8_t> d;
    constexpr int N = hw::Lanes(d);

    uint8_t* ptr = (uint8_t*)dst;

    for (auto i = 0; i < src_len; i += N) {
        const auto va = hw::LoadU(d, src + i);  // Can't assume source is aligned
        ptr += hw::StoreMaskBits(d, va != hw::Zero(d), ptr);
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

        bits_ = new (std::align_val_t(64)) uint64_t[height_ * double_word_stride_];
        // bits_ = (uint64_t*)hwy::AllocateAlignedBytes(height_ * double_word_stride_ * 8);
    }

    PackedBinaryImage(cv::Mat1b const& image) : PackedBinaryImage(image.rows, image.cols) {
        uint64_t mask = 0xFFFFFFFFFFFFFFFF >> (64 - (width_ % 64));
        for (int i = 0; i < height_; i++) {
            uint64_t* dst = bits_ + double_word_stride_ * i;
            HWY_NAMESPACE::__ToBinaryAlignedPadded(dst, image.ptr<uint8_t>(i), double_word_width_);

            // Bits that overhang the 64-bit boundary should have the remaining bits set to 0
            // e.g. a 32px wide image should have the top 32 bits set to 0
            dst[double_word_width_ - 1] &= mask;
        }
    }

    ~PackedBinaryImage() {
        if (bits_) delete[] bits_;
        // if (bits_) hwy::FreeAlignedBytes(bits_, nullptr, nullptr);
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

   private:
    PackedBinaryImage() = delete;
    uint64_t* bits_;
    size_t height_;
    size_t width_;

    // Actual length of row in memory referenced to uint64_t (includes padding)
    size_t double_word_stride_;

    // Number of uint64_t a
    size_t double_word_width_;
};

}  // namespace apriltag
