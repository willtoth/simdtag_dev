#include "ccl/disjoint_set.h"

#include <gtest/gtest.h>

#include <exception>

using namespace simdtag;

// Demonstrate some basic assertions.
TEST(DisjointSet, NewLabel) {
    constexpr size_t N = 5;
    DisjointSet ds(N);

    for (int i = 0; i < N; i++) {
        EXPECT_EQ(i, ds.NewLabel());
    }
}

TEST(DisjointSet, MergeFindRootSimple) {
    DisjointSet ds(100);
    for (int i = 0; i < 100; i++) {
        ds.NewLabel();
    }

    // 0    1      4    5
    //    2   3    6
    //  7
    // 8   9
    ds.Merge(1, 2);
    ds.Merge(1, 3);
    ds.Merge(4, 6);
    ds.Merge(2, 7);
    ds.Merge(7, 8);
    ds.Merge(7, 9);

    EXPECT_EQ(0, ds.FindRoot(0));
    EXPECT_EQ(1, ds.FindRoot(1));
    EXPECT_EQ(1, ds.FindRoot(2));
    EXPECT_EQ(1, ds.FindRoot(3));
    EXPECT_EQ(4, ds.FindRoot(4));
    EXPECT_EQ(4, ds.FindRoot(6));
    EXPECT_EQ(5, ds.FindRoot(5));
    EXPECT_EQ(1, ds.FindRoot(7));
    EXPECT_EQ(1, ds.FindRoot(8));
    EXPECT_EQ(1, ds.FindRoot(9));
}

TEST(DisjointSet, MergeTrees) {
    DisjointSet ds(100);
    for (int i = 0; i < 100; i++) {
        ds.NewLabel();
    }

    // 0    1        4
    //    2   3    5   6
    //  7
    ds.Merge(1, 2);
    ds.Merge(1, 3);
    ds.Merge(4, 5);
    ds.Merge(4, 6);
    ds.Merge(2, 7);

    EXPECT_EQ(4, ds.FindRoot(6));
    EXPECT_EQ(1, ds.FindRoot(7));

    // 0   1   -->  4
    //    2 3     5 6
    //    7 8
    ds.Merge(7, 6);
    EXPECT_EQ(1, ds.FindRoot(4));
    EXPECT_EQ(1, ds.FindRoot(5));
    EXPECT_EQ(1, ds.FindRoot(6));
    EXPECT_EQ(1, ds.FindRoot(7));
}

TEST(DisjointSet, MergeLongTree) {
    DisjointSet ds(100);
    for (int i = 0; i < 100; i++) {
        ds.NewLabel();
    }

    ds.Merge(1, 2);
    ds.Merge(2, 3);
    ds.Merge(3, 4);
    ds.Merge(4, 5);
    ds.Merge(5, 6);
    ds.Merge(6, 7);
    ds.Merge(7, 8);
    ds.Merge(8, 9);

    ds.Merge(10, 11);
    ds.Merge(10, 12);

    EXPECT_EQ(10, ds.FindRoot(12));
    EXPECT_EQ(1, ds.FindRoot(9));

    ds.Merge(8, 11);
    EXPECT_EQ(1, ds.FindRoot(10));
    EXPECT_EQ(1, ds.FindRoot(12));
    EXPECT_EQ(1, ds.FindRoot(9));
    EXPECT_EQ(1, ds.FindRoot(11));
}

TEST(DisjointSet, MergeLongTree2) {
    DisjointSet ds(100);
    for (int i = 0; i < 100; i++) {
        ds.NewLabel();
    }

    ds.Merge(10, 11);
    ds.Merge(11, 12);
    ds.Merge(12, 13);
    ds.Merge(13, 14);
    ds.Merge(14, 15);
    ds.Merge(15, 16);
    ds.Merge(16, 17);
    ds.Merge(17, 18);
    ds.Merge(18, 19);

    ds.Merge(1, 2);
    ds.Merge(1, 3);

    EXPECT_EQ(10, ds.FindRoot(19));
    EXPECT_EQ(1, ds.FindRoot(3));

    ds.Merge(3, 19);
    EXPECT_EQ(1, ds.FindRoot(10));
    EXPECT_EQ(1, ds.FindRoot(2));
    EXPECT_EQ(1, ds.FindRoot(19));
}

TEST(DisjointSet, Copy) {
    DisjointSet ds(100), ds2(100);
    for (int i = 0; i < 100; i++) {
        ds.NewLabel();
    }

    // 0    1        4
    //    2   3    5   6
    //  7
    ds.Merge(1, 2);
    ds.Merge(1, 3);
    ds.Merge(4, 5);
    ds.Merge(4, 6);
    ds.Merge(2, 7);

    EXPECT_EQ(4, ds.FindRoot(6));
    EXPECT_EQ(1, ds.FindRoot(7));

    ds2 = ds;

    // 0   1   -->  4
    //    2 3     5 6
    //    7
    ds.Merge(7, 6);
    EXPECT_EQ(1, ds.FindRoot(4));
    EXPECT_EQ(1, ds.FindRoot(5));
    EXPECT_EQ(1, ds.FindRoot(7));

    EXPECT_EQ(4, ds2.FindRoot(4));
    EXPECT_EQ(4, ds2.FindRoot(5));
    EXPECT_EQ(4, ds2.FindRoot(6));
    EXPECT_EQ(1, ds2.FindRoot(7));

    ds2.Merge(10, 3);
    EXPECT_EQ(1, ds2.FindRoot(10));
    EXPECT_EQ(10, ds.FindRoot(10));

    ds2 = ds;
    EXPECT_EQ(10, ds.FindRoot(10));
    EXPECT_EQ(10, ds2.FindRoot(10));
    EXPECT_EQ(1, ds2.FindRoot(4));
    EXPECT_EQ(1, ds2.FindRoot(5));
    EXPECT_EQ(1, ds2.FindRoot(7));
}
