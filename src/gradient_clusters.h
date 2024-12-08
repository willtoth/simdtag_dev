#pragma once

#define EMH_EXT

#include <fmt/format.h>
#include <hwy/highway.h>

#include <array>
#include <new>
#include <opencv2/core.hpp>
#include <string>
#include <vector>

#include "gradient_point.h"
#include "simdtag/highway_utils.h"
#include "third_party/emhash/hash_table5.hpp"

namespace hw = hwy::HWY_NAMESPACE;

namespace simdtag {

using ClusterStore = std::vector<uint32_t>;
using GradientClusterHash = emhash5::HashMap<uint32_t, ClusterStore>;

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
// TODO: Refactor SIMD 32-bit gradient point creation by moving it to gradient_point.h
template <int DX, int DY>
    requires(DX == 1 && DY == 0) || (DY == 1 && (DX >= -1 || DX <= 1))
inline V32 __CalculateValue(const uint8_t* img_A, const uint8_t* img_B, int x, int y) {
    constexpr hw::ScalableTag<uint32_t> d;
    constexpr int N = hw::Lanes(d);
    constexpr hw::FixedTag<uint8_t, N> d8;

    constexpr auto kSequenceBuffer = GenerateSequence(d);

    // Bits
    constexpr uint32_t dxy_mask = static_cast<uint32_t>(((DX + 1) << 2) + (DY + 1)) << 4;

    const auto vSequenceX = hw::LoadU(d, kSequenceBuffer.data());

    // 2 * x + dx & 0x0FFF x is x + (i) for simd width
    // 12 bits total, 2x that, so largert image is 2047 x 2047
    const auto vpx = (hw::ShiftLeft<1>(Set(d, x) + vSequenceX) + Set(d, DX)) & Set(d, 0x0FFF);

    // 2 * y + dy & 0x0FFF y is y + 0 or 1
    const auto vpy = (hw::ShiftLeft<1>(Set(d, y)) + Set(d, DY)) & Set(d, 0x0FFF);

    const auto v0 = hw::PromoteTo(d, LoadU(d8, img_A));
    const auto v1 = hw::PromoteTo(d, LoadU(d8, img_B));

    const auto vblack_to_white = hw::IfThenElseZero(v1 > v0, hw::Set(d, 1));
    const auto vpx_mask = hw::ShiftLeft<20>(vpx);
    const auto vpy_mask = hw::ShiftLeft<8>(vpy);

    return vpx_mask | vpy_mask | hw::Set(d, dxy_mask) | vblack_to_white;
}

// Create a bitwise mask for lanes which are valid
inline auto __CalculateMask(const uint8_t* img_A, const uint8_t* img_B, const uint32_t* labels_A,
                            const uint32_t* labels_B, int remaining_pixels) {
    // Result is an "and" of the below conditions
    // v0 != 127
    // v0 + v1 == 255
    // rep0.size > 24
    // rep1.size > 24
    // connected_last (dedup)

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
    mres = hw::And(mres, FirstN(d, remaining_pixels));

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
                                              int row, int col, int img_width,
                                              GradientClusterHash& hashmap) {
    constexpr hw::ScalableTag<uint32_t> d;
    constexpr hw::ScalableTag<uint64_t> d64;
    constexpr int N = hw::Lanes(d);
    constexpr int N64 = hw::Lanes(d64);

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
    auto mask = HWY_NAMESPACE::__CalculateMask(img, image_B + DX, labels, labels_B + DX,
                                               img_width - col);

    // Dedup, only for <-1, 1> case
    if constexpr (DX == -1 && DY == 1) {
        // From frc971/orin/apriltag.cc but reworded
        // We search the following 4 neighbors.
        //      ________
        //      | x | 0 |
        //  -------------
        //  | 3 | 2 | 1 |
        //  -------------
        //
        // If connection between block x and block 1 has the same IDs as the connection between
        // blocks 0 and 2, we will have a duplicate entry. This may not be so intuitive at first.
        // The easiest way to see it is this is the exact same point. It may look like they be
        // merged, since the gradient direction could be different. However that is not actually the
        // case, since this duplicate can only happen in two cases. All other cases will be filered
        // out. Apriltag comment says it a bit more clearly (Also thanks Austin)

        // Checking 1, 1 on the previous x, y, and -1, 1 on the current
        // x, y result in duplicate points in the final list.  Only
        // check the potential duplicate if adding this one won't
        // create a duplicate.

        // The cases are
        //       id(x) == id(0) and id(2) == id(1),
        //    or id(x) == id(2) and id(0) == id(1).
        // We need to pick a single element to _not_ add. We could choose block 1, however that
        // would make handling across the last edge harder. Insead, don't add block 3. In that case
        // it becomes
        //       id(x-1) == id(x) and id(2) == id(3),
        //    or id(x-1) == id(3) and id(x) == id(2).
        const auto v_idxm1 = hw::LoadU(d, labels - 1);
        const auto v_idx = hw::LoadU(d, labels);
        const auto v_id2 = hw::LoadU(d, labels_row2);
        const auto v_id3 = hw::LoadU(d, labels_row2 - 1);

        const auto mdup1 = hw::And(v_idxm1 == v_idx, v_id2 == v_id3);
        const auto mdup2 = hw::And(v_idxm1 == v_id3, v_idx == v_id2);
        const auto mdup = hw::Or(mdup1, mdup2);
        mask = hw::AndNot(mdup, mask);
    }

    uint32_t hash_buf[N];
    uint32_t value_buf[N];
    int cnt = hw::CompressStore(vhash, mask, d, hash_buf);
    hw::CompressStore(vvalues, mask, d, value_buf);

    for (int i = 0; i < cnt; i++) {
        ClusterStore* bucket = hashmap.try_get(hash_buf[i]);
        if (bucket == nullptr) {
            ClusterStore tmp;
            tmp.push_back(value_buf[i]);
            hashmap.insert_unique(hash_buf[i], tmp);
        } else {
            bucket->push_back(value_buf[i]);
        }
    }

    return cnt;
}

}  // namespace HWY_NAMESPACE
HWY_AFTER_NAMESPACE();

class GradientClusters {
   private:
    int points_;
    cv::Size size_;

   public:
    GradientClusters(cv::Size size) : size_(size) {
    }

    void Perform(cv::Mat1b& input, cv::Mat1i& labels, GradientClusterHash& hash) {
        constexpr hw::ScalableTag<uint32_t> d;
        constexpr int N = hw::Lanes(d);
        hash.clear();
        int cnt = 0;

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
                cnt += HWY_NAMESPACE::__CalculateAndStoreGradientVector<1, 0>(
                        pimg, pimg_next, pLabels, pLabels_next, r, c, input.cols, hash);
                cnt += HWY_NAMESPACE::__CalculateAndStoreGradientVector<0, 1>(
                        pimg, pimg_next, pLabels, pLabels_next, r, c, input.cols, hash);
                cnt += HWY_NAMESPACE::__CalculateAndStoreGradientVector<1, 1>(
                        pimg, pimg_next, pLabels, pLabels_next, r, c, input.cols, hash);
                cnt += HWY_NAMESPACE::__CalculateAndStoreGradientVector<-1, 1>(
                        pimg, pimg_next, pLabels, pLabels_next, r, c, input.cols, hash);
            }
        }
        points_ = cnt;
    }

#if 0
    void Print(GradientClusterBuffer& gcb, int cols = 4) {
        for (int i = 0; i < points_; i++) {
            uint64_t val = gcb.buffer_[i];
            uint32_t hash = val >> 32;
            GradientPoint p{static_cast<uint32_t>(val)};
            fmt::print("{} - x:{} y:{} ", hash, p.GetX(), p.GetY());

            if (i % cols == (cols - 1)) {
                fmt::println("");
            }
        }
    }
#endif

    cv::Mat1b Draw(GradientClusterHash& hash) {
        cv::Mat1b result = cv::Mat::zeros(size_, CV_8UC1);

        for (auto vit = hash.cbegin(); vit != hash.cend(); vit++) {
            ClusterStore cluster = vit->second;
            for (auto it = cluster.cbegin(); it != cluster.end(); it++) {
                GradientPoint p(*it);
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
        }

        return result;
    }

    int Size() {
        return points_;
    }
};

}  // namespace simdtag
