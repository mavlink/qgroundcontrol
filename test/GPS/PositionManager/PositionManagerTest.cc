#include "PositionManagerTest.h"

#include <QtCore/QIODevice>
#include <QtCore/QRegularExpression>
#include <QtCore/QTimeZone>
#include <QtTest/QSignalSpy>

#include <cstring>

#include "NMEAUtils.h"
#include "NMEAPositionSource.h"
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

void PositionManagerTest::_nmeaSourceProducesGcsPosition()
{
    QGCPositionManager manager;
    auto* pm = &manager;
    NmeaTestDevice input;
    auto* device = &input;
    NMEAPositionSource source(device);

    pm->setNmeaPositionSource(&source);
    device->feed(kNmeaSentences);

    QTRY_VERIFY_WITH_TIMEOUT(pm->gcsPosition().isValid(), TestTimeout::mediumMs());
    QVERIFY(qAbs(pm->gcsPosition().latitude() - kExpectedLat) < kCoordEpsilon);
    QVERIFY(qAbs(pm->gcsPosition().longitude() - kExpectedLon) < kCoordEpsilon);
    QVERIFY(pm->gcsPositionHorizontalAccuracy() < 100.);
}

void PositionManagerTest::_nmeaCourseFromRmc()
{
    NmeaTestDevice device;
    NMEAPositionSource source(&device);
    QGCPositionManager pm;
    pm.setNmeaPositionSource(&source);
    const auto feed = [&](const QByteArray& time, const QByteArray& knots) {
        QByteArray sentences;
        for (QByteArray line : QByteArray(kNmeaSentences).split('\n')) {
            if (!line.trimmed().isEmpty()) {
                line.replace("092750.000", time);
                line.replace("5321.6802", "0000.0000");
                line.replace("00630.3372", "00000.0000");
                line.replace(",0.02,31.66,", ',' + knots + ",31.66,");
                sentences += NMEAUtils::repairChecksum(line);
            }
        }
        device.feed(sentences);
    };
    feed("092750.000", "2.0");
    QTRY_VERIFY_WITH_TIMEOUT(qIsFinite(pm.gcsHeading()), TestTimeout::mediumMs());
    QCOMPARE(pm.gcsPosition().latitude(), 0);
    QCOMPARE(pm.gcsPosition().longitude(), 0);
    QCOMPARE(pm.gcsHeading(), 31.66);
    QVERIFY(!pm.geoPositionInfo().hasAttribute(QGeoPositionInfo::DirectionAccuracy));
    feed("092751.000", "0.0");
    QTRY_VERIFY_WITH_TIMEOUT(qIsNaN(pm.gcsHeading()), TestTimeout::mediumMs());
    QVERIFY(pm.gcsPosition().isValid());
    feed("092752.000", "2.0");
    QTRY_COMPARE_WITH_TIMEOUT(pm.gcsHeading(), 31.66, TestTimeout::mediumMs());
}

void PositionManagerTest::_positionValidation_data()
{
    QTest::addColumn<QGeoPositionInfo>("update");
    QTest::addColumn<bool>("accepted");
    QGeoPositionInfo update(QGeoCoordinate(47.5, 8.5), QDateTime::currentDateTimeUtc());
    update.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 5);
    QTest::newRow("ordinary-fix") << update << true;
    const auto coordinateRow = [&](const char* name, const QGeoCoordinate& coordinate, bool accepted) {
        auto sample = update;
        sample.setCoordinate(coordinate);
        QTest::newRow(name) << sample << accepted;
    };
    coordinateRow("equator", QGeoCoordinate(0, 8), true);
    coordinateRow("prime-meridian", QGeoCoordinate(47, 0), true);
    coordinateRow("origin", QGeoCoordinate(0, 0), true);
    coordinateRow("near-equator", QGeoCoordinate(-0.0005, 8), true);
    coordinateRow("near-prime-meridian", QGeoCoordinate(47, 0.0005), true);
    coordinateRow("north-pole", QGeoCoordinate(90, 180), true);
    coordinateRow("south-pole", QGeoCoordinate(-90, -180), true);
    coordinateRow("bad-latitude", QGeoCoordinate(91, 8), false);
    coordinateRow("bad-longitude", QGeoCoordinate(47, 181), false);
    coordinateRow("nan-coordinate", QGeoCoordinate(qQNaN(), 8), false);
    auto sample = update;
    sample.setTimestamp(QDateTime());
    QTest::newRow("missing-time") << sample << false;
    sample = update;
    sample.removeAttribute(QGeoPositionInfo::HorizontalAccuracy);
    QTest::newRow("missing-accuracy") << sample << false;
    const auto accuracyRow = [&](const char* name, double accuracy, bool accepted) {
        auto fix = update;
        fix.setAttribute(QGeoPositionInfo::HorizontalAccuracy, accuracy);
        QTest::newRow(name) << fix << accepted;
    };
    accuracyRow("zero-accuracy", 0, false);
    accuracyRow("negative-accuracy", -1, false);
    accuracyRow("infinite-accuracy", qInf(), false);
    accuracyRow("nan-accuracy", qQNaN(), false);
    accuracyRow("accuracy-at-limit", 100, true);
    accuracyRow("accuracy-over-limit", 101, false);
}

void PositionManagerTest::_positionValidation()
{
    QFETCH(QGeoPositionInfo, update);
    QFETCH(bool, accepted);
    NmeaTestDevice device;
    NMEAPositionSource source(&device);
    QGCPositionManager pm;
    pm.setNmeaPositionSource(&source);
    QGeoPositionInfo initial(QGeoCoordinate(47, 8), QDateTime::currentDateTimeUtc());
    initial.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 5);
    pm._positionUpdated(initial);
    const auto oldTimestamp = QDateTime::fromMSecsSinceEpoch(1000, QTimeZone::UTC);
    pm._gcsPositionTimestamp = oldTimestamp;
    pm._positionUpdated(update);
    if (accepted) {
        QCOMPARE(pm.gcsPosition(), update.coordinate());
        QVERIFY(pm.gcsPositionTimestamp() > oldTimestamp);
    } else {
        QVERIFY(!pm.gcsPosition().isValid());
        QVERIFY(!pm.gcsPositionTimestamp().isValid());
    }
}

void PositionManagerTest::_nmeaCourseValidation_data()
{
    QTest::addColumn<QGeoPositionInfo>("update");
    QTest::addColumn<double>("heading");
    QGeoPositionInfo update(QGeoCoordinate(47, 8), QDateTime::currentDateTimeUtc());
    update.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 5);
    update.setAttribute(QGeoPositionInfo::Direction, 45);
    update.setAttribute(QGeoPositionInfo::GroundSpeed, 1);
    QTest::newRow("moving-without-direction-accuracy") << update << 45.;
    const auto attributeRow = [&](const char* name, QGeoPositionInfo::Attribute attribute, double value,
                                  double heading = qQNaN()) {
        auto fix = update;
        fix.setAttribute(attribute, value);
        QTest::newRow(name) << fix << heading;
    };
    attributeRow("north", QGeoPositionInfo::Direction, 0, 0);
    attributeRow("north-360", QGeoPositionInfo::Direction, 360, 0);
    attributeRow("negative-course", QGeoPositionInfo::Direction, -1);
    attributeRow("course-over-range", QGeoPositionInfo::Direction, 361);
    attributeRow("nan-course", QGeoPositionInfo::Direction, qQNaN());
    attributeRow("infinite-course", QGeoPositionInfo::Direction, qInf());
    attributeRow("stationary", QGeoPositionInfo::GroundSpeed, 0);
    attributeRow("nearly-stationary", QGeoPositionInfo::GroundSpeed, 0.49);
    attributeRow("speed-at-limit", QGeoPositionInfo::GroundSpeed, 0.5, 45);
    attributeRow("negative-speed", QGeoPositionInfo::GroundSpeed, -1);
    attributeRow("nan-speed", QGeoPositionInfo::GroundSpeed, qQNaN());
    attributeRow("infinite-speed", QGeoPositionInfo::GroundSpeed, qInf());
    attributeRow("poor-position", QGeoPositionInfo::HorizontalAccuracy, 101);
    attributeRow("good-direction-accuracy", QGeoPositionInfo::DirectionAccuracy, 5, 45);
    attributeRow("poor-direction-accuracy", QGeoPositionInfo::DirectionAccuracy, 31);
    attributeRow("negative-direction-accuracy", QGeoPositionInfo::DirectionAccuracy, -1);
    attributeRow("nan-direction-accuracy", QGeoPositionInfo::DirectionAccuracy, qQNaN());
    auto sample = update;
    sample.removeAttribute(QGeoPositionInfo::Direction);
    QTest::newRow("missing-course") << sample << qQNaN();
    sample = update;
    sample.removeAttribute(QGeoPositionInfo::GroundSpeed);
    QTest::newRow("missing-speed") << sample << qQNaN();
    sample = update;
    sample.setCoordinate(QGeoCoordinate());
    QTest::newRow("invalid-position") << sample << qQNaN();
}

void PositionManagerTest::_nmeaCourseValidation()
{
    QFETCH(QGeoPositionInfo, update);
    QFETCH(double, heading);
    NmeaTestDevice device;
    NMEAPositionSource source(&device);
    QGCPositionManager pm;
    pm.setNmeaPositionSource(&source);
    pm._setGCSHeading(90);
    QSignalSpy changes(&pm, &QGCPositionManager::gcsHeadingChanged);
    pm._positionUpdated(update);
    if (qIsNaN(heading)) {
        QVERIFY(qIsNaN(pm.gcsHeading()));
    } else {
        QCOMPARE(pm.gcsHeading(), heading);
    }
    QCOMPARE(changes.size(), 1);
    pm._positionUpdated(update);
    QCOMPARE(changes.size(), 1);
}

void PositionManagerTest::_clearNmeaSourceDetachesAndClearsState()
{
    QGCPositionManager manager;
    auto* pm = &manager;
    NmeaTestDevice input;
    auto* device = &input;
    NMEAPositionSource source(device);

    pm->setNmeaPositionSource(&source);
    device->feed(kNmeaSentences);
    QTRY_VERIFY_WITH_TIMEOUT(pm->gcsPosition().isValid(), TestTimeout::mediumMs());

    QSignalSpy positionInfoSpy(pm, &QGCPositionManager::positionInfoUpdated);
    QVERIFY(positionInfoSpy.isValid());

    pm->clearNmeaPositionSource(&source);

    // Stale GCS state must be cleared on teardown
    QVERIFY(!pm->gcsPosition().isValid());
    QVERIFY(qIsInf(pm->gcsPositionHorizontalAccuracy()));
    QVERIFY(!positionInfoSpy.isEmpty());

    // The borrowed source survives removal but cannot update the manager.
    positionInfoSpy.clear();
    device->feed(kNmeaSentences);
    QVERIFY(!positionInfoSpy.wait(TestTimeout::shortMs()));
    QVERIFY(!pm->gcsPosition().isValid());

    // Second reset with no NMEA source is a no-op
    pm->clearNmeaPositionSource(&source);
}

void PositionManagerTest::_idleNmeaWaitsForFirstFix()
{
    NmeaTestDevice device;
    NMEAPositionSource source(&device);
    QGCPositionManager pm;
    pm._externalHealth._freshnessTimeoutMs = 50;
    pm.setNmeaPositionSource(&source);
    QSignalSpy updates(&pm, &QGCPositionManager::positionInfoUpdated);

    QVERIFY(!updates.wait(100));
    QVERIFY(!pm.gcsPosition().isValid());
    QCOMPARE(pm.gcsPositioningError(), QGeoPositionInfoSource::NoError);
    QVERIFY(!pm._externalHealth._positionTimer.isActive());

    device.feed(kNmeaSentences);
    QTRY_VERIFY_WITH_TIMEOUT(pm.gcsPosition().isValid(), TestTimeout::mediumMs());
    QVERIFY(pm._externalHealth._positionTimer.isActive());
    pm.clearNmeaPositionSource(&source);
}

void PositionManagerTest::_nmeaUpdatesStayHealthyUntilStale()
{
    NmeaTestDevice device;
    NMEAPositionSource source(&device);
    QGCPositionManager pm;
    pm._externalHealth._freshnessTimeoutMs = 300;
    pm.setNmeaPositionSource(&source);
    QSignalSpy errors(pm._nmeaSource.data(), &QGeoPositionInfoSource::errorOccurred);
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
    QVERIFY(!pm._externalHealth._positionTimer.isActive());

    feed();
    QTRY_VERIFY_WITH_TIMEOUT(pm.gcsPosition().isValid(), TestTimeout::mediumMs());
    QCOMPARE(pm.gcsPositioningError(), QGeoPositionInfoSource::NoError);
    QVERIFY(pm._externalHealth._positionTimer.isActive());
    pm.clearNmeaPositionSource(&source);
    QVERIFY(!pm._externalHealth._positionTimer.isActive());
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
    fix.vel_m_s = 1;
    fix.cog_rad = 1;
    fix.c_variance_rad = 0.01f;
    return fix;
}
}  // namespace

void PositionManagerTest::_receiverPriorityAndFallback()
{
    NmeaTestDevice device;
    NMEAPositionSource source(&device);
    RTKPositionSource receiver;
    QGCPositionManager pm;
    pm.setNmeaPositionSource(&source);
    device.feed(kNmeaSentences);
    QTRY_VERIFY_WITH_TIMEOUT(pm.gcsPosition().isValid(), TestTimeout::mediumMs());
    pm.setReceiverPositionSource(&receiver);
    QVERIFY(!pm.gcsPosition().isValid());
    receiver.updatePosition(receiverFix());
    QCOMPARE(pm.gcsPosition(), QGeoCoordinate(47, 8, 500));
    QCOMPARE(pm._currentSource, &receiver);
    // Changing a standby NMEA connection must not interrupt receiver fixes.
    pm.clearNmeaPositionSource(&source);
    pm.setNmeaPositionSource(&source);
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
    pm.clearNmeaPositionSource(&source);
}

void PositionManagerTest::_receiverFallbackOpensStandbyUdpSource()
{
    UdpIODevice device;
    NMEAPositionSource source(&device);
    QVERIFY(device.bind(QHostAddress::LocalHost, 0));
    RTKPositionSource receiver;
    QGCPositionManager pm;
    pm.setReceiverPositionSource(&receiver);
    receiver.updatePosition(receiverFix());
    pm.setNmeaPositionSource(&source);
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
    pm.clearNmeaPositionSource(&source);
}

void PositionManagerTest::_receiverInvalidAndStaleFixes()
{
    RTKPositionSource receiver;
    QGCPositionManager pm;
    pm._externalHealth._freshnessTimeoutMs = 50;
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
    QVERIFY(!pm._externalHealth._positionTimer.isActive());
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

void PositionManagerTest::_borrowedNmeaSourceLifetime()
{
    RTKPositionSource platform;
    QGCPositionManager pm;
    pm._defaultSource = &platform;
    auto source = std::make_unique<RTKPositionSource>();
    pm.setNmeaPositionSource(source.get());
    source->updatePosition(receiverFix());
    QVERIFY(pm.gcsPosition().isValid());
    auto replacement = std::make_unique<RTKPositionSource>();
    pm.setNmeaPositionSource(replacement.get());
    replacement->updatePosition(receiverFix());
    pm.clearNmeaPositionSource(source.get());
    source.reset();
    QCOMPARE(pm._currentSource, replacement.get());
    QVERIFY(pm.gcsPosition().isValid());
    RTKPositionSource receiver;
    pm.setReceiverPositionSource(&receiver);
    pm.clearReceiverPositionSource(&receiver);
    QCOMPARE(pm._currentSource, replacement.get());
    replacement.reset();
    QCOMPARE(pm._currentSource, &platform);
    QVERIFY(!pm.gcsPosition().isValid());
    QVERIFY(!pm.gcsPositionTimestamp().isValid());
    platform.updatePosition(receiverFix());
    QVERIFY(pm.gcsPosition().isValid());
}

void PositionManagerTest::_sharedHealthControlsPosition_data()
{
    QTest::addColumn<bool>("receiver");
    QTest::newRow("NMEA") << false;
    QTest::newRow("RTK") << true;
}

void PositionManagerTest::_sharedHealthControlsPosition()
{
    QFETCH(bool, receiver);
    RTKPositionSource source;
    GPSSourceHealth health;
    health._freshnessTimeoutMs = 100;
    QGCPositionManager pm;
    if (receiver) {
        pm.setReceiverPositionSource(&source, &health);
    } else {
        pm.setNmeaPositionSource(&source, &health);
    }
    QCOMPARE(pm.sourceHealth(), &health);
    QVERIFY(!pm.gcsPosition().isValid());
    // The session health is authoritative; a separate raw-source signal cannot bypass it.
    source.updatePosition(receiverFix());
    QVERIFY(!pm.gcsPosition().isValid());
    health.updatePosition(source.lastKnownPosition());
    QCOMPARE(pm.gcsPosition(), health.coordinate());
    QCOMPARE(pm.gcsPositionTimestamp(), health.receivedAt());
    QCOMPARE(pm.geoPositionInfo(), health.observation().position);
    QTRY_COMPARE_WITH_TIMEOUT(health.state(), GPSSourceHealth::Stale, TestTimeout::mediumMs());
    QVERIFY(!pm.gcsPosition().isValid());
    health.updatePosition(source.lastKnownPosition());
    QVERIFY(pm.gcsPosition().isValid());
    auto inaccurate = source.lastKnownPosition();
    inaccurate.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 101);
    health.updatePosition(inaccurate);
    QVERIFY(!health.usable());
    QVERIFY(!pm.gcsPosition().isValid());
    health.updatePosition(source.lastKnownPosition());
    connect(&pm, &QGCPositionManager::gcsPositionChanged, &pm, [&]() {
        if (pm.gcsPosition().isValid()) {
            pm.clearReceiverPositionSource(&source);
            pm.clearNmeaPositionSource(&source);
        }
    });
    auto moved = source.lastKnownPosition();
    moved.setCoordinate(QGeoCoordinate(48, 9));
    health.updatePosition(moved);
    QVERIFY(!pm.gcsPosition().isValid());
    QVERIFY(!pm.geoPositionInfo().isValid());
    QVERIFY(!pm.sourceHealth());
}

void PositionManagerTest::_healthLifetimeAndSelection()
{
    RTKPositionSource nmea;
    RTKPositionSource receiver;
    auto nmeaHealth = std::make_unique<GPSSourceHealth>();
    GPSSourceHealth receiverHealth;
    QGCPositionManager pm;
    pm.setNmeaPositionSource(&nmea, nmeaHealth.get());
    nmea.updatePosition(receiverFix());
    const auto position = nmea.lastKnownPosition();
    nmeaHealth->updatePosition(position);
    QVERIFY(pm.gcsPosition().isValid());
    pm.setReceiverPositionSource(&receiver, &receiverHealth);
    QVERIFY(!pm.gcsPosition().isValid());
    nmeaHealth->updatePosition(position);
    QVERIFY(!pm.gcsPosition().isValid());
    receiverHealth.updatePosition(position);
    QVERIFY(pm.gcsPosition().isValid());
    pm.clearReceiverPositionSource(&receiver);
    // Source selection waits for another observation, even if the standby cache is still fresh.
    QVERIFY(!pm.gcsPosition().isValid());
    receiverHealth.updatePosition(position);
    QVERIFY(!pm.gcsPosition().isValid());
    nmeaHealth->updatePosition(position);
    QVERIFY(pm.gcsPosition().isValid());
    nmeaHealth.reset();
    QVERIFY(!pm.gcsPosition().isValid());
    QCOMPARE(pm.sourceHealth(), &pm._externalHealth);
    nmea.updatePosition(receiverFix());
    QVERIFY(pm.gcsPosition().isValid());
}
