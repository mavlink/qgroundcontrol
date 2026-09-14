#include "PositionManagerTest.h"

#include <QtCore/QIODevice>
#include <QtCore/QRegularExpression>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtTest/QSignalSpy>

#include "ManualScheduler.h"
#include "NMEAUtils.h"
#include "PositionManager.h"
#include "SequentialTestDevice.h"
#include "SimulatedPosition.h"

namespace {

// Fix at 53.361337, -6.50562 (RMC provides the date, GGA provides HDOP for accuracy)
constexpr const char* kNmeaSentences =
    "$GPRMC,092750.000,A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A*43\r\n"
    "$GPGGA,092750.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,*76\r\n";

constexpr double kExpectedLat = 53.361337;
constexpr double kExpectedLon = -6.50562;
constexpr double kCoordEpsilon = 0.0001;

}  // namespace

void PositionManagerTest::init()
{
    UnitTest::init();
    // Headless CI has no real position source, so the internal GPS fallback
    // times out waiting for updates. Expected and benign in this fixture.
    ignoreLogMessage("GPS.PositionManager.GPSPositionService", QtWarningMsg,
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
    QGCPositionManager* pm = QGCPositionManager::instance();
    auto* device = new SequentialTestDevice();
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
    QGCPositionManager* pm = QGCPositionManager::instance();
    auto* device = new SequentialTestDevice();
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

void PositionManagerTest::_nmeaUpdatesStayHealthyUntilStale()
{
    ManualScheduler scheduler;
    SequentialTestDevice device(&scheduler);
    QGCPositionManager pm(nullptr, &scheduler);
    pm.setNmeaSourceDevice(&device);
    pm.sourceHealth()->setFreshnessTimeoutMs(300);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(10)));
    QCOMPARE(pm.sourceStatus(), GPSPositionService::SourceStatus::WaitingForFix);
    QCOMPARE(pm.gcsPositioningError(), QGeoPositionInfoSource::NoError);
    int nextSecond = 0;
    const auto feed = [&]() {
        const QByteArray time =
            QDateTime::currentDateTimeUtc().addSecs(nextSecond++).toString(QStringLiteral("hhmmss.zzz")).toLatin1();
        QByteArray sentences;
        for (QByteArray line : QByteArray(kNmeaSentences).split('\n')) {
            if (!line.trimmed().isEmpty()) {
                line.replace("092750.000", time);
                sentences += NMEAUtils::repairChecksum(line);
            }
        }
        device.feed(sentences);
    };
    feed();
    QTRY_VERIFY_WITH_TIMEOUT((scheduler.advanceBy(std::chrono::microseconds::zero()), pm.gcsPosition().isValid()),
                             TestTimeout::mediumMs());
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(299)));
    QVERIFY(pm.gcsPosition().isValid());
    QCOMPARE(pm.gcsPositioningError(), QGeoPositionInfoSource::NoError);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QVERIFY(!pm.gcsPosition().isValid());
    QCOMPARE(pm.gcsPositioningError(), QGeoPositionInfoSource::UpdateTimeoutError);
    QVERIFY(!pm.geoPositionInfo().isValid());
    QVERIFY(!pm.gcsPositionTimestamp().isValid());
    QVERIFY(qIsInf(pm.gcsPositionHorizontalAccuracy()));
    feed();
    QTRY_VERIFY_WITH_TIMEOUT((scheduler.advanceBy(std::chrono::microseconds::zero()), pm.gcsPosition().isValid()),
                             TestTimeout::mediumMs());
    QCOMPARE(pm.gcsPositioningError(), QGeoPositionInfoSource::NoError);
    pm.resetNmeaSourceDevice();
    QVERIFY(!pm.gcsPosition().isValid());
}

UT_REGISTER_TEST(PositionManagerTest, TestLabel::Unit)

void PositionManagerTest::_qmlPositionProperties()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQml
        import QGC
        QtObject {
            required property GPSPositionService service
            readonly property real latitude: service.gcsPosition.latitude
            readonly property bool usable: service.sourceHealth ? service.sourceHealth.usable : false
        }
    )",
                      QUrl());
    QTRY_VERIFY_WITH_TIMEOUT(component.isReady(), TestTimeout::mediumMs());
    QGCPositionManager service;
    std::unique_ptr<QObject> object(
        component.createWithInitialProperties({{QStringLiteral("service"), QVariant::fromValue(&service)}}));
    QVERIFY2(object, qPrintable(component.errorString()));
    SequentialTestDevice device;
    service.setNmeaSourceDevice(&device);
    device.feed(kNmeaSentences);
    QTRY_VERIFY_WITH_TIMEOUT(object->property("usable").toBool(), TestTimeout::mediumMs());
    QVERIFY(qAbs(object->property("latitude").toDouble() - kExpectedLat) < kCoordEpsilon);
    service.resetNmeaSourceDevice();
    QVERIFY(!object->property("usable").toBool());
}

void PositionManagerTest::_destructionDoesNotPublishPosition()
{
    SequentialTestDevice device;
    auto service = std::make_unique<QGCPositionManager>();
    service->setNmeaSourceDevice(&device);
    device.feed(kNmeaSentences);
    QTRY_VERIFY_WITH_TIMEOUT(service->gcsPosition().isValid(), TestTimeout::mediumMs());
    bool notified = false;
    connect(service.get(), &QGCPositionManager::gcsPositionChanged, this, [&]() { notified = true; });
    service.reset();
    QVERIFY(!notified);
}

void PositionManagerTest::_deviceDestructionRetiresNmea()
{
    auto device = std::make_unique<SequentialTestDevice>();
    QGCPositionManager service;
    service.setNmeaSourceDevice(device.get());
    device->feed(kNmeaSentences);
    QTRY_VERIFY_WITH_TIMEOUT(service.gcsPosition().isValid(), TestTimeout::mediumMs());
    device.reset();
    QCOMPARE(service.selectedSource(), GPSPositionService::SelectedSource::None);
    QVERIFY(!service.gcsPosition().isValid());
}

void PositionManagerTest::_simulatedPosition_data()
{
    QTest::addColumn<int>("intervalMs");
    QTest::newRow("one-second") << 1000;
    QTest::newRow("two-seconds") << 2000;
}

void PositionManagerTest::_simulatedPosition()
{
    QFETCH(int, intervalMs);
    ManualScheduler scheduler;
    SimulatedPosition source(nullptr, &scheduler);
    GPSPositionService service(nullptr, &scheduler);
    service.setSimulatedPositionSource(&source);
    source.stopUpdates();
    source.setUpdateInterval(intervalMs);
    source.startUpdates();
    const auto origin = source.lastKnownPosition(false).coordinate();
    const auto before = QDateTime::currentDateTimeUtc();
    QSignalSpy positions(&source, &QGeoPositionInfoSource::positionUpdated);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(intervalMs)));
    QCOMPARE(positions.size(), 1);
    QVERIFY(service.acceptedObservation());
    QCOMPARE(service.selectedSource(), GPSPositionService::SelectedSource::Simulated);
    QCOMPARE(service.gcsPosition().type(), QGeoCoordinate::Coordinate3D);
    QVERIFY(qAbs(origin.distanceTo(service.gcsPosition()) - 0.5 * intervalMs / 1000.0) < 0.001);
    QVERIFY(qAbs(service.gcsPosition().altitude() - origin.altitude() - 0.1 * intervalMs / 1000.0) < 0.001);
    const auto timestamp = service.geoPositionInfo().timestamp();
    QCOMPARE(timestamp.timeSpec(), Qt::UTC);
    QVERIFY(timestamp >= before);
    QVERIFY(timestamp <= QDateTime::currentDateTimeUtc());

    source.stopUpdates();
    const auto stopped = service.gcsPosition();
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(10)));
    QCOMPARE(positions.size(), 1);
    QVERIFY(!service.acceptedObservation());
    source.startUpdates();
    source.startUpdates();
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(intervalMs)));
    QCOMPARE(positions.size(), 2);
    QVERIFY(service.acceptedObservation());
    QVERIFY(qAbs(stopped.distanceTo(service.gcsPosition()) - 0.5 * intervalMs / 1000.0) < 0.001);
    connect(&source, &QGeoPositionInfoSource::positionUpdated, &source, &SimulatedPosition::stopUpdates);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(intervalMs)));
    QCOMPARE(positions.size(), 3);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(10)));
    QCOMPARE(positions.size(), 3);
}
