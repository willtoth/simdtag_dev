#include <fmt/format.h>
#include <gtest/gtest.h>

#include <opencv2/opencv.hpp>

#include "apriltag/packed_binary_image.h"
#include "ccl_samples.h"

// #define __DO_PRINT_BPI_TEST

void TestResult(cv::Mat1b& image, apriltag::PackedBinaryImage& pbi) {
    const int w = image.cols;
    const int h = image.rows;
    const int row_width = w / 64 + 1;
    uint64_t* data_compressed = new uint64_t[row_width];

    EXPECT_TRUE(row_width > 0);

    bool match = true;

#ifdef __DO_PRINT_BPI_TEST
    fmt::println("Good:");
#endif

    for (int i = 0; i < h; i++) {
        uint64_t* mbits = data_compressed;
        uint8_t* source = image.ptr<uint8_t>(i);
        for (int j = 0; j < w >> 6; j++) {
            uint8_t* base = source + (j << 6);
            uint64_t obits = 0;
            if (base[0]) obits |= 0x0000000000000001;
            if (base[1]) obits |= 0x0000000000000002;
            if (base[2]) obits |= 0x0000000000000004;
            if (base[3]) obits |= 0x0000000000000008;
            if (base[4]) obits |= 0x0000000000000010;
            if (base[5]) obits |= 0x0000000000000020;
            if (base[6]) obits |= 0x0000000000000040;
            if (base[7]) obits |= 0x0000000000000080;
            if (base[8]) obits |= 0x0000000000000100;
            if (base[9]) obits |= 0x0000000000000200;
            if (base[10]) obits |= 0x0000000000000400;
            if (base[11]) obits |= 0x0000000000000800;
            if (base[12]) obits |= 0x0000000000001000;
            if (base[13]) obits |= 0x0000000000002000;
            if (base[14]) obits |= 0x0000000000004000;
            if (base[15]) obits |= 0x0000000000008000;
            if (base[16]) obits |= 0x0000000000010000;
            if (base[17]) obits |= 0x0000000000020000;
            if (base[18]) obits |= 0x0000000000040000;
            if (base[19]) obits |= 0x0000000000080000;
            if (base[20]) obits |= 0x0000000000100000;
            if (base[21]) obits |= 0x0000000000200000;
            if (base[22]) obits |= 0x0000000000400000;
            if (base[23]) obits |= 0x0000000000800000;
            if (base[24]) obits |= 0x0000000001000000;
            if (base[25]) obits |= 0x0000000002000000;
            if (base[26]) obits |= 0x0000000004000000;
            if (base[27]) obits |= 0x0000000008000000;
            if (base[28]) obits |= 0x0000000010000000;
            if (base[29]) obits |= 0x0000000020000000;
            if (base[30]) obits |= 0x0000000040000000;
            if (base[31]) obits |= 0x0000000080000000;
            if (base[32]) obits |= 0x0000000100000000;
            if (base[33]) obits |= 0x0000000200000000;
            if (base[34]) obits |= 0x0000000400000000;
            if (base[35]) obits |= 0x0000000800000000;
            if (base[36]) obits |= 0x0000001000000000;
            if (base[37]) obits |= 0x0000002000000000;
            if (base[38]) obits |= 0x0000004000000000;
            if (base[39]) obits |= 0x0000008000000000;
            if (base[40]) obits |= 0x0000010000000000;
            if (base[41]) obits |= 0x0000020000000000;
            if (base[42]) obits |= 0x0000040000000000;
            if (base[43]) obits |= 0x0000080000000000;
            if (base[44]) obits |= 0x0000100000000000;
            if (base[45]) obits |= 0x0000200000000000;
            if (base[46]) obits |= 0x0000400000000000;
            if (base[47]) obits |= 0x0000800000000000;
            if (base[48]) obits |= 0x0001000000000000;
            if (base[49]) obits |= 0x0002000000000000;
            if (base[50]) obits |= 0x0004000000000000;
            if (base[51]) obits |= 0x0008000000000000;
            if (base[52]) obits |= 0x0010000000000000;
            if (base[53]) obits |= 0x0020000000000000;
            if (base[54]) obits |= 0x0040000000000000;
            if (base[55]) obits |= 0x0080000000000000;
            if (base[56]) obits |= 0x0100000000000000;
            if (base[57]) obits |= 0x0200000000000000;
            if (base[58]) obits |= 0x0400000000000000;
            if (base[59]) obits |= 0x0800000000000000;
            if (base[60]) obits |= 0x1000000000000000;
            if (base[61]) obits |= 0x2000000000000000;
            if (base[62]) obits |= 0x4000000000000000;
            if (base[63]) obits |= 0x8000000000000000;
            *mbits = obits, mbits++;
        }
        uint64_t obits_final = 0;
        int jbase = w - (w % 64);
        for (int j = 0; j < w % 64; j++) {
            if (source[jbase + j]) obits_final |= ((uint64_t)1 << j);
        }
        *mbits = obits_final;

        uint64_t* pbi_row = pbi.Row(i);
        for (int idx = 0; idx < row_width; idx++) {
            if (data_compressed[idx] != pbi_row[idx]) {
                match = false;
            }
        }

#ifdef __DO_PRINT_BPI_TEST
        if (!match) {
            fmt::println("Here:");
        }

        for (int j = 0; j < row_width; j++) {
            fmt::print("{:064b}", data_compressed[j]);
        }
        fmt::println("");
#endif
    }
#ifdef __DO_PRINT_BPI_TEST
    fmt::println("---------------------------------\n");
#endif
}

TEST(PackedBinaryImage, TestCaseCount) {
    for (auto const& [test_name, expected_value] : CclExpectedOuputs::TestCases) {
        cv::Mat1b image = cv::imread(CclExpectedOuputs::GetImage(test_name), cv::IMREAD_GRAYSCALE);
        apriltag::PackedBinaryImage pbi(image);

#ifdef __DO_PRINT_BPI_TEST
        fmt::println("{}", test_name);
        pbi.Print();
        fmt::println("==============================");
        fmt::println("==============================");
#endif

        TestResult(image, pbi);
    }
}