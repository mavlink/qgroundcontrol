#include "GPSPositionServiceTest.h"

#include <QtCore/QScopeGuard>
#include <QtCore/QThread>
#include <QtTest/QSignalSpy>

#include <functional>
#include <memory>

#include "GPSPositionService.h"
#include "ManualScheduler.h"

namespace {
using Kind = GPSPositionService::SelectedSource;
using Mode = GPSPositionService::SourceMode;
using Status = GPSPositionService::SourceStatus;

class PositionSource : public QGeoPositionInfoSource
{
public:
    PositionSource() : QGeoPositionInfoSource(nullptr) {}

    QGeoPositionInfo lastKnownPosition(bool = false) const override { return {}; }

    PositioningMethods supportedPositioningMethods() const override { return SatellitePositioningMethods; }

    int minimumUpdateInterval() const override { return 100; }

    Error error() const override { return NoError; }

    void startUpdates() override { active = true; }

    void stopUpdates() override
    {
        active = false;
        if (onStop) {
            const auto callback = onStop;
            callback();
        }
    }

    void requestUpdate(int = 0) override {}

    void publish(const QGeoPositionInfo& position) { emit positionUpdated(position); }

    void fail(Error error) { emit errorOccurred(error); }

    bool active = false;
    std::function<void()> onStop;
};

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
    QCOMPARE(service.acceptedObservation(GPSObservation::PositionUse::GroundStation)->sessionId, quint64(12));
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

void GPSPositionServiceTest::_notificationsCanSwitchOrDelete()
{
    ManualScheduler scheduler;
    PositionSource receiver;
    PositionSource nmea;
    auto service = std::make_unique<GPSPositionService>(nullptr, &scheduler);
    auto receiverRegistration = service->registerPositionSource(Kind::Receiver, &receiver, nullptr);
    auto nmeaRegistration = service->registerPositionSource(Kind::Nmea, &nmea, nullptr);
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
    const auto observation = service.acceptedObservation(GPSObservation::PositionUse::GroundStation);
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
