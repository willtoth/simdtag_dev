// Copyright (c) 2020, the YACCLAB contributors, as
// shown by the AUTHORS file. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>

namespace apriltag {

class DisjointSet {
   public:
    DisjointSet(size_t max_size) : size_(max_size), length_(0) { tree_ = new uint32_t[max_size]; }

    DisjointSet(const DisjointSet& other) { *this = other; }

    DisjointSet& operator=(const DisjointSet& other) {
        delete tree_;
        tree_ = new uint32_t[other.size_];
        length_ = other.length_;
        std::memcpy(tree_, other.tree_, length_ * sizeof(tree_[0]));
        size_ = other.size_;
        return *this;
    }

    ~DisjointSet() { delete tree_; }

    uint32_t NewLabel() {
        tree_[length_] = length_;
        return length_++;
    }

    uint32_t GetLabel(uint32_t index) { return tree_[index]; }

    uint32_t FindRoot(uint32_t root) {
        while (tree_[root] < root) {
            root = tree_[root];
        }
        return root;
    }

    uint32_t Merge(uint32_t i, uint32_t j) {
        i = FindRoot(i);
        j = FindRoot(j);

        if (i < j) return tree_[j] = i;
        return tree_[i] = j;
    }

    uint32_t Flatten() {
        uint32_t k = 1;
        for (uint32_t i = 1; i < length_; ++i) {
            if (tree_[i] < i) {
                tree_[i] = tree_[tree_[i]];
            } else {
                tree_[i] = k;
                k = k + 1;
            }
        }
        return k;
    }

   private:
    uint32_t* tree_;
    size_t length_;
    size_t size_;
};

}  // namespace apriltag
