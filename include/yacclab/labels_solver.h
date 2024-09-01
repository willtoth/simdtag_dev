// Copyright (c) 2020, the YACCLAB contributors, as
// shown by the AUTHORS file. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef YACCLAB_LABELS_SOLVER_H_
#define YACCLAB_LABELS_SOLVER_H_

// Union-find (UF)
class UF {
    // Maximum number of labels (included background) = 2^(sizeof(unsigned) x 8)
   public:
    static void Alloc(unsigned max_length) { P_ = new unsigned[max_length]; }
    static void Dealloc() { delete[] P_; }
    static void Setup() {
        P_[0] = 0;  // First label is for background pixels
        length_ = 1;
    }
    static unsigned NewLabel() {
        P_[length_] = length_;
        return length_++;
    }
    static unsigned GetLabel(unsigned index) { return P_[index]; }

    // Basic functions of the UF solver required only by the Light-Speed Labeling Algorithms:
    // - "UpdateTable" updates equivalences array without performing "Find" operations
    // - "FindRoot" finds the root of the tree of node i (for the other algorithms it is
    //		already included in the "Merge" and "Flatten" functions).
    static void UpdateTable(unsigned e, unsigned r) { P_[e] = r; }
    static unsigned FindRoot(unsigned root) {
        while (P_[root] < root) {
            root = P_[root];
        }
        return root;
    }

    static unsigned Merge(unsigned i, unsigned j) {
        // FindRoot(i)
        while (P_[i] < i) {
            i = P_[i];
        }

        // FindRoot(j)
        while (P_[j] < j) {
            j = P_[j];
        }

        if (i < j) return P_[j] = i;
        return P_[i] = j;
    }

    static unsigned Flatten() {
        unsigned k = 1;
        for (unsigned i = 1; i < length_; ++i) {
            if (P_[i] < i) {
                P_[i] = P_[P_[i]];
            } else {
                P_[i] = k;
                k = k + 1;
            }
        }
        return k;
    }

   private:
    static unsigned *P_;
    static unsigned length_;
};

// Union-Find (UF) with path compression (PC) as in:
// Two Strategies to Speed up Connected Component Labeling Algorithms
// Kesheng Wu, Ekow Otoo, Kenji Suzuki
class UFPC {
    // Maximum number of labels (included background) = 2^(sizeof(unsigned) x 8)
   public:
    static void Alloc(unsigned max_length) { P_ = new unsigned[max_length]; }
    static void Dealloc() { delete[] P_; }
    static void Setup() {
        P_[0] = 0;  // First label is for background pixels
        length_ = 1;
    }
    static unsigned NewLabel() {
        P_[length_] = length_;
        return length_++;
    }
    static unsigned GetLabel(unsigned index) { return P_[index]; }

    static unsigned Merge(unsigned i, unsigned j) {
        // FindRoot(i)
        unsigned root(i);
        while (P_[root] < root) {
            root = P_[root];
        }
        if (i != j) {
            // FindRoot(j)
            unsigned root_j(j);
            while (P_[root_j] < root_j) {
                root_j = P_[root_j];
            }
            if (root > root_j) {
                root = root_j;
            }
            // SetRoot(j, root);
            while (P_[j] < j) {
                unsigned t = P_[j];
                P_[j] = root;
                j = t;
            }
            P_[j] = root;
        }
        // SetRoot(i, root);
        while (P_[i] < i) {
            unsigned t = P_[i];
            P_[i] = root;
            i = t;
        }
        P_[i] = root;
        return root;
    }
    static unsigned Flatten() {
        unsigned k = 1;
        for (unsigned i = 1; i < length_; ++i) {
            if (P_[i] < i) {
                P_[i] = P_[P_[i]];
            } else {
                P_[i] = k;
                k = k + 1;
            }
        }
        return k;
    }

   private:
    static unsigned *P_;
    static unsigned length_;
    static MemVector<unsigned> mem_P_;
};

// Interleaved Rem algorithm with SPlicing (SP) as in:
// A New Parallel Algorithm for Two - Pass Connected Component Labeling
// S Gupta, D Palsetia, MMA Patwary
class RemSP {
    // Maximum number of labels (included background) = 2^(sizeof(unsigned) x 8)
   public:
    static void Alloc(unsigned max_length) { P_ = new unsigned[max_length]; }
    static void Dealloc() { delete[] P_; }
    static void Setup() {
        P_[0] = 0;  // First label is for background pixels
        length_ = 1;
    }
    static unsigned NewLabel() {
        P_[length_] = length_;
        return length_++;
    }
    static unsigned GetLabel(unsigned index) { return P_[index]; }

    static unsigned Merge(unsigned i, unsigned j) {
        unsigned root_i(i), root_j(j);

        while (P_[root_i] != P_[root_j]) {
            if (P_[root_i] > P_[root_j]) {
                if (root_i == P_[root_i]) {
                    P_[root_i] = P_[root_j];
                    return P_[root_i];
                }
                unsigned z = P_[root_i];
                P_[root_i] = P_[root_j];
                root_i = z;
            } else {
                if (root_j == P_[root_j]) {
                    P_[root_j] = P_[root_i];
                    return P_[root_i];
                }
                unsigned z = P_[root_j];
                P_[root_j] = P_[root_i];
                root_j = z;
            }
        }
        return P_[root_i];
    }
    static unsigned Flatten() {
        unsigned k = 1;
        for (unsigned i = 1; i < length_; ++i) {
            if (P_[i] < i) {
                P_[i] = P_[P_[i]];
            } else {
                P_[i] = k;
                k = k + 1;
            }
        }
        return k;
    }

   private:
    static unsigned *P_;
    static unsigned length_;
};

// Three Table Array as in:
// A Run-Based Two-Scan Labeling Algorithm
// Lifeng He, Yuyan Chao, Kenji Suzuki
class TTA {
    // Maximum number of labels (included background) = 2^(sizeof(unsigned) x 8) - 1:
    // the special value "-1" for next_ table array has been replace with UINT_MAX
   public:
    static void Alloc(unsigned max_length) {
        rtable_ = new unsigned[max_length];
        next_ = new unsigned[max_length];
        tail_ = new unsigned[max_length];
    }
    static void Dealloc() {
        delete[] rtable_;
        delete[] next_;
        delete[] tail_;
    }
    static void Setup() {
        rtable_[0] = 0;
        length_ = 1;
    }
    static unsigned NewLabel() {
        rtable_[length_] = length_;
        next_[length_] = UINT_MAX;
        tail_[length_] = length_;
        return length_++;
    }
    static unsigned GetLabel(unsigned index) { return rtable_[index]; }

    // Basic functions of the TTA solver required only by the Light-Speed Labeling Algorithms:
    // - "UpdateTable" updates equivalences tables without performing "Find" operations
    // - "FindRoot" finds the root of the tree of node i (for the other algorithms it is
    //		already included in the "Merge" and "Flatten" functions).
    static void UpdateTable(unsigned u, unsigned v) {
        if (u < v) {
            unsigned i = v;
            while (i != UINT_MAX) {
                rtable_[i] = u;
                i = next_[i];
            }
            next_[tail_[u]] = v;
            tail_[u] = tail_[v];
        } else if (u > v) {
            unsigned i = u;
            while (i != UINT_MAX) {
                rtable_[i] = v;
                i = next_[i];
            }
            next_[tail_[v]] = u;
            tail_[v] = tail_[u];
        }
    }
    static unsigned FindRoot(unsigned i) { return rtable_[i]; }

    static unsigned Merge(unsigned u, unsigned v) {
        // FindRoot(u);
        u = rtable_[u];
        // FindRoot(v);
        v = rtable_[v];

        if (u < v) {
            unsigned i = v;
            while (i != UINT_MAX) {
                rtable_[i] = u;
                i = next_[i];
            }
            next_[tail_[u]] = v;
            tail_[u] = tail_[v];
            return u;
        } else if (u > v) {
            unsigned i = u;
            while (i != UINT_MAX) {
                rtable_[i] = v;
                i = next_[i];
            }
            next_[tail_[v]] = u;
            tail_[v] = tail_[u];
            return v;
        }

        return u;  // equal to v
    }
    static unsigned Flatten() {
        unsigned cur_label = 1;
        for (unsigned k = 1; k < length_; k++) {
            if (rtable_[k] == k) {
                cur_label++;
                rtable_[k] = cur_label;
            } else
                rtable_[k] = rtable_[rtable_[k]];
        }

        return cur_label;
    }

   private:
    static unsigned *rtable_;
    static unsigned *next_;
    static unsigned *tail_;
    static unsigned length_;
};

#endif  // !YACCLAB_LABELS_SOLVER_H_