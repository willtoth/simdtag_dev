#include <fmt/format.h>
#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <map>
#include <opencv2/opencv.hpp>
#include <string>

#include "ccl_samples.h"
#include "yacclab/spaghetti.h"

TEST(YacclabSpaghetti, edge_cases) {
    EXPECT_TRUE(CclExpectedOuputs::TestCases.size() >= 17);
    for (auto const& [test_name, expected_value] : CclExpectedOuputs::TestCases) {
        cv::Mat1b image = cv::imread(CclExpectedOuputs::GetImage(test_name), cv::IMREAD_GRAYSCALE);
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
