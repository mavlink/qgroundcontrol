#include <QtCore/QCoreApplication>

#include "GPSPositionService.h"
#include "ManualScheduler.h"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    ManualScheduler scheduler;
    GPSPositionService service(nullptr, &scheduler);
    QObject producer;
    GPSSourceHealth health(nullptr, &scheduler);
    auto registration =
        service.registerPositionSource(GPSPositionService::SelectedSource::Receiver, &producer, &health, 7);
    GPSObservation fix;
    fix.sessionId = 7;
    fix.monotonicTimestampUs = scheduler.nowUs();
    fix.receivedAt = QDateTime::currentDateTimeUtc();
    fix.position = QGeoPositionInfo(QGeoCoordinate(47, 8, 500), fix.receivedAt);
    fix.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1);
    fix.position.setAttribute(QGeoPositionInfo::VerticalAccuracy, 1);
    health.updateObservation(fix);
    if (!registration || service.gcsPosition() != fix.position.coordinate()) {
        return 1;
    }
    if (!scheduler.advanceBy(std::chrono::seconds(5)) || service.gcsPosition().isValid()) {
        return 2;
    }
    registration.reset();
    return service.selectedSource() == GPSPositionService::SelectedSource::None ? 0 : 3;
}
