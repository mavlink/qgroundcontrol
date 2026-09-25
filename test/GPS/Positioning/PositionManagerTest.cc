#include "PositionManagerTest.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtTest/QSignalSpy>

#include "Fixtures/RAIIFixtures.h"
#include "LogManager.h"
#include "ManualScheduler.h"
#include "PositionManager.h"
#include "QGCLoggingCategoryManager.h"
#include "RTKSettings.h"
#include "SettingsManager.h"
#include "SimulatedPosition.h"
#include "Vehicle.h"

namespace {

constexpr double kExpectedLat = 53.361337;
constexpr double kExpectedLon = -6.50562;
constexpr double kCoordEpsilon = 0.0001;

GPSObservation receiverFix(RuntimeScheduler& scheduler)
{
    GPSObservation observation;
    observation.receivedAt = QDateTime::currentDateTimeUtc();
    observation.monotonicTimestampUs = scheduler.nowUs();
    observation.position = QGeoPositionInfo(QGeoCoordinate(kExpectedLat, kExpectedLon, 61.7), observation.receivedAt);
    observation.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 2);
    return observation;
}

}  // namespace

void PositionManagerTest::init()
{
    UnitTest::init();
    // Headless CI has no real position source, so the internal GPS fallback
    // times out waiting for updates. Expected and benign in this fixture.
    ignoreLogMessage("GPS.PositionManager.GPSPositionService", QtWarningMsg,
                     QRegularExpression(QStringLiteral("UpdateTimeoutError")));
}

UT_REGISTER_TEST(PositionManagerTest, TestLabel::Unit)

void PositionManagerTest::_qmlPositionProperties()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQml
        import QGroundControl.GPS
        QtObject {
            required property GPSPositionService service
            readonly property real latitude: service.gcsPosition.latitude
            readonly property bool usable: service.gcsPosition.isValid
        }
    )",
                      QUrl());
    QTRY_VERIFY_WITH_TIMEOUT(component.isReady(), TestTimeout::mediumMs());
    ManualScheduler scheduler;
    QGCPositionManager service(nullptr, &scheduler);
    std::unique_ptr<QObject> object(
        component.createWithInitialProperties({{QStringLiteral("service"), QVariant::fromValue(&service)}}));
    QVERIFY2(object, qPrintable(component.errorString()));
    GPSSourceHealth receiver(nullptr, &scheduler);
    auto registration = service.registerPositionSource(GPSPositionService::SelectedSource::Receiver, &receiver);
    receiver.updateObservation(receiverFix(scheduler));
    QVERIFY(object->property("usable").toBool());
    QVERIFY(qAbs(object->property("latitude").toDouble() - kExpectedLat) < kCoordEpsilon);
    registration.reset();
    QVERIFY(!object->property("usable").toBool());
}

void PositionManagerTest::_destructionDoesNotPublishPosition()
{
    ManualScheduler scheduler;
    auto service = std::make_unique<QGCPositionManager>(nullptr, &scheduler);
    GPSSourceHealth receiver(nullptr, &scheduler);
    auto registration = service->registerPositionSource(GPSPositionService::SelectedSource::Receiver, &receiver);
    receiver.updateObservation(receiverFix(scheduler));
    QVERIFY(service->gcsPosition().isValid());
    bool notified = false;
    connect(service.get(), &QGCPositionManager::gcsPositionChanged, this, [&]() { notified = true; });
    service.reset();
    QVERIFY(!notified);
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
    const auto timestamp = service.acceptedObservation()->position.timestamp();
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
    QSignalSpy reports(&manager, &GPSPositionService::gcsPositionChanged);
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
}

void PositionManagerTest::_sourceSettingSelectsMode()
{
    using Mode = GPSPositionService::SourceMode;
    TestFixtures::SettingsFixture saved;
    Fact* const setting = SettingsManager::instance()->rtkSettings()->gcsPositionSource();
    saved.setFactValue(setting, static_cast<int>(Mode::ReceiverOnly));
    ManualScheduler scheduler;
    QGCPositionManager manager(nullptr, &scheduler);
    QCOMPARE(manager.sourceMode(), Mode::Automatic);
    manager.init();
    QCOMPARE(manager.sourceMode(), Mode::ReceiverOnly);
    QCOMPARE(manager.sourceStatus(), GPSPositionService::SourceStatus::NoSource);
    QSignalSpy modes(&manager, &GPSPositionService::sourceModeChanged);
    manager.init();
    setting->setRawValue(static_cast<int>(Mode::InternalOnly));
    QCOMPARE(manager.sourceMode(), Mode::InternalOnly);
    QCOMPARE(modes.size(), 1);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
    QCOMPARE(manager.selectedSource(), GPSPositionService::SelectedSource::Simulated);
    setting->setRawValue(static_cast<int>(Mode::Automatic));
    QCOMPARE(manager.sourceMode(), Mode::Automatic);
    QCOMPARE(modes.size(), 2);
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
