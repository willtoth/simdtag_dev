#pragma once

#include <assert.h>
#include <fmt/format.h>

// clang-format off

#include <hwy/highway.h>

#include <hwy/contrib/math/math-inl.h>

#define VQSORT_ONLY_STATIC 1
#include <hwy/contrib/sort/vqsort-inl.h>
#include <hwy/contrib/sort/order.h>
#include <hwy/print-inl.h>

// clang-format on

#include <algorithm>
#include <opencv2/core.hpp>

#include "gradient_clusters.h"

namespace hw = hwy::HWY_NAMESPACE;

namespace simdtag {

HWY_BEFORE_NAMESPACE();
namespace HWY_NAMESPACE {

using V32 = hw::VFromD<hw::ScalableTag<uint32_t>>;
using VFloat = hw::VFromD<hw::ScalableTag<float>>;

V32 __GetXVector(const V32& vvalue) {
    return hw::ShiftRight<20>(vvalue);
}

V32 __GetYVector(const V32& vvalue) {
    hw::DFromV<V32> d;
    return hw::ShiftRight<8>(vvalue) & hw::Set(d, 0xFFFu);
}

V32 __GetGxVector(const V32& vvalue) {
    hw::DFromV<V32> d;
    return hw::ShiftRight<6>(vvalue) + hw::Set(d, 1);
}

V32 __GetGyVector(const V32& vvalue) {
    hw::DFromV<V32> d;
    return hw::ShiftRight<4>(vvalue) + hw::Set(d, 1);
}
inline void __SortBySlopeApriltag(std::vector<uint32_t>& cluster, std::pair<float, float>& center) {
    constexpr hw::ScalableTag<uint32_t> d;
    constexpr hw::ScalableTag<uint64_t> d64;
    constexpr hw::ScalableTag<float> dfloat;
    constexpr int N = hw::Lanes(d);

    std::vector<uint64_t> ret;
    ret.reserve(cluster.size() + N * 2);
    uint64_t* output_ptr = ret.data();

    // Copy exact from apriltag, compare to hw::atan2
    constexpr float quadrants[2][2] = {{-1 * (2 << 15), 0}, {2 * (2 << 15), 2 << 15}};

    const auto vcx = hw::Set(dfloat, center.first);
    const auto vcy = hw::Set(dfloat, center.second);

    uint32_t* buffer = cluster.data();
    size_t size = cluster.size();

    auto vdot = hw::Zero(dfloat);
    // TODO: This is size + 1 to get all plus some extra. We should make sure vector is padded (and
    // aligned would be neat too)
    for (int i = 0; i < size + 1; i += N) {
        const auto va = hw::LoadU(d, buffer + i);
        const auto vx = hw::ConvertTo(dfloat, __GetXVector(va));
        const auto vy = hw::ConvertTo(dfloat, __GetYVector(va));

        auto vdx = vx - vcx;
        auto vdy = vy - vcy;
        const auto vgx = hw::ConvertTo(dfloat, __GetGxVector(va));
        const auto vgy = hw::ConvertTo(dfloat, __GetGyVector(va));

        vdot = vdot + vdx * vgx + vdy * vgy;

        const auto mdx_lt = vdx < hw::Zero(dfloat);
        const auto mdy_lt = vdy < hw::Zero(dfloat);
        const auto mdx_ge = vdx >= hw::Zero(dfloat);
        const auto mdy_ge = vdy >= hw::Zero(dfloat);

        const auto vneg1 = hw::Set(dfloat, -1);
        const auto vneg_dx = vdx * vneg1;
        const auto vneg_dy = vdy * vneg1;

        // dy >= 0 && dx >= 0
        auto vquad = hw::Set(dfloat, quadrants[1][1]);

        // dy < 0 && dx < 0
        vquad = hw::IfThenElse(hw::And(mdx_lt, mdy_lt), hw::Set(dfloat, quadrants[0][0]), vquad);

        // dy < 0 && dx >= 0
        vquad = hw::IfThenElse(hw::And(mdx_lt, mdx_ge), hw::Set(dfloat, quadrants[1][0]), vquad);

        // dy >= 0 && dx < 0
        vquad = hw::IfThenElse(hw::And(mdy_ge, mdy_lt), hw::Set(dfloat, quadrants[0][1]), vquad);

        // vdx = hw::MaskedMulOr(vdx, mdy_lt, vdx, vneg1);
        // vdy = hw::MaskedMulOr(vdy, mdy_lt, vdy, vneg1);
        vdx = hw::IfThenElse(mdy_lt, vneg_dx, vdx);
        vdy = hw::IfThenElse(mdy_lt, vneg_dy, vdy);

        vdx = hw::IfThenElse(mdx_lt, vdy, vdx);
        vdy = hw::IfThenElse(mdx_lt, vneg_dx, vdy);

        // p->slope = quadrant + dy / dx;

        auto vslope = hw::ConvertTo(d, (vquad + vdy / vdx));
        hw::StoreInterleaved2(va, vslope, d, (uint32_t*)(output_ptr + i));
    }

    hw::VQSortStatic((hwy::K32V32*)output_ptr, size, hwy::SortAscending{});

    for (int i = 0; i < size + 1; i += N) {
        V32 _discard, vcpy;
        hw::LoadInterleaved2(d, (uint32_t*)output_ptr, vcpy, _discard);
        hw::StoreU(vcpy, d, buffer);
    }
}

inline float __SortBySlope(std::vector<uint32_t>& cluster, std::pair<float, float>& center) {
    constexpr hw::ScalableTag<uint32_t> d;
    constexpr hw::ScalableTag<uint64_t> d64;
    constexpr hw::ScalableTag<float> dfloat;
    constexpr int N = hw::Lanes(d);

    std::vector<uint64_t> ret(cluster.size() + N * 2);
    uint64_t* output_ptr = ret.data();

    // Each slope is calculated by quadrant in rand [0, 1) multiplied
    // by a scalar then the top two bits specify quadrant. Used to allow
    // uint32_t sort of slope
    const auto vquad0 = hw::Zero(d);
    const auto vquad1 = hw::Set(d, 1 << 30);
    const auto vquad2 = hw::Set(d, 2 << 30);
    const auto vquad3 = hw::Set(d, 3 << 30);
    const auto vscale = hw::Set(dfloat, 1 << 20);

    const auto vcx = hw::Set(dfloat, center.first);
    const auto vcy = hw::Set(dfloat, center.second);

    uint32_t* buffer = cluster.data();
    size_t size = cluster.size();

    auto vdot = hw::Zero(dfloat);
    // TODO: This is size + 1 to get all plus some extra. We should make sure vector is padded (and
    // aligned would be neat too)
    for (int i = 0; i < size + N; i += N) {
        const auto va = hw::LoadU(d, buffer + i);
        const auto vx = hw::ConvertTo(dfloat, __GetXVector(va));
        const auto vy = hw::ConvertTo(dfloat, __GetYVector(va));

        auto vdx = vx - vcx;
        auto vdy = vy - vcy;
        const auto vgx = hw::ConvertTo(dfloat, __GetGxVector(va));
        const auto vgy = hw::ConvertTo(dfloat, __GetGyVector(va));

        vdot = vdot + vdx * vgx + vdy * vgy;

        const auto mdx_lt = vdx < hw::Zero(dfloat);
        const auto mdy_lt = vdy < hw::Zero(dfloat);
        const auto mdx_ge = vdx >= hw::Zero(dfloat);
        const auto mdy_ge = vdy >= hw::Zero(dfloat);

        const auto mquad0 = hw::And(mdy_ge, mdx_ge);
        const auto mquad1 = hw::And(mdy_ge, mdx_lt);
        const auto mquad2 = hw::And(mdy_lt, mdx_lt);
        const auto mquad3 = hw::And(mdy_lt, mdx_ge);

        const auto vneg1 = hw::Set(dfloat, -1);
        const auto vneg_dx = vdx * vneg1;
        const auto vneg_dy = vdy * vneg1;

        // dy >= 0 && dx >= 0
        auto vquad = vquad0;
        vquad = hw::IfThenElse(RebindMask(d, mquad1), vquad1, vquad);
        vquad = hw::IfThenElse(RebindMask(d, mquad2), vquad2, vquad);
        vquad = hw::IfThenElse(RebindMask(d, mquad3), vquad3, vquad);

        auto vslope = vdy / (vdx + vdy);
        vslope = hw::IfThenElse(mquad1, vneg_dx / (vneg_dx + vdy), vslope);
        vslope = hw::IfThenElse(mquad2, vneg_dy / (vneg_dx - vdy), vslope);
        vslope = hw::IfThenElse(mquad3, vdx / (vdx - vdy), vslope);

        auto vresult = hw::ConvertTo(d, vslope * vscale) + vquad;
        hw::StoreInterleaved2(va, vresult, d, (uint32_t*)(output_ptr + i));
    }

    hw::VQSortStatic(output_ptr, size, hwy::SortAscending{});

    for (int i = 0; i < size + N; i += N) {
        V32 _discard, vcpy;
        hw::LoadInterleaved2(d, (uint32_t*)(output_ptr + i), vcpy, _discard);
        hw::StoreU(vcpy, d, buffer + i);
    }

#if 0
    for (int i = 0; i < size + N; i++) {
        uint64_t v = output_ptr[i];
        uint32_t rout = cluster[i];
        GradientPoint p((uint32_t)v);
        GradientPoint p2(rout);
        fmt::println("{:x} --> {}, {{{},{}}} ==> Copied into: {{{},{}}}", v, v >> 32, p.GetIntX(),
                     p.GetIntY(), p2.GetIntX(), p2.GetIntY());
    }
#endif

    return 0.0f;
}

inline void __SortBySlopeAtan2(std::vector<uint32_t>& cluster, std::pair<float, float>& center) {
    constexpr hw::ScalableTag<uint32_t> d;
    constexpr hw::ScalableTag<float> dfloat;
    constexpr int N = hw::Lanes(d);

    std::vector<uint64_t> ret;
    ret.reserve(cluster.size() + N * 2);
    uint64_t* output_ptr = ret.data();

    // Copy exact from apriltag, compare to hw::atan2
    constexpr float quadrants[2][2] = {{-1 * (2 << 15), 0}, {2 * (2 << 15), 2 << 15}};

    const auto vcx = hw::Set(dfloat, center.first);
    const auto vcy = hw::Set(dfloat, center.second);

    uint32_t* buffer = cluster.data();
    size_t size = cluster.size();

    auto vdot = hw::Zero(dfloat);
    for (int i = 0; i < size + 1; i += N) {
        const auto va = hw::LoadU(d, buffer + i);
        const auto vx = hw::ConvertTo(dfloat, __GetGxVector(va));
        const auto vy = hw::ConvertTo(dfloat, __GetYVector(va));

        auto vdx = vx - vcx;
        auto vdy = vy - vcy;
        const auto vgx = hw::ConvertTo(dfloat, __GetGxVector(va));
        const auto vgy = hw::ConvertTo(dfloat, __GetGyVector(va));

        vdot = vdot + vdx * vgx + vdy * vgy;

        auto vangle = hw::ConvertTo(d, hw::Atan2(dfloat, vdy, vdx) * hw::Set(dfloat, 8e6));
        hw::StoreInterleaved2(vangle, va, d, (uint32_t*)(output_ptr + i));
    }
}

// std::min std::max would likely vectorize here, std::minmax_element may not for some reason
// easy enough to just manually vectorize and not worry
inline std::pair<float, float> __FindCenterPoint(std::vector<uint32_t>& cluster) {
    constexpr hw::ScalableTag<uint32_t> d;
    constexpr int N = hw::Lanes(d);

    uint32_t* buffer = cluster.data();
    size_t size = cluster.size();

    // Lazy (but valid) way to deal with initial condition
    assert(size > 2 * N);

    const auto va = hw::LoadU(d, buffer);
    const auto vx = __GetXVector(va);
    const auto vy = __GetYVector(va);

    auto vx_min = vx;
    auto vx_max = vx;
    auto vy_min = vy;
    auto vy_max = vy;
    int i = N;

    // size is guarenteed to be > 24 so no need to handle case of overrun on first 2*N elements
    for (; i < size; i += N) {
        const auto va = hw::LoadU(d, buffer + i);
        const auto vx = __GetXVector(va);
        const auto vy = __GetYVector(va);

        vx_min = hw::Min(vx, vx_min);
        vx_max = hw::Max(vx, vx_max);
        vy_min = hw::Min(vy, vy_min);
        vy_max = hw::Max(vy, vy_max);
    }

    uint32_t x_min = hw::ReduceMin(d, vx_min);
    uint32_t x_max = hw::ReduceMax(d, vx_max);
    uint32_t y_min = hw::ReduceMin(d, vy_min);
    uint32_t y_max = hw::ReduceMax(d, vy_max);

    // Non-aligned remaining
    if (i != size) {
        uint32_t* buffer_end = buffer + i - N;
        for (int j = 0; j < N - (i - size); j++) {
            x_min = std::min(buffer_end[i], x_min);
            x_max = std::max(buffer_end[i], x_max);
            y_min = std::min(buffer_end[i], y_min);
            y_max = std::max(buffer_end[i], y_max);
        }
    }

    // from apriltag, I don't quite understand the point, but carry over anyway
    // added benefit of not having to check for divide by 0 errors in slope
    // calculations
    // add some noise to (cx,cy) so that pixels get a more diverse set
    // of theta estimates. This will help us remove more points.
    // (Only helps a small amount. The actual noise values here don't
    // matter much at all, but we want them [-1, 1]. (XXX with
    // fixed-point, should range be bigger?)
    float cx = (x_min + x_max) * 0.5 + 0.6118;    // 0.05118;
    float cy = (y_min + y_max) * 0.5 + -0.68581;  //-0.028581;

    return {cx, cy};
}

}  // namespace HWY_NAMESPACE
HWY_AFTER_NAMESPACE();

class FitQuads {
   public:
    static void Perform(GradientClusterHash& hash, cv::Size size) {
        for (auto vit = hash.cbegin(); vit != hash.cend(); vit++) {
            std::vector<uint32_t> cluster = vit->second;

            // Remove clusters that are too small, or larger than the outline of the view. A typical
            // point along an edge is added two times (because it has 2 unique neighbors). The
            // maximum perimeter is 2w+2h.
            if (cluster.size() < 24 || cluster.size() > 2 * (size.width * 2 + size.height * 2)) {
                continue;
            }

            FitQuad(cluster);
        }
    }

   private:
    static void FitQuad(std::vector<uint32_t>& cluster) {
        auto center = HWY_NAMESPACE::__FindCenterPoint(cluster);
        HWY_NAMESPACE::__SortBySlope(cluster, center);
    }
};

}  // namespace simdtag
