#pragma once

#include "TestFixtures.h"

/// Performance benchmark tests for QGroundControl.
/// These tests measure the performance of key operations and can be used to:
/// 1. Detect performance regressions
/// 2. Compare different implementations
/// 3. Establish baseline performance metrics
///
/// Run with: ctest -R PerformanceBenchmarkTest -V
/// Or: ./QGroundControl --unittest:PerformanceBenchmarkTest
/// Uses OfflineTest since it doesn't require a vehicle connection.
class PerformanceBenchmarkTest : public OfflineTest
{
    Q_OBJECT

public:
    PerformanceBenchmarkTest() = default;

private slots:
    void init() override;
    void cleanup() override;

    // Coordinate operation benchmarks
    void _benchmarkCoordinateCreation();
    void _benchmarkCoordinateDistance();
    void _benchmarkCoordinateOffset();

    // JSON operation benchmarks
    void _benchmarkJsonParsing();
    void _benchmarkJsonSerialization();

    // Mission operation benchmarks
    void _benchmarkMissionItemCreation();
    void _benchmarkWaypointListManipulation();

private:
    static constexpr int kIterations = 1000;
    static constexpr int kLargeIterations = 10000;
};
