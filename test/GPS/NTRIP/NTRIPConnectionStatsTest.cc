#include "NTRIPConnectionStatsTest.h"
#include "NTRIPConnectionStats.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

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

    stats.recordMessage(1024);
    stats.start();

    QVERIFY(rateSpy.wait(2000));
    QVERIFY(stats.dataRateBytesPerSec() >= 0.0);

    stats.stop();
    QCOMPARE(stats.dataRateBytesPerSec(), 0.0);
}

UT_REGISTER_TEST(NTRIPConnectionStatsTest, TestLabel::Unit)
