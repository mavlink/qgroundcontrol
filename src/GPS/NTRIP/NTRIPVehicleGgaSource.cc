#include "NTRIPVehicleGgaSource.h"

#include <QtCore/QDateTime>
#include <QtCore/QPointer>

#include "GPSProtocolMath.h"
#include "GPSSourceHealth.h"
#include "MAVLinkLib.h"
#include "MonotonicClock.h"
#include "MultiVehicleManager.h"
#include "NTRIPGgaReporter.h"
#include "NTRIPManager.h"
#include "RuntimeScheduler.h"
#include "Vehicle.h"
#include "VehicleGPSFactGroup.h"
#include "VehicleLinkManager.h"

namespace {

bool activeVehicleConnected()
{
    auto* manager = MultiVehicleManager::instance();
    Vehicle* const vehicle = manager ? manager->activeVehicle() : nullptr;
    return vehicle && !vehicle->isOfflineEditingVehicle() && vehicle->vehicleLinkManager() &&
           !vehicle->vehicleLinkManager()->communicationLost();
}

GPSObservation::FixQuality fixQuality(int fixType)
{
    using Quality = GPSObservation::FixQuality;
    switch (fixType) {
        case GPS_FIX_TYPE_2D_FIX:
            return Quality::Fix2D;
        case GPS_FIX_TYPE_3D_FIX:
        case GPS_FIX_TYPE_STATIC:
            return Quality::Fix3D;
        case GPS_FIX_TYPE_DGPS:
        case GPS_FIX_TYPE_PPP:
            return Quality::Differential;
        case GPS_FIX_TYPE_RTK_FLOAT:
            return Quality::RTKFloat;
        case GPS_FIX_TYPE_RTK_FIXED:
            return Quality::RTKFixed;
        default:
            return Quality::NoFix;
    }
}

GPSObservation vehicleObservation(const QGeoPositionInfo& position, GPSObservation::FixQuality quality,
                                  quint64 receivedAtUs)
{
    GPSObservation observation;
    observation.position = position;
    observation.monotonicTimestampUs = receivedAtUs;
    observation.altitudeDatum = GPSAltitudeDatum::MeanSeaLevel;
    observation.fixQuality = quality;
    return observation;
}

}  // namespace

NTRIPVehicleGgaSource::NTRIPVehicleGgaSource(RuntimeScheduler* scheduler, QObject* parent)
    : QObject(parent)
    , _scheduler(scheduler)
{}

void NTRIPVehicleGgaSource::attach(NTRIPManager* ntrip)
{
    using Source = NTRIPGgaReporter::PositionSource;
    using Observation = std::optional<GPSObservation>;
    // The tracked reports are the active vehicle's; the gate still requires its link.
    const auto gated = [source = QPointer(this)](Observation (NTRIPVehicleGgaSource::*observe)() const) {
        return [source, observe]() -> Observation {
            if (!source || !activeVehicleConnected()) {
                return std::nullopt;
            }
            return (source->*observe)();
        };
    };
    ntrip->setGgaPositionProvider(Source::VehicleGPS, gated(&NTRIPVehicleGgaSource::gpsObservation));
    ntrip->setGgaPositionProvider(Source::VehicleEKF, gated(&NTRIPVehicleGgaSource::estimateObservation));

    if (auto* vehicles = MultiVehicleManager::instance()) {
        (void) connect(vehicles, &MultiVehicleManager::activeVehicleChanged, this, &NTRIPVehicleGgaSource::setVehicle);
        setVehicle(vehicles->activeVehicle());
    }
}

void NTRIPVehicleGgaSource::setVehicle(Vehicle* vehicle)
{
    QObject::disconnect(_gpsConnection);
    QObject::disconnect(_messageConnection);
    _gps.reset();
    _estimate.reset();
    if (!vehicle) {
        return;
    }
    if (auto* gps = vehicle->gpsFactGroup()) {
        _gpsConnection = connect(gps, &VehicleGPSFactGroup::positionReported, this,
                                 [this, gps](const QGeoPositionInfo& position, int fixType) {
                                     _gps = GpsReport{.position = position,
                                                      .fixType = fixType,
                                                      .horizontalDop = gps->hdop()->rawValue().toDouble(),
                                                      .receivedAtUs = _scheduler->nowUs()};
                                 });
    }
    _messageConnection =
        connect(vehicle, &Vehicle::mavlinkMessageReceived, this, [this](const mavlink_message_t& message) {
            if (message.msgid != MAVLINK_MSG_ID_GLOBAL_POSITION_INT) {
                return;
            }
            mavlink_global_position_int_t position;
            mavlink_msg_global_position_int_decode(&message, &position);
            // ArduPilot reports 0/0 without a position, to carry the relative altitude.
            if (position.lat == 0 && position.lon == 0) {
                _estimate.reset();
                return;
            }
            _estimate =
                EstimateReport{.coordinate = QGeoCoordinate(GPSProtocolMath::degreesFromE7(position.lat),
                                                            GPSProtocolMath::degreesFromE7(position.lon),
                                                            GPSProtocolMath::metersFromMillimeters(position.alt)),
                               .receivedAtUs = _scheduler->nowUs()};
        });
}

bool NTRIPVehicleGgaSource::_fresh(quint64 receivedAtUs) const
{
    return MonotonicClock::fresh(receivedAtUs, _scheduler->nowUs(), GPSSourceHealth::FRESHNESS_TIMEOUT);
}

std::optional<GPSObservation> NTRIPVehicleGgaSource::gpsObservation() const
{
    if (!_gps || !_fresh(_gps->receivedAtUs)) {
        return std::nullopt;
    }
    GPSObservation observation = vehicleObservation(_gps->position, fixQuality(_gps->fixType), _gps->receivedAtUs);
    observation.receiverFixValid = observation.fixQuality != GPSObservation::FixQuality::NoFix;
    if (qIsFinite(_gps->horizontalDop)) {
        observation.horizontalDop = _gps->horizontalDop;
    }
    return observation.projected(GPSObservation::PositionUse::Gga);
}

std::optional<GPSObservation> NTRIPVehicleGgaSource::estimateObservation() const
{
    if (!_estimate || !_fresh(_estimate->receivedAtUs)) {
        return std::nullopt;
    }
    // GLOBAL_POSITION_INT carries no fix time, so the observation is stamped when it is built.
    return vehicleObservation(QGeoPositionInfo(_estimate->coordinate, QDateTime::currentDateTimeUtc()),
                              GPSObservation::FixQuality::Extrapolated, _estimate->receivedAtUs)
        .projected(GPSObservation::PositionUse::Gga);
}
