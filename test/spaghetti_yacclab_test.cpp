#include <fmt/format.h>
#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <map>
#include <opencv2/opencv.hpp>
#include <string>

#include "ccl_samples_test.h"
#include "yacclab/spaghetti.h"

namespace {
std::string GetImage(const char* testname) {
    std::string filename = std::string(CMAKE_PROJECT_SOURCE_DIR) +
                           std::string("/assets/yacclab/test/") + std::string(testname) +
                           std::string(".png");
    return filename;
}
}  // namespace

TEST(YacclabSpaghetti, edge_cases) {
    for (auto const& [test_name, expected_value] : CclExpectedOuputs::TestCases) {
        cv::Mat1b image = cv::imread(GetImage(test_name), cv::IMREAD_GRAYSCALE);
        cv::Mat1i labels;
        Spaghetti<UFPC> labeler{image, labels};

        labeler.PerformSPLabeling();

        EXPECT_EQ(labels.rows * labels.cols, expected_value.size()) << test_name;

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
