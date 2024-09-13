#include <fmt/format.h>
#include <gtest/gtest.h>

#include <opencv2/opencv.hpp>

#include "apriltag/packed_binary_image.h"
#include "ccl_samples.h"

void PrintImageGood(cv::Mat1b& image, apriltag::PackedBinaryImage& pbi) {
    const int w = image.cols;
    const int h = image.rows;
    const int row_width = w / 64 + 1;
    uint64_t* data_compressed = new uint64_t[row_width];

    EXPECT_TRUE(row_width > 0);

    bool match = true;
    // fmt::println("Good:");
    for (int i = 0; i < h; i++) {
        uint64_t* mbits = data_compressed;
        const uint8_t* source = image.ptr<uint8_t>(i);

        for (int j = 0; j < w >> 6; j++) {
            const uint8_t* base = source + (j << 6);

            // Load 64 pixels at once using AVX2
            __m256i pixel_chunk1 = _mm256_loadu_si256((__m256i*)base);
            __m256i pixel_chunk2 = _mm256_loadu_si256((__m256i*)(base + 32));

            // Compare each byte with zero to generate a mask
            __m256i mask1 = _mm256_cmpgt_epi8(pixel_chunk1, _mm256_setzero_si256());
            __m256i mask2 = _mm256_cmpgt_epi8(pixel_chunk2, _mm256_setzero_si256());

            // Compress the result into 64-bit integers
            uint32_t lower_bits = _mm256_movemask_epi8(mask1);
            uint32_t upper_bits = _mm256_movemask_epi8(mask2);
            uint64_t obits = ((uint64_t)upper_bits << 32) | lower_bits;

            *mbits++ = obits;
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

        // if (!match) {
        //     fmt::println("Here:");
        // }

        // for (int j = 0; j < row_width; j++) {
        //     fmt::print("{:064b}", data_compressed[j]);
        // }
        // fmt::println("");
    }

    // fmt::println("---------------------------------\n");
}

TEST(PackedBinaryImage, TestCaseCount) {
    for (auto const& [test_name, expected_value] : CclExpectedOuputs::TestCases) {
        cv::Mat1b image = cv::imread(CclExpectedOuputs::GetImage(test_name), cv::IMREAD_GRAYSCALE);
        apriltag::PackedBinaryImage pbi(image);
        // fmt::println("{}", test_name);
        // pbi.Print();
        // fmt::println("==============================");
        // fmt::println("==============================");
        PrintImageGood(image, pbi);
    }
}