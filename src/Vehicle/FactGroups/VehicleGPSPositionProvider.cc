#include "VehicleGPSPositionProvider.h"

#include "QGCLoggingCategory.h"
#include "Vehicle.h"
#include "VehicleGPSObservationStream.h"
#include "VehicleLinkManager.h"

QGC_LOGGING_CATEGORY(VehicleGPSPositionProviderLog, "GPS.VehicleGPSPositionProvider")

namespace {
bool sane(const GPSObservation& observation)
{
    const auto coordinate = observation.position.coordinate();
    return coordinate.isValid() && (coordinate.latitude() != 0 || coordinate.longitude() != 0);
}

}  // namespace

VehicleGPSPositionProvider::VehicleGPSPositionProvider(QObject* parent)
    : QObject(parent)
{
    qCDebug(VehicleGPSPositionProviderLog) << this;
}

VehicleGPSPositionProvider::~VehicleGPSPositionProvider()
{
    qCDebug(VehicleGPSPositionProviderLog) << this;
}

void VehicleGPSPositionProvider::setVehicle(Vehicle* vehicle)
{
    _vehicle = vehicle;
}

GPSObservation VehicleGPSPositionProvider::gpsPosition() const
{
    if (!_vehicle || _vehicle->vehicleLinkManager()->communicationLost()) {
        return {};
    }
    const auto observation = _vehicle->gpsObservationStream()->gps().position;
    return sane(observation) ? observation : GPSObservation();
}

GPSObservation VehicleGPSPositionProvider::ekfPosition() const
{
    if (!_vehicle || _vehicle->vehicleLinkManager()->communicationLost()) {
        return {};
    }
    const auto observation = _vehicle->gpsObservationStream()->fusedPosition();
    return sane(observation) ? observation : GPSObservation();
}
