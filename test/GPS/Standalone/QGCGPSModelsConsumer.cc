#include <QtCore/QCoreApplication>

#include <chrono>

#include "GPSRelativePositionModel.h"
#include "GPSSatelliteModel.h"
#include "ManualScheduler.h"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    ManualScheduler scheduler;
    GPSRelativePositionStore store(nullptr, 5000, &scheduler);
    GPSRelativePositionModel model(store);
    GPSSatelliteModel satellites;
    store.beginSession(QStringLiteral("receiver"), 1);
    GPSRelativeObservation observation;
    observation.sessionId = 1;
    observation.monotonicTimestampUs = scheduler.nowUs();
    observation.positionValid = true;
    observation.fixValid = true;
    observation.headingDegrees = 90;
    observation.positionNedMeters = {1, 2, 3};
    store.updateObservation(observation);
    if (!model.fresh() || model.north() != 1 || model.heading() != 90 || satellites.rowCount() != 0) {
        return 1;
    }
    if (!scheduler.advanceBy(std::chrono::seconds(5))) {
        return 2;
    }
    return model.fresh() || !qIsNaN(model.north()) ? 3 : 0;
}
