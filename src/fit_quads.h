#pragma once

#include <assert.h>
#include <fmt/format.h>

#include <opencv2/core.hpp>

#include "gradient_clusters.h"

namespace hw = hwy::HWY_NAMESPACE;

namespace simdtag {

struct QuadCluster {
    size_t start, length, minx, miny, maxx, maxy;
};

HWY_BEFORE_NAMESPACE();
namespace HWY_NAMESPACE {

using V32 = hw::VFromD<hw::ScalableTag<uint32_t>>;

V32 __GetXVector(const V32& vvalue) {
    return hw::ShiftRight<20>(vvalue);
}

V32 __GetYVector(const V32& vvalue) {
    return hw::ShiftRight<20>(ShiftLeft<12>(vvalue));
}

QuadCluster __FindQuadCluster(uint64_t* buffer, size_t start, size_t elements) {
    constexpr hw::ScalableTag<uint32_t> d;
    constexpr int N = hw::Lanes(d);
    // using VecT = Vec<decltype(d)>;

    // assert(elements > 0);

    // QuadCluster cluster;
    // size_t length;
    // cluster.start = start;

    // // Create a vector of the first encountered hash (top 32 bits) to detect when this changes on
    // a
    // // boundary between clusters
    // const auto vcurrent_hash = hw::LoadU(d, Set(d, static_cast<uint32_t>(buffer[0] >> 32)));
    // bool isBoundary = false;

    // hw::MFromD<decltype(d)> mask;
    // for (length = 0; length < elements && !isBoundary; length += N) {
    //     // TODO: Try either unrolling this a bit (manually) or just loading more of the buffer at
    //     a
    //     // time Load interleaved, reduction happens on lower half, boundary detection on top half
    //     VecT vvalue, vhash;
    //     uint32_t* element = ((uint32_t*)buffer) + length;
    //     hw::LoadInterleaved2(d, element, vvalue, vhash);

    //     // Values to X,Y
    //     const auto vx = __GetXVector(vvalue);
    //     const auto vy = __GetYVector(vvaleu);

    //     mask = vhash == vcurrent_hash;
    //     isBoundary = !hw::AllTrue(d, mask);
    // }

    // size_t end_count = hw::FindLastTrue(d, mask);
    // for (int i = 0; i < end_count; i++) {
    // }
}

}  // namespace HWY_NAMESPACE
HWY_AFTER_NAMESPACE();

class FitQuads {
   public:
    // Two things to test here: Is it better to condense down to a single buffer of all the
    // possibly valid quads (filter out a few bad candidates). OR store a list of indexes
    // in the original buffer
    FitQuads(GradientClusterBuffer& buffer, int clusters)
        : buffer_(buffer.Take()), clusters_(clusters), elements_(buffer.Size()) {
        // Find boundary points between each group, and filter out invalid cases
    }

   private:
    uint64_t* buffer_;
    int clusters_;
    size_t elements_;
};

}  // namespace simdtag
