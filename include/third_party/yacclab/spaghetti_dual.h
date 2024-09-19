// Copyright (c) 2020, the YACCLAB contributors, as
// shown by the AUTHORS file. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef YACCLAB_LABELING_BOLELLI_2019_DUAL_H_
#define YACCLAB_LABELING_BOLELLI_2019_DUAL_H_

#include <opencv2/core.hpp>
#include <vector>

#include "labels_solver.h"

#undef UPPER_BOUND_8_CONNECTIVITY
#define UPPER_BOUND_8_CONNECTIVITY \
    ((size_t)((img_.rows + 1) / 2) * (size_t)((img_.cols + 1) / 2) + 1)

#define BLACK_COLOR_VALUE 0
#define WHITE_COLOR_VALUE 255

template <typename LabelsSolver>
class SpaghettiDual {  // : public Labeling2D<Connectivity2D::CONN_8> {
   private:
    inline void SecondScanPerPixel(int c, const unsigned char* const img_row,
                                   const unsigned char* const img_row_fol,
                                   unsigned* const img_labels_row,
                                   unsigned* const img_labels_row_fol, unsigned color) {
        int iLabel = img_labels_row[c];
        if (iLabel > 0) {
            iLabel = LabelsSolver::GetLabel(iLabel);
            if (img_row[c] == color)
                img_labels_row[c] = iLabel;
            else
                img_labels_row[c] = 0;
            if (img_row[c + 1] == color)
                img_labels_row[c + 1] = iLabel;
            else
                img_labels_row[c + 1] = 0;
            if (img_row_fol[c] == color)
                img_labels_row_fol[c] = iLabel;
            else
                img_labels_row_fol[c] = 0;
            if (img_row_fol[c + 1] == color)
                img_labels_row_fol[c + 1] = iLabel;
            else
                img_labels_row_fol[c + 1] = 0;
        } else {
            img_labels_row[c] = 0;
            img_labels_row[c + 1] = 0;
            img_labels_row_fol[c] = 0;
            img_labels_row_fol[c + 1] = 0;
        }
    }

    inline void SecondScanLastColumn1(int c, const unsigned char* const img_row,
                                      const unsigned char* const img_row_fol,
                                      unsigned* const img_labels_row,
                                      unsigned* const img_labels_row_fol, unsigned color) {
        int iLabel = img_labels_row[c];
        if (iLabel > 0) {
            iLabel = LabelsSolver::GetLabel(iLabel);
            if (img_row[c] == color)
                img_labels_row[c] = iLabel;
            else
                img_labels_row[c] = 0;
            if (img_row_fol[c] == color)
                img_labels_row_fol[c] = iLabel;
            else
                img_labels_row_fol[c] = 0;
        } else {
            img_labels_row[c] = 0;
            img_labels_row_fol[c] = 0;
        }
    }

    inline void SecondScanLastColumn2(int c, const unsigned char* const img_row,
                                      unsigned* const img_labels_row, unsigned color) {
        int iLabel = img_labels_row[c];
        if (iLabel > 0) {
            iLabel = LabelsSolver::GetLabel(iLabel);
            if (img_row[c] == color)
                img_labels_row[c] = iLabel;
            else
                img_labels_row[c] = 0;
        } else {
            img_labels_row[c] = 0;
        }
    }

    inline void SecondScanLastRow(int c, const unsigned char* const img_row,
                                  unsigned* const img_labels_row, unsigned color) {
        int iLabel = img_labels_row[c];
        if (iLabel > 0) {
            iLabel = LabelsSolver::GetLabel(iLabel);
            if (img_row[c] == color)
                img_labels_row[c] = iLabel;
            else
                img_labels_row[c] = 0;
            if (img_row[c + 1] == color)
                img_labels_row[c + 1] = iLabel;
            else
                img_labels_row[c + 1] = 0;
        } else {
            img_labels_row[c] = 0;
            img_labels_row[c + 1] = 0;
        }
    }

   public:
    cv::Mat1b& img_;
    cv::Mat1i& img_labels_white;
    cv::Mat1i img_labels_black;
    // cv::Mat1i& img_labels_two;
    unsigned int n_labels_;
    SpaghettiDual(cv::Mat1b& input, cv::Mat1i& labels) : img_(input), img_labels_white(labels) {
    }

    void PerformSPLabeling() {
        const int h = img_.rows;
        const int w = img_.cols;

        int e_rows = h & 0xfffffffe;
        bool o_rows = h % 2 == 1;
        int e_cols = w & 0xfffffffe;
        bool o_cols = w % 2 == 1;

        img_labels_white = cv::Mat1i(img_.size());
        img_labels_black = cv::Mat1i(img_.size());

        LabelsSolver::Alloc(UPPER_BOUND_8_CONNECTIVITY);  // Memory allocation of the labels solver
        LabelsSolver::Setup();                            // Labels solver initialization

        // We work with 2x2 blocks
        // +-+-+-+
        // |P|Q|R|
        // +-+-+-+
        // |S|X|
        // +-+-+

        // The pixels are named as follows
        // +---+---+---+
        // |a b|c d|e f|
        // |g h|i j|k l|
        // +---+---+---+
        // |m n|o p|
        // |q r|s t|
        // +---+---+

        // Pixels a, f, l, q are not needed, since we need to understand the
        // the connectivity between these blocks and those pixels only matter
        // when considering the outer connectivities

        // A bunch of defines used to check if the pixels are foreground,
        // without going outside the image limits.

        // First scan

        // 		if (h == 1) {
        // 			// Single line
        // 			// int r = 0;

        //             // WHITE
        // 			const unsigned char* const img_row = img_.ptr<unsigned char>(0);
        // 			unsigned* img_labels_row = img_labels_white.ptr<unsigned>(0);
        // 			int c = -2;
        // #include "define_conditions_and_actions_white.inc.h"
        // #include "labeling_bolelli_2019_forest_sl.inc.h"
        // #include "undefine_conditions_and_actions.inc.h"

        //             // BLACK
        //             img_labels_row = img_labels_black.ptr<unsigned>(0);
        //             c = -2;
        // #include "define_conditions_and_actions_black.inc.h"
        // #include "labeling_bolelli_2019_forest_sl_black.inc.h"
        // #include "undefine_conditions_and_actions.inc.h"
        // 		}
        // else {
        {
            // First couple of lines
            {
                // int r = 0;

                // WHITE
                const unsigned char* const img_row = img_.ptr<unsigned char>(0);
                unsigned char* const img_row_fol =
                        (unsigned char*)(((char*)img_row) + img_.step.p[0]);
                unsigned* img_labels_row = img_labels_white.ptr<unsigned>(0);
                int c = -2;

#include "define_conditions_and_actions_white.inc.h"
#include "labeling_bolelli_2019_forest_fl.inc.h"
#include "undefine_conditions_and_actions.inc.h"

                // BLACK
                img_labels_row = img_labels_black.ptr<unsigned>(0);
                c = -2;
#include "define_conditions_and_actions_black.inc.h"
#include "labeling_bolelli_2019_forest_fl_black.inc.h"
#include "undefine_conditions_and_actions.inc.h"
            }

            // Every other line but the last one if image has an odd number of rows
            for (int r = 2; r < e_rows; r += 2) {
                // WHITE
                // Get rows pointer
                const unsigned char* const img_row = img_.ptr<unsigned char>(r);
                const unsigned char* const img_row_prev =
                        (unsigned char*)(((char*)img_row) - img_.step.p[0]);
                const unsigned char* const img_row_prev_prev =
                        (unsigned char*)(((char*)img_row_prev) - img_.step.p[0]);
                const unsigned char* const img_row_fol =
                        (unsigned char*)(((char*)img_row) + img_.step.p[0]);
                unsigned* img_labels_row = img_labels_white.ptr<unsigned>(r);
                unsigned* img_labels_row_prev_prev =
                        (unsigned*)(((char*)img_labels_row) - img_labels_white.step.p[0] -
                                    img_labels_white.step.p[0]);

                int c = -2;
                goto tree_0;

#include "define_conditions_and_actions_white.inc.h"
#include "labeling_bolelli_2019_forest.inc.h"
#include "undefine_conditions_and_actions.inc.h"

                // BLACK
                img_labels_row = img_labels_black.ptr<unsigned>(r);
                img_labels_row_prev_prev =
                        (unsigned*)(((char*)img_labels_row) - img_labels_black.step.p[0] -
                                    img_labels_black.step.p[0]);
                c = -2;
                goto tree_black_0;

#include "define_conditions_and_actions_black.inc.h"
#include "labeling_bolelli_2019_forest_black.inc.h"
#include "undefine_conditions_and_actions.inc.h"
            }

            // Last line (in case the rows are odd)
            if (o_rows) {
                // WHITE
                int r = h - 1;
                const unsigned char* const img_row = img_.ptr<unsigned char>(r);
                const unsigned char* const img_row_prev =
                        (unsigned char*)(((char*)img_row) - img_.step.p[0]);
                const unsigned char* const img_row_prev_prev =
                        (unsigned char*)(((char*)img_row_prev) - img_.step.p[0]);
                unsigned* img_labels_row = img_labels_white.ptr<unsigned>(r);
                unsigned* img_labels_row_prev_prev =
                        (unsigned*)(((char*)img_labels_row) - img_labels_white.step.p[0] -
                                    img_labels_white.step.p[0]);
                int c = -2;
#include "define_conditions_and_actions_white.inc.h"
#include "labeling_bolelli_2019_forest_ll.inc.h"
#include "undefine_conditions_and_actions.inc.h"

                // BLACK
                r = h - 1;
                img_labels_row = img_labels_black.ptr<unsigned>(r);
                img_labels_row_prev_prev =
                        (unsigned*)(((char*)img_labels_row) - img_labels_black.step.p[0] -
                                    img_labels_black.step.p[0]);
                c = -2;
#include "define_conditions_and_actions_black.inc.h"
#include "labeling_bolelli_2019_forest_ll_black.inc.h"
#include "undefine_conditions_and_actions.inc.h"
            }
        }

        // Second scan
        n_labels_ = LabelsSolver::Flatten();

        int r = 0;
        for (; r < e_rows; r += 2) {
            // Get rows pointer
            const unsigned char* const img_row = img_.ptr<unsigned char>(r);
            const unsigned char* const img_row_fol =
                    (unsigned char*)(((char*)img_row) + img_.step.p[0]);

            unsigned* const img_labels_row_white = img_labels_white.ptr<unsigned>(r);
            unsigned* const img_labels_row_fol_white =
                    (unsigned*)(((char*)img_labels_row_white) + img_labels_white.step.p[0]);
            unsigned* const img_labels_row_black = img_labels_black.ptr<unsigned>(r);
            unsigned* const img_labels_row_fol_black =
                    (unsigned*)(((char*)img_labels_row_black) + img_labels_black.step.p[0]);

            int c = 0;
            for (; c < e_cols; c += 2) {
                // second scan column
                SecondScanPerPixel(c, img_row, img_row_fol, img_labels_row_white,
                                   img_labels_row_fol_white, WHITE_COLOR_VALUE);
                SecondScanPerPixel(c, img_row, img_row_fol, img_labels_row_black,
                                   img_labels_row_fol_black, BLACK_COLOR_VALUE);
                img_labels_row_white[c] =
                        std::max(img_labels_row_white[c], img_labels_row_black[c]);
                img_labels_row_fol_white[c] =
                        std::max(img_labels_row_fol_white[c], img_labels_row_fol_black[c]);
                img_labels_row_white[c + 1] =
                        std::max(img_labels_row_white[c + 1], img_labels_row_black[c + 1]);
                img_labels_row_fol_white[c + 1] =
                        std::max(img_labels_row_fol_white[c + 1], img_labels_row_fol_black[c + 1]);
            }
            // Last column if the number of columns is odd
            if (o_cols) {
                SecondScanLastColumn1(c, img_row, img_row_fol, img_labels_row_white,
                                      img_labels_row_fol_white, WHITE_COLOR_VALUE);
                SecondScanLastColumn1(c, img_row, img_row_fol, img_labels_row_black,
                                      img_labels_row_fol_black, BLACK_COLOR_VALUE);
                img_labels_row_white[c] =
                        std::max(img_labels_row_white[c], img_labels_row_black[c]);
                img_labels_row_fol_white[c] =
                        std::max(img_labels_row_fol_white[c], img_labels_row_fol_black[c]);
            }
        }
        // Last row if the number of rows is odd
        if (o_rows) {
            // Get rows pointer
            const unsigned char* const img_row = img_.ptr<unsigned char>(r);
            unsigned* const img_labels_row_white = img_labels_white.ptr<unsigned>(r);
            unsigned* const img_labels_row_black = img_labels_black.ptr<unsigned>(r);
            int c = 0;
            for (; c < e_cols; c += 2) {
                SecondScanLastRow(c, img_row, img_labels_row_white, WHITE_COLOR_VALUE);
                SecondScanLastRow(c, img_row, img_labels_row_black, BLACK_COLOR_VALUE);
                img_labels_row_white[c] =
                        std::max(img_labels_row_white[c], img_labels_row_black[c]);
            }
            // Last column if the number of columns is odd
            if (o_cols) {
                SecondScanLastColumn2(c, img_row, img_labels_row_white, WHITE_COLOR_VALUE);
                SecondScanLastColumn2(c, img_row, img_labels_row_black, BLACK_COLOR_VALUE);
                img_labels_row_white[c] =
                        std::max(img_labels_row_white[c], img_labels_row_black[c]);
            }
        }

        LabelsSolver::Dealloc();
    }

   private:
    int e_rows;
    bool o_rows;
    int e_cols;
    bool o_cols;
};

#endif  // YACCLAB_LABELING_BOLELLI_2019_H_