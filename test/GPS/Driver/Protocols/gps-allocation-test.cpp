#include <array>
#include <fstream>

#include "AllocationTracker.h"
#include "UBX/GPSDriverUBX.h"
#include "UnitTest.h"

class GPSProtocolAllocationTest : public UnitTest
{
    Q_OBJECT

private slots:

    void _runtimeDelivery();
};

void GPSProtocolAllocationTest::_runtimeDelivery()
{
    std::array<uint8_t, 100> frame{};
    std::ifstream input(GPS_FIXTURE_DIR "/nav-pvt.ubx", std::ios::binary);
    QVERIFY2(input.read(reinterpret_cast<char*>(frame.data()), frame.size()),
             "Cannot read the independent NAV-PVT fixture");
    uint64_t now = 1000000;
    std::size_t positions = 0;
    GPSNativeUBX* receiver = nullptr;
    bool injectNested = false;
    bool nestedSnapshotValid = true;
    GPSProtocolIO io;
    io.nowUs = [&] { return now; };
    io.decoded = [&](const GPSDecodedBatch& batch) {
        for (const auto& event : batch.events) {
            positions += std::holds_alternative<GPSNativePositionReport>(event);
            if (const auto* report = std::get_if<GPSNativePositionReport>(&event); report && injectNested) {
                injectNested = false;
                const auto timestamp = report->timestamp;
                now += 200000;
                receiver->consume(frame);
                nestedSnapshotValid = report->timestamp == timestamp;
            }
        }
    };
    GPSNativePositionReport position;
    GPSNativeSatelliteReport satellites;
    GPSNativeUBX driver(io, &position, &satellites);
    receiver = &driver;
    driver.setDecodeContext({.navigation = true, .useNavPvt = true});
    driver.consume(frame);
    positions = 0;
    constexpr std::size_t ITERATIONS = 1000;
    QGCTest::AllocationTracker::Counts allocations;
    {
        QGCTest::AllocationTracker tracker;
        for (std::size_t i = 0; i < ITERATIONS; ++i) {
            now += 200000;
            driver.consume(frame);
        }
        allocations = tracker.counts();
    }
    QCOMPARE(positions, ITERATIONS);
    QCOMPARE(allocations.calls, size_t{0});
    const auto owned = driver.decode(frame);
    const auto timestamp = now;
    now += 200000;
    driver.consume(frame);
    QCOMPARE(owned.batch.events.size(), size_t{1});
    QVERIFY(std::holds_alternative<GPSNativePositionReport>(owned.batch.events.front()));
    QCOMPARE(std::get<GPSNativePositionReport>(owned.batch.events.front()).timestamp, timestamp);
    const auto beforeNested = positions;
    injectNested = true;
    driver.consume(frame);
    QVERIFY(nestedSnapshotValid);
    QCOMPARE(positions, beforeNested + 2);
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSProtocolAllocationTest, TestLabel::Unit)

#include "gps-allocation-test.moc"
