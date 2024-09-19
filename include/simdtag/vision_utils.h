#pragma once

#include <sys/time.h>

#include <opencv2/core.hpp>

namespace simdtag {

namespace {

uint8_t RandWithMin(uint8_t min = 40) {
    uint8_t val = rand() % 256;

    return val < min ? min : val;
}
}  // namespace

cv::Mat CreateLabeledImage(cv::Mat1i const& labels, int nLabels) {
    cv::Mat labeledImage{labels.size(), CV_8UC3};
    std::vector<cv::Vec3b> colors(nLabels);

    // Background color
    colors[0] = cv::Vec3b(0, 0, 0);

    // Assign random colors to each component, use same seed each time
    std::srand(0);
    for (int label = 1; label < nLabels; ++label) {
        colors[label] = cv::Vec3b(RandWithMin(), RandWithMin(), RandWithMin());
    }

    // Map the labels to the colors
    for (int r = 0; r < labels.rows; ++r) {
        for (int c = 0; c < labels.cols; ++c) {
            int label = labels.at<int>(r, c);
            cv::Vec3b& pixel = labeledImage.at<cv::Vec3b>(r, c);
            pixel = colors[label];
        }
    }

    return labeledImage;
}

}  // namespace simdtag
