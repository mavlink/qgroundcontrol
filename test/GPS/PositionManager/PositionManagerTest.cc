#include "PositionManagerTest.h"

#include <QtCore/QIODevice>
#include <QtCore/QRegularExpression>
#include <QtPositioning/QNmeaPositionInfoSource>
#include <QtTest/QSignalSpy>

#include <cstring>

#include "NMEAUtils.h"
#include "PositionManager.h"
#include "RTKPositionSource.h"
#include "UdpIODevice.h"

namespace {

// Fix at 53.361337, -6.50562 (RMC provides the date, GGA provides HDOP for accuracy)
constexpr const char* kNmeaSentences =
    "$GPRMC,092750.000,A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A*43\r\n"
    "$GPGGA,092750.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,*76\r\n";

constexpr double kExpectedLat = 53.361337;
constexpr double kExpectedLon = -6.50562;
constexpr double kCoordEpsilon = 0.0001;

/// Sequential QIODevice that hands queued NMEA bytes to QNmeaPositionInfoSource on demand.
class NmeaTestDevice : public QIODevice
{
public:
    NmeaTestDevice() { (void) open(QIODevice::ReadOnly); }

    bool isSequential() const override { return true; }
    qint64 bytesAvailable() const override { return _data.size() + QIODevice::bytesAvailable(); }
    bool canReadLine() const override { return _data.contains('\n') || QIODevice::canReadLine(); }

    void feed(const QByteArray &data)
    {
        _data.append(data);
        emit readyRead();
    }

protected:
    qint64 readData(char *data, qint64 maxSize) override
    {
        const qint64 count = qMin<qint64>(maxSize, _data.size());
        (void) memcpy(data, _data.constData(), count);
        _data.remove(0, count);
        return count;
    }

    qint64 writeData(const char *, qint64) override { return -1; }

private:
    QByteArray _data;
};

} // namespace

void PositionManagerTest::init()
{
    UnitTest::init();
    // Headless CI has no real position source, so the internal GPS fallback
    // times out waiting for updates. Expected and benign in this fixture.
    ignoreLogMessage("GPS.PositionManager.QGCPositionManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("UpdateTimeoutError")));
}

void PositionManagerTest::cleanup()
{
    // QGCPositionManager is an application-static singleton — always tear down the NMEA source
    // so a failed test can't leak state into the next one. The device must be deleted after the
    // source, since QNmeaPositionInfoSource holds a raw pointer to it.
    QGCPositionManager::instance()->resetNmeaSourceDevice();
    delete _nmeaDevice;
    _nmeaDevice = nullptr;

    UnitTest::cleanup();
}

void PositionManagerTest::_nmeaSourceProducesGcsPosition()
{
    QGCPositionManager *pm = QGCPositionManager::instance();
    auto *device = new NmeaTestDevice();
    _nmeaDevice = device;

    pm->setNmeaSourceDevice(device);
    device->feed(kNmeaSentences);

    QTRY_VERIFY_WITH_TIMEOUT(pm->gcsPosition().isValid(), TestTimeout::mediumMs());
    QVERIFY(qAbs(pm->gcsPosition().latitude() - kExpectedLat) < kCoordEpsilon);
    QVERIFY(qAbs(pm->gcsPosition().longitude() - kExpectedLon) < kCoordEpsilon);
    QVERIFY(pm->gcsPositionHorizontalAccuracy() < 100.);
}

void PositionManagerTest::_resetNmeaSourceTearsDownAndClearsState()
{
    QGCPositionManager *pm = QGCPositionManager::instance();
    auto *device = new NmeaTestDevice();
    _nmeaDevice = device;

    pm->setNmeaSourceDevice(device);
    device->feed(kNmeaSentences);
    QTRY_VERIFY_WITH_TIMEOUT(pm->gcsPosition().isValid(), TestTimeout::mediumMs());

    QSignalSpy positionInfoSpy(pm, &QGCPositionManager::positionInfoUpdated);
    QVERIFY(positionInfoSpy.isValid());

    pm->resetNmeaSourceDevice();

    // Stale GCS state must be cleared on teardown
    QVERIFY(!pm->gcsPosition().isValid());
    QVERIFY(qIsInf(pm->gcsPositionHorizontalAccuracy()));
    QVERIFY(!positionInfoSpy.isEmpty());

    // The NMEA source is gone: further data must not resurrect the position
    positionInfoSpy.clear();
    device->feed(kNmeaSentences);
    QVERIFY(!positionInfoSpy.wait(TestTimeout::shortMs()));
    QVERIFY(!pm->gcsPosition().isValid());

    // Second reset with no NMEA source is a no-op
    pm->resetNmeaSourceDevice();
}

void PositionManagerTest::_idleNmeaWaitsForFirstFix()
{
    NmeaTestDevice device;
    QGCPositionManager pm;
    pm._externalStaleTimer.setInterval(50);
    pm.setNmeaSourceDevice(&device);
    QSignalSpy updates(&pm, &QGCPositionManager::positionInfoUpdated);

    QVERIFY(!updates.wait(100));
    QVERIFY(!pm.gcsPosition().isValid());
    QCOMPARE(pm.gcsPositioningError(), QGeoPositionInfoSource::NoError);
    QVERIFY(!pm._externalStaleTimer.isActive());

    device.feed(kNmeaSentences);
    QTRY_VERIFY_WITH_TIMEOUT(pm.gcsPosition().isValid(), TestTimeout::mediumMs());
    QVERIFY(pm._externalStaleTimer.isActive());
    pm.resetNmeaSourceDevice();
}

void PositionManagerTest::_nmeaUpdatesStayHealthyUntilStale()
{
    NmeaTestDevice device;
    QGCPositionManager pm;
    pm._externalStaleTimer.setInterval(300);
    pm.setNmeaSourceDevice(&device);
    QSignalSpy errors(pm._nmeaSource, &QGeoPositionInfoSource::errorOccurred);
    QSignalSpy updates(&pm, &QGCPositionManager::positionInfoUpdated);
    const auto feed = [&]() {
        const QByteArray time = QDateTime::currentDateTimeUtc().toString(QStringLiteral("hhmmss.zzz")).toLatin1();
        QByteArray sentences;
        for (QByteArray line : QByteArray(kNmeaSentences).split('\n')) {
            if (!line.trimmed().isEmpty()) {
                line.replace("092750.000", time);
                sentences += NMEAUtils::repairChecksum(line);
            }
        }
        device.feed(sentences);
    };
    QTimer sender;
    connect(&sender, &QTimer::timeout, this, feed);
    sender.start(50);
    QTRY_VERIFY_WITH_TIMEOUT(updates.size() >= 6, TestTimeout::mediumMs());
    QVERIFY(pm.gcsPosition().isValid());
    QVERIFY(errors.isEmpty());
    QCOMPARE(pm.gcsPositioningError(), QGeoPositionInfoSource::NoError);

    sender.stop();
    QTRY_VERIFY_WITH_TIMEOUT(!pm.gcsPosition().isValid(), TestTimeout::mediumMs());
    QCOMPARE(pm.gcsPositioningError(), QGeoPositionInfoSource::UpdateTimeoutError);
    QVERIFY(!pm.geoPositionInfo().isValid());
    QVERIFY(!pm.gcsPositionTimestamp().isValid());
    QVERIFY(qIsInf(pm.gcsPositionHorizontalAccuracy()));
    QVERIFY(!pm._externalStaleTimer.isActive());

    feed();
    QTRY_VERIFY_WITH_TIMEOUT(pm.gcsPosition().isValid(), TestTimeout::mediumMs());
    QCOMPARE(pm.gcsPositioningError(), QGeoPositionInfoSource::NoError);
    QVERIFY(pm._externalStaleTimer.isActive());
    pm.resetNmeaSourceDevice();
    QVERIFY(!pm._externalStaleTimer.isActive());
}

UT_REGISTER_TEST(PositionManagerTest, TestLabel::Unit)

namespace {
sensor_gps_s receiverFix()
{
    sensor_gps_s fix{};
    fix.fix_type = sensor_gps_s::FIX_TYPE_RTK_FIXED;
    fix.latitude_deg = 47;
    fix.longitude_deg = 8;
    fix.altitude_msl_m = 500;
    fix.eph = 0.1f;
    fix.epv = 0.2f;
    fix.vel_ned_valid = true;
    fix.cog_rad = 1;
    fix.c_variance_rad = 0.01f;
    return fix;
}
}  // namespace

void PositionManagerTest::_receiverPriorityAndFallback()
{
    NmeaTestDevice device;
    RTKPositionSource receiver;
    QGCPositionManager pm;
    pm.setNmeaSourceDevice(&device);
    device.feed(kNmeaSentences);
    QTRY_VERIFY_WITH_TIMEOUT(pm.gcsPosition().isValid(), TestTimeout::mediumMs());
    pm.setReceiverPositionSource(&receiver);
    QVERIFY(!pm.gcsPosition().isValid());
    receiver.updatePosition(receiverFix());
    QCOMPARE(pm.gcsPosition(), QGeoCoordinate(47, 8, 500));
    QCOMPARE(pm._currentSource, &receiver);
    // Changing a standby NMEA connection must not interrupt receiver fixes.
    pm.resetNmeaSourceDevice();
    pm.setNmeaSourceDevice(&device);
    QCOMPARE(pm.gcsPosition(), QGeoCoordinate(47, 8, 500));
    pm._selectPositionSource();
    QCOMPARE(pm._currentSource, &receiver);
    device.feed(kNmeaSentences);
    pm.clearReceiverPositionSource(&receiver);
    QVERIFY(!pm.gcsPosition().isValid());
    QSignalSpy resumedUpdates(&pm, &QGCPositionManager::positionInfoUpdated);
    QVERIFY(!resumedUpdates.wait(TestTimeout::shortMs()));
    receiver.updatePosition(receiverFix());
    QVERIFY(!pm.gcsPosition().isValid());
    device.feed(kNmeaSentences);
    QTRY_VERIFY_WITH_TIMEOUT(pm.gcsPosition().isValid(), TestTimeout::mediumMs());
    QVERIFY(qAbs(pm.gcsPosition().latitude() - kExpectedLat) < kCoordEpsilon);
    pm.resetNmeaSourceDevice();
}

void PositionManagerTest::_receiverFallbackOpensStandbyUdpSource()
{
    UdpIODevice device;
    QVERIFY(device.bind(QHostAddress::LocalHost, 0));
    RTKPositionSource receiver;
    QGCPositionManager pm;
    pm.setReceiverPositionSource(&receiver);
    receiver.updatePosition(receiverFix());
    pm.setNmeaSourceDevice(&device);
    QVERIFY(!device.isOpen());
    QCOMPARE(pm._currentSource, &receiver);

    QUdpSocket sender;
    const QByteArray sentences(kNmeaSentences);
    QSignalSpy datagrams(&device, &QIODevice::readyRead);
    QCOMPARE(sender.writeDatagram(sentences, QHostAddress::LocalHost, device.localPort()), sentences.size());
    QTRY_VERIFY_WITH_TIMEOUT(!datagrams.isEmpty(), TestTimeout::mediumMs());

    pm.clearReceiverPositionSource(&receiver);
    QVERIFY(device.isReadable());
    QCOMPARE(pm._currentSource, pm._nmeaSource);
    QVERIFY(!pm.gcsPosition().isValid());
    QSignalSpy updates(&pm, &QGCPositionManager::positionInfoUpdated);
    QVERIFY(!updates.wait(100));
    QCOMPARE(sender.writeDatagram(sentences, QHostAddress::LocalHost, device.localPort()), sentences.size());
    QTRY_VERIFY_WITH_TIMEOUT(pm.gcsPosition().isValid(), TestTimeout::mediumMs());
    QVERIFY(qAbs(pm.gcsPosition().latitude() - kExpectedLat) < kCoordEpsilon);
    pm.resetNmeaSourceDevice();
}

void PositionManagerTest::_receiverInvalidAndStaleFixes()
{
    RTKPositionSource receiver;
    QGCPositionManager pm;
    pm._externalStaleTimer.setInterval(50);
    pm.setReceiverPositionSource(&receiver);
    auto fix = receiverFix();
    receiver.updatePosition(fix);
    QVERIFY(pm.geoPositionInfo().isValid());
    QVERIFY(qIsFinite(pm.gcsHeading()));
    fix.fix_type = sensor_gps_s::FIX_TYPE_NONE;
    receiver.updatePosition(fix);
    QVERIFY(!pm.geoPositionInfo().isValid());
    QVERIFY(!pm.gcsPosition().isValid());
    QVERIFY(!pm.gcsPositionTimestamp().isValid());
    QVERIFY(qIsNaN(pm.gcsHeading()));
    fix = receiverFix();
    receiver.updatePosition(fix);
    QCOMPARE(pm.gcsPositioningError(), QGeoPositionInfoSource::NoError);
    fix.eph = 101;
    receiver.updatePosition(fix);
    QVERIFY(!pm.gcsPosition().isValid());
    QVERIFY(!pm.geoPositionInfo().isValid());
    fix = receiverFix();
    fix.fix_type = sensor_gps_s::FIX_TYPE_2D;
    fix.vel_ned_valid = false;
    fix.latitude_deg = 0;
    fix.longitude_deg = 0;
    receiver.updatePosition(fix);
    QCOMPARE(pm.gcsPosition(), QGeoCoordinate(0, 0));
    QVERIFY(qIsNaN(pm.gcsHeading()));
    QVERIFY(qIsInf(pm._gcsPositionVerticalAccuracy));
    QTRY_VERIFY_WITH_TIMEOUT(!pm.gcsPosition().isValid(), TestTimeout::mediumMs());
    QCOMPARE(pm.gcsPositioningError(), QGeoPositionInfoSource::UpdateTimeoutError);
    QVERIFY(!pm.geoPositionInfo().isValid());
    QVERIFY(!pm.gcsPositionTimestamp().isValid());
    receiver.updatePosition(receiverFix());
    QCOMPARE(pm.gcsPosition(), QGeoCoordinate(47, 8, 500));
    QCOMPARE(pm.gcsPositioningError(), QGeoPositionInfoSource::NoError);
    pm.clearReceiverPositionSource(&receiver);
    QVERIFY(!pm._externalStaleTimer.isActive());
}

void PositionManagerTest::_receiverDestructionRestoresDefault()
{
    RTKPositionSource platform;
    QGCPositionManager pm;
    pm._defaultSource = &platform;
    auto receiver = std::make_unique<RTKPositionSource>();
    pm.setReceiverPositionSource(receiver.get());
    receiver->updatePosition(receiverFix());
    QVERIFY(pm.gcsPosition().isValid());
    receiver.reset();
    QCOMPARE(pm._currentSource, &platform);
    QVERIFY(!pm.gcsPosition().isValid());
    QVERIFY(!pm.geoPositionInfo().isValid());
    platform.updatePosition(receiverFix());
    QVERIFY(pm.gcsPosition().isValid());
}
