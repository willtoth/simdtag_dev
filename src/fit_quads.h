#pragma once

#include <assert.h>
#include <fmt/format.h>

#include <algorithm>
#include <opencv2/core.hpp>

#include "gradient_clusters.h"

namespace hw = hwy::HWY_NAMESPACE;

namespace simdtag {

HWY_BEFORE_NAMESPACE();
namespace HWY_NAMESPACE {

using V32 = hw::VFromD<hw::ScalableTag<uint32_t>>;

V32 __GetXVector(const V32& vvalue) {
    return hw::ShiftRight<20>(vvalue);
}

V32 __GetYVector(const V32& vvalue) {
    hw::DFromV<V32> d;
    return hw::ShiftRight<8>(vvalue) & hw::Set(d, 0xFFFu);
}

inline void __CalculateSlope(std::vector<uint32_t>& cluster, std::pair<float, float>& center) {
    constexpr hw::ScalableTag<float> d;
    constexpr int N = hw::Lanes(d);

    // Copy exact from apriltag, compare to hw::atan2
    constexpr float quadrants[2][2] = {{-1 * (2 << 15), 0}, {2 * (2 << 15), 2 << 15}};

    const auto vcx = hw::Set(d, center.first);
    const auto vcy = hw::Set(d, center.second);

    uint32_t* buffer = cluster.data();
    size_t size = cluster.size();

    auto vdot = hw::Zero(d);
    int i = 0;
    for (; i < size; i += N) {
        const auto va = hw::LoadU(d, buffer + i);
        const auto vx = __GetXVector(va);
        const auto vy = __GetYVector(va);

        const auto vdx = vx - vcx;
        const auto vdy = vy - vcy;

        dot += dx * p->gx + dy * p->gy;

        float quadrant = quadrants[dy > 0][dx > 0];
        if (dy < 0) {
            dy = -dy;
            dx = -dx;
        }

        if (dx < 0) {
            float tmp = dx;
            dx = dy;
            dy = -tmp;
        }
        p->slope = quadrant + dy / dx;
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
    // add some noise to (cx,cy) so that pixels get a more diverse set
    // of theta estimates. This will help us remove more points.
    // (Only helps a small amount. The actual noise values here don't
    // matter much at all, but we want them [-1, 1]. (XXX with
    // fixed-point, should range be bigger?)
    float cx = (x_min + x_max) * 0.5 + 0.05118;
    float cy = (y_min + y_max) * 0.5 + -0.028581;

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
                hash.erase(vit);
                continue;
            }

            FitQuad(cluster);
        }
    }

   private:
    static void FitQuad(std::vector<uint32_t>& cluster) {
        auto [cx, cy] = HWY_NAMESPACE::__FindCenterPoint(cluster);
    }
};

}  // namespace simdtag
