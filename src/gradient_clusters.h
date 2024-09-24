#pragma once

#include <fmt/format.h>

#include <opencv2/core.hpp>

#include "HalideBuffer.h"
#include "halide_gradient_clusters.h"

extern "C" uint32_t __CCL_GetLabelCount(uint32_t x) {
    return 55;
}

namespace simdtag {

void GradientClusters(cv::Mat1b const& input, cv::Mat1i const& labels,
                      BMRS const& connected_components) {
    Halide::Runtime::Buffer<uint8_t> grayscale = Halide::Runtime::Buffer<uint8_t>::make_interleaved(
            input.data, input.cols, input.rows, input.channels());

    Halide::Runtime::Buffer<int> halide_labels = Halide::Runtime::Buffer<int>::make_interleaved(
            (int*)labels.data, labels.cols, labels.rows, labels.channels());

    Halide::Runtime::Buffer<uint64_t> tmp{grayscale.width(), grayscale.height(), 4};

    int error = halide_gradient_clusters(grayscale, halide_labels, tmp);

    [[unlikely]]
    if (error) {
        fmt::println("Halide returned an error: %d\n", error);
        return;
    }

    // tmp.for_each_value([](uint64_t& value) {
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

}  // namespace simdtag
