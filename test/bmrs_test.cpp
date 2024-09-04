#include "ccl/bmrs.h"

#include <fmt/format.h>
#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <map>
#include <opencv2/opencv.hpp>
#include <string>

#include "ccl_samples.h"

TEST(Bmrs, TestCases) {
    EXPECT_TRUE(CclExpectedOuputs::TestCases.size() >= 17);
    for (auto const& [test_name, expected_value] : CclExpectedOuputs::TestCases) {
        cv::Mat1b image = cv::imread(CclExpectedOuputs::GetImage(test_name), cv::IMREAD_GRAYSCALE);
        apriltag::BMRS ccl{image};

        ccl.PerformLabeling();
        cv::Mat1i labels = *ccl.Labels();

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

TEST(Bmrs, RunTwice) {
    cv::Mat1b image = cv::imread(CclExpectedOuputs::GetImage("basic_random"), cv::IMREAD_GRAYSCALE);
    apriltag::BMRS ccl{image};

    for (int i = 0; i < 2; i++) {
        ccl.PerformLabeling();
        cv::Mat1i labels = *ccl.Labels();

        auto expected_value = CclExpectedOuputs::TestCases.find("basic_random")->second;
        EXPECT_EQ(labels.rows * labels.cols, expected_value.size());

        bool is_equal = true;
        for (int i = 0; i < expected_value.size(); i++) {
            if (((int*)labels.data)[i] != expected_value[i]) {
                is_equal = false;
            }
        }

        EXPECT_TRUE(is_equal);
    }
}
