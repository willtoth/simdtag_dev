// Modified into a class, but most is taken from:
// Copyright (c) 2020, the YACCLAB contributors, as
// shown by the AUTHORS file. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "disjoint_set.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>

namespace simdtag {

DisjointSet::DisjointSet(size_t max_size) : size_(max_size), length_(0), num_labels_(0) {
    tree_ = new uint32_t[max_size];
    label_count_ = new uint32_t[max_size];
}

DisjointSet::DisjointSet(const DisjointSet& other) {
    *this = other;
}

DisjointSet& DisjointSet::operator=(const DisjointSet& other) {
    delete tree_;
    tree_ = new uint32_t[other.size_];
    length_ = other.length_;
    std::memcpy(tree_, other.tree_, length_ * sizeof(tree_[0]));
    size_ = other.size_;
    return *this;
}

DisjointSet::~DisjointSet() {
    delete tree_;
}

void DisjointSet::Reset() {
    length_ = 0;
}

uint32_t DisjointSet::NewLabel() {
    assert(length_ < size_);
    tree_[length_] = length_;
    label_count_[length_] = 0;
    return length_++;
}

uint32_t DisjointSet::GetLabel(uint32_t index) {
    assert(index < length_);
    return tree_[index];
}

uint32_t DisjointSet::FindRoot(uint32_t root) {
    assert(root < length_);
    while (tree_[root] < root) {
        root = tree_[root];
    }
    return root;
}

uint32_t DisjointSet::Merge(uint32_t i, uint32_t j) {
    assert(i < length_);
    assert(j < length_);
    i = FindRoot(i);
    j = FindRoot(j);

    if (i < j) return tree_[j] = i;
    return tree_[i] = j;
}

uint32_t DisjointSet::Flatten() {
    uint32_t k = 1;
    for (uint32_t i = 1; i < length_; ++i) {
        if (tree_[i] < i) {
            tree_[i] = tree_[tree_[i]];
        } else {
            tree_[i] = k;
            k = k + 1;
        }
    }
    num_labels_ = k;
    return k;
}

void DisjointSet::__InternalCountLabel(uint32_t label) {
    label_count_[label]++;
}

size_t DisjointSet::GetNumLabels() {
    return num_labels_;
}

uint32_t DisjointSet::GetLabelCount(uint32_t label) {
    return label_count_[label];
}

}  // namespace simdtag
