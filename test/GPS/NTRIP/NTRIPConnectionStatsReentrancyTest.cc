#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "NTRIPConnectionStats.h"
#include "NTRIPConnectionStatsTest.h"

void NTRIPConnectionStatsTest::statisticsExpireDuringSilence()
{
    NTRIPConnectionStats stats;
    QSignalSpy rateChanges(&stats, &NTRIPConnectionStats::dataRateChanged);
    stats.start();
    stats.recordMessage(2048);
    QTRY_VERIFY_WITH_TIMEOUT(stats.dataRateBytesPerSec() > 0.0, TestTimeout::mediumMs());
    rateChanges.clear();
    QTRY_COMPARE_WITH_TIMEOUT(stats.dataRateBytesPerSec(), 0.0, TestTimeout::mediumMs());
    QVERIFY(!rateChanges.isEmpty());
    QCOMPARE(stats.bytesReceived(), quint64(2048));
    QCOMPARE(stats.messagesReceived(), quint32(1));
    QCOMPARE(stats.messageCountsById().first().count, quint64(1));
    stats.stop();
    QCOMPARE(stats.bytesReceived(), quint64(2048));
}
