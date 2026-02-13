/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "UnitTest.h"

/// @brief MAVLink message packing/unpacking benchmarks
///
/// These benchmarks measure the performance of core MAVLink operations
/// that are critical for real-time vehicle communication.
///
/// Run with: ./QGroundControl --unittest:MAVLinkBenchmark
///
/// Note: Results vary significantly on shared CI runners (~3x variance).
/// Use these for local profiling and trend tracking, not absolute measurements.
class MAVLinkBenchmark : public UnitTest
{
    Q_OBJECT

public:
    MAVLinkBenchmark() = default;

private slots:
    // Benchmark: Pack heartbeat message (most frequent message)
    void benchmarkHeartbeatPack();

    // Benchmark: Unpack heartbeat message
    void benchmarkHeartbeatUnpack();

    // Benchmark: Pack attitude message (high-frequency telemetry)
    void benchmarkAttitudePack();

    // Benchmark: Unpack attitude message
    void benchmarkAttitudeUnpack();

    // Benchmark: Pack GPS raw message (complex payload)
    void benchmarkGpsRawPack();

    // Benchmark: Full message round-trip (pack + CRC + unpack)
    void benchmarkMessageRoundTrip();

    // Benchmark: Message parsing from byte stream
    void benchmarkParseByteStream();
};
