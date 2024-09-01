#include <benchmark/benchmark.h>

#include "ccl/disjoint_set.h"

#include <cstdlib>

static constexpr int N = 100000;
static constexpr int NUM_SET_TO_MERGE = 100;
using TestSet = DisjointSet<N>;

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
  TestSet ds, timedDs;
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

BENCHMARK(BM_DisjointSetMerge);

BENCHMARK_MAIN();
