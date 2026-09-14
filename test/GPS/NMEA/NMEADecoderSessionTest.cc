#include "NMEADecoderSessionTest.h"

#include <QtCore/QBuffer>
#include <QtCore/QIODevice>
#include <QtPositioning/QGeoPositionInfoSource>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "ManualScheduler.h"
#include "NMEADecoderSession.h"
#include "NMEAUtils.h"
#include "SequentialTestDevice.h"

namespace {
const QByteArray FIX =
    "$GPRMC,092750.000,A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A*43\r\n"
    "$GPGGA,092750.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,*76\r\n";
}  // namespace

void NMEADecoderSessionTest::_decoderSessionRestart()
{
    SequentialTestDevice input;
    NMEADecoderSession session;
    QVERIFY(session.start(&input));
    session.positionSource()->startUpdates();
    // Mixed binary traffic and split sentences must still produce a usable fix.
    input.feed(QByteArray::fromHex("b5620000") + FIX.first(19));
    QVERIFY(!session.health()->usable());
    input.feed(FIX.mid(19));
    // This executable has no application test harness; allow Qt's realtime NMEA timer to fire.
    QTRY_VERIFY_WITH_TIMEOUT(session.health()->usable(), 5000);
    QVERIFY(session.health()->coordinate().isValid());

    session.stop();
    QVERIFY(input.isOpen());
    QCOMPARE(session.health()->state(), GPSSourceHealth::State::NoData);
    QVERIFY(session.start(&input));
    session.positionSource()->startUpdates();
    input.feed(NMEAUtils::repairChecksum("$GPRMC,092751.000,A,5321.6802,N,00630.3372,W,2.0,31.66,280511,,,A"));
    QSignalSpy fixes(session.positionSource(), &QGeoPositionInfoSource::positionUpdated);
    QTRY_VERIFY_WITH_TIMEOUT(!fixes.isEmpty(), 5000);
    const auto position = fixes.last().first().value<QGeoPositionInfo>();
    QVERIFY(!position.hasAttribute(QGeoPositionInfo::HorizontalAccuracy));
}

void NMEADecoderSessionTest::_fixLossInvalidatesHealth()
{
    SequentialTestDevice input;
    NMEADecoderSession session;
    QVERIFY(session.start(&input));
    session.positionSource()->startUpdates();
    input.feed(FIX);
    QTRY_VERIFY_WITH_TIMEOUT(session.health()->usable(), 1000);
    input.feed(NMEAUtils::repairChecksum("$GPGGA,092751.000,,,,,0,0,,,,,,,"));
    QTRY_COMPARE_WITH_TIMEOUT(session.health()->state(), GPSSourceHealth::State::Invalid, 1000);
    QVERIFY(!session.health()->acceptedObservation());
}

UT_REGISTER_TEST(NMEADecoderSessionTest, TestLabel::Unit)

void NMEADecoderSessionTest::_activityAndExpiry()
{
    ManualScheduler scheduler(nullptr, MonotonicClock::nowUs());
    SequentialTestDevice input(&scheduler);
    NMEADecoderSession session(nullptr, &scheduler);
    QVERIFY(session.start(&input));
    QVERIFY(!session.receiving());
    QVERIFY(!session.hasReceivedData());
    // Raw traffic is distinct from either a valid sentence or a usable position.
    input.feed("not NMEA\n");
    QVERIFY(session.hasReceivedData());
    QVERIFY(session.receiving());
    QVERIFY(!session.health()->usable());
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(5)));
    QVERIFY(!session.receiving());
    QVERIFY(session.hasReceivedData());
    session.stop();
    QVERIFY(!session.receiving());
    QVERIFY(!session.hasReceivedData());
    QVERIFY(input.isOpen());
}

void NMEADecoderSessionTest::_closeRetiresHealth()
{
    SequentialTestDevice input;
    NMEADecoderSession session;
    QVERIFY(session.start(&input));
    input.feed(FIX);
    QTRY_VERIFY_WITH_TIMEOUT(session.health()->usable(), TestTimeout::mediumMs());
    QCOMPARE(session.health()->satellitesInUseCount(), 8);
    input.close();
    QTRY_VERIFY_WITH_TIMEOUT(!session.positionSource(), TestTimeout::shortMs());
    QVERIFY(!session.health()->usable());
    QCOMPARE(session.health()->satellitesInUseCount(), -1);
    QVERIFY(!session.receiving());
}

void NMEADecoderSessionTest::_ordinaryDeviceUsesSessionClock()
{
    ManualScheduler scheduler;
    QBuffer input;
    input.setData("not NMEA\n");
    QVERIFY(input.open(QIODevice::ReadOnly));
    NMEADecoderSession session(nullptr, &scheduler);
    QVERIFY(session.start(&input));
    QTRY_VERIFY_WITH_TIMEOUT(session.hasReceivedData(), TestTimeout::shortMs());
    QVERIFY(session.receiving());
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(5)));
    QVERIFY(!session.receiving());
}

void NMEADecoderSessionTest::_delayedInputRetainsReceiptAge()
{
    ManualScheduler scheduler;
    SequentialTestDevice input(&scheduler);
    const auto receipt = scheduler.nowUs();
    input.feed(FIX, false);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(6)));
    NMEADecoderSession session(nullptr, &scheduler);
    QVERIFY(session.start(&input));
    QTRY_VERIFY_WITH_TIMEOUT(session.hasReceivedData(), TestTimeout::shortMs());
    QCOMPARE(input.lastReadTimestampUs(), receipt);
    QVERIFY(!session.receiving());
    QVERIFY(!session.health()->usable());
    input.feed("new traffic\n");
    QCOMPARE(input.lastReadTimestampUs(), scheduler.nowUs());
    QVERIFY(session.receiving());
}
