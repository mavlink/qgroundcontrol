#include "NTRIPConnectionStatsTest.h"

#include <limits>

#include <QtCore/QRegularExpression>
#include <QtCore/QTimer>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "MonotonicClock.h"
#include "NTRIPConnectionStats.h"

void NTRIPConnectionStatsTest::testInitialState()
{
    NTRIPConnectionStats stats;
    QCOMPARE(stats.bytesReceived(), quint64(0));
    QCOMPARE(stats.messagesReceived(), quint32(0));
    QCOMPARE(stats.dataRateBytesPerSec(), 0.0);
    QVERIFY(!stats.dataStale());
}

void NTRIPConnectionStatsTest::testRecordMessage()
{
    NTRIPConnectionStats stats;

    stats.recordMessage(100);
    QCOMPARE(stats.bytesReceived(), quint64(100));
    QCOMPARE(stats.messagesReceived(), quint32(1));

    stats.recordMessage(200);
    QCOMPARE(stats.bytesReceived(), quint64(300));
    QCOMPARE(stats.messagesReceived(), quint32(2));
}

void NTRIPConnectionStatsTest::testReset()
{
    NTRIPConnectionStats stats;
    QSignalSpy bytesSpy(&stats, &NTRIPConnectionStats::bytesReceivedChanged);

    stats.recordMessage(500, 1005, static_cast<qint64>(MonotonicClock::nowUs() / 1000) - 6000);
    QCOMPARE(stats.bytesReceived(), quint64(500));
    QVERIFY(stats.dataStale());

    stats.reset();
    QCOMPARE(stats.bytesReceived(), quint64(0));
    QCOMPARE(stats.messagesReceived(), quint32(0));
    QCOMPARE(stats.dataRateBytesPerSec(), 0.0);
    QCOMPARE(stats.correctionAgeSec(), -1.0);
    QVERIFY(!stats.dataStale());
    QVERIFY(bytesSpy.count() > 0);

    stats.recordMessage(100);
    QCOMPARE(stats.messagesReceived(), quint32(1));
    QVERIFY(stats.correctionAgeSec() >= 0.0);
    QVERIFY(stats.correctionAgeSec() < 1.0);
    QVERIFY(!stats.dataStale());
}

void NTRIPConnectionStatsTest::testDataRate()
{
    NTRIPConnectionStats stats;
    QSignalSpy rateSpy(&stats, &NTRIPConnectionStats::dataRateChanged);

    stats.recordMessage(1024);
    QTimer producer;
    connect(&producer, &QTimer::timeout, &stats, [&stats]() { stats.recordMessage(1024); });
    producer.start(50);
    QVERIFY_SIGNAL_WAIT(rateSpy, TestTimeout::mediumMs());
    producer.stop();

    QVERIFY(rateSpy.count() >= 1);
    QVERIFY(stats.dataRateBytesPerSec() > 0.0);

    const auto messageCount = stats.messagesReceived();
    const double ageBeforeStop = stats.correctionAgeSec();
    stats.stop();
    QCOMPARE(stats.dataRateBytesPerSec(), 0.0);
    QCOMPARE(stats.messagesReceived(), messageCount);
    QVERIFY(stats.correctionAgeSec() >= ageBeforeStop);
}

void NTRIPConnectionStatsTest::testCorrectionAgeInitial()
{
    NTRIPConnectionStats stats;
    QCOMPARE(stats.correctionAgeSec(), -1.0);
}

void NTRIPConnectionStatsTest::testCorrectionAgeAfterMessage_data()
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

void NTRIPConnectionStatsTest::testCorrectionAgeAfterMessage()
{
    QFETCH(QList<qint64>, agesMs);
    QFETCH(qint64, expectedAgeMs);
    QFETCH(bool, stale);
    QFETCH(int, staleChanges);
    NTRIPConnectionStats stats;
    QSignalSpy staleSpy(&stats, &NTRIPConnectionStats::dataStaleChanged);
    const qint64 nowMs = static_cast<qint64>(MonotonicClock::nowUs() / 1000);
    for (const qint64 ageMs : agesMs) {
        stats.recordMessage(100, 1005, nowMs - ageMs);
    }

    QCOMPARE(stats.bytesReceived(), quint64(100 * agesMs.size()));
    QCOMPARE(stats.messagesReceived(), quint32(agesMs.size()));
    QCOMPARE(stats.messageCountsById().first().toList().at(1).toUInt(), quint32(agesMs.size()));
    QVERIFY(stats.correctionAgeSec() >= expectedAgeMs / 1000.0);
    QVERIFY(stats.correctionAgeSec() < expectedAgeMs / 1000.0 + 1.0);
    QCOMPARE(stats.dataStale(), stale);
    QCOMPARE(staleSpy.size(), staleChanges);

    stats.stop();
    QCOMPARE(stats.dataStale(), stale);
    QVERIFY(stats.correctionAgeSec() >= expectedAgeMs / 1000.0);
}

void NTRIPConnectionStatsTest::testInvalidReceiptTimestamp_data()
{
    QTest::addColumn<qint64>("receivedAtMs");
    QTest::newRow("missing") << qint64(0);
    QTest::newRow("negative") << qint64(-1);
    QTest::newRow("future") << (std::numeric_limits<qint64>::max)();
}

void NTRIPConnectionStatsTest::testInvalidReceiptTimestamp()
{
    QFETCH(qint64, receivedAtMs);
    NTRIPConnectionStats stats;
    expectLogMessage("GPS.NTRIPConnectionStats", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Invalid RTCM receipt timestamp")));
    stats.recordMessage(100, 1005, receivedAtMs);
    verifyExpectedLogMessage();
    QCOMPARE(stats.correctionAgeSec(), -1.0);
    QVERIFY(!stats.dataStale());

    stats.recordMessage(100, 1005, static_cast<qint64>(MonotonicClock::nowUs() / 1000) - 6000);
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

void NTRIPConnectionStatsTest::testMessageCountsByIdSortedAndReset()
{
    NTRIPConnectionStats stats;

    stats.recordMessage(10, 1077);
    stats.recordMessage(10, 1005);
    stats.recordMessage(10, 1077);
    stats.recordMessage(10, 0);

    const QVariantList counts = stats.messageCountsById();
    QCOMPARE(counts.size(), 3);

    const QVariantList unknown = counts.at(0).toList();
    QCOMPARE(unknown.at(0).toInt(), 0);
    QCOMPARE(unknown.at(1).toUInt(), quint32(1));

    const QVariantList base = counts.at(1).toList();
    QCOMPARE(base.at(0).toInt(), 1005);
    QCOMPARE(base.at(1).toUInt(), quint32(1));

    const QVariantList msm = counts.at(2).toList();
    QCOMPARE(msm.at(0).toInt(), 1077);
    QCOMPARE(msm.at(1).toUInt(), quint32(2));

    stats.reset();
    QVERIFY(stats.messageCountsById().isEmpty());
}

void NTRIPConnectionStatsTest::testDataStaleAfterNoRecentMessages()
{
    NTRIPConnectionStats stats;
    QSignalSpy staleSpy(&stats, &NTRIPConnectionStats::dataStaleChanged);

    stats.recordMessage(100, 1005, static_cast<qint64>(MonotonicClock::nowUs() / 1000) - 4000);
    QVERIFY(!stats.dataStale());
    stats.start();

    QVERIFY_SIGNAL_WAIT(staleSpy, TestTimeout::mediumMs());
    QVERIFY(stats.dataStale());

    stats.recordMessage(100, 1005);
    QVERIFY(!stats.dataStale());
}

UT_REGISTER_TEST(NTRIPConnectionStatsTest, TestLabel::Unit)
