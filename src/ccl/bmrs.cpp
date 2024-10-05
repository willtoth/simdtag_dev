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

#include <hwy/highway.h>

#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <sstream>

#include "bit_scan_forward.h"
#include "disjoint_set.h"
#include "simdtag/highway_utils.h"
#include "simdtag/packed_binary_image.h"

namespace hw = hwy::HWY_NAMESPACE;

namespace simdtag {

HWY_BEFORE_NAMESPACE();
namespace HWY_NAMESPACE {

inline void __MergeRows(uint64_t* __restrict dst, uint64_t* __restrict rowA,
                        const uint64_t* __restrict rowB, size_t double_word_width) {
    constexpr hw::ScalableTag<uint64_t> d;
    constexpr int N = hw::Lanes(d);

    uint8_t* ptr = (uint8_t*)dst;

#if HWY_TARGET == HWY_SCALAR || HWY_TARGET == HWY_EMU128
// TODO: Restore old here...
#else
    for (auto i = 0; i < double_word_width; i += N) {
        const auto va = hw::Load(d, rowA + i);
        const auto vb = hw::Load(d, rowB + i);
        hw::Store(va | vb, d, dst + i);
    }
#endif
}

inline void __GenerateFlagBits(PackedBinaryImage& data_compressed, PackedBinaryImage& data_flags) {
    constexpr hw::ScalableTag<uint64_t> d;
    constexpr int N = hw::Lanes(d);

    // generate flag bits
    for (int i = 0; i < data_flags.Height(); i++) {
        uint64_t* bits_u = data_compressed[2 * i + 1];
        uint64_t* bits_d = data_compressed[2 * i + 2];
        uint64_t* bits_dest = data_flags[i];

#if HWY_TARGET == HWY_SCALAR || HWY_TARGET == HWY_EMU128
        uint64_t u0 = bits_u[0];
        uint64_t d0 = bits_d[0];
        bits_dest[0] = (u0 | (u0 << 1)) & (d0 | (d0 << 1));
        for (int j = 1; j < data_compressed.DoubleWordWidth(); j++) {
            uint64_t u = bits_u[j];
            uint64_t u_shl = u << 1;
            uint64_t d = bits_d[j];
            uint64_t d_shl = d << 1;
            if (bits_u[j - 1] & 0x8000000000000000) u_shl |= 1;
            if (bits_d[j - 1] & 0x8000000000000000) d_shl |= 1;
            bits_dest[j] = (u | u_shl) & (d | d_shl);
        }
#else
        for (int j = 0; j < data_compressed.DoubleWordWidth(); j += N) {
            const auto vu = hw::Load(d, &bits_u[j]);
            const auto vd = hw::Load(d, &bits_d[j]);
            const auto vu_shl = ShiftLeftOneWithCarry(d, vu);
            const auto vd_shl = ShiftLeftOneWithCarry(d, vd);
            hw::Store((vu | vu_shl) & (vd | vd_shl), d, &bits_dest[j]);
        }
#endif
    }
}

inline void __LabelImage(cv::Mat1i& labels, PackedBinaryImage& data_compressed,
                         BMRS::Runs& data_runs, DisjointSet& label_solver, size_t h_merge) {
    constexpr hw::ScalableTag<uint8_t> d;
    constexpr int N = hw::Lanes(d);

    // New version (uses 1-byte per pixel input)
    BMRS::Run* runs = data_runs.runs;
    for (int i = 0; i < h_merge; i++) {
        const uint64_t* const data_u =
                data_compressed[0] + data_compressed.DoubleWordStride() * 2 * i;
        const uint64_t* const data_d = data_u + data_compressed.DoubleWordStride();
        unsigned* const labels_u = labels.ptr<unsigned>(2 * i);
        unsigned* const labels_d = labels.ptr<unsigned>(2 * i + 1);

#if 1  // HWY_TARGET == HWY_SCALAR || HWY_TARGET == HWY_EMU128
        for (;; runs++) {
            unsigned short start_pos = runs->start_pos;
            if (start_pos == 0xFFFF) {
                runs++;
                break;
            }
            unsigned short end_pos = runs->end_pos;
            int label = label_solver.GetLabel(runs->label);
            for (int j = start_pos; j < end_pos; j++) {
                if (data_u[j >> 6] & (1ull << (j & 0x3F))) {
                    labels_u[j] = label;
                    label_solver.__InternalCountLabel(label);
                }
                if (data_d[j >> 6] & (1ull << (j & 0x3F))) {
                    labels_d[j] = label;
                    label_solver.__InternalCountLabel(label);
                }
            }
        }
#else
        int label = label_solver.GetLabel(runs->label);

        for (auto i = 0; i < data_compressed.Width(); i += N) {
            const auto va = hw::Set(d, label);
            const auto v_data_u = hw::Load(d, labels_u[i]);
            const auto v_data_d = hw::Load(d, labels_d[i]);
        }
#endif
    }
}

// NOLINTNEXTLINE(google-readability-namespace-comments)
}  // namespace HWY_NAMESPACE
HWY_AFTER_NAMESPACE();

namespace {
template <size_t CONNECTIVITY>
static constexpr inline size_t LabelSolverUpperBound(size_t w, size_t h) {
    static_assert(CONNECTIVITY == 8 || CONNECTIVITY == 4);
    if (CONNECTIVITY == 4) return ((h * w + 1) / 2 + 1) * 2;
    if (CONNECTIVITY == 8) return (((h + 1) / 2) * ((w + 1) / 2) + 1) * 2;
}
}  // namespace

BMRS::BMRS(cv::Size size) : BMRS(size.width, size.height) {
}

BMRS::BMRS(size_t w, size_t h) : w_(w), h_(h), label_solver_(LabelSolverUpperBound<8>(w, h)) {
    int h_merge = h / 2 + h % 2;

    data_runs.Alloc(h_merge, w);
    data_runs_black.Alloc(h_merge, w);
}

BMRS::~BMRS() {
    data_runs.Dealloc();
    data_runs_black.Dealloc();
}

void BMRS::PerformLabeling(cv::Mat1b const& input, cv::Mat1i& labels) {
    assert(input.rows == h_);
    assert(input.cols == w_);
    assert(labels.rows == h_);
    assert(labels.cols == w_);
    int w(w_);
    int h(h_);

    label_solver_.Reset();

    int h_merge = h / 2 + h % 2;
    PackedBinaryImage data_compressed{input};
    PackedBinaryImage data_merged{h_merge, w};
    PackedBinaryImage data_flags{h_merge - 1, w};

    // generate merged data
    int data_width = data_compressed.DoubleWordWidth();
    for (int i = 0; i < h_merge; i++) {
        uint64_t* pdata_source1 = data_compressed.Row(2 * i);
        uint64_t* pdata_source2 = data_compressed.Row(2 * i + 1);
        uint64_t* pdata_merged = data_merged.Row(i);
        HWY_NAMESPACE::__MergeRows(pdata_merged, pdata_source1, pdata_source2, data_width);
    }

    // generate flag bits
    HWY_NAMESPACE::__GenerateFlagBits(data_compressed, data_flags);

    // Create label '0' for background
    label_solver_.NewLabel();

    FindRuns(data_merged[0], data_flags[0], h_merge, data_width, data_compressed.DoubleWordStride(),
             data_runs.runs);
    n_labels_ = label_solver_.Flatten();
    HWY_NAMESPACE::__LabelImage(labels, data_compressed, data_runs, label_solver_, h_merge);
}

void BMRS::PerformLabelingDual(cv::Mat1b const& input, cv::Mat1i& labels) {
    assert(input.rows == h_);
    assert(input.cols == w_);
    assert(labels.rows == h_);
    assert(labels.cols == w_);
    int w(w_);
    int h(h_);

    label_solver_.Reset();

    int h_merge = h / 2 + h % 2;
    PackedBinaryImage data_compressed_white = PackedBinaryImage::CreateFromMask<255>(input);
    PackedBinaryImage data_compressed_black = PackedBinaryImage::CreateFromMask<0>(input);
    PackedBinaryImage data_merged_white{h_merge, w};
    PackedBinaryImage data_flags_white{h_merge - 1, w};
    PackedBinaryImage data_merged_black{h_merge, w};
    PackedBinaryImage data_flags_black{h_merge - 1, w};

    // generate merged data
    int data_width = data_compressed_white.DoubleWordWidth();
    for (int i = 0; i < h_merge; i++) {
        uint64_t* pdata_source1 = data_compressed_white.Row(2 * i);
        uint64_t* pdata_source2 = data_compressed_white.Row(2 * i + 1);
        uint64_t* pdata_merged = data_merged_white.Row(i);
        HWY_NAMESPACE::__MergeRows(pdata_merged, pdata_source1, pdata_source2, data_width);

        pdata_source1 = data_compressed_black.Row(2 * i);
        pdata_source2 = data_compressed_black.Row(2 * i + 1);
        pdata_merged = data_merged_black.Row(i);
        HWY_NAMESPACE::__MergeRows(pdata_merged, pdata_source1, pdata_source2, data_width);
    }

    // generate flag bits
    HWY_NAMESPACE::__GenerateFlagBits(data_compressed_white, data_flags_white);
    HWY_NAMESPACE::__GenerateFlagBits(data_compressed_black, data_flags_black);

    // Create label '0' for background
    label_solver_.NewLabel();

    FindRuns(data_merged_white[0], data_flags_white[0], h_merge, data_width,
             data_compressed_white.DoubleWordStride(), data_runs.runs);

    FindRuns(data_merged_black[0], data_flags_black[0], h_merge, data_width,
             data_compressed_black.DoubleWordStride(), data_runs_black.runs);

    n_labels_ = label_solver_.Flatten();

    // New version (uses 1-byte per pixel input)
    Run* runs = data_runs.runs;
    Run* runs_black = data_runs_black.runs;
    for (int i = 0; i < h_merge; i++) {
        {
            const uint64_t* const data_u =
                    data_compressed_white[0] + data_compressed_white.DoubleWordStride() * 2 * i;
            const uint64_t* const data_d = data_u + data_compressed_white.DoubleWordStride();
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
                    if (data_u[j >> 6] & (1ull << (j & 0x3F))) {
                        labels_u[j] = label;
                        // TODO: Make this faster!
                        // Can we track this as we process instead of during assignment
                        // then combine as we merge?
                        // can we count 1's or something?
                        label_solver_.__InternalCountLabel(label);
                    }
                    if (data_d[j >> 6] & (1ull << (j & 0x3F))) {
                        labels_d[j] = label;
                        label_solver_.__InternalCountLabel(label);
                    }
                }
            }
        }

        {
            // black
            const uint64_t* const data_u =
                    data_compressed_black[0] + data_compressed_black.DoubleWordStride() * 2 * i;
            const uint64_t* const data_d = data_u + data_compressed_black.DoubleWordStride();
            unsigned* const labels_u = labels.ptr<unsigned>(2 * i);
            unsigned* const labels_d = labels.ptr<unsigned>(2 * i + 1);

            for (;; runs_black++) {
                unsigned short start_pos = runs_black->start_pos;
                if (start_pos == 0xFFFF) {
                    runs_black++;
                    break;
                }
                unsigned short end_pos = runs_black->end_pos;
                int label = label_solver_.GetLabel(runs_black->label);

                for (int j = start_pos; j < end_pos; j++) {
                    if (data_u[j >> 6] & (1ull << (j & 0x3F))) {
                        labels_u[j] = label;
                        label_solver_.__InternalCountLabel(label);
                    }
                    if (data_d[j >> 6] & (1ull << (j & 0x3F))) {
                        labels_d[j] = label;
                        label_solver_.__InternalCountLabel(label);
                    }
                }
            }
        }
    }

    // Filter out labels that are too small
    for (int r = 0; r < labels.rows; r++) {
        for (int c = 0; c < labels.cols; c++) {
            auto& ref = labels.at<unsigned>(r, c);
            if (label_solver_.GetLabelCount(ref) < 25) {
                ref = 0;
            }
        }
    }
}

void BMRS::FindRuns(const uint64_t* bits_start, const uint64_t* bits_flag, int height,
                    int data_width, int data_stride, Run* runs) {
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
        const uint64_t* bits_f = bits_flag + data_stride * (row - 1);
        const uint64_t* bits = bits_start + data_stride * row;
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
    // n_labels_ = label_solver_.Flatten();
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

int BMRS::LabelCount() const {
    return n_labels_ - 1;
}

uint32_t BMRS::GetLabelCount(uint32_t label) const {
    return label_solver_.GetLabelCount(label);
}

}  // namespace simdtag
