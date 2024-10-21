#include <benchmark/benchmark.h>
#include <fmt/format.h>
#include <hwy/highway.h>

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <utility>

std::pair<int, int> minMaxStd(int* a, size_t len) {
    int ret_min = a[0];
    int ret_max = a[0];
    for (int i = 1; i < len; i++) {
        ret_min = std::min(a[i], ret_min);
        ret_max = std::max(a[i], ret_max);
    }
    return {ret_min, ret_max};
}

namespace hw = hwy::HWY_NAMESPACE;

HWY_BEFORE_NAMESPACE();
namespace HWY_NAMESPACE {

std::pair<int, int> minMaxVec(int* a, size_t len) {
    constexpr hw::ScalableTag<int> d;
    constexpr int N = hw::Lanes(d);

    auto vret_min = hw::LoadU(d, a);
    auto vret_max = hw::LoadU(d, a);
    int i = N;
    for (; i < len; i += N) {
        const auto va = hw::LoadU(d, a + i);
        vret_min = hw::Min(va, vret_min);
        vret_max = hw::Max(va, vret_max);
    }

    int ret_min = hw::ReduceMin(d, vret_min);
    int ret_max = hw::ReduceMax(d, vret_max);

    // Non-aligned remaining
    if (i != len) {
        int* buffer_end = a + i - N;
        for (int j = 0; j < N - (i - len); j++) {
            ret_min = std::min(buffer_end[i], ret_min);
            ret_max = std::max(buffer_end[i], ret_max);
        }
    }

    return {ret_min, ret_max};
}

}  // namespace HWY_NAMESPACE
HWY_AFTER_NAMESPACE();

constexpr int arraylen = 1000000;

static void BM_StdMin(benchmark::State& state) {
    int* buffer = new int[arraylen];
    std::srand(0);

    for (int i = 0; i < arraylen; i++) {
        buffer[i] = std::rand();
    }

    for (auto _ : state) {
        benchmark::DoNotOptimize(minMaxStd(buffer, arraylen));
    }

    free(buffer);
}

static void BM_StdMinElement(benchmark::State& state) {
    int* buffer = new int[arraylen];
    std::srand(0);

    for (int i = 0; i < arraylen; i++) {
        buffer[i] = std::rand();
    }
    // fmt::println("min element min {}", *std::min_element(buffer, buffer + arraylen));

    for (auto _ : state) {
        benchmark::DoNotOptimize(std::min_element(buffer, buffer + arraylen));
    }
}

static void BM_HandMinMax(benchmark::State& state) {
    int* buffer = new int[arraylen];
    std::srand(0);

    for (int i = 0; i < arraylen; i++) {
        buffer[i] = std::rand();
    }

    // auto tmp = HWY_NAMESPACE::minMaxVec(buffer, arraylen);
    // fmt::println("expected min {} -- expected max {}", *std::min_element(buffer, buffer +
    // arraylen),
    //              *std::max_element(buffer, buffer + arraylen));
    // fmt::println("vec min {} -- max {}", tmp.first, tmp.second);

    for (auto _ : state) {
        benchmark::DoNotOptimize(HWY_NAMESPACE::minMaxVec(buffer, arraylen));
    }
}

BENCHMARK(BM_StdMin);
BENCHMARK(BM_StdMinElement);
BENCHMARK(BM_HandMinMax);

BENCHMARK_MAIN();
