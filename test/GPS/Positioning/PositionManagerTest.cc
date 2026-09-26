#include "PositionManagerTest.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QPermissions>
#include <QtCore/QRegularExpression>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtTest/QSignalSpy>

#include "GPSTestHelpers.h"
#include "ManualScheduler.h"
#include "PositionManager.h"
#include "SimulatedPosition.h"

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
            required property PositionManager service
            readonly property real latitude: service.gcsPosition.latitude
            readonly property bool usable: service.gcsPosition.isValid
        }
    )",
                      QUrl());
    QTRY_VERIFY_WITH_TIMEOUT(component.isReady(), TestTimeout::mediumMs());
    ManualScheduler scheduler;
    PositionManager service(nullptr, &scheduler);
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
    auto service = std::make_unique<PositionManager>(nullptr, &scheduler);
    GPSSourceHealth receiver(nullptr, &scheduler);
    auto registration = service->registerPositionSource(GPSPositionService::SelectedSource::Receiver, &receiver);
    receiver.updateObservation(receiverFix(scheduler));
    QVERIFY(service->gcsPosition().isValid());
    bool notified = false;
    connect(service.get(), &PositionManager::gcsPositionChanged, this, [&]() { notified = true; });
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

void PositionManagerTest::_simulatedReferencePosition()
{
    ManualScheduler scheduler;
    SimulatedPosition source(nullptr, &scheduler);
    const QGeoCoordinate reference(48, 9, 550);
    source.setReferencePosition(reference);
    QCOMPARE(source.lastKnownPosition(false).coordinate(), reference);
    source.setReferencePosition(QGeoCoordinate());
    QCOMPARE(source.lastKnownPosition(false).coordinate(), reference);

    QSignalSpy positions(&source, &QGeoPositionInfoSource::positionUpdated);
    source.startUpdates();
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
    QCOMPARE(positions.size(), 1);
    const auto moved = positions.first().first().value<QGeoPositionInfo>().coordinate();
    QVERIFY(qAbs(reference.distanceTo(moved) - 0.5) < 0.001);
}

void PositionManagerTest::_facadeUsesInjectedScheduler()
{
    ManualScheduler scheduler;
    PositionManager manager(nullptr, &scheduler);
    manager.setSimulated(true);
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

    PositionManager defaultManager;
    defaultManager.setSimulated(true);
    defaultManager.init();
    QCOMPARE(defaultManager.findChildren<RuntimeScheduler*>().size(), 1);
}

void PositionManagerTest::_configurationSelectsMode()
{
    using Mode = GPSPositionService::SourceMode;
    ManualScheduler scheduler;
    PositionManager manager(nullptr, &scheduler);
    manager.setSimulated(true);
    QSignalSpy modes(&manager, &GPSPositionService::sourceModeChanged);
    manager.setConfiguration({.sourceMode = Mode::ReceiverOnly});
    QCOMPARE(manager.sourceMode(), Mode::ReceiverOnly);
    manager.setConfiguration({.sourceMode = Mode::ReceiverOnly});
    QCOMPARE(modes.size(), 1);
    manager.init();
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
    QCOMPARE(manager.selectedSource(), GPSPositionService::SelectedSource::None);

    manager.setConfiguration({.sourceMode = Mode::InternalOnly});
    QCOMPARE(manager.sourceMode(), Mode::InternalOnly);
    QCOMPARE(modes.size(), 2);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
    QCOMPARE(manager.selectedSource(), GPSPositionService::SelectedSource::Simulated);
}

void PositionManagerTest::_initSelectsSource_data()
{
    QTest::addColumn<bool>("simulated");
    QTest::newRow("simulated") << true;
    QTest::newRow("platform") << false;
}

void PositionManagerTest::_initSelectsSource()
{
    QFETCH(bool, simulated);
    QLocationPermission permission;
    permission.setAccuracy(QLocationPermission::Precise);
    if (!simulated && QCoreApplication::instance()->checkPermission(permission) != Qt::PermissionStatus::Granted) {
        QSKIP("The platform source is created only after location permission is granted");
    }
    ManualScheduler scheduler;
    PositionManager manager(nullptr, &scheduler);
    manager.setConfiguration({.sourceMode = GPSPositionService::SourceMode::InternalOnly});
    manager.setSimulated(simulated);
    QList<QObject*> factoryParents;
    GPSTestHelpers::PositionSource* platformSource = nullptr;
    manager.setPlatformSourceFactory([&](QObject* parent) {
        factoryParents.append(parent);
        platformSource = new GPSTestHelpers::PositionSource;
        platformSource->setParent(parent);
        return platformSource;
    });
    QSignalSpy simulatedCreated(&manager, &PositionManager::simulatedPositionCreated);
    manager.init();

    if (simulated) {
        QVERIFY(factoryParents.isEmpty());
        QCOMPARE(simulatedCreated.size(), 1);
        auto* const source = simulatedCreated.first().first().value<SimulatedPosition*>();
        QVERIFY(source);
        QCOMPARE(source->parent(), &manager);
        QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
        QCOMPARE(manager.selectedSource(), GPSPositionService::SelectedSource::Simulated);
        return;
    }
    QVERIFY(simulatedCreated.isEmpty());
    QCOMPARE(factoryParents, QList<QObject*>{&manager});
    QCOMPARE(manager.sourceStatus(), GPSPositionService::SourceStatus::WaitingForFix);
    QVERIFY(platformSource->active);
    const GPSObservation observation = receiverFix(scheduler);
    platformSource->publish(observation.position);
    QCOMPARE(manager.selectedSource(), GPSPositionService::SelectedSource::Internal);
    QCOMPARE(manager.selectedSourceName(), QStringLiteral("Plugin positioning"));
    QCOMPARE(manager.gcsPosition().latitude(), kExpectedLat);
}

void PositionManagerTest::_shutdownReleasesSources()
{
    using Mode = GPSPositionService::SourceMode;
    ManualScheduler scheduler;
    PositionManager manager(nullptr, &scheduler);
    manager.setSimulated(true);
    manager.init();
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
    QCOMPARE(manager.selectedSource(), GPSPositionService::SelectedSource::Simulated);
    QVERIFY(manager.gcsPosition().isValid());

    manager.shutdown();
    QCOMPARE(manager.selectedSource(), GPSPositionService::SelectedSource::None);
    QVERIFY(!manager.gcsPosition().isValid());
    manager.setConfiguration({.sourceMode = Mode::ReceiverOnly});
    QCOMPARE(manager.sourceMode(), Mode::Automatic);
    manager.init();
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(10)));
    QCOMPARE(manager.selectedSource(), GPSPositionService::SelectedSource::None);
    QVERIFY(!manager.acceptedObservation());
}
