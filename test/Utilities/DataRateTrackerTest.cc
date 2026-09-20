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

void DataRateTrackerTest::testRefreshDuringSilence()
{
    quint64 nowUs = 1000000;
    DataRateTracker tracker([&nowUs]() { return nowUs; });
    tracker.recordBytes(2048);
    nowUs += 999999;
    tracker.refresh();
    QVERIFY(!tracker.rateUpdated());
    QCOMPARE(tracker.bytesPerSec(), 0.0);

    ++nowUs;
    tracker.refresh();
    QVERIFY(tracker.rateUpdated());
    QCOMPARE(tracker.bytesPerSec(), 2048.0);
    QCOMPARE(tracker.kBps(), 2.0);
    nowUs += 1000000;
    tracker.refresh();
    QVERIFY(tracker.rateUpdated());
    QCOMPARE(tracker.bytesPerSec(), 0.0);
    QCOMPARE(tracker.totalBytes(), quint64(2048));

    nowUs += 500000;
    tracker.recordBytes(1024);
    QVERIFY(!tracker.rateUpdated());
    nowUs += 500000;
    tracker.recordBytes(1024);
    QVERIFY(tracker.rateUpdated());
    QCOMPARE(tracker.bytesPerSec(), 2048.0);
    QCOMPARE(tracker.totalBytes(), quint64(4096));

    nowUs += 3000000;
    tracker.refresh();
    QCOMPARE(tracker.bytesPerSec(), 0.0);
    QCOMPARE(tracker.totalBytes(), quint64(4096));
    tracker.reset();
    QCOMPARE(tracker.totalBytes(), quint64(0));
    QVERIFY(!tracker.rateUpdated());
}

QGC_REGISTER_PORTABLE_TEST(DataRateTrackerTest, TestLabel::Unit)
