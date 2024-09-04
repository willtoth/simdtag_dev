#include "ccl/disjoint_set.h"

#include <benchmark/benchmark.h>

#include <cstdlib>

#include "third_party/yacclab/labels_solver.h"

static constexpr int N = 100000;
static constexpr int NUM_SET_TO_MERGE = 100;
using TestSet = apriltag::DisjointSet;

void InitializeSet(TestSet& ds) {
    for (int i = 0; i < N; i++) {
        ds.NewLabel();
    }

    for (int i = 1; i < 100; i++) {
        for (int j = 0; j < NUM_SET_TO_MERGE; j++) {
            ds.Merge(i + (j * 100), (i + (j * 100)) - 1);
        }
    }
}

static void BM_DisjointSetMerge(benchmark::State& state) {
    TestSet ds(N), timedDs(N);
    InitializeSet(ds);

    for (auto _ : state) {
        state.PauseTiming();
        timedDs = ds;
        state.ResumeTiming();

        for (int i = 0; i < (NUM_SET_TO_MERGE - 1); i++) {
            ds.Merge((i * 100) + 50, ((i + 1) * 100) + 50);
        }
    }
}

void InitializeSetYACCLAB() {
    for (int i = 0; i < N; i++) {
        UF::NewLabel();
    }

    for (int i = 1; i < 100; i++) {
        for (int j = 0; j < NUM_SET_TO_MERGE; j++) {
            UF::Merge(i + (j * 100), (i + (j * 100)) - 1);
        }
    }
}

unsigned* UF::P_;
unsigned UF::length_;

static void BM_DisjointSetMergeYACCLAB(benchmark::State& state) {
    UF::Alloc(N);
    for (auto _ : state) {
        state.PauseTiming();
        UF::Dealloc();
        UF::Alloc(N);
        UF::Setup();
        InitializeSetYACCLAB();
        state.ResumeTiming();

        for (int i = 0; i < (NUM_SET_TO_MERGE - 1); i++) {
            UF::Merge((i * 100) + 50, ((i + 1) * 100) + 50);
        }
    }
}

BENCHMARK(BM_DisjointSetMerge);
BENCHMARK(BM_DisjointSetMergeYACCLAB);

BENCHMARK_MAIN();
