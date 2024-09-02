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

#ifndef YACCLAB_LABELING_BMRS
#define YACCLAB_LABELING_BMRS
#include <immintrin.h>

#include <opencv2/core.hpp>
#include <vector>

#include "bit_scan_forward.h"
#include "labels_solver.h"

#define UPPER_BOUND_4_CONNECTIVITY (((size_t)img_.rows * (size_t)img_.cols + 1) / 2 + 1)
#define UPPER_BOUND_8_CONNECTIVITY \
    ((size_t)((img_.rows + 1) / 2) * (size_t)((img_.cols + 1) / 2) + 1)

template <typename LabelsSolver>
class BMRS {
    struct Data_Compressed {
        uint64_t* bits;
        int height;
        int width;
        int data_width;
        uint64_t* operator[](uint64_t row) {
            return bits + data_width * row;
        }
        void Alloc(int _height, int _width) {
            height = _height, width = _width;
            data_width = _width / 64 + 1;
            bits = new uint64_t[height * data_width];
        }
        void Dealloc() {
            delete[] bits;
        }
    };
    struct Run {
        unsigned short start_pos;
        unsigned short end_pos;
        unsigned label;
    };
    struct Runs {
        Run* runs;
        unsigned height;
        unsigned width;
        void Alloc(int _height, int _width) {
            height = _height, width = _width;
            runs = new Run[height * (width / 2 + 2) + 1];
        }
        void Dealloc() {
            delete[] runs;
        }
    };
    Data_Compressed data_compressed;
    Data_Compressed data_merged;
    Data_Compressed data_flags;
    Runs data_runs;

   public:
    cv::Mat1b& img_;
    cv::Mat1i& img_labels_;
    unsigned int n_labels_;

    BMRS(cv::Mat1b& input, cv::Mat1i& labels) : img_(input), img_labels_(labels) {
    }
    void PerformYLLabeling() {
        int w(img_.cols);
        int h(img_.rows);

        data_compressed.Alloc(h, w);
        InitCompressedData(data_compressed);

        // generate merged data
        int h_merge = h / 2 + h % 2;
        int data_width = data_compressed.data_width;
        data_merged.Alloc(h_merge, w);
        for (int i = 0; i < h / 2; i++) {
            uint64_t* pdata_source1 = data_compressed[2 * i];
            uint64_t* pdata_source2 = data_compressed[2 * i + 1];
            uint64_t* pdata_merged = data_merged[i];
            for (int j = 0; j < data_width; j++)
                pdata_merged[j] = pdata_source1[j] | pdata_source2[j];
        }
        if (h % 2) {
            uint64_t* pdata_source = data_compressed[h - 1];
            uint64_t* pdata_merged = data_merged[h / 2];
            for (int j = 0; j < data_width; j++) pdata_merged[j] = pdata_source[j];
        }

        // generate flag bits
        data_flags.Alloc(h_merge - 1, w);
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

        // find runs
        data_runs.Alloc(h_merge, w);
        LabelsSolver::Alloc(UPPER_BOUND_8_CONNECTIVITY);
        LabelsSolver::Setup();
        FindRuns(data_merged.bits, data_flags.bits, h_merge, data_width, data_runs.runs);

        img_labels_ = cv::Mat1i(img_.size(), 0);  // (0-init)

        // New version (uses 1-byte per pixel input)
        Run* runs = data_runs.runs;
        for (int i = 0; i < h / 2; i++) {
            const uint64_t* const data_u =
                    data_compressed.bits + data_compressed.data_width * 2 * i;
            const uint64_t* const data_d = data_u + data_compressed.data_width;
            unsigned* const labels_u = img_labels_.ptr<unsigned>(2 * i);
            unsigned* const labels_d = img_labels_.ptr<unsigned>(2 * i + 1);

            for (;; runs++) {
                unsigned short start_pos = runs->start_pos;
                if (start_pos == 0xFFFF) {
                    runs++;
                    break;
                }
                unsigned short end_pos = runs->end_pos;
                int label = LabelsSolver::GetLabel(runs->label);

                for (int j = start_pos; j < end_pos; j++) {
                    if (data_u[j >> 6] & (1ull << (j & 0x3F))) labels_u[j] = label;
                    if (data_d[j >> 6] & (1ull << (j & 0x3F))) labels_d[j] = label;
                }
            }
        }
        if (h % 2) {
            unsigned* const labels = img_labels_.ptr<unsigned>(h - 1);
            for (;; runs++) {
                unsigned short start_pos = runs->start_pos;
                if (start_pos == 0xFFFF) {
                    break;
                }
                unsigned short end_pos = runs->end_pos;
                int label = LabelsSolver::GetLabel(runs->label);
                for (int j = start_pos; j < end_pos; j++) {
                    labels[j] = label;
                }
            }
        }

        LabelsSolver::Dealloc();
        data_runs.Dealloc();
        data_flags.Dealloc();
        data_merged.Dealloc();
        data_compressed.Dealloc();
    }

   private:
    void InitCompressedData(Data_Compressed& data_compressed) {
        int w(img_.cols);
        int h(img_.rows);

        for (int i = 0; i < h; i++) {
            uint64_t* mbits = data_compressed[i];
            uchar* source = img_.ptr<uchar>(i);

            for (int j = 0; j < w >> 6; j++) {
                uchar* base = source + (j << 6);

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
    }

    void FindRuns(const uint64_t* bits_start, const uint64_t* bits_flag, int height, int data_width,
                  Run* runs) {
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
            runs->label = LabelsSolver::NewLabel();
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
                for (; runs_up->end_pos < start_pos; runs_up++)
                    ;

                // No upper run meets this
                if (runs_up->start_pos > end_pos) {
                    runs->start_pos = start_pos;
                    runs->end_pos = end_pos;
                    runs->label = LabelsSolver::NewLabel();
                    continue;
                };

                // Next upper run can not meet this
                unsigned short cross_st =
                        (start_pos >= runs_up->start_pos) ? start_pos : runs_up->start_pos;
                if (end_pos <= runs_up->end_pos) {
                    if (is_connected(bits_f, cross_st, end_pos))
                        runs->label = LabelsSolver::GetLabel(runs_up->label);
                    else
                        runs->label = LabelsSolver::NewLabel();
                    runs->start_pos = start_pos;
                    runs->end_pos = end_pos;
                    continue;
                }

                unsigned label;
                if (is_connected(bits_f, cross_st, runs_up->end_pos))
                    label = LabelsSolver::GetLabel(runs_up->label);
                else
                    label = 0;
                runs_up++;

                // Find next upper runs meet this
                for (; runs_up->start_pos <= end_pos; runs_up++) {
                    if (end_pos <= runs_up->end_pos) {
                        if (is_connected(bits_f, runs_up->start_pos, end_pos)) {
                            unsigned label_other = LabelsSolver::GetLabel(runs_up->label);
                            if (label != label_other) {
                                label = (label) ? LabelsSolver::Merge(label, label_other)
                                                : label_other;
                            }
                        }
                        break;
                    } else {
                        if (is_connected(bits_f, runs_up->start_pos, runs_up->end_pos)) {
                            unsigned label_other = LabelsSolver::GetLabel(runs_up->label);
                            if (label != label_other) {
                                label = (label) ? LabelsSolver::Merge(label, label_other)
                                                : label_other;
                            }
                        }
                    }
                }

                if (label)
                    runs->label = label;
                else
                    runs->label = LabelsSolver::NewLabel();
                runs->start_pos = start_pos;
                runs->end_pos = end_pos;
            }
        out2:
            runs_up = runs_save;
        }
        n_labels_ = LabelsSolver::Flatten();
    }

    uint64_t is_connected(const uint64_t* flag_bits, unsigned start, unsigned end) {
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

   private:
    void Alloc() {
        // Memory allocation of the labels solver
        int w(img_.cols);
        int h(img_.rows);
        int h_merge = h / 2 + h % 2;

        // Memory allocation for others
        img_labels_ = cv::Mat1i(img_.size());
        data_compressed.Alloc(h, w);
        data_merged.Alloc(h_merge, w);
        data_flags.Alloc(h_merge - 1, w);
        memset(img_labels_.data, 0, img_labels_.dataend - img_labels_.datastart);
        memset(data_compressed.bits, 0,
               data_compressed.height * data_compressed.data_width * sizeof(uint64_t));
        memset(data_merged.bits, 0, data_merged.height * data_merged.data_width * sizeof(uint64_t));
        memset(data_flags.bits, 0, data_flags.height * data_flags.data_width * sizeof(uint64_t));

        // Run metadata allocation time will be calculated later
        data_runs.Alloc(h_merge, w);

        memset(img_labels_.data, 0, img_labels_.dataend - img_labels_.datastart);
        memset(data_compressed.bits, 0,
               data_compressed.height * data_compressed.data_width * sizeof(uint64_t));
        memset(data_merged.bits, 0, data_merged.height * data_merged.data_width * sizeof(uint64_t));
        memset(data_flags.bits, 0, data_flags.height * data_flags.data_width * sizeof(uint64_t));
    }
    void Dealloc() {
        LabelsSolver::Dealloc();
        data_runs.Dealloc();
        data_flags.Dealloc();
        data_merged.Dealloc();
        data_compressed.Dealloc();
        // No free for img_labels_ because it is required at the end of the algorithm
    }
};
#endif
