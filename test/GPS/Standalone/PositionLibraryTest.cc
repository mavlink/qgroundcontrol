#include <QtTest/QTest>

#include <utility>

#include "GPSPositionService.h"
#include "GPSPositionSourceSelector.h"
#include "ManualScheduler.h"

namespace {
class PositionSource final : public QGeoPositionInfoSource
{
public:
    PositionSource()
        : QGeoPositionInfoSource(nullptr)
    {}

    QGeoPositionInfo lastKnownPosition(bool = false) const override { return {}; }

    PositioningMethods supportedPositioningMethods() const override { return SatellitePositioningMethods; }

    int minimumUpdateInterval() const override { return 0; }

    Error error() const override { return NoError; }

    void startUpdates() override { active = true; }

    void stopUpdates() override { active = false; }

    void requestUpdate(int = 0) override {}

    bool active = false;
};

GPSObservation position(RuntimeScheduler& scheduler, double latitude = 47)
{
    GPSObservation result;
    result.position = QGeoPositionInfo(QGeoCoordinate(latitude, 8, 500), QDateTime::currentDateTimeUtc());
    result.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1);
    result.receivedAt = QDateTime::currentDateTimeUtc();
    result.monotonicTimestampUs = scheduler.nowUs();
    return result;
}
}  // namespace

class PositionLibraryTest : public QObject
{
    Q_OBJECT
private slots:

    void permissionAndBackendFailure()
    {
        GPSPositionService service;
        service.setSourceMode(GPSPositionService::SourceMode::InternalOnly);
        service.setInternalPositionSource(nullptr, GPSPositionService::SourceStatus::PermissionRequired);
        QCOMPARE(service.sourceStatus(), GPSPositionService::SourceStatus::PermissionRequired);
        service.setInternalPositionSource(nullptr, GPSPositionService::SourceStatus::PermissionDenied);
        QCOMPARE(service.sourceStatus(), GPSPositionService::SourceStatus::PermissionDenied);
        service.setInternalPositionSource(nullptr, GPSPositionService::SourceStatus::BackendUnavailable);
        QCOMPARE(service.sourceStatus(), GPSPositionService::SourceStatus::BackendUnavailable);
        QVERIFY(!service.gcsPosition().isValid());
    }

    void arbitrationAndRecovery()
    {
        GPSPositionSourceSelector selector;
        std::array<GPSPositionSourceSelector::Candidate, 3> sources{
            {{1, true, false}, {2, true, true}, {3, true, true}}};
        QCOMPARE(selector.select(sources, 1, 1000, 2000), 2);
        sources[0].healthy = true;
        QCOMPARE(selector.select(sources, 2, 1000, 2000), 2);
        QCOMPARE(selector.select(sources, 2, 2999, 2000), 2);
        QCOMPARE(selector.select(sources, 2, 3000, 2000), 1);
    }

    void platformAndSimulationReplacement()
    {
        ManualScheduler scheduler;
        PositionSource platform;
        PositionSource replacement;
        PositionSource simulated;
        GPSPositionService service(nullptr, &scheduler);
        service.setSimulatedPositionSource(&simulated);
        QCOMPARE(service.selectedSource(), GPSPositionService::SelectedSource::Simulated);
        QVERIFY(simulated.active);
        service.setInternalPositionSource(&platform, GPSPositionService::SourceStatus::WaitingForFix);
        QCOMPARE(service.selectedSource(), GPSPositionService::SelectedSource::Internal);
        QVERIFY(platform.active);
        QVERIFY(!simulated.active);
        emit platform.positionUpdated(position(scheduler).position);
        QVERIFY(service.gcsPosition().isValid());
        service.setInternalPositionSource(&replacement, GPSPositionService::SourceStatus::WaitingForFix, true);
        QVERIFY(!platform.active);
        QVERIFY(replacement.active);
        QVERIFY(!service.gcsPosition().isValid());
        QCOMPARE(service.selectedSourceName(), QStringLiteral("Plugin positioning"));
        emit platform.positionUpdated(position(scheduler).position);
        QVERIFY(!service.gcsPosition().isValid());
        emit replacement.positionUpdated(position(scheduler).position);
        QVERIFY(service.gcsPosition().isValid());
        service.setInternalPositionSource(nullptr, GPSPositionService::SourceStatus::BackendUnavailable);
        QCOMPARE(service.selectedSource(), GPSPositionService::SelectedSource::Simulated);
        QVERIFY(simulated.active);
        service.setSimulatedPositionSource(nullptr);
        QVERIFY(!simulated.active);
        QCOMPARE(service.selectedSource(), GPSPositionService::SelectedSource::None);
    }

    void completeServiceRecovery()
    {
        ManualScheduler scheduler;
        QObject preferred;
        QObject fallback;
        GPSSourceHealth preferredHealth(nullptr, &scheduler);
        GPSSourceHealth fallbackHealth(nullptr, &scheduler);
        preferredHealth.setFreshnessTimeoutMs(1000);
        fallbackHealth.setFreshnessTimeoutMs(60000);
        GPSPositionService service(nullptr, &scheduler);
        auto first =
            service.registerPositionSource(GPSPositionService::SelectedSource::Receiver, &preferred, &preferredHealth);
        auto second =
            service.registerPositionSource(GPSPositionService::SelectedSource::Nmea, &fallback, &fallbackHealth);
        service.setSourceMode(GPSPositionService::SourceMode::Automatic);
        preferredHealth.updateObservation(position(scheduler));
        fallbackHealth.updateObservation(position(scheduler, 48));
        QCOMPARE(service.selectedSource(), GPSPositionService::SelectedSource::Receiver);
        QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
        QCOMPARE(service.selectedSource(), GPSPositionService::SelectedSource::Nmea);
        QCOMPARE(service.gcsPosition().latitude(), 48);
        preferredHealth.setFreshnessTimeoutMs(60000);
        preferredHealth.updateObservation(position(scheduler));
        QVERIFY(scheduler.advanceBy(std::chrono::seconds(2)));
        preferredHealth.invalidatePosition();
        QVERIFY(scheduler.advanceBy(std::chrono::seconds(4)));
        QCOMPARE(service.selectedSource(), GPSPositionService::SelectedSource::Nmea);
        preferredHealth.updateObservation(position(scheduler));
        QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(4999)));
        QCOMPARE(service.selectedSource(), GPSPositionService::SelectedSource::Nmea);
        QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
        QCOMPARE(service.selectedSource(), GPSPositionService::SelectedSource::Receiver);
        QCOMPARE(service.gcsPosition().latitude(), 47);
    }

    void nestedRegistrationRetainsNewestSource()
    {
        GPSPositionService service;
        QObject first;
        QObject second;
        GPSSourceHealth firstHealth;
        GPSSourceHealth secondHealth;
        GPSPositionSourceRegistration replacement;
        bool replaced = false;
        connect(&service, &GPSPositionService::sourceHealthChanged, &service, [&]() {
            if (!std::exchange(replaced, true)) {
                replacement = service.registerPositionSource(GPSPositionService::SelectedSource::Receiver, &second,
                                                             &secondHealth);
            }
        });
        auto retired =
            service.registerPositionSource(GPSPositionService::SelectedSource::Receiver, &first, &firstHealth);
        retired.reset();
        QCOMPARE(service.sourceHealth(), &secondHealth);
        replacement.reset();
        QCOMPARE(service.selectedSource(), GPSPositionService::SelectedSource::None);
    }

    void pendingRecoveryLifetime_data()
    {
        QTest::addColumn<QString>("destroy");
        QTest::newRow("source") << QStringLiteral("source");
        QTest::newRow("service") << QStringLiteral("service");
        QTest::newRow("scheduler") << QStringLiteral("scheduler");
        QTest::newRow("service-during-recovery") << QStringLiteral("callback");
    }

    void pendingRecoveryLifetime()
    {
        QFETCH(QString, destroy);
        auto scheduler = std::make_unique<ManualScheduler>();
        auto preferred = std::make_unique<QObject>();
        QObject fallback;
        GPSSourceHealth preferredHealth(nullptr, scheduler.get());
        GPSSourceHealth fallbackHealth(nullptr, scheduler.get());
        preferredHealth.setFreshnessTimeoutMs(60000);
        fallbackHealth.setFreshnessTimeoutMs(60000);
        auto service = std::make_unique<GPSPositionService>(nullptr, scheduler.get());
        auto first = service->registerPositionSource(GPSPositionService::SelectedSource::Receiver, preferred.get(),
                                                     &preferredHealth);
        auto second =
            service->registerPositionSource(GPSPositionService::SelectedSource::Nmea, &fallback, &fallbackHealth);
        service->setSourceMode(GPSPositionService::SourceMode::Automatic);
        fallbackHealth.updateObservation(position(*scheduler, 48));
        preferredHealth.updateObservation(position(*scheduler));
        QCOMPARE(service->selectedSource(), GPSPositionService::SelectedSource::Nmea);
        if (destroy == QStringLiteral("source")) {
            preferred.reset();
            QVERIFY(scheduler->advanceBy(std::chrono::seconds(5)));
            QCOMPARE(service->selectedSource(), GPSPositionService::SelectedSource::Nmea);
        } else if (destroy == QStringLiteral("scheduler")) {
            scheduler.reset();
            QVERIFY(!service->acceptedObservation(GPSObservation::PositionUse::GroundStation));
        } else if (destroy == QStringLiteral("service")) {
            service.reset();
            QVERIFY(scheduler->advanceBy(std::chrono::seconds(5)));
        } else {
            connect(service.get(), &GPSPositionService::selectionChanged, &fallback, [&]() {
                if (service && service->selectedSource() == GPSPositionService::SelectedSource::Receiver) {
                    service.reset();
                }
            });
            QVERIFY(scheduler->advanceBy(std::chrono::seconds(5)));
            QVERIFY(!service);
        }
        first.reset();
        second.reset();
    }

    void supersededRegistrationCannotRetireReplacement()
    {
        GPSPositionService service;
        QObject first;
        QObject second;
        GPSSourceHealth firstHealth;
        GPSSourceHealth secondHealth;
        service.setSourceMode(GPSPositionService::SourceMode::ReceiverOnly);
        auto old =
            service.registerPositionSource(GPSPositionService::SelectedSource::Receiver, &first, &firstHealth, 1);
        auto replacement =
            service.registerPositionSource(GPSPositionService::SelectedSource::Receiver, &second, &secondHealth, 2);
        old.reset();
        QCOMPARE(service.selectedSource(), GPSPositionService::SelectedSource::Receiver);
        QCOMPARE(service.sourceHealth(), &secondHealth);
        replacement.reset();
        QCOMPARE(service.selectedSource(), GPSPositionService::SelectedSource::None);
    }
};
QTEST_GUILESS_MAIN(PositionLibraryTest)
#include "PositionLibraryTest.moc"
