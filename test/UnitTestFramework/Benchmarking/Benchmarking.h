#pragma once

/// @file Benchmarking.h
/// @brief Benchmarking helpers (Qt QBENCHMARK + nanobench) for unit tests.
/// @see https://github.com/martinus/nanobench

#include <QtTest/QTest>

#include <nanobench.h>

/// Run a nanobench benchmark. @p name describes it; the trailing block is measured.
#define BENCH_QT(name, ...)                                        \
    do {                                                           \
        ankerl::nanobench::Bench().run(name, [&] { __VA_ARGS__ }); \
    } while (false)

/// Compare two implementations and show relative performance.
#define BENCH_QT_COMPARE(baseline_name, baseline_code, test_name, test_code) \
    do {                                                                     \
        ankerl::nanobench::Bench _bench;                                     \
        _bench.relative(true);                                               \
        _bench.run(baseline_name, [&] { baseline_code });                    \
        _bench.run(test_name, [&] { test_code });                            \
    } while (false)

namespace qgc::bench {

/// Pre-configured benchmark for CI: reduced iterations, stable settings.
inline ankerl::nanobench::Bench ciConfig()
{
    return ankerl::nanobench::Bench().warmup(10).epochs(100).minEpochIterations(10);
}

/// Pre-configured benchmark for thorough local testing.
inline ankerl::nanobench::Bench thoroughConfig()
{
    return ankerl::nanobench::Bench().warmup(100).epochs(1000).minEpochIterations(100);
}

}  // namespace qgc::bench

/// Warm up @p code @p n times, then run it under Qt's QBENCHMARK.
#define QBENCHMARK_ITERATIONS(n, code)                                         \
    do {                                                                       \
        for (int _qbench_warmup = 0; _qbench_warmup < (n); ++_qbench_warmup) { \
            code;                                                              \
        }                                                                      \
        QBENCHMARK {                                                           \
            code;                                                              \
        }                                                                      \
    } while (0)

/// Prevent the compiler from optimizing away a value inside QBENCHMARK.
template <typename T>
inline void qtBenchmarkKeep(T const& value)
{
    ankerl::nanobench::doNotOptimizeAway(value);
}

#define QT_BENCHMARK_KEEP(x) qtBenchmarkKeep(x)
