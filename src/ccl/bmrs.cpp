// Copyright (c) 2021, the YACCLAB contributors, as
// shown by the AUTHORS file, plus additional authors
// listed below. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.
//
// Additional Authors:
// Wonsang Lee
// Department of Physics
// Konkuk University, Korea

#include "bmrs.h"

#include <hwy/contrib/algo/copy-inl.h>
#include <hwy/highway.h>
#include <immintrin.h>

#include <iostream>
#include <opencv2/core.hpp>

#include "bit_scan_forward.h"
#include "disjoint_set.h"

namespace hw = hwy::HWY_NAMESPACE;

namespace apriltag {

namespace {
template <size_t CONNECTIVITY>
static constexpr inline size_t LabelSolverUpperBound(size_t w, size_t h) {
    static_assert(CONNECTIVITY == 8 || CONNECTIVITY == 4);
    if (CONNECTIVITY == 4) return ((h * w + 1) / 2 + 1);
    if (CONNECTIVITY == 8) return (((h + 1) / 2) * ((w + 1) / 2) + 1);
}
}  // namespace

BMRS::BMRS(cv::Mat1b const& input) : BMRS(input.cols, input.rows) {
}

BMRS::BMRS(size_t w, size_t h) : w_(w), h_(h), label_solver_(LabelSolverUpperBound<8>(w, h)) {
    int h_merge = h / 2 + h % 2;

    data_compressed.Alloc(h, w);
    data_merged.Alloc(h_merge, w);
    data_flags.Alloc(h_merge - 1, w);
    data_runs.Alloc(h_merge, w);
}

BMRS::~BMRS() {
    data_runs.Dealloc();
    data_flags.Dealloc();
    data_merged.Dealloc();
    data_compressed.Dealloc();
}

void BMRS::PerformLabeling(cv::Mat1b const& input, cv::Mat1i& labels) {
    assert(input.rows == h_);
    assert(input.cols == w_);
    assert(labels.rows == h_);
    assert(labels.cols == w_);
    int w(w_);
    int h(h_);

    label_solver_.Reset();

    InitCompressedData(input, data_compressed);

    // generate merged data
    int h_merge = h / 2 + h % 2;
    int data_width = data_compressed.data_width;
    for (int i = 0; i < h / 2; i++) {
        uint64_t* pdata_source1 = data_compressed[2 * i];
        uint64_t* pdata_source2 = data_compressed[2 * i + 1];
        uint64_t* pdata_merged = data_merged[i];
        for (int j = 0; j < data_width; j++) pdata_merged[j] = pdata_source1[j] | pdata_source2[j];
    }
    if (h % 2) {
        uint64_t* pdata_source = data_compressed[h - 1];
        uint64_t* pdata_merged = data_merged[h / 2];
        for (int j = 0; j < data_width; j++) pdata_merged[j] = pdata_source[j];
    }

    // generate flag bits
    for (int i = 0; i < data_flags.height; i++) {
        uint64_t* bits_u = data_compressed[2 * i + 1];
        uint64_t* bits_d = data_compressed[2 * i + 2];
        uint64_t* bits_dest = data_flags[i];

        uint64_t u0 = bits_u[0];
        uint64_t d0 = bits_d[0];
        bits_dest[0] = (u0 | (u0 << 1)) & (d0 | (d0 << 1));
        for (int j = 1; j < data_width; j++) {
            uint64_t u = bits_u[j];
            uint64_t u_shl = u << 1;
            uint64_t d = bits_d[j];
            uint64_t d_shl = d << 1;
            if (bits_u[j - 1] & 0x8000000000000000) u_shl |= 1;
            if (bits_d[j - 1] & 0x8000000000000000) d_shl |= 1;
            bits_dest[j] = (u | u_shl) & (d | d_shl);
        }
    }

    // Create label '0' for background
    label_solver_.NewLabel();

    FindRuns(data_merged.bits, data_flags.bits, h_merge, data_width, data_runs.runs);

    // New version (uses 1-byte per pixel input)
    Run* runs = data_runs.runs;
    for (int i = 0; i < h / 2; i++) {
        const uint64_t* const data_u = data_compressed.bits + data_compressed.data_width * 2 * i;
        const uint64_t* const data_d = data_u + data_compressed.data_width;
        unsigned* const labels_u = labels.ptr<unsigned>(2 * i);
        unsigned* const labels_d = labels.ptr<unsigned>(2 * i + 1);

        for (;; runs++) {
            unsigned short start_pos = runs->start_pos;
            if (start_pos == 0xFFFF) {
                runs++;
                break;
            }
            unsigned short end_pos = runs->end_pos;
            int label = label_solver_.GetLabel(runs->label);

            for (int j = start_pos; j < end_pos; j++) {
                if (data_u[j >> 6] & (1ull << (j & 0x3F))) labels_u[j] = label;
                if (data_d[j >> 6] & (1ull << (j & 0x3F))) labels_d[j] = label;
            }
        }
    }
    if (h % 2) {
        unsigned int* const labels_end = labels.ptr<unsigned int>(h - 1);
        for (;; runs++) {
            unsigned short start_pos = runs->start_pos;
            if (start_pos == 0xFFFF) {
                break;
            }
            unsigned short end_pos = runs->end_pos;
            int label = label_solver_.GetLabel(runs->label);
            for (int j = start_pos; j < end_pos; j++) {
                labels_end[j] = label;
            }
        }
    }
}

// template <size_t LEN>
// void ArrayToBinary(uint64_t* __restrict dst, const uint8_t* __restrict src) {
//     constexpr hw::ScalableTag<uint8_t> d;
//     constexpr int N = hw::Lanes(d);
//     static_assert(LEN % 64 == 0);

//     uint8_t* workingPtr = (uint8_t*)dst;

//     for (auto i = 0; i < LEN; i += N) {
//         const auto va = hw::Load(d, src + i);
//         workingPtr += hw::StoreMaskBits(d, va != hw::Zero(d), workingPtr);
//     }
// }

void BMRS::InitCompressedData(cv::Mat1b const& input, Data_Compressed& data_compressed) {
    int w(w_);
    int h(h_);

    // constexpr hw::ScalableTag<uint8_t> d;
    // constexpr int N = hw::Lanes(d);
    // uint8_t* mbits = (uint8_t*)data_compressed.bits;
    // unsigned long long orig = (unsigned long long)mbits;
    // uint8_t* source = input.data;
    // for (int i = 0; i < w * h; i += N) {
    //     const auto va = hw::Load(d, source + i);
    //     mbits += hw::StoreMaskBits(d, va != hw::Zero(d), mbits);
    // }

    // std::cout << "HERE!!" << std::endl;
    // data_compressed.Show();

    // std::cout << std::endl << " ============================= " << std::endl;

    for (int i = 0; i < h; i++) {
        uint64_t* mbits = data_compressed[i];
        const uint8_t* source = input.ptr<uchar>(i);

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
    }

    // data_compressed.Show();
    // std::cout << std::endl
    //           << " ----------------- " << std::endl
    //           << std::endl
    //           << std::endl
    //           << std::endl;
}

void BMRS::FindRuns(const uint64_t* bits_start, const uint64_t* bits_flag, int height,
                    int data_width, Run* runs) {
    Run* runs_up = runs;

    // process runs in the first merged row
    const uint64_t* bits = bits_start;
    const uint64_t* bit_final = bits + data_width;
    uint64_t working_bits = *bits;
    unsigned long basepos = 0, bitpos = 0;
    for (;; runs++) {
        // find starting position
        while (!YacclabBitScanForward64(&bitpos, working_bits)) {
            bits++, basepos += 64;
            if (bits == bit_final) {
                runs->start_pos = (short)0xFFFF;
                runs->end_pos = (short)0xFFFF;
                runs++;
                goto out;
            }
            working_bits = *bits;
        }
        runs->start_pos = short(basepos + bitpos);

        // find ending position
        working_bits = (~working_bits) & (0xFFFFFFFFFFFFFFFF << bitpos);
        while (!YacclabBitScanForward64(&bitpos, working_bits)) {
            bits++, basepos += 64;
            working_bits = ~(*bits);
        }
        working_bits = (~working_bits) & (0xFFFFFFFFFFFFFFFF << bitpos);
        runs->end_pos = short(basepos + bitpos);
        runs->label = label_solver_.NewLabel();
    }
out:

    // process runs in the rests
    for (int row = 1; row < height; row++) {
        Run* runs_save = runs;
        const uint64_t* bits_f = bits_flag + data_width * (row - 1);
        const uint64_t* bits = bits_start + data_width * row;
        const uint64_t* bit_final = bits + data_width;
        uint64_t working_bits = *bits;
        unsigned long basepos = 0, bitpos = 0;

        for (;; runs++) {
            // find starting position
            while (!YacclabBitScanForward64(&bitpos, working_bits)) {
                bits++, basepos += 64;
                if (bits == bit_final) {
                    runs->start_pos = (short)0xFFFF;
                    runs->end_pos = (short)0xFFFF;
                    runs++;
                    goto out2;
                }
                working_bits = *bits;
            }
            unsigned short start_pos = short(basepos + bitpos);

            // find ending position
            working_bits = (~working_bits) & (0xFFFFFFFFFFFFFFFF << bitpos);
            while (!YacclabBitScanForward64(&bitpos, working_bits)) {
                bits++, basepos += 64;
                working_bits = ~(*bits);
            }
            working_bits = (~working_bits) & (0xFFFFFFFFFFFFFFFF << bitpos);
            unsigned short end_pos = short(basepos + bitpos);

            // Skip upper runs end before this slice starts
            for (; runs_up->end_pos < start_pos; runs_up++);

            // No upper run meets this
            if (runs_up->start_pos > end_pos) {
                runs->start_pos = start_pos;
                runs->end_pos = end_pos;
                runs->label = label_solver_.NewLabel();
                continue;
            };

            // Next upper run can not meet this
            unsigned short cross_st =
                    (start_pos >= runs_up->start_pos) ? start_pos : runs_up->start_pos;
            if (end_pos <= runs_up->end_pos) {
                if (is_connected(bits_f, cross_st, end_pos))
                    runs->label = label_solver_.GetLabel(runs_up->label);
                else
                    runs->label = label_solver_.NewLabel();
                runs->start_pos = start_pos;
                runs->end_pos = end_pos;
                continue;
            }

            unsigned label;
            if (is_connected(bits_f, cross_st, runs_up->end_pos))
                label = label_solver_.GetLabel(runs_up->label);
            else
                label = 0;
            runs_up++;

            // Find next upper runs meet this
            for (; runs_up->start_pos <= end_pos; runs_up++) {
                if (end_pos <= runs_up->end_pos) {
                    if (is_connected(bits_f, runs_up->start_pos, end_pos)) {
                        unsigned label_other = label_solver_.GetLabel(runs_up->label);
                        if (label != label_other) {
                            label = (label) ? label_solver_.Merge(label, label_other) : label_other;
                        }
                    }
                    break;
                } else {
                    if (is_connected(bits_f, runs_up->start_pos, runs_up->end_pos)) {
                        unsigned label_other = label_solver_.GetLabel(runs_up->label);
                        if (label != label_other) {
                            label = (label) ? label_solver_.Merge(label, label_other) : label_other;
                        }
                    }
                }
            }

            if (label)
                runs->label = label;
            else
                runs->label = label_solver_.NewLabel();
            runs->start_pos = start_pos;
            runs->end_pos = end_pos;
        }
    out2:
        runs_up = runs_save;
    }
    n_labels_ = label_solver_.Flatten();
}

uint64_t BMRS::is_connected(const uint64_t* flag_bits, unsigned start, unsigned end) {
    if (start == end) return flag_bits[start >> 6] & ((uint64_t)1 << (start & 0x0000003F));

    unsigned st_base = start >> 6;
    unsigned st_bits = start & 0x0000003F;
    unsigned ed_base = (end + 1) >> 6;
    unsigned ed_bits = (end + 1) & 0x0000003F;
    if (st_base == ed_base) {
        uint64_t cutter = (0xFFFFFFFFFFFFFFFF << st_bits) ^ (0xFFFFFFFFFFFFFFFF << ed_bits);
        return flag_bits[st_base] & cutter;
    }

    for (unsigned i = st_base + 1; i < ed_base; i++) {
        if (flag_bits[i]) return true;
    }
    uint64_t cutter_st = 0xFFFFFFFFFFFFFFFF << st_bits;
    uint64_t cutter_ed = ~(0xFFFFFFFFFFFFFFFF << ed_bits);
    if (flag_bits[st_base] & cutter_st) return true;
    if (flag_bits[ed_base] & cutter_ed) return true;
    return false;
}
}  // namespace apriltag