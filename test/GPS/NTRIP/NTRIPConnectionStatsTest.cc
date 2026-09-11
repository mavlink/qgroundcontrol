#include "NTRIPConnectionStatsTest.h"

#include <QtCore/QTimer>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "NTRIPConnectionStats.h"

void NTRIPConnectionStatsTest::testInitialState()
{
    NTRIPConnectionStats stats;
    QCOMPARE(stats.bytesReceived(), quint64(0));
    QCOMPARE(stats.messagesReceived(), quint32(0));
    QCOMPARE(stats.dataRateBytesPerSec(), 0.0);
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

    stats.recordMessage(500);
    QCOMPARE(stats.bytesReceived(), quint64(500));

    stats.reset();
    QCOMPARE(stats.bytesReceived(), quint64(0));
    QCOMPARE(stats.messagesReceived(), quint32(0));
    QCOMPARE(stats.dataRateBytesPerSec(), 0.0);
    QVERIFY(bytesSpy.count() > 0);
}

void NTRIPConnectionStatsTest::testDataRate()
{
    NTRIPConnectionStats stats;
    QSignalSpy rateSpy(&stats, &NTRIPConnectionStats::dataRateChanged);

    QTimer producer;
    producer.setInterval(100);
    connect(&producer, &QTimer::timeout, &stats, [&stats]() { stats.recordMessage(1024); });
    producer.start();
    QTRY_VERIFY_WITH_TIMEOUT(rateSpy.count() >= 1 && stats.dataRateBytesPerSec() > 0.0, TestTimeout::mediumMs());
    producer.stop();

    stats.stop();
    QCOMPARE(stats.dataRateBytesPerSec(), 0.0);
}

void NTRIPConnectionStatsTest::testCorrectionAgeInitial()
{
    NTRIPConnectionStats stats;
    QCOMPARE(stats.correctionAgeSec(), -1.0);
}

void NTRIPConnectionStatsTest::testCorrectionAgeAfterMessage()
{
    NTRIPConnectionStats stats;

    stats.recordMessage(100);
    QVERIFY(stats.correctionAgeSec() >= 0.0);
    QVERIFY(stats.correctionAgeSec() < 1.0);

    stats.reset();
    QCOMPARE(stats.correctionAgeSec(), -1.0);
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

void NTRIPConnectionStatsTest::testDataStaleAfterNoRecentMessages_data()
{
    QTest::addColumn<int>("initialFrame");
    QTest::newRow("no-valid-data") << 0;
    QTest::newRow("accepted") << 1;
    QTest::newRow("filtered-valid") << 2;
}

void NTRIPConnectionStatsTest::testDataStaleAfterNoRecentMessages()
{
    QFETCH(int, initialFrame);
    NTRIPConnectionStats stats;
    QSignalSpy staleSpy(&stats, &NTRIPConnectionStats::dataStaleChanged);

    stats.start();
    if (initialFrame == 1) {
        stats.recordMessage(100, 1005);
    } else if (initialFrame == 2) {
        stats.recordValidatedFrame(true);
    }
    stats.recordNetworkBytes(200);
    QVERIFY(!stats.dataStale());

    QVERIFY(staleSpy.wait(6500));
    QVERIFY(stats.dataStale());

    stats.recordValidatedFrame(true);
    QVERIFY(!stats.dataStale());
    QCOMPARE(stats.filteredFrames(), quint64(initialFrame == 2 ? 2 : 1));
}

UT_REGISTER_TEST(NTRIPConnectionStatsTest, TestLabel::Unit)
