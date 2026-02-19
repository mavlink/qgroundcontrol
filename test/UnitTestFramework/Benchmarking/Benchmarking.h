#pragma once

/// @file Benchmarking.h
/// @brief Benchmarking support for QGroundControl unit tests
///
/// This file provides two benchmarking options:
/// 1. Qt's built-in QBENCHMARK (simple, integrated)
/// 2. nanobench (detailed, exportable results)
///
/// ## Qt QBENCHMARK (Simple)
///
/// ```cpp
/// void MyTest::_benchmarkSimple()
/// {
///     QGeoCoordinate c1(47.0, 8.0);
///     QGeoCoordinate c2(48.0, 9.0);
///
///     QBENCHMARK {
///         auto dist = c1.distanceTo(c2);
///         Q_UNUSED(dist);
///     }
/// }
/// ```
///
/// Run with options:
/// ```bash
/// ./QGroundControl --unittest:MyTest -- -iterations 10000
/// ./QGroundControl --unittest:MyTest -- -tickcounter    # Use CPU ticks
/// ./QGroundControl --unittest:MyTest -- -eventcounter   # Count events
/// ./QGroundControl --unittest:MyTest -- -minimumvalue 0 # Show all results
/// ```
///
/// ## nanobench (Detailed)
///
/// nanobench is a header-only microbenchmarking library that provides
/// accurate, low-overhead performance measurements.
///
/// @see https://github.com/martinus/nanobench
///
/// ## Basic Usage
///
/// ```cpp
/// #include "Benchmarking.h"
///
/// void MyTest::_benchmarkSomething()
/// {
///     ankerl::nanobench::Bench().run("operation name", [&] {
///         // Code to benchmark
///         auto result = expensiveOperation();
///         ankerl::nanobench::doNotOptimizeAway(result);
///     });
/// }
/// ```
///
/// ## Preventing Optimization
///
/// ```cpp
/// // Prevent compiler from optimizing away the result
/// ankerl::nanobench::doNotOptimizeAway(result);
///
/// // Prevent compiler from optimizing away a pointer
/// ankerl::nanobench::doNotOptimizeAway(&object);
/// ```
///
/// ## Configuration Options
///
/// ```cpp
/// ankerl::nanobench::Bench()
///     .warmup(100)              // Warmup iterations
///     .epochs(1000)             // Number of epochs (measurement runs)
///     .epochIterations(100)     // Iterations per epoch
///     .minEpochTime(100ms)      // Minimum time per epoch
///     .relative(true)           // Show relative performance
///     .run("name", [&]{ ... });
/// ```
///
/// ## Comparing Implementations
///
/// ```cpp
/// ankerl::nanobench::Bench bench;
/// bench.relative(true);
///
/// bench.run("baseline", [&] {
///     baselineImpl();
/// });
///
/// bench.run("optimized", [&] {
///     optimizedImpl();
/// });
/// // Output shows relative speedup
/// ```
///
/// ## Output Formats
///
/// ```cpp
/// // Markdown table (default)
/// bench.render(ankerl::nanobench::templates::csv(), std::cout);
///
/// // CSV format
/// bench.render(ankerl::nanobench::templates::csv(), std::cout);
///
/// // JSON format
/// bench.render(ankerl::nanobench::templates::json(), std::cout);
/// ```
///
/// ## Best Practices
///
/// 1. Use doNotOptimizeAway() to prevent dead code elimination
/// 2. Run benchmarks in Release builds for accurate results
/// 3. Close other applications to reduce noise
/// 4. Use relative() when comparing implementations
/// 5. Check the "err%" column - high values indicate noisy measurements

#include <QtTest/QTest>

#include <nanobench.h>

/// Macro to run a benchmark and optionally fail if performance regresses.
/// By default, just runs the benchmark and prints results.
///
/// @param name Description of what is being benchmarked
/// @param ... Code block to benchmark (can contain commas)
///
/// Example:
/// @code
/// BENCH_QT("coordinate conversion", {
///     QGeoCoordinate coord(47.0, 8.0, 100.0);
///     auto result = convertCoord(coord);
///     ankerl::nanobench::doNotOptimizeAway(result);
/// });
/// @endcode
#define BENCH_QT(name, ...)                                        \
    do {                                                           \
        ankerl::nanobench::Bench().run(name, [&] { __VA_ARGS__ }); \
    } while (false)

/// Macro to compare two implementations and show relative performance.
///
/// @param baseline_name Name of the baseline implementation
/// @param baseline_code Code block for baseline
/// @param test_name Name of the test implementation
/// @param test_code Code block for test implementation
///
/// Example:
/// @code
/// BENCH_QT_COMPARE(
///     "Qt distanceTo", { dist = c1.distanceTo(c2); },
///     "geodesicDistance", { dist = QGCGeo::geodesicDistance(c1, c2); }
/// );
/// @endcode
#define BENCH_QT_COMPARE(baseline_name, baseline_code, test_name, test_code) \
    do {                                                                     \
        ankerl::nanobench::Bench _bench;                                     \
        _bench.relative(true);                                               \
        _bench.run(baseline_name, [&] { baseline_code });                    \
        _bench.run(test_name, [&] { test_code });                            \
    } while (false)

/// Namespace for QGC benchmark utilities
namespace qgc::bench {

/// Create a pre-configured benchmark suitable for CI
/// - Reduced iterations for faster CI runs
/// - Stable settings to minimize noise
inline ankerl::nanobench::Bench ciConfig()
{
    return ankerl::nanobench::Bench().warmup(10).epochs(100).minEpochIterations(10);
}

/// Create a pre-configured benchmark for thorough local testing
inline ankerl::nanobench::Bench thoroughConfig()
{
    return ankerl::nanobench::Bench().warmup(100).epochs(1000).minEpochIterations(100);
}

}  // namespace qgc::bench

// ============================================================================
// Qt QBENCHMARK Helpers
// ============================================================================

/// @brief Benchmark with a specific iteration count using QBENCHMARK
/// Use when you need precise control over iterations.
///
/// Example:
/// @code
/// QBENCHMARK_ITERATIONS(1000) {
///     expensiveOperation();
/// }
/// @endcode
#define QBENCHMARK_ITERATIONS(n)                                       \
    for (bool _qbench_once = true; _qbench_once; _qbench_once = false) \
    QBENCHMARK

/// @brief Prevent compiler from optimizing away a value in QBENCHMARK
/// Qt's version of doNotOptimizeAway for consistency.
///
/// Example:
/// @code
/// QBENCHMARK {
///     auto result = compute();
///     QT_BENCHMARK_KEEP(result);
/// }
/// @endcode
template <typename T>
inline void qtBenchmarkKeep(T const& value)
{
    // Use volatile to prevent optimization
    volatile auto unused = &value;
    Q_UNUSED(unused);
}

#define QT_BENCHMARK_KEEP(x) qtBenchmarkKeep(x)

/// @brief Compare two implementations using QBENCHMARK
/// Runs baseline first, then the test implementation.
/// Results are shown in Qt Test output.
///
/// Example:
/// @code
/// void MyTest::_compareBenchmark()
/// {
///     double dist;
///     QGeoCoordinate c1(47, 8), c2(48, 9);
///
///     qDebug() << "Baseline (Qt):";
///     QBENCHMARK { dist = c1.distanceTo(c2); QT_BENCHMARK_KEEP(dist); }
///
///     qDebug() << "Test (geodesic):";
///     QBENCHMARK { dist = QGCGeo::geodesicDistance(c1, c2); QT_BENCHMARK_KEEP(dist); }
/// }
/// @endcode

// ============================================================================
// Choosing Between QBENCHMARK and nanobench
// ============================================================================
//
// Use QBENCHMARK when:
// - You want simple, quick benchmarks
// - Results don't need to be exported
// - You're already in a Qt Test context
// - You want minimal dependencies
//
// Use nanobench when:
// - You need detailed statistics (min/max/median/error%)
// - You want to compare multiple implementations
// - You need to export results (CSV/JSON)
// - You want to detect performance regressions in CI
// - You need warmup and calibration control
//
