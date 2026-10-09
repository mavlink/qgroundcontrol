#include "NTRIPConnectionStatsTest.h"

#include <chrono>
#include <limits>

#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "ManualScheduler.h"
#include "NTRIPConnectionStats.h"

using namespace std::chrono_literals;

namespace {
/// Far enough from the clock's epoch for receipts seconds in the past.
constexpr quint64 START_US = 100'000'000;
constexpr std::chrono::milliseconds STALE_AFTER = 5s;
}  // namespace

void NTRIPConnectionStatsTest::_noFirstCorrectionBecomesStale()
{
    ManualScheduler scheduler(nullptr, START_US);
    NTRIPConnectionStats stats(STALE_AFTER, nullptr, &scheduler);
    stats.start();
    QCOMPARE(stats.correctionAgeSec(), -1.0);
    QVERIFY(scheduler.advanceBy(5s));
    QVERIFY(stats.dataStale());
    QCOMPARE(stats.messagesReceived(), quint32(0));
    QCOMPARE(stats.correctionAgeSec(), -1.0);
    stats.recordMessage(100, 0, scheduler.nowMs());
    QVERIFY(!stats.dataStale());
}

void NTRIPConnectionStatsTest::_reset()
{
    ManualScheduler scheduler(nullptr, START_US);
    NTRIPConnectionStats stats(STALE_AFTER, nullptr, &scheduler);
    QSignalSpy changes(&stats, &NTRIPConnectionStats::statsChanged);

    stats.recordMessage(500, 1005, scheduler.nowMs() - 6000);
    QCOMPARE(stats.bytesReceived(), quint64(500));
    QVERIFY(stats.dataStale());

    stats.reset();
    QCOMPARE(stats.bytesReceived(), quint64(0));
    QCOMPARE(stats.messagesReceived(), quint32(0));
    QCOMPARE(stats.dataRateBytesPerSec(), 0.0);
    QCOMPARE(stats.correctionAgeSec(), -1.0);
    QVERIFY(!stats.dataStale());
    QVERIFY(changes.count() > 0);

    stats.recordMessage(100, 0, scheduler.nowMs());
    QCOMPARE(stats.messagesReceived(), quint32(1));
    QVERIFY(stats.correctionAgeSec() >= 0.0);
    QVERIFY(stats.correctionAgeSec() < 1.0);
    QVERIFY(!stats.dataStale());
}

void NTRIPConnectionStatsTest::_dataRate()
{
    ManualScheduler scheduler(nullptr, START_US);
    NTRIPConnectionStats stats(STALE_AFTER, nullptr, &scheduler);
    stats.recordMessage(1024, 0, scheduler.nowMs());
    for (int i = 0; i < 25 && stats.dataRateBytesPerSec() == 0.0; ++i) {
        QVERIFY(scheduler.advanceBy(50ms));
        stats.recordMessage(1024, 0, scheduler.nowMs());
    }

    QVERIFY(stats.dataRateBytesPerSec() > 0.0);
    QSignalSpy changes(&stats, &NTRIPConnectionStats::statsChanged);

    const auto messageCount = stats.messagesReceived();
    const auto byteCount = stats.bytesReceived();
    const double ageBeforeStop = stats.correctionAgeSec();
    stats.stop();
    QCOMPARE(changes.count(), 1);
    QCOMPARE(stats.dataRateBytesPerSec(), 0.0);
    QCOMPARE(stats.messagesReceived(), messageCount);
    QCOMPARE(stats.bytesReceived(), byteCount);
    stats.stop();
    QCOMPARE(stats.bytesReceived(), byteCount);
    QVERIFY(stats.correctionAgeSec() >= ageBeforeStop);
}

void NTRIPConnectionStatsTest::_correctionAgeAfterMessage_data()
{
    QTest::addColumn<QList<qint64>>("agesMs");
    QTest::addColumn<qint64>("expectedAgeMs");
    QTest::addColumn<bool>("stale");
    QTest::addColumn<int>("staleChanges");
    QTest::newRow("fresh") << QList<qint64>{0} << qint64(0) << false << 0;
    QTest::newRow("expired") << QList<qint64>{6000} << qint64(6000) << true << 1;
    QTest::newRow("older-after-fresh") << QList<qint64>{1000, 6000} << qint64(1000) << false << 0;
    QTest::newRow("older-after-stale") << QList<qint64>{6000, 7000} << qint64(6000) << true << 1;
    QTest::newRow("newer-still-stale") << QList<qint64>{7000, 6000} << qint64(6000) << true << 1;
    QTest::newRow("fresh-after-stale") << QList<qint64>{6000, 0} << qint64(0) << false << 2;
}

void NTRIPConnectionStatsTest::_correctionAgeAfterMessage()
{
    QFETCH(QList<qint64>, agesMs);
    QFETCH(qint64, expectedAgeMs);
    QFETCH(bool, stale);
    QFETCH(int, staleChanges);
    ManualScheduler scheduler(nullptr, START_US);
    NTRIPConnectionStats stats(STALE_AFTER, nullptr, &scheduler);
    // Before start(), only a change of the stale state notifies.
    QSignalSpy staleSpy(&stats, &NTRIPConnectionStats::statsChanged);
    const qint64 nowMs = scheduler.nowMs();
    for (const qint64 ageMs : agesMs) {
        stats.recordMessage(100, 1005, nowMs - ageMs);
    }

    QCOMPARE(stats.bytesReceived(), quint64(100 * agesMs.size()));
    QCOMPARE(stats.messagesReceived(), quint32(agesMs.size()));
    QCOMPARE(stats.messageCountsById().first().count, quint64(agesMs.size()));
    QVERIFY(stats.correctionAgeSec() >= expectedAgeMs / 1000.0);
    QVERIFY(stats.correctionAgeSec() < expectedAgeMs / 1000.0 + 1.0);
    QCOMPARE(stats.dataStale(), stale);
    QCOMPARE(staleSpy.size(), staleChanges);

    stats.stop();
    QCOMPARE(stats.dataStale(), stale);
    QVERIFY(stats.correctionAgeSec() >= expectedAgeMs / 1000.0);
}

void NTRIPConnectionStatsTest::_invalidReceiptTimestamp_data()
{
    QTest::addColumn<qint64>("receivedAtMs");
    QTest::newRow("missing") << qint64(0);
    QTest::newRow("negative") << qint64(-1);
    QTest::newRow("future") << (std::numeric_limits<qint64>::max)();
}

void NTRIPConnectionStatsTest::_invalidReceiptTimestamp()
{
    QFETCH(qint64, receivedAtMs);
    ManualScheduler scheduler(nullptr, START_US);
    NTRIPConnectionStats stats(STALE_AFTER, nullptr, &scheduler);
    expectLogMessage("GPS.NTRIPConnectionStats", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Invalid RTCM receipt timestamp")));
    stats.recordMessage(100, 1005, receivedAtMs);
    verifyExpectedLogMessage();
    QCOMPARE(stats.correctionAgeSec(), -1.0);
    QVERIFY(!stats.dataStale());

    stats.recordMessage(100, 1005, scheduler.nowMs() - 6000);
    QVERIFY(stats.dataStale());
    expectLogMessage("GPS.NTRIPConnectionStats", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Invalid RTCM receipt timestamp")));
    stats.recordMessage(100, 1005, receivedAtMs);
    verifyExpectedLogMessage();
    QVERIFY(stats.correctionAgeSec() >= 6.0);
    QVERIFY(stats.dataStale());
    QCOMPARE(stats.messagesReceived(), quint32(3));
    QCOMPARE(stats.bytesReceived(), quint64(300));
}

void NTRIPConnectionStatsTest::_messageCountsByIdSortedAndReset()
{
    ManualScheduler scheduler(nullptr, START_US);
    NTRIPConnectionStats stats(STALE_AFTER, nullptr, &scheduler);

    stats.recordMessage(10, 1077, scheduler.nowMs());
    stats.recordMessage(10, 1005, scheduler.nowMs());
    stats.recordMessage(10, 1077, scheduler.nowMs());
    stats.recordMessage(10, 0, scheduler.nowMs());

    // Ascending ID order, with ID 0 counting unidentified frames.
    const QList<RTCMMessageCount> expected{{0, 1}, {1005, 1}, {1077, 2}};
    QCOMPARE(stats.messageCountsById(), expected);

    stats.reset();
    QVERIFY(stats.messageCountsById().isEmpty());
}

void NTRIPConnectionStatsTest::_dataStaleAfterNoRecentMessages()
{
    ManualScheduler scheduler(nullptr, START_US);
    NTRIPConnectionStats stats(STALE_AFTER, nullptr, &scheduler);
    QSignalSpy staleSpy(&stats, &NTRIPConnectionStats::statsChanged);

    stats.recordMessage(100, 1005, scheduler.nowMs() - 4000);
    QVERIFY(!stats.dataStale());
    stats.start();

    QVERIFY(scheduler.advanceBy(1s));
    QVERIFY(!staleSpy.isEmpty());
    QVERIFY(stats.dataStale());

    stats.recordMessage(100, 1005, scheduler.nowMs());
    QVERIFY(!stats.dataStale());
}

void NTRIPConnectionStatsTest::_statisticsExpireDuringSilence()
{
    ManualScheduler scheduler(nullptr, START_US);
    NTRIPConnectionStats stats(STALE_AFTER, nullptr, &scheduler);
    QSignalSpy rateChanges(&stats, &NTRIPConnectionStats::statsChanged);
    stats.start();
    stats.recordMessage(2048, 0, scheduler.nowMs());
    QVERIFY(scheduler.advanceBy(1s));
    QVERIFY(stats.dataRateBytesPerSec() > 0.0);
    rateChanges.clear();
    QVERIFY(scheduler.advanceBy(1s));
    QCOMPARE(stats.dataRateBytesPerSec(), 0.0);
    QVERIFY(!rateChanges.isEmpty());
    QCOMPARE(stats.bytesReceived(), quint64(2048));
    QCOMPARE(stats.messagesReceived(), quint32(1));
    QCOMPARE(stats.messageCountsById().first().count, quint64(1));
    stats.stop();
    QCOMPARE(stats.bytesReceived(), quint64(2048));
}

UT_REGISTER_TEST_LIGHTWEIGHT(NTRIPConnectionStatsTest, TestLabel::Unit)
