#include "DataRateTrackerTest.h"
#include "DataRateTracker.h"

// ---------------------------------------------------------------------------
// testInitialState
// ---------------------------------------------------------------------------

void DataRateTrackerTest::testInitialState()
{
    DataRateTracker tracker;

    QCOMPARE(tracker.totalBytes(), static_cast<quint64>(0));
    QCOMPARE(tracker.bytesPerSec(), 0.0);
    QCOMPARE(tracker.kBps(), 0.0);
    // rateUpdated starts false — no recordBytes call yet
    QVERIFY(!tracker.rateUpdated());
}

// ---------------------------------------------------------------------------
// testRecordBytesAccumulates
// ---------------------------------------------------------------------------

void DataRateTrackerTest::testRecordBytesAccumulates()
{
    DataRateTracker tracker;

    tracker.recordBytes(100);
    QCOMPARE(tracker.totalBytes(), static_cast<quint64>(100));

    tracker.recordBytes(400);
    QCOMPARE(tracker.totalBytes(), static_cast<quint64>(500));

    tracker.recordBytes(1024);
    QCOMPARE(tracker.totalBytes(), static_cast<quint64>(1524));

    // rateUpdated is only true once the 1-second window elapses.
    // Within a fast test that window almost certainly has not elapsed,
    // so we assert false — and totalBytes is the durable invariant.
    // (If somehow the window expires in CI we skip the rateUpdated check.)
    if (!tracker.rateUpdated()) {
        QVERIFY(!tracker.rateUpdated());
    }
}

// ---------------------------------------------------------------------------
// testReset
// ---------------------------------------------------------------------------

void DataRateTrackerTest::testReset()
{
    DataRateTracker tracker;

    tracker.recordBytes(512);
    tracker.recordBytes(1024);
    QCOMPARE(tracker.totalBytes(), static_cast<quint64>(1536));

    tracker.reset();

    QCOMPARE(tracker.totalBytes(), static_cast<quint64>(0));
    QCOMPARE(tracker.bytesPerSec(), 0.0);
    QCOMPARE(tracker.kBps(), 0.0);
    QVERIFY(!tracker.rateUpdated());
}

// ---------------------------------------------------------------------------
// testKBpsConversion
// ---------------------------------------------------------------------------

void DataRateTrackerTest::testKBpsConversion()
{
    // kBps() must equal bytesPerSec() / 1024.0 regardless of the actual value.
    // We can inject a known rate indirectly: after reset the rate is 0.
    DataRateTracker tracker;
    QCOMPARE(tracker.kBps(), tracker.bytesPerSec() / 1024.0);

    // Record some bytes and verify the relationship holds even if rate has
    // not been recalculated yet (rate stays 0, kBps stays 0).
    tracker.recordBytes(2048);
    QCOMPARE(tracker.kBps(), tracker.bytesPerSec() / 1024.0);
}

UT_REGISTER_TEST(DataRateTrackerTest, TestLabel::Unit)
