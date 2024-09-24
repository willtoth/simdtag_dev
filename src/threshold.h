#pragma once

#include <fmt/format.h>

#include <opencv2/core.hpp>

#include "HalideBuffer.h"
#include "adaptive_threshold.h"

namespace simdtag {

void AdaptiveThreshold(cv::Mat1b const& input, cv::Mat1b& output) {
    Halide::Runtime::Buffer<uint8_t> grayscale = Halide::Runtime::Buffer<uint8_t>::make_interleaved(
            input.data, input.cols, input.rows, input.channels());

    Halide::Runtime::Buffer<uint8_t> tmp = Halide::Runtime::Buffer<uint8_t>::make_interleaved(
            output.data, output.cols, output.rows, output.channels());

    int error = adaptive_threshold(grayscale, tmp);

    [[unlikely]]
    if (error) {
        fmt::println("Halide returned an error: %d\n", error);
        return;
    }
}

}  // namespace simdtag
