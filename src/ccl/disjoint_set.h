// Copyright (c) 2020, the YACCLAB contributors, as 
// shown by the AUTHORS file. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include <array>
#include <cassert>

#include <hwy/highway.h>
#include <memory>
#include <iostream>

template <size_t N>
class DisjointSet {
public:
    DisjointSet() : tree_(), length_(0) {}

    uint32_t NewLabel() {
        assert(length_ < N);

        tree_[length_] = length_;
        return length_++;
    }

    uint32_t GetLabel(uint32_t index) {
        assert(index < N);

        return tree_[index];
    }

    uint32_t FindRoot(uint32_t root) {
        assert(root < N);
        while (tree_[root] < root) {
            root = tree_[root];
        }
        return root;
    }

    uint32_t Merge(uint32_t i, uint32_t j) {
        assert(i < N);
        assert(j < N);

        i = FindRoot(i);
        j = FindRoot(j);

        if (i < j)
            return tree_[j] = i;
        return tree_[i] = j;
    }

    uint32_t Flatten()
    {
        uint32_t k = 1;
        for (uint32_t i = 1; i < length_; ++i) {
            if (tree_[i] < i) {
                tree_[i] = tree_[tree_[i]];
            }
            else {
                tree_[i] = k;
                k = k + 1;
            }
        }
        return k;
    }

private:
    std::array<uint32_t, N> tree_;
    size_t length_;
};
