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

namespace simdtag {

class DisjointSet {
   public:
    DisjointSet(size_t max_size);
    DisjointSet(const DisjointSet& other);
    DisjointSet& operator=(const DisjointSet& other);
    ~DisjointSet();
    void Reset();
    uint32_t NewLabel();
    uint32_t GetLabel(uint32_t index);
    uint32_t FindRoot(uint32_t root);
    uint32_t Merge(uint32_t i, uint32_t j);
    uint32_t Flatten();

    void __InternalCountLabel(uint32_t label, uint32_t count = 1);

    uint32_t GetLabelCount(uint32_t label) const;
    size_t GetNumLabels() const;

   private:
    uint32_t* tree_;
    uint32_t* label_count_;
    size_t length_;
    size_t size_;
    size_t num_labels_;
};

}  // namespace simdtag
