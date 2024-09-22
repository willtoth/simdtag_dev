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

// class Threshold {
//    public:
//     Threshold(cv::Size size) : Threshold(size.width, size.height) {
//     }

//     Threshold(size_t width, size_t height) {
//         // bits_ = new (std::align_val_t(64)) uint64_t[alloc_height * double_word_stride_];
//     }

//    private:
// };

}  // namespace simdtag
