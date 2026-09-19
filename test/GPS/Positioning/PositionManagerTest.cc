#include "PositionManagerTest.h"

#include <QtCore/QIODevice>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtTest/QSignalSpy>

#include "LogManager.h"
#include "ManualScheduler.h"
#include "NMEAUtils.h"
#include "PositionManager.h"
#include "QGCLoggingCategoryManager.h"
#include "SequentialTestDevice.h"
#include "SimulatedPosition.h"
#include "Vehicle.h"

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

void PositionManagerTest::_nmeaLifecycleDiagnostics_data()
{
    QTest::addColumn<int>("retirement");
    QTest::addColumn<QString>("reason");
    QTest::newRow("explicit-reset") << 0 << QStringLiteral("reset requested");
    QTest::newRow("device-closed") << 1 << QStringLiteral("device closed");
    QTest::newRow("device-destroyed") << 2 << QStringLiteral("device destroyed");
    QTest::newRow("device-replaced") << 3 << QStringLiteral("device replacement");
    QTest::newRow("device-cleared") << 4 << QStringLiteral("device cleared");
    QTest::newRow("shutdown") << 5 << QStringLiteral("manager shutdown");
    QTest::newRow("scheduler-destroyed") << 6 << QStringLiteral("scheduler destroyed");
}

void PositionManagerTest::_nmeaLifecycleDiagnostics()
{
    QFETCH(int, retirement);
    QFETCH(QString, reason);
    const QString category = QStringLiteral("GPS.PositionManager.QGCPositionManager");
    auto* logging = QGCLoggingCategoryManager::instance();
    const bool wasEnabled = logging->isCategoryEnabled(category);
    if (!wasEnabled) {
        logging->setCategoryEnabled(category, true);
    }
    const auto restoreLogging = qScopeGuard([logging, category, wasEnabled] {
        if (!wasEnabled) {
            logging->setCategoryEnabled(category, false);
        }
    });
    auto scheduler = std::make_unique<ManualScheduler>();
    auto device = std::make_unique<SequentialTestDevice>(scheduler.get());
    SequentialTestDevice replacement(scheduler.get());
    expectLogMessage("GPS.PositionManager.QGCPositionManager", QtDebugMsg,
                     QRegularExpression(QStringLiteral("^QGCPositionManager\\(")));
    auto service = std::make_unique<QGCPositionManager>(nullptr, scheduler.get());
    verifyExpectedLogMessage();
    expectLogMessage("GPS.PositionManager.QGCPositionManager", QtDebugMsg,
                     QRegularExpression(QStringLiteral("NMEA session installed:.*revision: 1.*registered: true")));
    service->setNmeaSourceDevice(device.get());
    verifyExpectedLogMessage();
    const QPointer<GPSSourceHealth> originalHealth(service->nmeaHealth());
    QVERIFY(originalHealth);
    expectLogMessage(
        "GPS.PositionManager.QGCPositionManager", QtDebugMsg,
        QRegularExpression(
            QStringLiteral("NMEA session retired:.*reason: %1.*revision: 1").arg(QRegularExpression::escape(reason))));
    if (retirement == 3) {
        expectLogMessage("GPS.PositionManager.QGCPositionManager", QtDebugMsg,
                         QRegularExpression(QStringLiteral("NMEA session installed:.*revision: 2.*registered: true")));
    } else if (retirement == 5) {
        expectLogMessage("GPS.PositionManager.QGCPositionManager", QtDebugMsg,
                         QRegularExpression(QStringLiteral("Position manager shutdown:")));
    }
    switch (retirement) {
        case 0:
            service->resetNmeaSourceDevice();
            break;
        case 1:
            device->close();
            break;
        case 2:
            device.reset();
            break;
        case 3:
            service->setNmeaSourceDevice(&replacement);
            break;
        case 4:
            service->setNmeaSourceDevice(nullptr);
            break;
        case 5:
            service.reset();
            break;
        case 6:
            scheduler.reset();
            break;
    }
    QTRY_VERIFY_WITH_TIMEOUT(originalHealth.isNull(), TestTimeout::shortMs());
    verifyExpectedLogMessage();
    if (retirement == 3 || retirement == 5) {
        verifyExpectedLogMessage();
    }
    if (service) {
        QCOMPARE(service->nmeaHealth() != nullptr, retirement == 3);
        if (retirement != 3) {
            const auto logCount = LogManager::capturedMessages(category).size();
            service->resetNmeaSourceDevice();
            QCOMPARE(LogManager::capturedMessages(category).size(), logCount);
        }
        expectLogMessage("GPS.PositionManager.QGCPositionManager", QtDebugMsg,
                         QRegularExpression(QStringLiteral("Position manager shutdown:")));
        if (retirement == 3) {
            expectLogMessage("GPS.PositionManager.QGCPositionManager", QtDebugMsg,
                             QRegularExpression(QStringLiteral("NMEA session retired:.*reason: manager shutdown"
                                                               ".*revision: 2")));
        }
        service.reset();
        verifyExpectedLogMessage();
        if (retirement == 3) {
            verifyExpectedLogMessage();
        }
    }
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

void PositionManagerTest::_facadeUsesInjectedScheduler()
{
    ManualScheduler scheduler;
    QGCPositionManager manager(nullptr, &scheduler);
    manager.init();
    QSignalSpy reports(&manager, &GPSPositionService::positionInfoUpdated);
    QVERIFY(!manager.acceptedObservation());
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(999)));
    QVERIFY(reports.isEmpty());
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QCOMPARE(reports.size(), 1);
    QCOMPARE(manager.selectedSource(), GPSPositionService::SelectedSource::Simulated);
    QVERIFY(manager.acceptedObservation());
    QCOMPARE(manager.acceptedObservation()->monotonicTimestampUs, scheduler.nowUs());
    QVERIFY(manager.findChildren<RuntimeScheduler*>().isEmpty());

    QGCPositionManager defaultManager;
    defaultManager.init();
    QCOMPARE(defaultManager.findChildren<RuntimeScheduler*>().size(), 1);
    SequentialTestDevice device;
    defaultManager.setNmeaSourceDevice(&device);
    QVERIFY(defaultManager.nmeaHealth());
    auto* session = defaultManager.nmeaHealth()->parent();
    QVERIFY(session);
    QVERIFY(session->findChildren<RuntimeScheduler*>().isEmpty());
}

void PositionManagerTest::_facadeSchedulerDestruction()
{
    auto scheduler = std::make_unique<ManualScheduler>();
    QGCPositionManager manager(nullptr, scheduler.get());
    manager.init();
    SequentialTestDevice device;
    manager.setNmeaSourceDevice(&device);
    QVERIFY(manager.nmeaHealth());
    scheduler.reset();
    QVERIFY(!manager.nmeaHealth());
    QVERIFY(!manager.acceptedObservation());
    QCOMPARE(manager.selectedSource(), GPSPositionService::SelectedSource::None);
    expectLogMessage("GPS.PositionManager.QGCPositionManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Positioning requires a live scheduler")));
    manager.init();
    verifyExpectedLogMessage();
    expectLogMessage(
        "GPS.PositionManager.QGCPositionManager", QtWarningMsg,
        QRegularExpression(QStringLiteral("NMEA device requires matching thread affinity and a live scheduler")));
    manager.setNmeaSourceDevice(&device);
    verifyExpectedLogMessage();
    QVERIFY(!manager.nmeaHealth());
    QVERIFY(manager.findChildren<RuntimeScheduler*>().isEmpty());
}

void PositionManagerTest::_simulatedHomeSelection_data()
{
    QTest::addColumn<bool>("latestAlreadyValid");
    QTest::addColumn<bool>("oldestFirst");
    QTest::newRow("pending-oldest-first") << false << true;
    QTest::newRow("pending-newest-first") << false << false;
    QTest::newRow("valid-oldest-first") << true << true;
    QTest::newRow("valid-newest-first") << true << false;
}

void PositionManagerTest::_simulatedHomeSelection()
{
    QFETCH(bool, latestAlreadyValid);
    QFETCH(bool, oldestFirst);
    ManualScheduler scheduler;
    SimulatedPosition source(nullptr, &scheduler);
    Vehicle first(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR);
    Vehicle second(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR);
    Vehicle latest(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR);
    QGeoCoordinate latestHome(48, 9, 550);
    if (latestAlreadyValid) {
        latest._setHomePosition(latestHome);
    }
    const auto origin = source.lastKnownPosition(false).coordinate();
    for (auto* vehicle : {&first, &second, &latest}) {
        QVERIFY(QMetaObject::invokeMethod(&source, "_vehicleAdded", Qt::DirectConnection, Q_ARG(Vehicle*, vehicle)));
    }
    const auto updateOlderHomes = [&]() {
        QGeoCoordinate firstHome(46, 7, 450);
        QGeoCoordinate secondHome(47, 8, 500);
        first._setHomePosition(firstHome);
        second._setHomePosition(secondHome);
    };
    if (oldestFirst) {
        updateOlderHomes();
        QCOMPARE(source.lastKnownPosition(false).coordinate(), latestAlreadyValid ? latestHome : origin);
    }
    latest._setHomePosition(latestHome);
    QCOMPARE(source.lastKnownPosition(false).coordinate(), latestHome);
    if (!oldestFirst) {
        updateOlderHomes();
    }
    QGeoCoordinate changedHome(49, 10, 600);
    for (auto* vehicle : {&first, &second, &latest}) {
        vehicle->_setHomePosition(changedHome);
        QCOMPARE(source.lastKnownPosition(false).coordinate(), latestHome);
    }
}
