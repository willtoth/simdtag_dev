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
#include "simdtag/highway_utils.h"
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

// "Knuth's Multiplicative Hash" same as used by apriltag
inline V64 __SimpleHash(const V64& v64) {
    const hw::DFromV<V64> d;
    const auto vscale = hw::Set(d, 2654435761ull);
    return hw::ShiftRight<32>(v64 * vscale);
}

// Calculate hash for 32-bit label neighbors, reads N labels from each buffer where N
// is the number of SIMD lanes. Returns a vector with the hashes.
inline V32 __CalculateHashes(const uint32_t* labels_A, const uint32_t* labels_B) {
    constexpr hw::ScalableTag<uint32_t> d32;
    constexpr hw::ScalableTag<uint64_t> d64;

    // Do initial calculation as 32-bit
    const auto vrep0 = hw::LoadU(d32, labels_A);
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

// Generate a sequence from START to START + #Lanes
// Type is derived from the D typed passed in
template <int START = 0, class D>
constexpr auto GenerateSequence(D d) {
    constexpr int N = hw::Lanes(d);
    std::array<hw::TFromD<D>, N> result;
    for (int i = 0; i < N; i++) result[i] = i + START;
    return result;
}

// Restricted to 32bit lane size output
template <int DX, int DY>
    requires(DX == 1 && DY == 0) || (DY == 1 && (DX >= -1 || DX <= 1))
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

    const auto v0 = hw::PromoteTo(d, LoadU(d8, img_A));
    const auto v1 = hw::PromoteTo(d, LoadU(d8, img_B));

    const auto vblack_to_white = hw::IfThenElseZero(v1 > v0, hw::Set(d, 1));
    const auto vpx_mask = hw::ShiftLeft<20>(vpx);
    const auto vpy_mask = hw::ShiftLeft<8>(vpy);

    return vpx_mask | vpy_mask | hw::Set(d, dxy_mask) | vblack_to_white;
}

// Create a bitwise mask for lanes which are valid
inline auto __CalculateMask(const uint8_t* img_A, const uint8_t* img_B, const uint32_t* labels_A,
                            const uint32_t* labels_B) {
    // Result is an "and" of the below conditions
    // v0 != 127
    // v0 + v1 == 255
    // rep0.size > 24
    // rep1.size > 24
    // connected_last

    constexpr hw::ScalableTag<uint32_t> d;
    constexpr int N = hw::Lanes(d);
    constexpr hw::FixedTag<uint8_t, N> d8;

    // Small buffer for intermediate calculations
    alignas(64) static uint32_t rep_buffer[N];

    // Load image into 32bit lanes
    const auto v0 = hw::PromoteTo(d, LoadU(d8, img_A));
    const auto v1 = hw::PromoteTo(d, LoadU(d8, img_B));

    auto mres = v0 != hw::Set(d, 127);
    mres = hw::And(mres, ((v0 + v1) == hw::Set(d, 255)));

    const auto vlabelsA = hw::LoadU(d, labels_A);
    const auto vlabelsB = hw::LoadU(d, labels_B);
    auto labels_mask = hw::And(vlabelsA > hw::Zero(d), vlabelsB > hw::Zero(d));
    mres = hw::And(mres, labels_mask);

    // Compiler should unroll automatically
    // TODO: Is endieness backwards here?
    // TODO: This is SLOW here, likely due to cache hit from below lookup
    // TODO: It may end up faster to just eat the cost of not calculating CCL label counts at all,
    // remove it from the CCL algo as well, then filter these out based on bucket size after
    // gradient clusters.
    // for (int i = 0; i < N; i++) {
    //     rep_buffer[i] =
    //             (ccl.GetLabelCount(labels_A[i]) > 24) && (ccl.GetLabelCount(labels_B[i]) > 24) ?
    //             255
    //                                                                                            :
    //                                                                                            0;
    // }
    // mres = hw::And(mres, MaskFromVec(Load(d, rep_buffer)));

    return mres;
}

// Return number of uint64_t written to output
template <int DX, int DY>
    requires(DX == 1 && DY == 0) || (DY == 1 && (DX >= -1 || DX <= 1))
inline auto __CalculateAndStoreGradientVector(const uint8_t* img, const uint8_t* img_row2,
                                              const uint32_t* labels, const uint32_t* labels_row2,
                                              int row, int col, uint64_t* output) {
    constexpr hw::ScalableTag<uint32_t> d;
    constexpr hw::ScalableTag<uint64_t> d64;
    constexpr int N = hw::Lanes(d);
    constexpr int N64 = hw::Lanes(d64);
    constexpr hw::FixedTag<uint32_t, N / 2> dHalf;

    // First pointer is always img or labels, but second pointer depends on DX and DY
    // DY selects between row1 and row2. DX can simply be added to that
    const uint8_t* image_B;
    const uint32_t* labels_B;
    if constexpr (DY == 0) {
        image_B = img;
        labels_B = labels;
    } else {
        image_B = img_row2;
        labels_B = labels_row2;
    }

    // Actual calculations
    const auto vvalues = HWY_NAMESPACE::__CalculateValue<DX, DY>(img, image_B + DX, col, row);
    const auto vhash = HWY_NAMESPACE::__CalculateHashes(labels, labels_B + DX);
    const auto mask = HWY_NAMESPACE::__CalculateMask(img, image_B + DX, labels, labels_B + DX);

    // Convert 4 32bit into two 64 bit (hash << 32 | value)
    const auto vhash64_u = hw::PromoteUpperTo(d64, vhash);
    const auto vhash64_l = hw::PromoteLowerTo(d64, vhash);
    const auto vvalues_u = hw::PromoteUpperTo(d64, vvalues);
    const auto vvalues_l = hw::PromoteLowerTo(d64, vvalues);
    const auto out_u = hw::ShiftLeft<32>(vhash64_u) | vvalues_u;
    const auto out_l = hw::ShiftLeft<32>(vhash64_l) | vvalues_l;

    // The storage locality costs far outweight the cost of computation for this problem
    // so speed up will all come down to memory speed.
    const auto mask_u = hw::PromoteMaskTo(d64, dHalf, UpperHalfOfMask(dHalf, mask));
    const auto mask_l = hw::PromoteMaskTo(d64, dHalf, LowerHalfOfMask(dHalf, mask));

    // if (hw::CountTrue(d, mask)) {
    //     HWY_NAMESPACE::PrintMask(d, "mask>", mask);
    //     HWY_NAMESPACE::PrintMask(d64, "mask_u>", mask_u);
    //     HWY_NAMESPACE::PrintMask(d64, "mask_l>", mask_l);
    // }

    int mask_l_size = hw::CountTrue(d64, mask_l);
    hw::CompressBlendedStore(out_l, mask_l, d64, output);
    hw::CompressBlendedStore(out_u, mask_u, d64, output + mask_l_size);

    return mask_l_size + hw::CountTrue(d64, mask_u);
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
    std::vector<uint64_t> output_;

   public:
    GradientClusters(cv::Size size) : sparse_gradient_points_{size.width, size.height, 4} {
        compressed_gradient_points_ =
                new (std::align_val_t(64)) uint64_t[size.width * size.height * 4];
        output_.reserve(size.width * size.height * 4);
    }

    void Perform(cv::Mat1b& input, cv::Mat1i& labels, BMRS const& connected_components) {
        // TODO: Note this whole function should move into HWY_NAMESPACE above to get lane counts
        constexpr hw::ScalableTag<uint32_t> d;
        constexpr int N = hw::Lanes(d);
        int top = 0;
        for (int r = 0; r < input.rows - 1; r++) {
            uint32_t* pLabels_start = labels.ptr<uint32_t>(r);
            uint32_t* pLabels_next_start = labels.ptr<uint32_t>(r + 1);
            uint8_t* pimg_start = input.ptr<uint8_t>(r);
            uint8_t* pimg_next_start = input.ptr<uint8_t>(r + 1);

            for (int c = 1; c < input.cols - 1; c += N) {
                uint32_t* pLabels = pLabels_start + c;
                uint32_t* pLabels_next = pLabels_next_start + c;
                uint8_t* pimg = pimg_start + c;
                uint8_t* pimg_next = pimg_next_start + c;
                top += HWY_NAMESPACE::__CalculateAndStoreGradientVector<1, 0>(
                        pimg, pimg_next, pLabels, pLabels_next, r, c,
                        compressed_gradient_points_ + top);
                top += HWY_NAMESPACE::__CalculateAndStoreGradientVector<1, 1>(
                        pimg, pimg_next, pLabels, pLabels_next, r, c,
                        compressed_gradient_points_ + top);
                top += HWY_NAMESPACE::__CalculateAndStoreGradientVector<-1, 1>(
                        pimg, pimg_next, pLabels, pLabels_next, r, c,
                        compressed_gradient_points_ + top);
                top += HWY_NAMESPACE::__CalculateAndStoreGradientVector<0, 1>(
                        pimg, pimg_next, pLabels, pLabels_next, r, c,
                        compressed_gradient_points_ + top);
            }
        }
        // fmt::println("{}", top);
        hw::VQSortStatic(compressed_gradient_points_, top, hwy::SortDescending{});
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
