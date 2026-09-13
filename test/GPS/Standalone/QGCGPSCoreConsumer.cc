#include <QtCore/QCoreApplication>

#include <chrono>
#include <limits>

#include "GPSIntegrityStore.h"
#include "GPSSourceHealth.h"
#include "ManualScheduler.h"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    ManualScheduler scheduler;
    GPSSourceHealth health(nullptr, &scheduler);
    GPSIntegrityStore integrity(nullptr, &scheduler);
    integrity.beginSession(7);
    GPSObservation position;
    if (position.ageMilliseconds() != -1 || GPSSourceHealth::ageMilliseconds(0) != -1 ||
        GPSObservation::ageMilliseconds(std::numeric_limits<quint64>::max()) != -1) {
        return 5;
    }
    position.monotonicTimestampUs = scheduler.nowUs();
    position.position = QGeoPositionInfo(QGeoCoordinate(47, 8, 500), QDateTime::currentDateTimeUtc());
    position.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1.0);
    position.fixQuality = GPSObservation::FixQuality::Fix3D;
    health.updateObservation(position);
    GPSIntegrityObservation diagnostic;
    diagnostic.monotonicTimestampUs = scheduler.nowUs();
    diagnostic.sessionId = 7;
    diagnostic.correctionsCrcFailed = false;
    integrity.updateObservation(diagnostic);
    if (!health.usable() || !integrity.available()) {
        return 1;
    }
    if (!scheduler.advanceBy(std::chrono::seconds(5))) {
        return 2;
    }
    if (health.state() != GPSSourceHealth::Stale || integrity.available()) {
        return 3;
    }
    integrity.beginSession(8);
    diagnostic.monotonicTimestampUs = scheduler.nowUs();
    integrity.updateObservation(diagnostic);
    return integrity.available() ? 4 : 0;
}
