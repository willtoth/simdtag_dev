#include "ccl/bmrs.h"

#include <fmt/format.h>
#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <map>
#include <opencv2/opencv.hpp>
#include <string>

#include "ccl_samples_test.h"

namespace {
std::string GetImage(const char* testname) {
    std::string filename = std::string(CMAKE_PROJECT_SOURCE_DIR) +
                           std::string("/assets/yacclab/test/") + std::string(testname) +
                           std::string(".png");
    return filename;
}
}  // namespace

TEST(Bmrs, edge_cases) {
    for (auto const& [test_name, expected_value] : CclExpectedOuputs::EdgeCases) {
        cv::Mat1b image = cv::imread(GetImage(test_name), cv::IMREAD_GRAYSCALE);
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

TEST(Bmrs, BasicTest) {
    cv::Mat1b image = cv::imread(GetImage("basic_random"), cv::IMREAD_GRAYSCALE);
    apriltag::BMRS ccl{image};

    ccl.PerformLabeling();
    cv::Mat1i labels = *ccl.Labels();

    EXPECT_TRUE(0 == std::memcmp(CclExpectedOuputs::BasicRandom, labels.data,
                                 sizeof(CclExpectedOuputs::BasicRandom)));
}