#include "PositionManagerTest.h"

#include <chrono>
#include <memory>
#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QPermissions>
#include <QtTest/QSignalSpy>

#include "ManualScheduler.h"
#include "PositionManager.h"
#include "Support/GPSQmlTestHelpers.h"
#include "Support/GPSTestHelpers.h"

using namespace std::chrono_literals;

namespace {
using Kind = PositionManager::SelectedSource;
using Mode = PositionManager::SourceMode;
using Status = PositionManager::SourceStatus;

using GPSTest::PositionSource;

GPSObservation fix(ManualScheduler& scheduler, double latitude = 47, quint64 session = 0)
{
    GPSObservation observation;
    observation.sessionId = session;
    observation.monotonicTimestampUs = scheduler.nowUs();
    observation.position = QGeoPositionInfo(QGeoCoordinate(latitude, 8, 500), QDateTime::currentDateTimeUtc());
    observation.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1);
    observation.position.setAttribute(QGeoPositionInfo::VerticalAccuracy, 1);
    observation.position.setAttribute(QGeoPositionInfo::GroundSpeed, 2);
    observation.position.setAttribute(QGeoPositionInfo::Direction, 90);
    return observation;
}
}  // namespace

void PositionManagerTest::_selectionStatusMatchesPublication_data()
{
    QTest::addColumn<int>("mode");
    QTest::newRow("receiver-only") << int(Mode::ReceiverOnly);
    QTest::newRow("internal-only") << int(Mode::InternalOnly);
}

void PositionManagerTest::_selectionStatusMatchesPublication()
{
    QFETCH(int, mode);
    ManualScheduler scheduler;
    GPSSourceHealth primary(nullptr, &scheduler);
    PositionSource internal;
    PositionManager service(nullptr, &scheduler);
    auto receiverRegistration = service.registerReceiver(&primary);
    service.setInternalPositionSource(&internal, Status::WaitingForFix);
    service.setConfiguration({.sourceMode = Mode::Automatic});
    primary.updateObservation(fix(scheduler));
    internal.publish(fix(scheduler, 48).position);
    const auto checkPublication = [&]() {
        QCOMPARE(service.sourceStatus() == Status::Active, service.gcsPosition().isValid());
    };
    connect(&service, &PositionManager::selectionChanged, &service, checkPublication);
    connect(&service, &PositionManager::gcsPositionChanged, &service, checkPublication);
    service.setConfiguration({.sourceMode = Mode(mode)});
    QCOMPARE(service.sourceStatus(), Status::WaitingForFix);
    QVERIFY(!service.acceptedObservation());
    QVERIFY(!service.gcsPosition().isValid());
    if (Mode(mode) == Mode::InternalOnly) {
        internal.publish(fix(scheduler).position);
    } else {
        primary.updateObservation(fix(scheduler));
    }
    QCOMPARE(service.sourceStatus(), Status::Active);
    QVERIFY(service.acceptedObservation());
    QSignalSpy reports(&service, &PositionManager::gcsPositionChanged);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(5)));
    QCOMPARE(service.sourceStatus(), Status::Stale);
    QVERIFY(!service.gcsPosition().isValid());
    QCOMPARE(reports.size(), 1);
}

void PositionManagerTest::_automaticRejectionMatchesPublication_data()
{
    QTest::addColumn<bool>("providedHealth");
    QTest::addColumn<bool>("expire");
    QTest::newRow("provided-inaccurate") << true << false;
    QTest::newRow("provided-expired") << true << true;
    QTest::newRow("raw-inaccurate") << false << false;
    QTest::newRow("raw-expired") << false << true;
}

void PositionManagerTest::_automaticRejectionMatchesPublication()
{
    QFETCH(bool, providedHealth);
    QFETCH(bool, expire);
    ManualScheduler scheduler;
    PositionSource source;
    GPSSourceHealth health(nullptr, &scheduler);
    PositionManager service(nullptr, &scheduler);
    GPSPositionSourceRegistration registration;
    if (providedHealth) {
        registration = service.registerReceiver(&health);
        QVERIFY(registration);
    } else {
        service.setInternalPositionSource(&source, Status::WaitingForFix);
    }
    service.setConfiguration({.sourceMode = Mode::Automatic});
    const auto publish = [&](const GPSObservation& observation) {
        if (providedHealth) {
            health.updateObservation(observation);
        } else {
            source.publish(observation.position);
        }
    };
    publish(fix(scheduler));
    QCOMPARE(service.sourceStatus(), Status::Active);
    QVERIFY(service.gcsPosition().isValid());

    QSignalSpy selections(&service, &PositionManager::selectionChanged);
    QSignalSpy positions(&service, &PositionManager::gcsPositionChanged);
    const auto checkPublication = [&]() {
        const bool active = service.sourceStatus() == Status::Active;
        QCOMPARE(service.gcsPosition().isValid(), active);
        QCOMPARE(service.acceptedObservation().has_value(), active);
    };
    connect(&service, &PositionManager::selectionChanged, &service, checkPublication);
    connect(&service, &PositionManager::gcsPositionChanged, &service, checkPublication);
    if (expire) {
        QVERIFY(scheduler.advanceBy(std::chrono::seconds(5)));
    } else {
        auto inaccurate = fix(scheduler);
        inaccurate.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 101);
        publish(inaccurate);
    }
    QCOMPARE(service.sourceStatus(), expire ? Status::Stale : Status::InvalidFix);
    QCOMPARE(service.selectedSource(), providedHealth ? Kind::Receiver : Kind::Internal);
    QCOMPARE(selections.size(), 1);
    QCOMPARE(positions.size(), 1);
    QVERIFY(!service.gcsPosition().isValid());

    publish(fix(scheduler, 48));
    QCOMPARE(service.sourceStatus(), Status::Active);
    QCOMPARE(service.gcsPosition().latitude(), 48);
    QCOMPARE(selections.size(), 2);
    QCOMPARE(positions.size(), 2);
}

void PositionManagerTest::_sourcesShareAcceptance_data()
{
    QTest::addColumn<bool>("custom");
    QTest::newRow("platform") << false;
    QTest::newRow("custom") << true;
}

void PositionManagerTest::_sourcesShareAcceptance()
{
    QFETCH(bool, custom);
    ManualScheduler scheduler;
    PositionSource source;
    PositionManager service(nullptr, &scheduler);
    service.setInternalPositionSource(&source, Status::WaitingForFix, custom);
    QVERIFY(source.active);
    QCOMPARE(service.selectedSource(), Kind::Internal);
    auto observation = fix(scheduler, 0);
    source.publish(observation.position);
    QCOMPARE(service.gcsPosition(), observation.coordinate());
    QCOMPARE(service.gcsHeading(), observation.heading());
    QVERIFY(service.acceptedObservation());
    QCOMPARE(service._sourceHealth(service.selectedSource())->_observation().altitudeDatum, GPSAltitudeDatum::Unknown);
    // Remote ID still receives the altitude, whose datum it checks.
    const auto remoteId = service.acceptedObservation(GPSObservation::PositionUse::RemoteID);
    QVERIFY(remoteId);
    QCOMPARE(remoteId->position.coordinate(), observation.position.coordinate());
    QCOMPARE(remoteId->altitudeDatum, GPSAltitudeDatum::Unknown);
    QCOMPARE(service.selectedSourceName(),
             custom ? PositionManager::tr("Plugin positioning") : PositionManager::tr("Internal positioning"));
    observation.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 101);
    source.publish(observation.position);
    QCOMPARE(service.sourceStatus(), Status::InvalidFix);
    QVERIFY(!service.gcsPosition().isValid());
    QVERIFY(!service.acceptedObservation());
    source.publish(fix(scheduler).position);
    QVERIFY(service.gcsPosition().isValid());
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(5)));
    QCOMPARE(service.sourceStatus(), Status::Stale);
    QVERIFY(!service.gcsPosition().isValid());
    QVERIFY(qIsNaN(service.gcsHeading()));
    source.publish(fix(scheduler).position);
    QCOMPARE(service.sourceStatus(), Status::Active);
}

void PositionManagerTest::_deviceAltitudeDatum_data()
{
    QTest::addColumn<GPSAltitudeDatum>("declared");
    QTest::addColumn<bool>("verticalAccuracy");
    QTest::addColumn<GPSAltitudeDatum>("expected");
    QTest::newRow("ellipsoid") << GPSAltitudeDatum::Ellipsoid << true << GPSAltitudeDatum::Ellipsoid;
    QTest::newRow("mean-sea-level") << GPSAltitudeDatum::MeanSeaLevel << true << GPSAltitudeDatum::MeanSeaLevel;
    QTest::newRow("unverified-altitude") << GPSAltitudeDatum::MeanSeaLevel << false << GPSAltitudeDatum::Unknown;
    QTest::newRow("undeclared") << GPSAltitudeDatum::Unknown << true << GPSAltitudeDatum::Unknown;
}

void PositionManagerTest::_deviceAltitudeDatum()
{
    QFETCH(GPSAltitudeDatum, declared);
    QFETCH(bool, verticalAccuracy);
    QFETCH(GPSAltitudeDatum, expected);
    ManualScheduler scheduler;
    PositionSource source;
    PositionManager service(nullptr, &scheduler);
    service.setInternalPositionSource(&source, Status::WaitingForFix, false, declared);
    auto position = fix(scheduler).position;
    if (!verticalAccuracy) {
        position.removeAttribute(QGeoPositionInfo::VerticalAccuracy);
    }
    source.publish(position);

    const auto remoteId = service.acceptedObservation(GPSObservation::PositionUse::RemoteID);
    QVERIFY(remoteId);
    QCOMPARE(remoteId->altitudeDatum, expected);
}

void PositionManagerTest::_registrationReplacementAndSessions()
{
    ManualScheduler scheduler;
    PositionManager service(nullptr, &scheduler);
    GPSSourceHealth health(nullptr, &scheduler);
    auto first = service.registerReceiver(&health, 11);
    health.updateObservation(fix(scheduler, 47, 12));
    QVERIFY(!service.gcsPosition().isValid());
    health.updateObservation(fix(scheduler, 47, 11));
    QVERIFY(service.gcsPosition().isValid());
    auto second = service.registerReceiver(&health, 12);
    auto moved = std::move(first);
    QVERIFY(!first);
    moved.reset();
    QCOMPARE(service._sourceHealth(service.selectedSource()), &health);
    QVERIFY(!service.gcsPosition().isValid());
    health.updateObservation(fix(scheduler, 47, 11));
    QVERIFY(!service.gcsPosition().isValid());
    health.updateObservation(fix(scheduler, 48, 12));
    QCOMPARE(service.gcsPosition().latitude(), 48);
    QCOMPARE(service.acceptedObservation()->sessionId, quint64(12));
    second.reset();
    QVERIFY(!service._sourceHealth(service.selectedSource()));
    health.updateObservation(fix(scheduler, 47, 12));
    QVERIFY(!service.gcsPosition().isValid());
}

void PositionManagerTest::_automaticFailoverAndRecovery()
{
    ManualScheduler scheduler;
    GPSSourceHealth primary(nullptr, &scheduler);
    PositionSource internal;
    primary.setFreshnessTimeout(1000ms);
    PositionManager service(nullptr, &scheduler);
    auto receiverRegistration = service.registerReceiver(&primary);
    service.setInternalPositionSource(&internal, Status::WaitingForFix);
    service._sourceHealth(Kind::Internal)->setFreshnessTimeout(60000ms);
    primary.updateObservation(fix(scheduler));
    internal.publish(fix(scheduler, 48).position);
    QCOMPARE(service.selectedSource(), Kind::Receiver);
    primary.invalidatePosition();
    QCOMPARE(service.selectedSource(), Kind::Internal);
    QCOMPARE(service.gcsPosition().latitude(), 48);
    primary.setFreshnessTimeout(60000ms);
    primary.updateObservation(fix(scheduler));
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(2)));
    primary.invalidatePosition();
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(4)));
    QCOMPARE(service.selectedSource(), Kind::Internal);
    primary.updateObservation(fix(scheduler));
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(4999)));
    QCOMPARE(service.selectedSource(), Kind::Internal);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QCOMPARE(service.selectedSource(), Kind::Receiver);
    service.setConfiguration({.sourceMode = Mode::InternalOnly});
    internal.publish(fix(scheduler, 48).position);
    receiverRegistration.reset();
    QCOMPARE(service.gcsPosition().latitude(), 48);
    QCOMPARE(service.selectedSource(), Kind::Internal);
}

void PositionManagerTest::_sourceAndHealthLifetime()
{
    ManualScheduler scheduler;
    auto platform = std::make_unique<PositionSource>();
    auto health = std::make_unique<GPSSourceHealth>(nullptr, &scheduler);
    PositionManager service(nullptr, &scheduler);
    service.setInternalPositionSource(platform.get(), Status::WaitingForFix);
    auto registration = service.registerReceiver(health.get());
    health->updateObservation(fix(scheduler));
    QCOMPARE(service.selectedSource(), Kind::Receiver);
    QVERIFY(service.gcsPosition().isValid());
    // A lost producer retires its role; nothing falls back to another object.
    health.reset();
    QCOMPARE(service.selectedSource(), Kind::Internal);
    QVERIFY(!service.gcsPosition().isValid());
    platform->publish(fix(scheduler).position);
    QVERIFY(service.gcsPosition().isValid());
    // A lost backend retires its role the same way.
    platform.reset();
    QCOMPARE(service.selectedSource(), Kind::None);
    QVERIFY(!service.gcsPosition().isValid());
    QVERIFY(!service.acceptedObservation());
}

void PositionManagerTest::_standbyReportsDoNotRepublish()
{
    ManualScheduler scheduler;
    GPSSourceHealth primary(nullptr, &scheduler);
    PositionSource internal;
    primary.setFreshnessTimeout(60000ms);
    PositionManager service(nullptr, &scheduler);
    auto receiverRegistration = service.registerReceiver(&primary);
    service.setInternalPositionSource(&internal, Status::WaitingForFix);
    service._sourceHealth(Kind::Internal)->setFreshnessTimeout(60000ms);
    service.setConfiguration({.sourceMode = Mode::Automatic});
    primary.updateObservation(fix(scheduler));
    QSignalSpy coordinates(&service, &PositionManager::gcsPositionChanged);
    internal.publish(fix(scheduler, 48).position);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
    internal.publish(fix(scheduler, 49).position);
    QCOMPARE(coordinates.size(), 0);

    primary.updateObservation(fix(scheduler));
    QCOMPARE(coordinates.size(), 0);
    QCOMPARE(service.acceptedObservation()->monotonicTimestampUs, scheduler.nowUs());
    primary.invalidatePosition();
    QCOMPARE(service.selectedSource(), Kind::Internal);
    QCOMPARE(service.gcsPosition().latitude(), 49);
    coordinates.clear();

    primary.updateObservation(fix(scheduler));
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(4999)));
    QCOMPARE(service.selectedSource(), Kind::Internal);
    QVERIFY(coordinates.isEmpty());
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QCOMPARE(service.selectedSource(), Kind::Receiver);
    QCOMPARE(coordinates.size(), 1);
    QCOMPARE(coordinates.last().first().value<QGeoCoordinate>().latitude(), 47);
}

void PositionManagerTest::_consumerMaximumAge()
{
    // How a consumer's age limit combines with source freshness is GPSSourceHealthTest::_maximumAge's; the manager
    // measures the age on the source's clock rather than its own.
    using Use = GPSObservation::PositionUse;
    ManualScheduler serviceClock;
    ManualScheduler sourceClock;
    QVERIFY(sourceClock.advanceBy(1h));
    GPSSourceHealth health(nullptr, &sourceClock);
    health.setFreshnessTimeout(10000ms);
    PositionManager service(nullptr, &serviceClock);
    auto registration = service.registerReceiver(&health, 7);
    const auto observation = fix(sourceClock, 47, 7);
    health.updateObservation(observation);
    QVERIFY(serviceClock.advanceBy(24h));
    QVERIFY(sourceClock.advanceBy(4999ms));
    const auto accepted = service.acceptedObservation(Use::RemoteID, 5000ms);
    QVERIFY(accepted);
    QCOMPARE(accepted->monotonicTimestampUs, observation.monotonicTimestampUs);
    QVERIFY(sourceClock.advanceBy(1ms));
    QVERIFY(!service.acceptedObservation(Use::RemoteID, 5000ms));
    QVERIFY(service.acceptedObservation(Use::RemoteID));
}

void PositionManagerTest::_policySelectionGates()
{
    using Use = GPSObservation::PositionUse;
    ManualScheduler scheduler;
    GPSSourceHealth firstHealth(nullptr, &scheduler);
    GPSSourceHealth replacementHealth(nullptr, &scheduler);
    PositionManager service(nullptr, &scheduler);
    service.setConfiguration({.sourceMode = Mode::ReceiverOnly});
    auto oldRegistration = service.registerReceiver(&firstHealth, 1);
    firstHealth.updateObservation(fix(scheduler, 47, 1));
    replacementHealth.updateObservation(fix(scheduler, 48, 2));
    auto registration = service.registerReceiver(&replacementHealth, 2);
    oldRegistration.reset();
    firstHealth.updateObservation(fix(scheduler, 49, 1));
    replacementHealth.setFreshnessTimeout(10000ms);
    for (const auto use : {Use::GroundStation, Use::Motion, Use::RemoteID, Use::Gga}) {
        QVERIFY(!service.acceptedObservation(use));
    }
    replacementHealth.updateObservation(fix(scheduler, 48, 1));
    QVERIFY(!service.acceptedObservation(Use::Motion));
    QVERIFY(!service.acceptedObservation(Use::Gga));
    replacementHealth.updateObservation(fix(scheduler, 48, 2));
    for (const auto use : {Use::GroundStation, Use::Motion, Use::RemoteID, Use::Gga}) {
        const auto accepted = service.acceptedObservation(use);
        QVERIFY(accepted);
        QCOMPARE(accepted->position.coordinate().latitude(), 48);
        QCOMPARE(accepted->sessionId, quint64(2));
    }
    registration.reset();
    QVERIFY(!service.acceptedObservation(Use::RemoteID));
    QVERIFY(!service.acceptedObservation(Use::Gga));
}

void PositionManagerTest::_backendStatus()
{
    ManualScheduler scheduler;
    PositionSource platform;
    PositionManager service(nullptr, &scheduler);
    service._setInternalPositionStatus(Status::PermissionRequired);
    QCOMPARE(service.sourceStatus(), Status::PermissionRequired);
    service.setInternalPositionSource(&platform, Status::WaitingForFix);
    platform.publish(fix(scheduler).position);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(5)));
    QCOMPARE(service.sourceStatus(), Status::Stale);
    platform.fail(QGeoPositionInfoSource::AccessError);
    QCOMPARE(service.gcsPositioningError(), QGeoPositionInfoSource::AccessError);
    QCOMPARE(service.sourceStatus(), Status::PermissionDenied);
    platform.publish(fix(scheduler).position);
    QCOMPARE(service.sourceStatus(), Status::Active);
    platform.fail(QGeoPositionInfoSource::ClosedError);
    QCOMPARE(service.sourceStatus(), Status::BackendUnavailable);
    QCOMPARE(service.gcsPositioningError(), QGeoPositionInfoSource::ClosedError);
    PositionSource custom;
    service.setInternalPositionSource(&platform, Status::WaitingForFix);
    QSignalSpy selection(&service, &PositionManager::selectionChanged);
    service.setInternalPositionSource(&custom, Status::WaitingForFix, true);
    QVERIFY(!selection.isEmpty());
    QCOMPARE(service.selectedSourceName(), PositionManager::tr("Plugin positioning"));
}

void PositionManagerTest::_backendTimeoutPreservesFreshness_data()
{
    QTest::addColumn<bool>("hasFix");
    QTest::newRow("before-fix") << false;
    QTest::newRow("fresh-fix") << true;
}

void PositionManagerTest::_backendTimeoutPreservesFreshness()
{
    QFETCH(bool, hasFix);
    ManualScheduler scheduler;
    PositionSource source;
    PositionManager service(nullptr, &scheduler);
    service.setInternalPositionSource(&source, Status::WaitingForFix);
    if (hasFix) {
        source.publish(fix(scheduler).position);
    }
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(2)));
    QSignalSpy positions(&service, &PositionManager::gcsPositionChanged);
    source.fail(QGeoPositionInfoSource::UpdateTimeoutError);
    QCOMPARE(service.gcsPositioningError(), QGeoPositionInfoSource::UpdateTimeoutError);
    QCOMPARE(service.sourceStatus(), hasFix ? Status::Active : Status::WaitingForFix);
    QCOMPARE(service.gcsPosition().isValid(), hasFix);
    QVERIFY(positions.isEmpty());
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(3)));
    QCOMPARE(service.sourceStatus(), hasFix ? Status::Stale : Status::WaitingForFix);
    QVERIFY(!service.gcsPosition().isValid());
    source.publish(fix(scheduler).position);
    QCOMPARE(service.sourceStatus(), Status::Active);
    QCOMPARE(service.gcsPositioningError(), QGeoPositionInfoSource::NoError);
}

void PositionManagerTest::_deviceRunsUnlessReceiverOnly()
{
    ManualScheduler scheduler;
    PositionSource source;
    PositionManager service(nullptr, &scheduler);
    service.setConfiguration({.sourceMode = Mode::ReceiverOnly});
    service.setInternalPositionSource(&source, Status::WaitingForFix);
    QVERIFY(!source.active);
    QCOMPARE(service.selectedSource(), Kind::None);
    service.setConfiguration({.sourceMode = Mode::Automatic});
    QVERIFY(source.active);
    source.publish(fix(scheduler).position);
    auto* const health = service._sourceHealth(Kind::Internal);
    QVERIFY(health->acceptedObservation());
    // Stopping the device discards its fix, and a report delivered after the stop is ignored.
    service.setConfiguration({.sourceMode = Mode::ReceiverOnly});
    QCOMPARE(service.configuration().sourceMode, Mode::ReceiverOnly);
    QVERIFY(!source.active);
    QCOMPARE(service.selectedSource(), Kind::None);
    QCOMPARE(health->state(), GPSSourceHealth::State::NoData);
    source.publish(fix(scheduler).position);
    QCOMPARE(health->state(), GPSSourceHealth::State::NoData);
    QVERIFY(!health->acceptedObservation());
    service.setConfiguration({.sourceMode = Mode::InternalOnly});
    QVERIFY(source.active);
    QCOMPARE(service.selectedSource(), Kind::Internal);
    QVERIFY(!service.acceptedObservation());
    source.publish(fix(scheduler, 48).position);
    QCOMPARE(service.acceptedObservation()->position.coordinate().latitude(), 48);
}

void PositionManagerTest::_producerLoss_data()
{
    QTest::addColumn<Mode>("mode");
    QTest::newRow("automatic") << Mode::Automatic;
    QTest::newRow("receiver-only") << Mode::ReceiverOnly;
}

void PositionManagerTest::_producerLoss()
{
    QFETCH(Mode, mode);
    ManualScheduler scheduler;
    auto health = std::make_unique<GPSSourceHealth>(nullptr, &scheduler);
    GPSSourceHealth replacement(nullptr, &scheduler);
    PositionManager service(nullptr, &scheduler);
    auto registration = service.registerReceiver(health.get());
    service.setConfiguration({.sourceMode = mode});
    health->updateObservation(fix(scheduler));
    QVERIFY(service.gcsPosition().isValid());
    // Losing the producer retires its role.
    health.reset();
    QCOMPARE(service.selectedSource(), Kind::None);
    QVERIFY(!service.gcsPosition().isValid());
    QVERIFY(!service.acceptedObservation());
    auto next = service.registerReceiver(&replacement);
    QVERIFY(next);
    replacement.updateObservation(fix(scheduler, 48));
    QCOMPARE(service.selectedSource(), Kind::Receiver);
    QVERIFY(service.acceptedObservation());
    QCOMPARE(service.gcsPosition().latitude(), 48);
    // The superseded registration cannot retire its replacement.
    registration.reset();
    QCOMPARE(service._sourceHealth(Kind::Receiver), &replacement);
    next.reset();
    QVERIFY(!service._sourceHealth(Kind::Receiver));
}

void PositionManagerTest::_accuracyNotifiesOnlyChanges()
{
    ManualScheduler scheduler;
    PositionSource source;
    PositionManager service(nullptr, &scheduler);
    service.setInternalPositionSource(&source, Status::WaitingForFix);
    QSignalSpy accuracy(&service, &PositionManager::gcsPositionHorizontalAccuracyChanged);
    auto position = fix(scheduler).position;
    source.publish(position);
    QCOMPARE(accuracy.size(), 1);
    source.publish(position);
    QCOMPARE(accuracy.size(), 1);
    position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 2);
    source.publish(position);
    QCOMPARE(accuracy.size(), 2);
    QCOMPARE(accuracy.last().first().toDouble(), 2);
    position.removeAttribute(QGeoPositionInfo::HorizontalAccuracy);
    source.publish(position);
    QCOMPARE(accuracy.size(), 3);
    QVERIFY(qIsInf(accuracy.last().first().toDouble()));
    source.publish(position);
    QCOMPARE(accuracy.size(), 3);
}

void PositionManagerTest::_destructionStopsDevice()
{
    ManualScheduler scheduler;
    PositionSource source;
    GPSSourceHealth receiver(nullptr, &scheduler);
    auto service = std::make_unique<PositionManager>(nullptr, &scheduler);
    auto registration = service->registerReceiver(&receiver);
    service->setInternalPositionSource(&source, Status::WaitingForFix);
    receiver.updateObservation(fix(scheduler, 48));
    source.publish(fix(scheduler).position);
    QCOMPARE(service->selectedSource(), Kind::Receiver);
    QVERIFY(source.active);
    int notifications = 0;
    connect(service.get(), &PositionManager::selectionChanged, this, [&]() { ++notifications; });
    connect(service.get(), &PositionManager::gcsPositionChanged, this, [&]() { ++notifications; });
    service.reset();
    QVERIFY(!source.active);
    QCOMPARE(notifications, 0);
    // The surviving producer and registration no longer reach the destroyed service.
    receiver.updateObservation(fix(scheduler));
    registration.reset();
}

void PositionManagerTest::_qmlPositionProperties()
{
    GPSTest::QmlEngine engine;
    ManualScheduler scheduler;
    PositionManager service(nullptr, &scheduler);
    std::unique_ptr<QObject> object = engine.create(QByteArray(R"(
        import QtQml
        import QGroundControl
        QtObject {
            required property PositionManager service
            readonly property real latitude: service.gcsPosition.latitude
            readonly property bool usable: service.gcsPosition.isValid
        }
    )"),
                                                    {{QStringLiteral("service"), QVariant::fromValue(&service)}});
    QVERIFY2(object, qPrintable(engine.lastError()));
    GPSSourceHealth receiver(nullptr, &scheduler);
    auto registration = service.registerReceiver(&receiver);
    receiver.updateObservation(fix(scheduler, 53.25));
    QVERIFY(object->property("usable").toBool());
    QCOMPARE(object->property("latitude").toDouble(), 53.25);
    registration.reset();
    QVERIFY(!object->property("usable").toBool());
}

void PositionManagerTest::_initUsesPlatformSourceFactory()
{
    QLocationPermission permission;
    permission.setAccuracy(QLocationPermission::Precise);
    if (QCoreApplication::instance()->checkPermission(permission) != Qt::PermissionStatus::Granted) {
        QSKIP("The platform source is created only after location permission is granted");
    }
    ManualScheduler scheduler;
    PositionManager manager(nullptr, &scheduler);
    manager.setConfiguration({.sourceMode = Mode::InternalOnly});
    QList<QObject*> factoryParents;
    PositionSource* platformSource = nullptr;
    manager.setPlatformSourceFactory([&](QObject* parent) {
        factoryParents.append(parent);
        platformSource = new PositionSource;
        platformSource->setParent(parent);
        return platformSource;
    });
    manager.init();

    QCOMPARE(factoryParents, QList<QObject*>{&manager});
    QCOMPARE(manager.sourceStatus(), Status::WaitingForFix);
    QVERIFY(platformSource->active);
    platformSource->publish(fix(scheduler).position);
    QCOMPARE(manager.selectedSource(), Kind::Internal);
    QCOMPARE(manager.selectedSourceName(), QStringLiteral("Plugin positioning"));
    QCOMPARE(manager.gcsPosition().latitude(), 47);
}

void PositionManagerTest::_shutdownReleasesSources()
{
    ManualScheduler scheduler;
    PositionManager manager(nullptr, &scheduler);
    PositionSource source;
    int factoryCalls = 0;
    manager.setPlatformSourceFactory([&](QObject*) {
        ++factoryCalls;
        return nullptr;
    });
    manager.setInternalPositionSource(&source, Status::WaitingForFix);
    source.publish(fix(scheduler).position);
    QCOMPARE(manager.selectedSource(), Kind::Internal);
    QVERIFY(manager.gcsPosition().isValid());

    manager.shutdown();
    QVERIFY(!source.active);
    QCOMPARE(manager.selectedSource(), Kind::None);
    QVERIFY(!manager.gcsPosition().isValid());
    manager.setConfiguration({.sourceMode = Mode::ReceiverOnly});
    QCOMPARE(manager.configuration().sourceMode, Mode::Automatic);
    manager.init();
    QCOMPARE(factoryCalls, 0);
    QVERIFY(!manager.acceptedObservation());
}

UT_REGISTER_TEST(PositionManagerTest, TestLabel::Unit)
