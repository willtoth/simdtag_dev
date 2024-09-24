#pragma once

#include <fmt/format.h>

#include <new>
#include <opencv2/core.hpp>

#include "HalideBuffer.h"
#include "halide_gradient_clusters.h"

// clang-format off

#include <hwy/highway.h>

#define VQSORT_ONLY_STATIC 1
#include <hwy/contrib/sort/vqsort-inl.h>
#include <hwy/contrib/sort/order.h>
#include <hwy/contrib/algo/copy-inl.h>

// clang-format on

static simdtag::BMRS const* __ccl = nullptr;

extern "C" uint32_t __CCL_GetLabelCount(uint32_t x) {
    return __ccl->GetLabelCount(x);
}

namespace hw = hwy::HWY_NAMESPACE;

namespace simdtag {

HWY_BEFORE_NAMESPACE();
namespace HWY_NAMESPACE {

inline int __CopyIf(uint64_t* __restrict dst, const uint64_t* __restrict src, size_t double_words) {
    constexpr hw::ScalableTag<uint64_t> d;
    constexpr int N = hw::Lanes(d);

    uint64_t* end = hw::CopyIf(d, src, double_words, dst,
                               [](const auto d, const auto v) { return v == hw::Zero(d); });

    return (int)(end - dst);
}

}  // namespace HWY_NAMESPACE
HWY_AFTER_NAMESPACE();

class GradientClusters {
   private:
    // TODO: This will be a planar layout, experiment with interleaved and linear
    Halide::Runtime::Buffer<uint64_t> sparse_gradient_points_;
    uint64_t* compressed_gradient_points_;

   public:
    GradientClusters(cv::Size size) : sparse_gradient_points_{size.width, size.height, 4} {
        compressed_gradient_points_ =
                new (std::align_val_t(64)) uint64_t[size.width * size.height * 4];
    }

    void Perform(cv::Mat1b const& input, cv::Mat1i const& labels,
                 BMRS const& connected_components) {
        Halide::Runtime::Buffer<uint8_t> halide_threshold =
                Halide::Runtime::Buffer<uint8_t>::make_interleaved(input.data, input.cols,
                                                                   input.rows, input.channels());

        Halide::Runtime::Buffer<int> halide_labels = Halide::Runtime::Buffer<int>::make_interleaved(
                (int*)labels.data, labels.cols, labels.rows, labels.channels());

        __ccl = &connected_components;

        int error =
                halide_gradient_clusters(halide_threshold, halide_labels, sparse_gradient_points_);

        volatile size_t double_words = sparse_gradient_points_.size_in_bytes() / 8;

        // int cnt = HWY_NAMESPACE::__CopyIf(compressed_gradient_points_,
        //                                   sparse_gradient_points_.data(), double_words);

        // hw::VQSortStatic(sparse_gradient_points_.data(), double_words, hwy::SortAscending{});

        [[unlikely]]
        if (error) {
            fmt::println("Halide returned an error: %d\n", error);
            return;
        }

        // sparse_gradient_points_.for_each_value([](uint64_t& value) {
        //     if (value == 0) {
        //         return;
        //     }
        //     int rep0 = value >> 44;
        //     int rep1 = (value >> 24) & 0xFFFFF;
        //     int x = value >> 13 & 0x7FF;
        //     int y = value >> 2 & 0x7FF;
        //     int dir = value & 3;
        //     fmt::print("{{{}-{}, {}x{} : {} }} ", rep0, rep1, x, y, dir);
        // });
    }
};

}  // namespace simdtag
