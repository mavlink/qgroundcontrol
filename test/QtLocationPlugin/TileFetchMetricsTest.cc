#include "TileFetchMetricsTest.h"

#include "TileFetchMetrics.h"

void TileFetchMetricsTest::init()
{
    UnitTest::init();
    // Process-wide singleton: reset so each test starts from a known baseline.
    TileFetchMetrics::instance().reset();
}

void TileFetchMetricsTest::_testCacheHitMissCounters()
{
    TileFetchMetrics &m = TileFetchMetrics::instance();
    const TileFetchStats before = m.snapshot();

    m.recordCacheHit();
    m.recordCacheHit();
    m.recordCacheMiss();

    const TileFetchStats after = m.snapshot();
    QCOMPARE(after.cacheHits, before.cacheHits + 2);
    QCOMPARE(after.cacheMisses, before.cacheMisses + 1);
}

void TileFetchMetricsTest::_testNetworkSuccessBytesAndLatency()
{
    TileFetchMetrics &m = TileFetchMetrics::instance();
    const TileFetchStats before = m.snapshot();

    m.recordNetworkSuccess(2048, 120);

    const TileFetchStats after = m.snapshot();
    QCOMPARE(after.networkSuccess, before.networkSuccess + 1);
    QCOMPARE(after.bytesDownloaded, before.bytesDownloaded + 2048);
    QCOMPARE(after.totalLatencyMs, before.totalLatencyMs + 120);
    QCOMPARE(after.latencySamples, before.latencySamples + 1);
}

void TileFetchMetricsTest::_testNotModifiedCounter()
{
    TileFetchMetrics &m = TileFetchMetrics::instance();
    const TileFetchStats before = m.snapshot();

    m.recordNotModified(30);

    const TileFetchStats after = m.snapshot();
    QCOMPARE(after.notModified, before.notModified + 1);
    QCOMPARE(after.latencySamples, before.latencySamples + 1);
}

void TileFetchMetricsTest::_testNetworkErrorCounter()
{
    TileFetchMetrics &m = TileFetchMetrics::instance();
    const TileFetchStats before = m.snapshot();

    m.recordNetworkError(50);

    const TileFetchStats after = m.snapshot();
    QCOMPARE(after.networkErrors, before.networkErrors + 1);
}

void TileFetchMetricsTest::_testReset()
{
    TileFetchMetrics &m = TileFetchMetrics::instance();
    m.recordCacheHit();
    m.recordNetworkSuccess(100, 10);

    m.reset();

    const TileFetchStats after = m.snapshot();
    QCOMPARE(after.cacheHits, static_cast<quint64>(0));
    QCOMPARE(after.networkSuccess, static_cast<quint64>(0));
    QCOMPARE(after.bytesDownloaded, static_cast<quint64>(0));
    QCOMPARE(after.totalFetches(), static_cast<quint64>(0));
    QCOMPARE(after.averageLatencyMs(), 0.0);
}

UT_REGISTER_TEST(TileFetchMetricsTest, TestLabel::Unit)
