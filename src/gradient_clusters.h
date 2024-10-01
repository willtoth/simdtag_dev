#pragma once

#define EMH_EXT

#include <fmt/format.h>

#include <array>
#include <new>
#include <opencv2/core.hpp>
#include <vector>

#include "HalideBuffer.h"
#include "ccl/bmrs.h"
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

extern "C" uint32_t __HashMapInsert(void* hashmap, uint32_t hash, uint32_t value) {
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

using V32 = hw::VFromD<hw::ScalableTag<uint32_t>>;
using V64 = hw::VFromD<hw::ScalableTag<uint64_t>>;

// Same hash used by apriltag
inline V64 __SimpleHash(const V64& v64) {
    const hw::DFromV<V64> d;
    const auto vscale = hw::Set(d, 2654435761ull);
    return hw::ShiftRight<32>(v64 * vscale);
}

// labels_A must be aligned, labels_B does not
inline V32 __CalculateHashes(const uint32_t* labels_A, const uint32_t* labels_B) {
    constexpr hw::ScalableTag<uint32_t> d32;
    constexpr hw::ScalableTag<uint64_t> d64;

    // Do initial calculation as 32-bit
    const auto vrep0 = hw::Load(d32, labels_A);
    const auto vrep1 = hw::LoadU(d32, labels_B);

    const auto vrepmin = hw::Min(vrep0, vrep1);
    const auto vrepmax = hw::Max(vrep0, vrep1);

    // Convert to 64-bit, hash, then back to 32-bit
    const auto vrepmin_64_u = hw::PromoteUpperTo(d64, vrepmin);
    const auto vrepmin_64_l = hw::PromoteLowerTo(d64, vrepmin);
    const auto vrepmax_64_u = hw::PromoteUpperTo(d64, vrepmax);
    const auto vrepmax_64_l = hw::PromoteLowerTo(d64, vrepmax);
    const auto vrep_u = __SimpleHash(hw::ShiftLeft<32>(vrepmin_64_u) | vrepmax_64_u);
    const auto vrep_l = __SimpleHash(hw::ShiftLeft<32>(vrepmin_64_l) | vrepmax_64_l);

    return hw::OrderedTruncate2To(d32, vrep_l, vrep_u);
}

template <int START = 0, class D>
constexpr auto GenerateSequence(D d) {
    constexpr int N = hw::Lanes(d);
    std::array<hw::TFromD<D>, N> result;
    for (int i = 0; i < N; i++) result[i] = i + START;
    return result;
}

// Restricted to 32bit lane size output
template <int DX, int DY>
    requires(DX >= -1 && DX <= 1) && (DY == 0 || DY == 1)
inline V32 __CalculateValue(const uint8_t* img_A, const uint8_t* img_B, int x, int y) {
    constexpr hw::ScalableTag<uint32_t> d;
    constexpr int N = hw::Lanes(d);
    constexpr hw::FixedTag<uint8_t, N> d8;

    constexpr auto kSequenceBuffer = GenerateSequence(d);

    constexpr uint32_t dxy_mask = static_cast<uint32_t>(-(DX - 1) + DY) << 1;

    const auto vSequenceX = [&]() constexpr {
        if constexpr (DX == 0) {
            return hw::Zero(d);
        } else {
            return hw::Load(d, kSequenceBuffer.data());
        }
    }();

    const auto vSequenceY = [&]() constexpr {
        if constexpr (DY == 1) {
            return hw::Load(d, kSequenceBuffer.data());
        } else {
            return hw::Zero(d);
        }
    }();

    // 2 * x + dx & 0x0FFF
    // 12 bits total, 2x that, so largert image is 2047 x 2047
    const auto vpx = (hw::ShiftLeft<1>(Set(d, x) + vSequenceX) + Set(d, DX)) & Set(d, 0x0FFF);
    const auto vpy = (hw::ShiftLeft<1>(Set(d, y) + vSequenceY) + Set(d, DY)) & Set(d, 0x0FFF);

    const auto v0 = hw::PromoteTo(d, Load(d8, img_A));
    const auto v1 = hw::PromoteTo(d, LoadU(d8, img_B));

    const auto vblack_to_white = hw::IfThenElseZero(v1 > v0, hw::Set(d, 1));
    const auto vpx_mask = hw::ShiftLeft<20>(vpx);
    const auto vpy_mask = hw::ShiftLeft<8>(vpy);

    return vpx_mask | vpy_mask | hw::Set(d, dxy_mask) | vblack_to_white;
}

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
    Halide::Runtime::Buffer<uint64_t> sparse_gradient_points_;
    uint64_t* compressed_gradient_points_;
    HashMapType hash_map_{1000000};

   public:
    GradientClusters(cv::Size size) : sparse_gradient_points_{size.width, size.height, 4} {
        compressed_gradient_points_ =
                new (std::align_val_t(64)) uint64_t[size.width * size.height * 4];
    }

    void Perform(cv::Mat1b const& input, cv::Mat1i& labels, BMRS const& connected_components) {
        for (int i = 0; i < input.rows; i++) {
        }
    }

    void PerformHalide(cv::Mat1b const& input, cv::Mat1i& labels,
                       BMRS const& connected_components) {
        Halide::Runtime::Buffer<uint8_t> halide_threshold =
                Halide::Runtime::Buffer<uint8_t>::make_interleaved(input.data, input.cols,
                                                                   input.rows, input.channels());

        Halide::Runtime::Buffer<int> halide_labels = Halide::Runtime::Buffer<int>::make_interleaved(
                (int*)labels.data, labels.cols, labels.rows, labels.channels());

        int error = halide_gradient_clusters(halide_threshold, halide_labels, &hash_map_,
                                             sparse_gradient_points_);

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
};

}  // namespace simdtag
