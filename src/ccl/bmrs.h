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

#pragma once

#include <cstdint>
#include <opencv2/core.hpp>

#include "disjoint_set.h"

namespace apriltag {

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

   public:
    BMRS(cv::Mat1b const& input);
    BMRS(size_t w, size_t h);
    ~BMRS();
    void PerformLabeling(cv::Mat1b const& input, cv::Mat1i& labels);

   private:
    void InitCompressedData(cv::Mat1b const& input, Data_Compressed& data_compressed);
    void FindRuns(const uint64_t* bits_start, const uint64_t* bits_flag, int height, int data_width,
                  Run* runs);
    uint64_t is_connected(const uint64_t* flag_bits, unsigned start, unsigned end);

    Data_Compressed data_compressed;
    Data_Compressed data_merged;
    Data_Compressed data_flags;
    Runs data_runs;
    DisjointSet label_solver_;
    int w_, h_;
    unsigned int n_labels_;
};

}  // namespace apriltag
