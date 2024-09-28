#pragma once

#define EMH_EXT

#include <fmt/format.h>

#include <new>
#include <opencv2/core.hpp>
#include <vector>

#include "HalideBuffer.h"
#include "halide_gradient_clusters.h"
#include "third_party/emhash/hash_table7.hpp"

// clang-format off

#include <hwy/highway.h>

#define VQSORT_ONLY_STATIC 1
#include <hwy/contrib/sort/vqsort-inl.h>
#include <hwy/contrib/sort/order.h>
#include <hwy/contrib/algo/copy-inl.h>

// clang-format on

using HashMapType = emhash7::HashMap<uint32_t, std::vector<uint32_t>>;

extern "C" int32_t __HashMapInsert(void* hashmap, uint32_t hash, uint32_t value) {
    if (value == 0) {
        return 0;
    }

    HashMapType* m = (HashMapType*)hashmap;
    auto* pvalue = m->try_get(hash);
    if (pvalue) {
        pvalue->push_back(value);
    } else {
        std::vector<uint32_t> storage;
        storage.reserve(2048);
        m->emplace_unique(hash, std::move(storage));
    }
    return 0;
}

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

class GradientClusters {
   private:
    // TODO: This will be a planar layout, experiment with interleaved and linear
    Halide::Runtime::Buffer<uint32_t> sparse_gradient_points_;
    uint64_t* compressed_gradient_points_;
    HashMapType hash_map_{1000000};

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

        int error = halide_gradient_clusters(halide_threshold, halide_labels, &hash_map_,
                                             sparse_gradient_points_);

        // size_t double_words = sparse_gradient_points_.size_in_bytes() / 4;

        // int cnt = HWY_NAMESPACE::__CopyIf(compressed_gradient_points_,
        //                                   sparse_gradient_points_.data(), double_words);

        // hw::VQSortStatic(compressed_gradient_points_, cnt, hwy::SortDescending{});

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
};

}  // namespace simdtag
