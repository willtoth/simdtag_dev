#include "ccl/bmrs.h"

#include <fmt/format.h>
#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <iostream>
#include <map>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <string>

#include "apriltag/memory_pool.h"
#include "apriltag/vision_utils.h"
#include "ccl_samples.h"

TEST(Bmrs, TestCaseCount) {
    EXPECT_TRUE(CclExpectedOuputs::TestCases.size() >= 17);
}

TEST(Bmrs, TestCases) {
    for (auto const& [test_name, expected_value] : CclExpectedOuputs::TestCases) {
        std::cout << "name:" << test_name << std::endl;
        cv::Mat1b image = cv::imread(CclExpectedOuputs::GetImage(test_name), cv::IMREAD_GRAYSCALE);
        cv::Mat1i labels = cv::Mat1i{image.size(), 0};
        apriltag::BMRS ccl{image};

        ccl.PerformLabeling(image, labels);

        EXPECT_EQ(labels.rows * labels.cols, expected_value.size()) << test_name;

        if (labels.rows * labels.cols != expected_value.size()) {
            continue;
        }

        bool is_equal = true;
        for (int i = 0; i < expected_value.size(); i++) {
            if (((int*)labels.data)[i] != expected_value[i]) {
                is_equal = false;
            }
        }

        if (0 == std::strcmp("sanity", test_name)) {
            EXPECT_FALSE(is_equal) << test_name;
        } else {
            EXPECT_TRUE(is_equal) << test_name;
        }
    }
}

TEST(Bmrs, DualLabel) {
    for (auto const& [test_name, expected_value] : CclExpectedOuputs::TestCases) {
        std::cout << "name:" << test_name << std::endl;
        cv::Mat1b image = cv::imread(CclExpectedOuputs::GetImage(test_name), cv::IMREAD_GRAYSCALE);
        cv::Mat1i labels = cv::Mat1i{image.size(), 0};
        apriltag::BMRS ccl{image};

        ccl.PerformLabelingDual(image, labels);

        auto labeledImage = apriltag::CreateLabeledImage(labels, ccl.LabelCount());

        std::stringstream filename;
        filename << CMAKE_PROJECT_BUILD_DIR << "/bmrs_imgs/" << test_name << ".png";

        cv::imwrite(filename.str(), labeledImage);
    }
}