#include "NMEAPositionSourceTest.h"

#include <QtCore/QEvent>
#include <QtCore/QIODevice>
#include <QtPositioning/QNmeaPositionInfoSource>
#include <QtTest/QSignalSpy>

#include <cstring>

#include "GPSReadTimestamp.h"
#include "NMEAPositionSource.h"
#include "NMEAUtils.h"

namespace {
const QByteArray kFix =
    "$GPRMC,092750.000,A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A*43\r\n"
    "$GPGGA,092750.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,*76\r\n";

class NMEAInput : public QIODevice, public GPSReadTimestamp
{
public:
    NMEAInput() { open(QIODevice::ReadOnly); }

    quint64 receivedAtUs = 0;

    quint64 lastReadTimestampUs() const override
    {
        return receivedAtUs ? receivedAtUs : GPSObservation::monotonicNowUs();
    }

    bool isSequential() const override { return true; }

    qint64 bytesAvailable() const override { return _data.size() + QIODevice::bytesAvailable(); }

    bool canReadLine() const override { return _data.contains('\n') || QIODevice::canReadLine(); }

    void feed(const QByteArray& data)
    {
        _data.append(data);
        emit readyRead();
    }

protected:
    qint64 readData(char* data, qint64 maxSize) override
    {
        const qint64 count = qMin<qint64>(maxSize, _data.size());
        memcpy(data, _data.constData(), count);
        _data.remove(0, count);
        return count;
    }

    qint64 writeData(const char*, qint64) override { return -1; }

private:
    QByteArray _data;
};
}  // namespace

void NMEAPositionSourceTest::_restartClearsParserState()
{
    NMEAInput device;
    NMEAPositionSource source(&device);
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    source.startUpdates();
    device.feed(kFix);
    QTRY_VERIFY_WITH_TIMEOUT(!updates.isEmpty(), TestTimeout::mediumMs());
    QVERIFY(source.lastKnownPosition().isValid());
    source.stopUpdates();
    device.feed(kFix);
    source.startUpdates();
    QVERIFY(!source.lastKnownPosition().isValid());
    updates.clear();
    // An RMC without HDOP must not inherit the accuracy cached before the restart.
    device.feed(NMEAUtils::repairChecksum("$GPRMC,092751.000,A,5321.6802,N,00630.3372,W,2.0,31.66,280511,,,A"));
    QTRY_VERIFY_WITH_TIMEOUT(!updates.isEmpty(), TestTimeout::mediumMs());
    const auto fix = updates.last().first().value<QGeoPositionInfo>();
    QVERIFY(!fix.hasAttribute(QGeoPositionInfo::HorizontalAccuracy));
    QVERIFY(device.isOpen());
}

void NMEAPositionSourceTest::_pendingRequestSurvivesStopAndStart()
{
    NMEAInput device;
    NMEAPositionSource source(&device);
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    source.requestUpdate(1000);
    source.stopUpdates();
    source.startUpdates();
    source.stopUpdates();
    device.feed(kFix);
    QTRY_COMPARE_WITH_TIMEOUT(updates.size(), 1, TestTimeout::mediumMs());
    QVERIFY(source.lastKnownPosition().isValid());
    QCOMPARE(source.error(), QGeoPositionInfoSource::NoError);
}

void NMEAPositionSourceTest::_requestTimeoutAndRecovery()
{
    NMEAInput device;
    NMEAPositionSource source(&device);
    QSignalSpy errors(&source, &QGeoPositionInfoSource::errorOccurred);
    source.requestUpdate(50);
    source.requestUpdate(1000);
    source.requestUpdate(-1);
    QTRY_COMPARE_WITH_TIMEOUT(errors.size(), 1, TestTimeout::mediumMs());
    QCOMPARE(source.error(), QGeoPositionInfoSource::UpdateTimeoutError);
    source.requestUpdate(1000);
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    device.feed(kFix);
    QTRY_COMPARE_WITH_TIMEOUT(updates.size(), 1, TestTimeout::mediumMs());
    QCOMPARE(source.error(), QGeoPositionInfoSource::NoError);
}

void NMEAPositionSourceTest::_queuedUpdateCannotSurviveRestart()
{
    NMEAInput device;
    NMEAPositionSource source(&device);
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    source.startUpdates();
    QSignalSpy decoded(source._decoder.get(), &QGeoPositionInfoSource::positionUpdated);
    device.feed(kFix);
    if (decoded.isEmpty()) {
        QVERIFY(decoded.wait(TestTimeout::mediumMs()));
    }
    QVERIFY(source.lastKnownPosition().isValid());
    QVERIFY(updates.isEmpty());
    source.stopUpdates();
    source.startUpdates();
    QCoreApplication::sendPostedEvents(&source, QEvent::MetaCall);
    QVERIFY(updates.isEmpty());
    QVERIFY(!source.lastKnownPosition().isValid());
    device.feed(kFix);
    QTRY_VERIFY_WITH_TIMEOUT(!updates.isEmpty(), TestTimeout::mediumMs());
}

UT_REGISTER_TEST(NMEAPositionSourceTest, TestLabel::Unit)

void NMEAPositionSourceTest::_fixMetadata_data()
{
    QTest::addColumn<int>("quality");
    QTest::addColumn<GPSObservation::FixQuality>("expected");
    QTest::newRow("autonomous") << 1 << GPSObservation::FixQuality::Fix3D;
    QTest::newRow("differential") << 2 << GPSObservation::FixQuality::Differential;
    QTest::newRow("rtk-fixed") << 4 << GPSObservation::FixQuality::RTKFixed;
    QTest::newRow("rtk-float") << 5 << GPSObservation::FixQuality::RTKFloat;
}

void NMEAPositionSourceTest::_fixMetadata()
{
    QFETCH(int, quality);
    QFETCH(GPSObservation::FixQuality, expected);
    NMEAInput device;
    NMEAPositionSource source(&device);
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    source.startUpdates();
    device.receivedAtUs = GPSObservation::monotonicNowUs() - 2000000;
    const QByteArray rmc =
        NMEAUtils::repairChecksum("$GPRMC,092750.000,A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A");
    const QByteArray gga = NMEAUtils::repairChecksum("$GPGGA,092750.000,5321.6802,N,00630.3372,W," +
                                                     QByteArray::number(quality) + ",8,1.03,61.7,M,55.2,M,,");
    device.feed(rmc + gga + NMEAUtils::repairChecksum("$GPGSA,A,3,02,,,,,,,,,,,,1.0,1.03,0.6"));
    QTRY_VERIFY_WITH_TIMEOUT(!updates.isEmpty(), TestTimeout::mediumMs());
    const auto observation = source.lastObservation();
    QCOMPARE(observation.fixQuality, expected);
    QCOMPARE(observation.satellitesUsed, std::optional<int>(8));
    QCOMPARE(observation.horizontalDop, std::optional<double>(1.03));
    QCOMPARE(observation.verticalDop, std::optional<double>(0.6));
    QCOMPARE(observation.altitudeDatum, GPSObservation::AltitudeDatum::MeanSeaLevel);
    QVERIFY(observation.altitudeEllipsoidMeters.has_value());
    QVERIFY(qAbs(*observation.altitudeEllipsoidMeters - 116.9) < 0.00001);
    QCOMPARE(observation.monotonicTimestampUs, device.receivedAtUs);
}

void NMEAPositionSourceTest::_metadataDoesNotCrossEpochs()
{
    NMEAInput device;
    NMEAPositionSource source(&device);
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    source.startUpdates();
    device.feed(kFix);
    QTRY_VERIFY_WITH_TIMEOUT(!updates.isEmpty(), TestTimeout::mediumMs());
    QVERIFY(source.lastObservation().satellitesUsed.has_value());
    updates.clear();
    device.feed(NMEAUtils::repairChecksum("$GPRMC,092751.000,A,5321.6802,N,00630.3372,W,2.0,31.66,280511,,,A"));
    QTRY_VERIFY_WITH_TIMEOUT(!updates.isEmpty(), TestTimeout::mediumMs());
    const auto observation = source.lastObservation();
    QCOMPARE(observation.fixQuality, GPSObservation::FixQuality::Unknown);
    QVERIFY(!observation.satellitesUsed);
    QVERIFY(!observation.horizontalDop);
    QVERIFY(!observation.position.hasAttribute(QGeoPositionInfo::HorizontalAccuracy));
    QVERIFY(!observation.verticalDop);
    QVERIFY(!observation.altitudeEllipsoidMeters);
    QCOMPARE(observation.altitudeDatum, GPSObservation::AltitudeDatum::Unknown);
}

void NMEAPositionSourceTest::_gstAccuracy_data()
{
    QTest::addColumn<bool>("beforeFix");
    QTest::addColumn<bool>("validChecksum");
    QTest::newRow("before-fix") << true << true;
    QTest::newRow("after-fix") << false << true;
    QTest::newRow("bad-checksum") << false << false;
}

void NMEAPositionSourceTest::_gstAccuracy()
{
    QFETCH(bool, beforeFix);
    QFETCH(bool, validChecksum);
    NMEAInput device;
    NMEAPositionSource source(&device);
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    source.startUpdates();
    auto gst = NMEAUtils::repairChecksum("$GPGST,235959.000,1,1,1,0,3,4,6");
    if (!validChecksum) {
        gst[gst.indexOf('*') + 1] = 'X';
    }
    const auto fix = NMEAUtils::repairChecksum("$GPRMC,235959.000,A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A") +
                     NMEAUtils::repairChecksum("$GPGGA,235959.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,");
    device.feed((beforeFix ? gst + fix : fix + gst) +
                NMEAUtils::repairChecksum("$GPGSA,A,3,02,,,,,,,,,,,,1.0,1.03,0.6"));
    QTRY_VERIFY_WITH_TIMEOUT(!updates.isEmpty(), TestTimeout::mediumMs());
    const auto observation = source.lastObservation();
    if (validChecksum) {
        QCOMPARE(observation.position.attribute(QGeoPositionInfo::HorizontalAccuracy), 5.0);
        QCOMPARE(observation.position.attribute(QGeoPositionInfo::VerticalAccuracy), 6.0);
        QVERIFY(observation.accuracyTimestampUs != 0);
    } else {
        QVERIFY(observation.position.attribute(QGeoPositionInfo::HorizontalAccuracy) != 5.0);
        QCOMPARE(observation.accuracyTimestampUs, 0U);
    }
    updates.clear();
    device.feed(NMEAUtils::repairChecksum("$GPRMC,000000.000,A,5321.6802,N,00630.3372,W,0.02,31.66,290511,,,A"));
    QTRY_VERIFY_WITH_TIMEOUT(!updates.isEmpty(), TestTimeout::mediumMs());
    QVERIFY(!source.lastObservation().position.hasAttribute(QGeoPositionInfo::HorizontalAccuracy));
    QCOMPARE(source.lastObservation().accuracyTimestampUs, 0U);
    QCOMPARE(source.lastObservation().position.timestamp().date(), QDate(2011, 5, 29));
}
