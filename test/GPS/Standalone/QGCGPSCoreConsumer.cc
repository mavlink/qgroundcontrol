#include <QtCore/QCoreApplication>

#include <chrono>

#include "GPSSourceHealth.h"
#include "ManualScheduler.h"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    ManualScheduler scheduler;
    GPSSourceHealth health(nullptr, &scheduler);
    GPSObservation position;
    position.monotonicTimestampUs = scheduler.nowUs();
    position.position = QGeoPositionInfo(QGeoCoordinate(47, 8, 500), QDateTime::currentDateTimeUtc());
    position.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1.0);
    position.fixQuality = GPSObservation::FixQuality::Fix3D;
    health.updateObservation(position);
    if (!health.usable() || !health.acceptedObservation()) {
        return 1;
    }
    if (!scheduler.advanceBy(std::chrono::seconds(5))) {
        return 2;
    }
    if (health.state() != GPSSourceHealth::State::Stale) {
        return 3;
    }
    return health.acceptedObservation() ? 4 : 0;
}
