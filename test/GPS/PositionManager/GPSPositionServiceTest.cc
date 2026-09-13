#include "GPSPositionServiceTest.h"

#include <QtCore/QScopeGuard>
#include <QtCore/QThread>
#include <QtTest/QSignalSpy>

#include <memory>
#include <utility>

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

void GPSPositionServiceTest::_sharedProducerRoles()
{
    ManualScheduler scheduler;
    GPSPositionService service(nullptr, &scheduler);
    PositionSource producer;
    GPSSourceHealth health(nullptr, &scheduler);
    auto receiver = service.registerPositionSource(Kind::Receiver, &producer, &health, 1);
    auto nmea = service.registerPositionSource(Kind::Nmea, &producer, &health, 2);
    service.setSourceMode(Mode::Automatic);
    health.updateObservation(fix(scheduler, 47, 1));
    QCOMPARE(service.selectedSource(), Kind::Receiver);
    QVERIFY(service.gcsPosition().isValid());

    health.updateObservation(fix(scheduler, 48, 2));
    QCOMPARE(service.selectedSource(), Kind::Nmea);
    QCOMPARE(service.selectedSourceName(), QStringLiteral("NMEA"));
    QCOMPARE(service.gcsPosition().latitude(), 48);
    QVERIFY(service.acceptedObservation());
    QCOMPARE(service.acceptedObservation()->sessionId, 2u);

    service.setSourceMode(Mode::ReceiverOnly);
    QCOMPARE(service.selectedSource(), Kind::Receiver);
    QVERIFY(!service.gcsPosition().isValid());
    service.setSourceMode(Mode::NmeaOnly);
    QCOMPARE(service.selectedSource(), Kind::Nmea);
    health.updateObservation(fix(scheduler, 49, 2));
    QCOMPARE(service.gcsPosition().latitude(), 49);
    receiver.reset();
    QCOMPARE(service.selectedSource(), Kind::Nmea);
    nmea.reset();
    QCOMPARE(service.selectedSource(), Kind::None);
}

void GPSPositionServiceTest::_rawSourceSharingRejected_data()
{
    QTest::addColumn<int>("firstKind");
    QTest::addColumn<int>("secondKind");
    for (const auto first : {Kind::Receiver, Kind::Nmea, Kind::Internal, Kind::Simulated}) {
        for (const auto second : {Kind::Receiver, Kind::Nmea, Kind::Internal, Kind::Simulated}) {
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
            return service.registerPositionSource(kind, &source, nullptr);
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
    QTest::newRow("nmea-only") << int(Mode::NmeaOnly);
    QTest::newRow("legacy-priority") << int(Mode::LegacyPriority);
}

void GPSPositionServiceTest::_selectionStatusMatchesPublication()
{
    QFETCH(int, mode);
    ManualScheduler scheduler;
    QObject receiver;
    QObject nmea;
    GPSSourceHealth primary(nullptr, &scheduler);
    GPSSourceHealth secondary(nullptr, &scheduler);
    GPSPositionService service(nullptr, &scheduler);
    auto receiverRegistration = service.registerPositionSource(Kind::Receiver, &receiver, &primary);
    auto nmeaRegistration = service.registerPositionSource(Kind::Nmea, &nmea, &secondary);
    service.setSourceMode(Mode::Automatic);
    primary.updateObservation(fix(scheduler));
    secondary.updateObservation(fix(scheduler, 48));
    const auto checkPublication = [&]() {
        QCOMPARE(service.sourceStatus() == Status::Active, service.gcsPosition().isValid());
        QCOMPARE(service.sourceStatus() == Status::Active, service.geoPositionInfo().isValid());
    };
    connect(&service, &GPSPositionService::selectionChanged, &service, checkPublication);
    connect(&service, &GPSPositionService::gcsPositionChanged, &service, checkPublication);
    service.setSourceMode(Mode(mode));
    QCOMPARE(service.sourceStatus(), Status::WaitingForFix);
    QVERIFY(!service.acceptedObservation());
    QVERIFY(!service.gcsPosition().isValid());
    auto& selectedHealth = Mode(mode) == Mode::NmeaOnly ? secondary : primary;
    selectedHealth.updateObservation(fix(scheduler));
    QCOMPARE(service.sourceStatus(), Status::Active);
    QVERIFY(service.acceptedObservation());
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(5)));
    QCOMPARE(service.sourceStatus(), Status::Stale);
    QVERIFY(!service.gcsPosition().isValid());
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
    auto registration = service.registerPositionSource(Kind::Receiver, &source, providedHealth ? &health : nullptr);
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
    QSignalSpy positions(&service, &GPSPositionService::positionInfoUpdated);
    const auto checkPublication = [&]() {
        const bool active = service.sourceStatus() == Status::Active;
        QCOMPARE(service.gcsPosition().isValid(), active);
        QCOMPARE(service.geoPositionInfo().isValid(), active);
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
    QTest::newRow("NMEA") << int(Kind::Nmea) << false;
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
        registration = service.registerPositionSource(Kind(kind), &source, nullptr);
    }
    QVERIFY(source.active);
    QCOMPARE(service.selectedSource(), Kind(kind));
    auto observation = fix(scheduler, 0);
    source.publish(observation.position);
    QCOMPARE(service.gcsPosition(), observation.coordinate());
    QCOMPARE(service.gcsHeading(), observation.heading());
    QVERIFY(service.gcsPositionTimestamp().isValid());
    if (Kind(kind) == Kind::Internal) {
        QCOMPARE(service.sourceHealth()->observation().sourceId,
                 custom ? QStringLiteral("Plugin") : QStringLiteral("Platform"));
        QCOMPARE(service.updateInterval(), source.minimumUpdateInterval());
    }
    observation.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 101);
    source.publish(observation.position);
    QCOMPARE(service.sourceStatus(), Status::InvalidFix);
    QVERIFY(!service.gcsPosition().isValid());
    QVERIFY(!service.geoPositionInfo().isValid());
    source.publish(fix(scheduler).position);
    QVERIFY(service.gcsPosition().isValid());
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(5)));
    QCOMPARE(service.sourceStatus(), Status::Stale);
    QVERIFY(!service.gcsPosition().isValid());
    QVERIFY(!service.gcsPositionTimestamp().isValid());
    QVERIFY(qIsNaN(service.gcsHeading()));
    source.publish(fix(scheduler).position);
    QCOMPARE(service.sourceStatus(), Status::Active);
}

void GPSPositionServiceTest::_registrationReplacementAndSessions()
{
    ManualScheduler scheduler;
    GPSPositionService service(nullptr, &scheduler);
    PositionSource producer;
    GPSSourceHealth health(nullptr, &scheduler);
    auto first = service.registerPositionSource(Kind::Receiver, &producer, &health, 11);
    producer.publish(fix(scheduler).position);
    QVERIFY(!service.gcsPosition().isValid());
    health.updateObservation(fix(scheduler, 47, 11));
    QVERIFY(service.gcsPosition().isValid());
    auto second = service.registerPositionSource(Kind::Receiver, &producer, &health, 12);
    auto moved = std::move(first);
    QVERIFY(!first);
    moved.reset();
    QCOMPARE(service.sourceHealth(), &health);
    QVERIFY(!service.gcsPosition().isValid());
    health.updateObservation(fix(scheduler, 47, 11));
    QVERIFY(!service.gcsPosition().isValid());
    health.updateObservation(fix(scheduler, 48, 12));
    QCOMPARE(service.gcsPosition().latitude(), 48);
    QCOMPARE(service.acceptedObservation()->sessionId, quint64(12));
    second.reset();
    QVERIFY(!service.sourceHealth());
    health.updateObservation(fix(scheduler, 47, 12));
    QVERIFY(!service.gcsPosition().isValid());
}

void GPSPositionServiceTest::_automaticFailoverAndRecovery()
{
    ManualScheduler scheduler;
    QObject receiver;
    QObject nmea;
    GPSSourceHealth primary(nullptr, &scheduler);
    GPSSourceHealth secondary(nullptr, &scheduler);
    primary.setFreshnessTimeoutMs(1000);
    secondary.setFreshnessTimeoutMs(60000);
    GPSPositionService service(nullptr, &scheduler);
    auto receiverRegistration = service.registerPositionSource(Kind::Receiver, &receiver, &primary);
    auto nmeaRegistration = service.registerPositionSource(Kind::Nmea, &nmea, &secondary);
    primary.updateObservation(fix(scheduler));
    secondary.updateObservation(fix(scheduler, 48));
    primary.invalidatePosition();
    QCOMPARE(service.selectedSource(), Kind::Receiver);
    QVERIFY(!service.gcsPosition().isValid());
    service.setSourceMode(Mode::Automatic);
    QCOMPARE(service.selectedSource(), Kind::Nmea);
    QCOMPARE(service.gcsPosition().latitude(), 48);
    primary.setFreshnessTimeoutMs(60000);
    primary.updateObservation(fix(scheduler));
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(2)));
    primary.invalidatePosition();
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(4)));
    QCOMPARE(service.selectedSource(), Kind::Nmea);
    primary.updateObservation(fix(scheduler));
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(4999)));
    QCOMPARE(service.selectedSource(), Kind::Nmea);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QCOMPARE(service.selectedSource(), Kind::Receiver);
    service.setSourceMode(Mode::NmeaOnly);
    secondary.updateObservation(fix(scheduler, 48));
    receiverRegistration.reset();
    QCOMPARE(service.gcsPosition().latitude(), 48);
    QCOMPARE(service.selectedSource(), Kind::Nmea);
}

void GPSPositionServiceTest::_sourceAndHealthLifetime()
{
    ManualScheduler scheduler;
    PositionSource platform;
    auto receiver = std::make_unique<PositionSource>();
    auto health = std::make_unique<GPSSourceHealth>(nullptr, &scheduler);
    auto service = std::make_unique<GPSPositionService>(nullptr, &scheduler);
    service->setInternalPositionSource(&platform, Status::WaitingForFix);
    auto registration = service->registerPositionSource(Kind::Receiver, receiver.get(), health.get());
    health->updateObservation(fix(scheduler));
    QVERIFY(service->gcsPosition().isValid());
    health.reset();
    QVERIFY(!service->gcsPosition().isValid());
    receiver->publish(fix(scheduler).position);
    QVERIFY(service->gcsPosition().isValid());
    receiver.reset();
    QCOMPARE(service->selectedSource(), Kind::Internal);
    QVERIFY(!service->gcsPosition().isValid());
    platform.publish(fix(scheduler).position);
    QVERIFY(service->gcsPosition().isValid());
    service.reset();
    QVERIFY(!platform.active);
    registration.reset();
}

void GPSPositionServiceTest::_notificationsCanSwitchOrDelete_data()
{
    QTest::addColumn<int>("mode");
    QTest::newRow("legacy") << int(Mode::LegacyPriority);
    QTest::newRow("automatic") << int(Mode::Automatic);
}

void GPSPositionServiceTest::_notificationsCanSwitchOrDelete()
{
    QFETCH(int, mode);
    ManualScheduler scheduler;
    PositionSource receiver;
    PositionSource nmea;
    auto service = std::make_unique<GPSPositionService>(nullptr, &scheduler);
    auto receiverRegistration = service->registerPositionSource(Kind::Receiver, &receiver, nullptr);
    auto nmeaRegistration = service->registerPositionSource(Kind::Nmea, &nmea, nullptr);
    service->setSourceMode(Mode(mode));
    QObject observer;
    connect(service.get(), &GPSPositionService::gcsPositionHorizontalAccuracyChanged, &observer, [&]() {
        if (service->gcsPosition().isValid() && service->sourceMode() != Mode::NmeaOnly) {
            service->setSourceMode(Mode::NmeaOnly);
        }
    });
    receiver.publish(fix(scheduler).position);
    QCOMPARE(service->selectedSource(), Kind::Nmea);
    QVERIFY(!service->gcsPosition().isValid());
    receiver.publish(fix(scheduler).position);
    QVERIFY(!service->gcsPosition().isValid());
    nmea.publish(fix(scheduler).position);
    QVERIFY(service->gcsPosition().isValid());
    connect(service.get(), &GPSPositionService::gcsPositionChanged, &observer, [&]() { service.reset(); });
    nmeaRegistration.reset();
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
    QTest::newRow("nmea-before-fix") << false << false;
    QTest::newRow("nmea-fresh-fix") << false << true;
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
        registration = service.registerPositionSource(Kind::Nmea, &source, nullptr);
    }
    if (hasFix) {
        source.publish(fix(scheduler).position);
    }
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(2)));
    QSignalSpy positions(&service, &GPSPositionService::positionInfoUpdated);
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
    GPSPositionSourceAdapter adapter(nullptr, &scheduler);
    adapter.configure(&source, nullptr, QStringLiteral("source"), false);
    adapter.setActive(true);
    source.publish(fix(scheduler).position);
    const auto connection =
        connect(&adapter, &GPSPositionSourceAdapter::backendError, &adapter, [&]() { adapter.setActive(false); });
    if (error == QGeoPositionInfoSource::NoError) {
        source.publish(fix(scheduler).position);
    } else {
        source.fail(QGeoPositionInfoSource::Error(error));
    }
    QVERIFY(!source.active);
    QCOMPARE(adapter.health()->state(), GPSSourceHealth::State::NoData);
    QVERIFY(!adapter.health()->acceptedObservation());
    disconnect(connection);
    adapter.setActive(true);
    source.publish(fix(scheduler).position);
    QVERIFY(adapter.health()->acceptedObservation());
}

void GPSPositionServiceTest::_adapterReentrantReplacement()
{
    ManualScheduler scheduler;
    PositionSource first;
    auto next = std::make_unique<PositionSource>();
    GPSPositionSourceAdapter adapter(nullptr, &scheduler);
    adapter.configure(&first, nullptr, QStringLiteral("first"), false);
    adapter.setActive(true);
    first.onStop = [&]() { next.reset(); };
    adapter.configure(next.get(), nullptr, QStringLiteral("next"), false);
    QVERIFY(!adapter.source());
}

UT_REGISTER_TEST(GPSPositionServiceTest, TestLabel::Unit)

void GPSPositionServiceTest::_nestedObservationNotifications()
{
    ManualScheduler scheduler;
    QObject receiver;
    QObject standby;
    GPSSourceHealth health(nullptr, &scheduler);
    GPSSourceHealth standbyHealth(nullptr, &scheduler);
    GPSPositionService service(nullptr, &scheduler);
    auto registration = service.registerPositionSource(Kind::Receiver, &receiver, &health);
    GPSPositionSourceRegistration standbyRegistration;
    QSignalSpy positions(&service, &GPSPositionService::gcsPositionChanged);
    QSignalSpy headings(&service, &GPSPositionService::gcsHeadingChanged);
    bool nested = false;
    connect(&service, &GPSPositionService::gcsPositionHorizontalAccuracyChanged, &service, [&]() {
        if (!nested && service.gcsPosition().isValid()) {
            nested = true;
            standbyRegistration = service.registerPositionSource(Kind::Nmea, &standby, &standbyHealth);
            health.updateObservation(fix(scheduler));
        }
    });
    health.updateObservation(fix(scheduler));
    QCOMPARE(positions.size(), 1);
    QCOMPARE(headings.size(), 1);
    QCOMPARE(positions.first().first().value<QGeoCoordinate>(), service.gcsPosition());
}

void GPSPositionServiceTest::_rawRegistrationCarriesSession()
{
    ManualScheduler scheduler;
    PositionSource source;
    GPSPositionService service(nullptr, &scheduler);
    auto registration = service.registerPositionSource(Kind::Nmea, &source, nullptr, 7);
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
    auto registration = service.registerPositionSource(Kind::Receiver, &source, nullptr);
    auto worker = std::unique_ptr<QThread>(
        QThread::create([registration = std::move(registration)]() mutable { registration.reset(); }));
    worker->start();
    QVERIFY(worker->wait(TestTimeout::mediumMs()));
    QTRY_COMPARE_WITH_TIMEOUT(service.selectedSource(), Kind::None, TestTimeout::mediumMs());
    QVERIFY(!source.active);
}

void GPSPositionServiceTest::_foreignSchedulerRejected()
{
    QThread worker;
    ManualScheduler scheduler;
    auto* owner = QThread::currentThread();
    scheduler.moveToThread(&worker);
    worker.start();
    const auto cleanup = qScopeGuard([&]() {
        QMetaObject::invokeMethod(&scheduler, [&]() { scheduler.moveToThread(owner); }, Qt::BlockingQueuedConnection);
        worker.quit();
        worker.wait();
    });
    expectLogMessage("GPS.PositionManager.GPSPositionService", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Scheduler must share")));
    GPSPositionService service(nullptr, &scheduler);
    verifyExpectedLogMessage();
    PositionSource source;
    QVERIFY(!service.registerPositionSource(Kind::Receiver, &source, nullptr));
    QVERIFY(!source.active);
}

void GPSPositionServiceTest::_sharedHealthLoss_data()
{
    QTest::addColumn<Mode>("mode");
    QTest::addColumn<bool>("retireReceiver");
    for (const auto mode : {Mode::LegacyPriority, Mode::Automatic, Mode::ReceiverOnly, Mode::NmeaOnly}) {
        for (const bool retireReceiver : {false, true}) {
            const auto name = QByteArray::number(int(mode)) + (retireReceiver ? "-retire-receiver" : "-retire-nmea");
            QTest::newRow(name.constData()) << mode << retireReceiver;
        }
    }
}

void GPSPositionServiceTest::_sharedHealthLoss()
{
    QFETCH(Mode, mode);
    QFETCH(bool, retireReceiver);
    ManualScheduler scheduler;
    PositionSource source;
    auto health = std::make_unique<GPSSourceHealth>(nullptr, &scheduler);
    int stops = 0;
    source.onStop = [&]() { ++stops; };
    GPSPositionService service(nullptr, &scheduler);
    auto receiver = service.registerPositionSource(Kind::Receiver, &source, health.get());
    auto nmea = service.registerPositionSource(Kind::Nmea, &source, health.get());
    service.setSourceMode(mode);
    health->updateObservation(fix(scheduler));
    QVERIFY(service.gcsPosition().isValid());
    health.reset();
    QCOMPARE(stops, 0);
    QVERIFY(!source.active);
    QVERIFY(!service.gcsPosition().isValid());
    source.publish(fix(scheduler).position);
    QVERIFY(!service.acceptedObservation());
    if (retireReceiver) {
        receiver.reset();
    } else {
        nmea.reset();
    }
    service.setSourceMode(retireReceiver ? Mode::NmeaOnly : Mode::ReceiverOnly);
    QVERIFY(source.active);
    source.publish(fix(scheduler, 48).position);
    QCOMPARE(service.selectedSource(), retireReceiver ? Kind::Nmea : Kind::Receiver);
    QVERIFY(service.acceptedObservation());
    QCOMPARE(service.gcsPosition().latitude(), 48);
    receiver.reset();
    nmea.reset();
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
    GPSPositionSourceAdapter adapter(nullptr, &scheduler);
    adapter.configure(&source, nullptr, QStringLiteral("source"), false);
    adapter.setActive(true);
    source.publish(fix(scheduler).position);
    bool nested = false;
    connect(&adapter, &GPSPositionSourceAdapter::backendError, &adapter, [&]() {
        if (std::exchange(nested, true)) {
            return;
        }
        if (restart) {
            adapter.setActive(false);
            adapter.setActive(true);
        } else {
            source.publish(fix(scheduler, 48).position);
        }
    });
    if (outerError) {
        source.fail(QGeoPositionInfoSource::AccessError);
    } else {
        source.publish(fix(scheduler).position);
    }
    const auto accepted = adapter.health()->acceptedObservation();
    if (restart) {
        QVERIFY(!accepted);
        QCOMPARE(adapter.health()->state(), GPSSourceHealth::State::NoData);
    } else {
        QVERIFY(accepted);
        QCOMPARE(accepted->position.coordinate().latitude(), 48);
    }
    QVERIFY(source.active);
    source.publish(fix(scheduler, 49).position);
    QVERIFY(adapter.health()->acceptedObservation());
    QCOMPARE(adapter.health()->acceptedObservation()->position.coordinate().latitude(), 49);
}

void GPSPositionServiceTest::_accuracyNotifiesOnlyChanges()
{
    ManualScheduler scheduler;
    PositionSource source;
    GPSPositionService service(nullptr, &scheduler);
    auto registration = service.registerPositionSource(Kind::Receiver, &source, nullptr);
    QSignalSpy accuracy(&service, &GPSPositionService::gcsPositionHorizontalAccuracyChanged);
    QSignalSpy positions(&service, &GPSPositionService::positionInfoUpdated);
    auto position = fix(scheduler).position;
    source.publish(position);
    QCOMPARE(accuracy.size(), 1);
    source.publish(position);
    QCOMPARE(accuracy.size(), 1);
    QCOMPARE(positions.size(), 2);
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
    QCOMPARE(positions.size(), 5);
}
