#include "GPSGgaSources.h"

#include <optional>

#include <QtCore/QDateTime>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPositionInfo>

#include "GPSObservation.h"
#include "GPSPositionService.h"
#include "GPSRtk.h"
#include "GPSSourceHealth.h"
#include "MonotonicClock.h"
#include "MultiVehicleManager.h"
#include "NTRIPGgaProvider.h"
#include "NTRIPManager.h"
#include "Vehicle.h"
#include "VehicleGPSFactGroup.h"
#include "VehicleLinkManager.h"

namespace {

Vehicle* activeVehicleForGga()
{
    auto* manager = MultiVehicleManager::instance();
    Vehicle* vehicle = manager ? manager->activeVehicle() : nullptr;
    if (!vehicle || vehicle->isOfflineEditingVehicle() || !vehicle->vehicleLinkManager() ||
        vehicle->vehicleLinkManager()->communicationLost()) {
        return nullptr;
    }
    return vehicle;
}

PositionResult ggaPosition(const QGeoCoordinate& coordinate, const QString& label, GPSAltitudeDatum datum)
{
    if (!coordinate.isValid() || !qIsFinite(coordinate.altitude())) {
        return {};
    }
    return {coordinate, label, datum};
}

PositionResult ggaPosition(const std::optional<GPSObservation>& observation, const QString& label)
{
    if (!observation || observation->altitudeDatum != GPSAltitudeDatum::MeanSeaLevel) {
        return {};
    }
    auto result = ggaPosition(observation->position.coordinate(), label, observation->altitudeDatum);
    result.fixQuality = observation->fixQuality;
    result.satellitesUsed = observation->satellitesUsed;
    result.horizontalDop = observation->horizontalDop;
    return result;
}

GPSObservation vehicleEstimate(const QGeoCoordinate& coordinate, const QString& sourceId)
{
    GPSObservation observation;
    observation.receivedAt = QDateTime::currentDateTimeUtc();
    observation.monotonicTimestampUs = MonotonicClock::nowUs();
    observation.position = QGeoPositionInfo(coordinate, observation.receivedAt);
    observation.altitudeDatum = GPSAltitudeDatum::MeanSeaLevel;
    observation.fixQuality = GPSObservation::FixQuality::Extrapolated;
    observation.sourceId = sourceId;
    return observation;
}

}  // namespace

GPSGgaSources::GPSGgaSources(NTRIPManager* ntrip, GPSRtk* rtk, GPSPositionService* groundStation, QObject* parent)
    : QObject(parent)
    , _ntrip(ntrip)
    , _rtk(rtk)
    , _groundStation(groundStation)
    , _vehicleEstimateHealth(new GPSSourceHealth(this))
{}

void GPSGgaSources::init()
{
    if (!_ntrip) {
        return;
    }
    using Source = NTRIPGgaProvider::PositionSource;
    _ntrip->setGgaPositionProvider(Source::VehicleGPS, []() -> PositionResult {
        Vehicle* vehicle = activeVehicleForGga();
        if (!vehicle) {
            return {};
        }
        const auto* gps = qobject_cast<VehicleGPSFactGroup*>(vehicle->gpsFactGroup());
        return gps ? ggaPosition(gps->acceptedObservation(), QStringLiteral("Vehicle GPS")) : PositionResult{};
    });
    _ntrip->setGgaPositionProvider(
        Source::VehicleEKF, [health = QPointer<GPSSourceHealth>(_vehicleEstimateHealth)]() -> PositionResult {
            // The tracked estimates are the active vehicle's; activeVehicleForGga still gates on its link.
            return health && activeVehicleForGga()
                       ? ggaPosition(health->acceptedObservation(GPSObservation::PositionUse::Gga),
                                     QStringLiteral("Vehicle EKF"))
                       : PositionResult{};
        });
    _ntrip->setGgaPositionProvider(Source::RTKReceiver, [rtk = _rtk]() -> PositionResult {
        return rtk ? ggaPosition(rtk->acceptedPositionObservation(GPSObservation::PositionUse::Gga),
                                 QStringLiteral("RTK Receiver"))
                   : PositionResult{};
    });
    _ntrip->setSortPositionProvider([]() {
        auto* manager = MultiVehicleManager::instance();
        Vehicle* vehicle = manager ? manager->activeVehicle() : nullptr;
        return vehicle ? vehicle->coordinate() : QGeoCoordinate();
    });
    _ntrip->setGgaPositionProvider(Source::GCSPosition, [groundStation = _groundStation]() -> PositionResult {
        return groundStation ? ggaPosition(groundStation->acceptedObservation(GPSObservation::PositionUse::Gga),
                                           QStringLiteral("GCS Position"))
                             : PositionResult{};
    });

    if (auto* vehicles = MultiVehicleManager::instance()) {
        (void) connect(vehicles, &MultiVehicleManager::activeVehicleChanged, this, &GPSGgaSources::setActiveVehicle);
        setActiveVehicle(vehicles->activeVehicle());
    }
}

void GPSGgaSources::setActiveVehicle(Vehicle* vehicle)
{
    QObject::disconnect(_vehiclePositionConnection);
    _vehicleEstimateHealth->reset();
    if (!vehicle) {
        return;
    }
    _vehiclePositionConnection = connect(
        vehicle, &Vehicle::positionReported, this,
        [this, sourceId = QStringLiteral("vehicle/%1/ekf").arg(vehicle->id())](const QGeoCoordinate& coordinate) {
            if (coordinate.isValid()) {
                _vehicleEstimateHealth->updateObservation(vehicleEstimate(coordinate, sourceId));
            } else {
                _vehicleEstimateHealth->invalidatePosition();
            }
        });
}
