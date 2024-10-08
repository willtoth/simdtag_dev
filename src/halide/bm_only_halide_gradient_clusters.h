#pragma once

#include <fmt/format.h>

#include <array>
#include <new>
#include <opencv2/core.hpp>
#include <string>
#include <vector>

#include "HalideBuffer.h"
#include "halide_gradient_clusters.h"
#include "simdtag/highway_utils.h"

// clang-format off

#include <hwy/highway.h>

#define VQSORT_ONLY_STATIC 1
#include <hwy/contrib/sort/vqsort-inl.h>
#include <hwy/contrib/sort/order.h>
#include <hwy/contrib/algo/copy-inl.h>

// clang-format on

namespace hw = hwy::HWY_NAMESPACE;

namespace simdtag {

HWY_BEFORE_NAMESPACE();
namespace HWY_NAMESPACE {

inline int __CopyIf(uint64_t* __restrict dst, const uint64_t* __restrict src, size_t count) {
    constexpr hw::ScalableTag<uint64_t> d;
    constexpr int N = hw::Lanes(d);

    // uint64_t* end = hw::CopyIf(d, src, double_words, dst,
    //                            [](const auto d, const auto v) {
    //                             return v == hw::Zero(d);
    //                             });
    uint64_t* __restrict ptr = dst;
    size_t idx = 0;
    if (count >= N) {
        for (; idx <= count - N; idx += N) {
            auto v = Load(d, src + idx);
            ptr += CompressBlendedStore(v, v != hw::Zero(d), d, ptr);
        }
    }

    return (int)(ptr - dst);
}

}  // namespace HWY_NAMESPACE
HWY_AFTER_NAMESPACE();

class HalideGradientClusters {
   private:
    // TODO: This will be a planar layout, experiment with interleaved and linear
    Halide::Runtime::Buffer<uint64_t> sparse_gradient_points_;
    uint64_t* compressed_gradient_points_;
    int points_;
    cv::Size size_;

   public:
    HalideGradientClusters(cv::Size size)
        : sparse_gradient_points_{size.width, size.height, 4}, size_(size) {
        compressed_gradient_points_ =
                new (std::align_val_t(64)) uint64_t[size.width * size.height * 4];
    }

    void Print(int cols = 4) {
        for (int i = 0; i < points_; i++) {
            uint64_t val = compressed_gradient_points_[i];
            uint32_t hash = val >> 32;
            GradientPoint p{static_cast<uint32_t>(val)};
            fmt::print("{} - x:{} y:{} ", hash, p.GetX(), p.GetY());

            if (i % cols == (cols - 1)) {
                fmt::println("");
            }
        }
    }

    cv::Mat1b Draw() {
        cv::Mat1b result = cv::Mat::zeros(size_, CV_8UC1);

        for (int i = 0; i < points_; i++) {
            GradientPoint p{static_cast<uint32_t>(compressed_gradient_points_[i])};
            int y = (int)p.GetY();
            int x = (int)p.GetX();
            if (y < 0 || y >= size_.height || x < 0 || x >= size_.width) {
                fmt::println("Out-of-bounds access: (y, x) = ({},{}", y, x);
                continue;  // Skip this point to avoid crashing
            }

            assert((int)p.GetY() < size_.height);
            assert((int)p.GetX() < size_.width);
            result.at<uint8_t>((int)p.GetY(), (int)p.GetX()) += 255 >> 2;
        }

        return result;
    }

    void Perform(cv::Mat1b const& input, cv::Mat1i& labels) {
        Halide::Runtime::Buffer<uint8_t> halide_threshold =
                Halide::Runtime::Buffer<uint8_t>::make_interleaved(input.data, input.cols,
                                                                   input.rows, input.channels());

        Halide::Runtime::Buffer<int> halide_labels = Halide::Runtime::Buffer<int>::make_interleaved(
                (int*)labels.data, labels.cols, labels.rows, labels.channels());

        int error =
                halide_gradient_clusters(halide_threshold, halide_labels, sparse_gradient_points_);

        size_t double_words = sparse_gradient_points_.size_in_bytes() / 8;

        int cnt = HWY_NAMESPACE::__CopyIf(compressed_gradient_points_,
                                          sparse_gradient_points_.data(), double_words);

        hw::VQSortStatic(compressed_gradient_points_, cnt, hwy::SortDescending{});

        [[unlikely]]
        if (error) {
            fmt::println("Halide returned an error: %d\n", error);
            return;
        }

        // for (auto& k : hash_map_) {
        //     fmt::print("{}: ", k.first);

        //     for (auto& value : k.second) {
        //         int x = (value >> 20) & 0x7FF;
        //         int y = (value >> 8) & 0x7FF;
        //         int dir = value & 3;
        //         fmt::print("{{{}x{} : {} }} ", x, y, dir);
        //     }
        //     fmt::println("");
        // }

        // sparse_gradient_points_.for_each_value([](uint32_t& value) {
        //     if (value == 0) {
        //         return;
        //     }
        //     int x = (value >> 20) & 0x7FF;
        //     int y = (value >> 8) & 0x7FF;
        //     int dir = value & 3;
        //     fmt::print("{{{}x{} : {} }} ", x, y, dir);
        // });
    }

    int Size() {
        return points_;
    }

    uint64_t* GetBuffer() {
        return compressed_gradient_points_;
    }
};

}  // namespace simdtag
