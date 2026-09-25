#include "GPSPositionServiceTest.h"

#include <memory>
#include <utility>

#include <QtCore/QScopeGuard>
#include <QtCore/QThread>
#include <QtTest/QSignalSpy>

#include "GPSPositionService.h"
#include "GpsTestHelpers.h"
#include "ManualScheduler.h"

namespace {
using Kind = GPSPositionService::SelectedSource;
using Mode = GPSPositionService::SourceMode;
using Status = GPSPositionService::SourceStatus;

using GpsTestHelpers::PositionSource;

GPSObservation fix(ManualScheduler& scheduler, double latitude = 47, quint64 session = 0)
{
    GPSObservation observation;
    observation.sessionId = session;
    observation.sourceId = QStringLiteral("test-receiver");
    observation.receivedAt = QDateTime::currentDateTimeUtc();
    observation.monotonicTimestampUs = scheduler.nowUs();
    observation.position = QGeoPositionInfo(QGeoCoordinate(latitude, 8, 500), observation.receivedAt);
    observation.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1);
    observation.position.setAttribute(QGeoPositionInfo::VerticalAccuracy, 1);
    observation.position.setAttribute(QGeoPositionInfo::GroundSpeed, 2);
    observation.position.setAttribute(QGeoPositionInfo::Direction, 90);
    return observation;
}
}  // namespace

void GPSPositionServiceTest::_rawSourceSharingRejected_data()
{
    QTest::addColumn<int>("firstKind");
    QTest::addColumn<int>("secondKind");
    for (const auto first : {Kind::Receiver, Kind::Internal, Kind::Simulated}) {
        for (const auto second : {Kind::Receiver, Kind::Internal, Kind::Simulated}) {
            if (first != second) {
                const auto name = QByteArray::number(int(first)) + "-then-" + QByteArray::number(int(second));
                QTest::newRow(name.constData()) << int(first) << int(second);
            }
        }
    }
}

void GPSPositionServiceTest::_rawSourceSharingRejected()
{
    QFETCH(int, firstKind);
    QFETCH(int, secondKind);
    ManualScheduler scheduler;
    PositionSource source;
    GPSPositionService service(nullptr, &scheduler);
    const auto bind = [&](Kind kind) {
        if (kind == Kind::Internal) {
            service.setInternalPositionSource(&source, Status::WaitingForFix);
        } else if (kind == Kind::Simulated) {
            service.setSimulatedPositionSource(&source);
        } else {
            return service.registerPositionSource(kind, &source);
        }
        return GPSPositionSourceRegistration{};
    };
    auto first = bind(Kind(firstKind));
    auto second = bind(Kind(secondKind));
    QVERIFY(!second);
    QCOMPARE(service.selectedSource(), Kind(firstKind));
    if (Kind(secondKind) == Kind::Internal) {
        service.setInternalPositionSource(nullptr, Status::NoSource);
    } else if (Kind(secondKind) == Kind::Simulated) {
        service.setSimulatedPositionSource(nullptr);
    } else {
        second.reset();
    }
    QVERIFY(source.active);
    source.publish(fix(scheduler).position);
    QCOMPARE(service.gcsPosition(), fix(scheduler).coordinate());
    QCOMPARE(service.sourceStatus(), Status::Active);
}

void GPSPositionServiceTest::_selectionStatusMatchesPublication_data()
{
    QTest::addColumn<int>("mode");
    QTest::newRow("receiver-only") << int(Mode::ReceiverOnly);
    QTest::newRow("internal-only") << int(Mode::InternalOnly);
}

void GPSPositionServiceTest::_selectionStatusMatchesPublication()
{
    QFETCH(int, mode);
    ManualScheduler scheduler;
    GPSSourceHealth primary(nullptr, &scheduler);
    PositionSource internal;
    GPSPositionService service(nullptr, &scheduler);
    auto receiverRegistration = service.registerPositionSource(Kind::Receiver, &primary);
    service.setInternalPositionSource(&internal, Status::WaitingForFix);
    service.setSourceMode(Mode::Automatic);
    primary.updateObservation(fix(scheduler));
    internal.publish(fix(scheduler, 48).position);
    const auto checkPublication = [&]() {
        QCOMPARE(service.sourceStatus() == Status::Active, service.gcsPosition().isValid());
    };
    connect(&service, &GPSPositionService::selectionChanged, &service, checkPublication);
    connect(&service, &GPSPositionService::gcsPositionChanged, &service, checkPublication);
    service.setSourceMode(Mode(mode));
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
    QSignalSpy reports(&service, &GPSPositionService::gcsPositionChanged);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(5)));
    QCOMPARE(service.sourceStatus(), Status::Stale);
    QVERIFY(!service.gcsPosition().isValid());
    QCOMPARE(reports.size(), 1);
}

void GPSPositionServiceTest::_automaticRejectionMatchesPublication_data()
{
    QTest::addColumn<bool>("providedHealth");
    QTest::addColumn<bool>("expire");
    QTest::newRow("provided-inaccurate") << true << false;
    QTest::newRow("provided-expired") << true << true;
    QTest::newRow("raw-inaccurate") << false << false;
    QTest::newRow("raw-expired") << false << true;
}

void GPSPositionServiceTest::_automaticRejectionMatchesPublication()
{
    QFETCH(bool, providedHealth);
    QFETCH(bool, expire);
    ManualScheduler scheduler;
    PositionSource source;
    GPSSourceHealth health(nullptr, &scheduler);
    GPSPositionService service(nullptr, &scheduler);
    auto registration = providedHealth ? service.registerPositionSource(Kind::Receiver, &health)
                                       : service.registerPositionSource(Kind::Receiver, &source);
    QVERIFY(registration);
    service.setSourceMode(Mode::Automatic);
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

    QSignalSpy selections(&service, &GPSPositionService::selectionChanged);
    QSignalSpy positions(&service, &GPSPositionService::gcsPositionChanged);
    const auto checkPublication = [&]() {
        const bool active = service.sourceStatus() == Status::Active;
        QCOMPARE(service.gcsPosition().isValid(), active);
        QCOMPARE(service.acceptedObservation().has_value(), active);
    };
    connect(&service, &GPSPositionService::selectionChanged, &service, checkPublication);
    connect(&service, &GPSPositionService::gcsPositionChanged, &service, checkPublication);
    if (expire) {
        QVERIFY(scheduler.advanceBy(std::chrono::seconds(5)));
    } else {
        auto inaccurate = fix(scheduler);
        inaccurate.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 101);
        publish(inaccurate);
    }
    QCOMPARE(service.sourceStatus(), expire ? Status::Stale : Status::InvalidFix);
    QCOMPARE(service.selectedSource(), Kind::Receiver);
    QCOMPARE(selections.size(), 1);
    QCOMPARE(positions.size(), 1);
    QVERIFY(!service.gcsPosition().isValid());

    publish(fix(scheduler, 48));
    QCOMPARE(service.sourceStatus(), Status::Active);
    QCOMPARE(service.gcsPosition().latitude(), 48);
    QCOMPARE(selections.size(), 2);
    QCOMPARE(positions.size(), 2);
}

void GPSPositionServiceTest::_sourcesShareAcceptance_data()
{
    QTest::addColumn<int>("kind");
    QTest::addColumn<bool>("custom");
    QTest::newRow("platform") << int(Kind::Internal) << false;
    QTest::newRow("custom") << int(Kind::Internal) << true;
    QTest::newRow("receiver") << int(Kind::Receiver) << false;
}

void GPSPositionServiceTest::_sourcesShareAcceptance()
{
    QFETCH(int, kind);
    QFETCH(bool, custom);
    ManualScheduler scheduler;
    PositionSource source;
    GPSPositionService service(nullptr, &scheduler);
    GPSPositionSourceRegistration registration;
    if (Kind(kind) == Kind::Internal) {
        service.setInternalPositionSource(&source, Status::WaitingForFix, custom);
    } else {
        registration = service.registerPositionSource(Kind(kind), &source);
    }
    QVERIFY(source.active);
    QCOMPARE(service.selectedSource(), Kind(kind));
    auto observation = fix(scheduler, 0);
    source.publish(observation.position);
    QCOMPARE(service.gcsPosition(), observation.coordinate());
    QCOMPARE(service.gcsHeading(), observation.heading());
    QVERIFY(service.acceptedObservation());
    QCOMPARE(service.selectedHealth()->observation().altitudeDatum, GPSAltitudeDatum::Unknown);
    const auto remoteId = service.acceptedObservation(GPSObservation::PositionUse::RemoteID);
    QVERIFY(remoteId);
    QVERIFY(remoteId->position.coordinate().isValid());
    QVERIFY(qIsNaN(remoteId->position.coordinate().altitude()));
    QVERIFY(!remoteId->position.hasAttribute(QGeoPositionInfo::VerticalAccuracy));
    QCOMPARE(remoteId->altitudeDatum, GPSAltitudeDatum::Unknown);
    if (Kind(kind) == Kind::Internal) {
        QCOMPARE(service.selectedHealth()->observation().sourceId,
                 custom ? QStringLiteral("Plugin") : QStringLiteral("Platform"));
        QCOMPARE(service.updateInterval(), source.minimumUpdateInterval());
    }
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

void GPSPositionServiceTest::_registrationReplacementAndSessions()
{
    ManualScheduler scheduler;
    GPSPositionService service(nullptr, &scheduler);
    GPSSourceHealth health(nullptr, &scheduler);
    auto first = service.registerPositionSource(Kind::Receiver, &health, 11);
    health.updateObservation(fix(scheduler, 47, 12));
    QVERIFY(!service.gcsPosition().isValid());
    health.updateObservation(fix(scheduler, 47, 11));
    QVERIFY(service.gcsPosition().isValid());
    auto second = service.registerPositionSource(Kind::Receiver, &health, 12);
    auto moved = std::move(first);
    QVERIFY(!first);
    moved.reset();
    QCOMPARE(service.selectedHealth(), &health);
    QVERIFY(!service.gcsPosition().isValid());
    health.updateObservation(fix(scheduler, 47, 11));
    QVERIFY(!service.gcsPosition().isValid());
    health.updateObservation(fix(scheduler, 48, 12));
    QCOMPARE(service.gcsPosition().latitude(), 48);
    QCOMPARE(service.acceptedObservation()->sessionId, quint64(12));
    second.reset();
    QVERIFY(!service.selectedHealth());
    health.updateObservation(fix(scheduler, 47, 12));
    QVERIFY(!service.gcsPosition().isValid());
}

void GPSPositionServiceTest::_automaticFailoverAndRecovery()
{
    ManualScheduler scheduler;
    GPSSourceHealth primary(nullptr, &scheduler);
    PositionSource internal;
    primary.setFreshnessTimeoutMs(1000);
    GPSPositionService service(nullptr, &scheduler);
    auto receiverRegistration = service.registerPositionSource(Kind::Receiver, &primary);
    service.setInternalPositionSource(&internal, Status::WaitingForFix);
    service.sourceHealth(Kind::Internal)->setFreshnessTimeoutMs(60000);
    primary.updateObservation(fix(scheduler));
    internal.publish(fix(scheduler, 48).position);
    QCOMPARE(service.selectedSource(), Kind::Receiver);
    primary.invalidatePosition();
    QCOMPARE(service.selectedSource(), Kind::Internal);
    QCOMPARE(service.gcsPosition().latitude(), 48);
    primary.setFreshnessTimeoutMs(60000);
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
    service.setSourceMode(Mode::InternalOnly);
    internal.publish(fix(scheduler, 48).position);
    receiverRegistration.reset();
    QCOMPARE(service.gcsPosition().latitude(), 48);
    QCOMPARE(service.selectedSource(), Kind::Internal);
}

void GPSPositionServiceTest::_sourceAndHealthLifetime()
{
    ManualScheduler scheduler;
    PositionSource platform;
    auto receiver = std::make_unique<PositionSource>();
    auto health = std::make_unique<GPSSourceHealth>(nullptr, &scheduler);
    auto service = std::make_unique<GPSPositionService>(nullptr, &scheduler);
    service->setInternalPositionSource(&platform, Status::WaitingForFix);
    auto registration = service->registerPositionSource(Kind::Receiver, health.get());
    health->updateObservation(fix(scheduler));
    QCOMPARE(service->selectedSource(), Kind::Receiver);
    QVERIFY(service->gcsPosition().isValid());
    // A lost producer retires its role; nothing falls back to another object.
    health.reset();
    QCOMPARE(service->selectedSource(), Kind::Internal);
    QVERIFY(!service->gcsPosition().isValid());
    auto backend = service->registerPositionSource(Kind::Receiver, receiver.get());
    QVERIFY(backend);
    receiver->publish(fix(scheduler).position);
    QCOMPARE(service->selectedSource(), Kind::Receiver);
    QVERIFY(service->gcsPosition().isValid());
    receiver.reset();
    QCOMPARE(service->selectedSource(), Kind::Internal);
    QVERIFY(!service->gcsPosition().isValid());
    platform.publish(fix(scheduler).position);
    QVERIFY(service->gcsPosition().isValid());
    service.reset();
    QVERIFY(!platform.active);
    registration.reset();
    backend.reset();
}

void GPSPositionServiceTest::_standbyReportsDoNotRepublish()
{
    ManualScheduler scheduler;
    GPSSourceHealth primary(nullptr, &scheduler);
    PositionSource internal;
    primary.setFreshnessTimeoutMs(60000);
    GPSPositionService service(nullptr, &scheduler);
    auto receiverRegistration = service.registerPositionSource(Kind::Receiver, &primary);
    service.setInternalPositionSource(&internal, Status::WaitingForFix);
    service.sourceHealth(Kind::Internal)->setFreshnessTimeoutMs(60000);
    service.setSourceMode(Mode::Automatic);
    primary.updateObservation(fix(scheduler));
    QSignalSpy coordinates(&service, &GPSPositionService::gcsPositionChanged);
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

void GPSPositionServiceTest::_consumerPolicies_data()
{
    QTest::addColumn<double>("speed");
    QTest::addColumn<double>("course");
    QTest::addColumn<double>("verticalAccuracy");
    QTest::addColumn<double>("altitude");
    QTest::addColumn<double>("ellipsoid");
    QTest::addColumn<bool>("horizontalAccuracy");
    QTest::newRow("moving") << 2.0 << 90.0 << 1.0 << 500.0 << qQNaN() << true;
    QTest::newRow("slow") << 0.1 << 90.0 << 1.0 << 500.0 << qQNaN() << true;
    QTest::newRow("no-course") << 2.0 << qQNaN() << 1.0 << 500.0 << qQNaN() << true;
    QTest::newRow("no-speed") << qQNaN() << 90.0 << 1.0 << 500.0 << qQNaN() << true;
    QTest::newRow("no-vertical-accuracy") << 2.0 << 90.0 << qQNaN() << 500.0 << qQNaN() << true;
    QTest::newRow("poor-vertical-accuracy") << 2.0 << 90.0 << 11.0 << 500.0 << qQNaN() << true;
    QTest::newRow("ellipsoid") << 2.0 << 90.0 << 11.0 << 500.0 << 550.0 << true;
    QTest::newRow("ellipsoid-only") << 2.0 << 90.0 << qQNaN() << qQNaN() << 550.0 << true;
    QTest::newRow("no-altitude") << 2.0 << 90.0 << qQNaN() << qQNaN() << qQNaN() << true;
    QTest::newRow("no-horizontal-accuracy") << 2.0 << 90.0 << 1.0 << 500.0 << qQNaN() << false;
}

void GPSPositionServiceTest::_consumerPolicies()
{
    QFETCH(double, speed);
    QFETCH(double, course);
    QFETCH(double, verticalAccuracy);
    QFETCH(double, altitude);
    QFETCH(double, ellipsoid);
    QFETCH(bool, horizontalAccuracy);
    using Use = GPSObservation::PositionUse;
    ManualScheduler scheduler;
    GPSSourceHealth health(nullptr, &scheduler);
    GPSPositionService service(nullptr, &scheduler);
    auto registration = service.registerPositionSource(Kind::Receiver, &health, 7);
    auto observation = fix(scheduler, 47, 7);
    observation.position.setCoordinate(QGeoCoordinate(47, 8, altitude));
    const auto attribute = [&](QGeoPositionInfo::Attribute name, double value) {
        if (qIsFinite(value)) {
            observation.position.setAttribute(name, value);
        } else {
            observation.position.removeAttribute(name);
        }
    };
    attribute(QGeoPositionInfo::GroundSpeed, speed);
    attribute(QGeoPositionInfo::Direction, course);
    attribute(QGeoPositionInfo::VerticalAccuracy, verticalAccuracy);
    if (!horizontalAccuracy) {
        observation.position.removeAttribute(QGeoPositionInfo::HorizontalAccuracy);
    }
    observation.altitudeDatum = GPSAltitudeDatum::MeanSeaLevel;
    observation.fixQuality = GPSObservation::FixQuality::Fix3D;
    observation.satellitesUsed = 12;
    observation.horizontalDop = 0.8;
    if (qIsFinite(ellipsoid)) {
        observation.altitudeEllipsoidMeters = ellipsoid;
    }
    health.updateObservation(observation);
    for (const auto use : {Use::GroundStation, Use::Motion, Use::RemoteID, Use::Gga}) {
        const auto accepted = service.acceptedObservation(use);
        QCOMPARE(accepted.has_value(), horizontalAccuracy || use == Use::Gga);
        if (!accepted) {
            continue;
        }
        QCOMPARE(accepted->sourceId, observation.sourceId);
        QCOMPARE(accepted->sessionId, observation.sessionId);
        QCOMPARE(accepted->monotonicTimestampUs, observation.monotonicTimestampUs);
        QCOMPARE(accepted->receivedAt, observation.receivedAt);
        QCOMPARE(accepted->position.coordinate().latitude(), 47);
        const double expectedAltitude = use == Use::Gga || use == Use::Motion ? altitude
                                        : use == Use::RemoteID                ? ellipsoid
                                        : verticalAccuracy <= 10              ? altitude
                                                                              : qQNaN();
        if (qIsFinite(expectedAltitude)) {
            QCOMPARE(accepted->position.coordinate().altitude(), expectedAltitude);
        } else {
            QVERIFY(qIsNaN(accepted->position.coordinate().altitude()));
        }
        if (use == Use::Motion) {
            QCOMPARE(accepted->position.hasAttribute(QGeoPositionInfo::Direction),
                     qIsFinite(course) && qIsFinite(speed) && speed >= 0.5);
        }
        if (use == Use::RemoteID && !qIsFinite(ellipsoid)) {
            QVERIFY(!accepted->position.hasAttribute(QGeoPositionInfo::VerticalAccuracy));
            QCOMPARE(accepted->altitudeDatum, GPSAltitudeDatum::Unknown);
        }
        if (use == Use::Gga) {
            QCOMPARE(accepted->position, observation.position);
            QCOMPARE(accepted->altitudeDatum, GPSAltitudeDatum::MeanSeaLevel);
            QCOMPARE(accepted->satellitesUsed, observation.satellitesUsed);
            QCOMPARE(accepted->horizontalDop, observation.horizontalDop);
        }
        QCOMPARE(service.gcsPosition().isValid(), horizontalAccuracy);
    }
    QCOMPARE(health.observation().position, observation.position);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(4999)));
    QCOMPARE(service.acceptedObservation(Use::RemoteID).has_value(), horizontalAccuracy);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    for (const auto use : {Use::GroundStation, Use::Motion, Use::RemoteID, Use::Gga}) {
        QVERIFY(!service.acceptedObservation(use));
    }
}

void GPSPositionServiceTest::_consumerMaximumAge()
{
    using namespace std::chrono_literals;
    using Use = GPSObservation::PositionUse;
    ManualScheduler serviceClock;
    ManualScheduler sourceClock;
    QVERIFY(sourceClock.advanceBy(1h));
    GPSSourceHealth health(nullptr, &sourceClock);
    health.setFreshnessTimeoutMs(10000);
    GPSPositionService service(nullptr, &serviceClock);
    auto registration = service.registerPositionSource(Kind::Receiver, &health, 7);
    const auto observation = fix(sourceClock, 47, 7);
    health.updateObservation(observation);
    QVERIFY(service.acceptedObservation(Use::RemoteID, 5000ms));
    QVERIFY(serviceClock.advanceBy(24h));
    QVERIFY(sourceClock.advanceBy(4999ms));
    const auto accepted = service.acceptedObservation(Use::RemoteID, 5000ms);
    QVERIFY(accepted);
    QCOMPARE(accepted->monotonicTimestampUs, observation.monotonicTimestampUs);
    QCOMPARE(accepted->receivedAt, observation.receivedAt);
    QVERIFY(sourceClock.advanceBy(1ms));
    QVERIFY(!service.acceptedObservation(Use::RemoteID, 5000ms));
    QVERIFY(service.acceptedObservation(Use::RemoteID));
    health.updateObservation(fix(sourceClock, 48, 7));
    QVERIFY(service.acceptedObservation(Use::RemoteID, 5000ms));
    health.setFreshnessTimeoutMs(1000);
    QVERIFY(sourceClock.advanceBy(1s));
    QVERIFY(!service.acceptedObservation(Use::RemoteID, 5000ms));
}

void GPSPositionServiceTest::_policySelectionGates()
{
    using Use = GPSObservation::PositionUse;
    ManualScheduler scheduler;
    GPSSourceHealth firstHealth(nullptr, &scheduler);
    GPSSourceHealth replacementHealth(nullptr, &scheduler);
    GPSPositionService service(nullptr, &scheduler);
    service.setSourceMode(Mode::ReceiverOnly);
    auto oldRegistration = service.registerPositionSource(Kind::Receiver, &firstHealth, 1);
    firstHealth.updateObservation(fix(scheduler, 47, 1));
    replacementHealth.updateObservation(fix(scheduler, 48, 2));
    auto registration = service.registerPositionSource(Kind::Receiver, &replacementHealth, 2);
    oldRegistration.reset();
    firstHealth.updateObservation(fix(scheduler, 49, 1));
    replacementHealth.setFreshnessTimeoutMs(10000);
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

void GPSPositionServiceTest::_notificationsCanSwitchOrDelete_data()
{
    QTest::addColumn<int>("mode");
    QTest::newRow("automatic") << int(Mode::Automatic);
}

void GPSPositionServiceTest::_notificationsCanSwitchOrDelete()
{
    QFETCH(int, mode);
    ManualScheduler scheduler;
    PositionSource receiver;
    PositionSource internal;
    auto service = std::make_unique<GPSPositionService>(nullptr, &scheduler);
    auto receiverRegistration = service->registerPositionSource(Kind::Receiver, &receiver);
    service->setInternalPositionSource(&internal, Status::WaitingForFix);
    service->setSourceMode(Mode(mode));
    QObject observer;
    connect(service.get(), &GPSPositionService::gcsPositionHorizontalAccuracyChanged, &observer, [&]() {
        if (service->gcsPosition().isValid() && service->sourceMode() != Mode::InternalOnly) {
            service->setSourceMode(Mode::InternalOnly);
        }
    });
    receiver.publish(fix(scheduler).position);
    QCOMPARE(service->selectedSource(), Kind::Internal);
    QVERIFY(!service->gcsPosition().isValid());
    receiver.publish(fix(scheduler).position);
    QVERIFY(!service->gcsPosition().isValid());
    internal.publish(fix(scheduler).position);
    QVERIFY(service->gcsPosition().isValid());
    connect(service.get(), &GPSPositionService::gcsPositionChanged, &observer, [&]() { service.reset(); });
    service->setInternalPositionSource(nullptr, Status::NoSource);
    QVERIFY(!service);
}

void GPSPositionServiceTest::_backendStatus()
{
    ManualScheduler scheduler;
    PositionSource platform;
    GPSPositionService service(nullptr, &scheduler);
    service.setInternalPositionStatus(Status::PermissionRequired);
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
    QSignalSpy selection(&service, &GPSPositionService::selectionChanged);
    service.setInternalPositionSource(&custom, Status::WaitingForFix, true);
    QVERIFY(!selection.isEmpty());
    QCOMPARE(service.selectedSourceName(), GPSPositionService::tr("Plugin positioning"));
}

void GPSPositionServiceTest::_backendTimeoutPreservesFreshness_data()
{
    QTest::addColumn<bool>("platform");
    QTest::addColumn<bool>("hasFix");
    QTest::newRow("platform-before-fix") << true << false;
    QTest::newRow("platform-fresh-fix") << true << true;
    QTest::newRow("receiver-before-fix") << false << false;
    QTest::newRow("receiver-fresh-fix") << false << true;
}

void GPSPositionServiceTest::_backendTimeoutPreservesFreshness()
{
    QFETCH(bool, platform);
    QFETCH(bool, hasFix);
    ManualScheduler scheduler;
    PositionSource source;
    GPSPositionService service(nullptr, &scheduler);
    GPSPositionSourceRegistration registration;
    if (platform) {
        service.setInternalPositionSource(&source, Status::WaitingForFix);
    } else {
        registration = service.registerPositionSource(Kind::Receiver, &source);
    }
    if (hasFix) {
        source.publish(fix(scheduler).position);
    }
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(2)));
    QSignalSpy positions(&service, &GPSPositionService::gcsPositionChanged);
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

void GPSPositionServiceTest::_adapterReentrantDeactivation_data()
{
    QTest::addColumn<int>("error");
    QTest::newRow("position") << int(QGeoPositionInfoSource::NoError);
    QTest::newRow("error") << int(QGeoPositionInfoSource::AccessError);
}

void GPSPositionServiceTest::_adapterReentrantDeactivation()
{
    QFETCH(int, error);
    ManualScheduler scheduler;
    PositionSource source;
    GPSPositionService service(nullptr, &scheduler);
    service.setInternalPositionSource(&source, Status::WaitingForFix);
    source.publish(fix(scheduler).position);
    auto* health = service.selectedHealth();
    if (error == QGeoPositionInfoSource::NoError) {
        source.fail(QGeoPositionInfoSource::AccessError);
    }
    bool deactivated = false;
    const auto connection = connect(&service, &GPSPositionService::selectionChanged, &service, [&]() {
        if (!std::exchange(deactivated, true)) {
            service.setSourceMode(Mode::ReceiverOnly);
        }
    });
    if (error == QGeoPositionInfoSource::NoError) {
        source.publish(fix(scheduler).position);
    } else {
        source.fail(QGeoPositionInfoSource::Error(error));
    }
    QVERIFY(!source.active);
    QCOMPARE(health->state(), GPSSourceHealth::State::NoData);
    QVERIFY(!health->acceptedObservation());
    disconnect(connection);
    service.setSourceMode(Mode::InternalOnly);
    source.publish(fix(scheduler).position);
    QVERIFY(service.acceptedObservation());
}

void GPSPositionServiceTest::_adapterReentrantReplacement()
{
    ManualScheduler scheduler;
    PositionSource first;
    auto next = std::make_unique<PositionSource>();
    GPSPositionService service(nullptr, &scheduler);
    service.setInternalPositionSource(&first, Status::WaitingForFix);
    first.onStop = [&]() { next.reset(); };
    service.setInternalPositionSource(next.get(), Status::WaitingForFix);
    QCOMPARE(service.selectedSource(), Kind::None);
    QVERIFY(!service.selectedHealth());
}

UT_REGISTER_TEST(GPSPositionServiceTest, TestLabel::Unit)

void GPSPositionServiceTest::_nestedObservationNotifications_data()
{
    QTest::addColumn<Mode>("mode");
    QTest::addColumn<bool>("fromHealth");
    QTest::newRow("automatic") << Mode::Automatic << false;
    QTest::newRow("automatic-health-callback") << Mode::Automatic << true;
}

void GPSPositionServiceTest::_nestedObservationNotifications()
{
    QFETCH(Mode, mode);
    QFETCH(bool, fromHealth);
    ManualScheduler scheduler;
    GPSSourceHealth health(nullptr, &scheduler);
    PositionSource standby;
    GPSPositionService service(nullptr, &scheduler);
    bool nested = false;
    const auto registerStandby = [&]() {
        if (!nested && (fromHealth || service.gcsPosition().isValid())) {
            nested = true;
            service.setInternalPositionSource(&standby, Status::WaitingForFix);
            if (!fromHealth) {
                health.updateObservation(fix(scheduler, 48));
            }
        }
    };
    if (fromHealth) {
        connect(&health, &GPSSourceHealth::positionChanged, &service, registerStandby);
    } else {
        connect(&service, &GPSPositionService::gcsPositionHorizontalAccuracyChanged, &service, registerStandby);
    }
    auto registration = service.registerPositionSource(Kind::Receiver, &health);
    service.setSourceMode(mode);
    QSignalSpy positions(&service, &GPSPositionService::gcsPositionChanged);
    QSignalSpy headings(&service, &GPSPositionService::gcsHeadingChanged);
    health.updateObservation(fix(scheduler));
    QVERIFY(nested);
    QCOMPARE(positions.size(), 1);
    QCOMPARE(headings.size(), 1);
    QCOMPARE(positions.first().first().value<QGeoCoordinate>(), service.gcsPosition());
    QCOMPARE(service.gcsPosition().latitude(), fromHealth ? 47 : 48);
}

void GPSPositionServiceTest::_rawRegistrationCarriesSession()
{
    ManualScheduler scheduler;
    PositionSource source;
    GPSPositionService service(nullptr, &scheduler);
    auto registration = service.registerPositionSource(Kind::Receiver, &source, 7);
    source.publish(fix(scheduler).position);
    const auto observation = service.acceptedObservation();
    QVERIFY(observation);
    QCOMPARE(observation->sessionId, quint64(7));
}

void GPSPositionServiceTest::_registrationRetiresFromWorker()
{
    ManualScheduler scheduler;
    PositionSource source;
    GPSPositionService service(nullptr, &scheduler);
    auto registration = service.registerPositionSource(Kind::Receiver, &source);
    auto worker = std::unique_ptr<QThread>(
        QThread::create([registration = std::move(registration)]() mutable { registration.reset(); }));
    worker->start();
    QVERIFY(worker->wait(TestTimeout::mediumMs()));
    QTRY_COMPARE_WITH_TIMEOUT(service.selectedSource(), Kind::None, TestTimeout::mediumMs());
    QVERIFY(!source.active);
}

void GPSPositionServiceTest::_producerLoss_data()
{
    QTest::addColumn<Mode>("mode");
    QTest::newRow("automatic") << Mode::Automatic;
    QTest::newRow("receiver-only") << Mode::ReceiverOnly;
}

void GPSPositionServiceTest::_producerLoss()
{
    QFETCH(Mode, mode);
    ManualScheduler scheduler;
    PositionSource source;
    auto health = std::make_unique<GPSSourceHealth>(nullptr, &scheduler);
    int stops = 0;
    source.onStop = [&]() { ++stops; };
    GPSPositionService service(nullptr, &scheduler);
    auto registration = service.registerPositionSource(Kind::Receiver, health.get());
    service.setSourceMode(mode);
    health->updateObservation(fix(scheduler));
    QVERIFY(service.gcsPosition().isValid());
    // Losing the producer retires its role.
    health.reset();
    QCOMPARE(service.selectedSource(), Kind::None);
    QVERIFY(!service.gcsPosition().isValid());
    QVERIFY(!service.acceptedObservation());
    auto backend = service.registerPositionSource(Kind::Receiver, &source);
    QVERIFY(backend);
    QVERIFY(source.active);
    source.publish(fix(scheduler, 48).position);
    QCOMPARE(service.selectedSource(), Kind::Receiver);
    QVERIFY(service.acceptedObservation());
    QCOMPARE(service.gcsPosition().latitude(), 48);
    // The superseded registration cannot retire the backend.
    registration.reset();
    QVERIFY(source.active);
    backend.reset();
    QCOMPARE(stops, 1);
    QVERIFY(!source.active);
}

void GPSPositionServiceTest::_adapterNestedBackendEvent_data()
{
    QTest::addColumn<bool>("outerError");
    QTest::addColumn<bool>("restart");
    QTest::newRow("position-position") << false << false;
    QTest::newRow("error-position") << true << false;
    QTest::newRow("position-restart") << false << true;
    QTest::newRow("error-restart") << true << true;
}

void GPSPositionServiceTest::_adapterNestedBackendEvent()
{
    QFETCH(bool, outerError);
    QFETCH(bool, restart);
    ManualScheduler scheduler;
    PositionSource source;
    GPSPositionService service(nullptr, &scheduler);
    service.setInternalPositionSource(&source, Status::WaitingForFix);
    source.publish(fix(scheduler).position);
    if (!outerError) {
        source.fail(QGeoPositionInfoSource::AccessError);
    }
    bool nested = false;
    connect(&service, &GPSPositionService::selectionChanged, &service, [&]() {
        if (std::exchange(nested, true)) {
            return;
        }
        if (restart) {
            service.setSourceMode(Mode::ReceiverOnly);
            service.setSourceMode(Mode::InternalOnly);
        } else {
            source.publish(fix(scheduler, 48).position);
        }
    });
    if (outerError) {
        source.fail(QGeoPositionInfoSource::AccessError);
    } else {
        source.publish(fix(scheduler).position);
    }
    QVERIFY(nested);
    const auto accepted = service.acceptedObservation();
    if (restart) {
        QVERIFY(!accepted);
        QCOMPARE(service.selectedHealth()->state(), GPSSourceHealth::State::NoData);
    } else {
        QVERIFY(accepted);
        QCOMPARE(accepted->position.coordinate().latitude(), 48);
    }
    QVERIFY(source.active);
    source.publish(fix(scheduler, 49).position);
    QVERIFY(service.acceptedObservation());
    QCOMPARE(service.acceptedObservation()->position.coordinate().latitude(), 49);
}

void GPSPositionServiceTest::_accuracyNotifiesOnlyChanges()
{
    ManualScheduler scheduler;
    PositionSource source;
    GPSPositionService service(nullptr, &scheduler);
    auto registration = service.registerPositionSource(Kind::Receiver, &source);
    QSignalSpy accuracy(&service, &GPSPositionService::gcsPositionHorizontalAccuracyChanged);
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

void GPSPositionServiceTest::_destructionDisconnectsBindings()
{
    ManualScheduler scheduler;
    auto first = std::make_unique<PositionSource>();
    PositionSource second;
    auto service = std::make_unique<GPSPositionService>(nullptr, &scheduler);
    auto receiver = service->registerPositionSource(Kind::Receiver, first.get());
    service->setInternalPositionSource(&second, Status::WaitingForFix);
    service->setSourceMode(Mode::Automatic);
    QVERIFY(first->active && second.active);
    second.onStop = [&]() { first.reset(); };
    int publications = 0;
    connect(service.get(), &GPSPositionService::gcsPositionChanged, this, [&]() { ++publications; });
    service.reset();
    QVERIFY(!first);
    QVERIFY(!second.active);
    QCOMPARE(publications, 0);
}
