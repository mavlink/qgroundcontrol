#include <QtTest/QTest>

#include "GPSPositionService.h"
#include "GPSPositionSourceSelector.h"

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
