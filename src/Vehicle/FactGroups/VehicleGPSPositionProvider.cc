#include "VehicleGPSPositionProvider.h"

#include "QGCLoggingCategory.h"
#include "Vehicle.h"
#include "VehicleGPSObservation.h"
#include "VehicleLinkManager.h"

QGC_LOGGING_CATEGORY(VehicleGPSPositionProviderLog, "GPS.VehicleGPSPositionProvider")

namespace {
bool sane(const GPSObservation& observation)
{
    const auto coordinate = observation.position.coordinate();
    return coordinate.isValid() && (coordinate.latitude() != 0 || coordinate.longitude() != 0);
}

GPSObservation position(double latitude, double longitude, double altitude, GPSObservation::FixQuality quality)
{
    GPSObservation result;
    result.receivedAt = QDateTime::currentDateTimeUtc();
    result.monotonicTimestampUs = GPSObservation::monotonicNowUs();
    result.position = QGeoPositionInfo(QGeoCoordinate(latitude, longitude, altitude), result.receivedAt);
    result.altitudeDatum = GPSObservation::AltitudeDatum::MeanSeaLevel;
    result.fixQuality = quality;
    return result;
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
    if (_vehicle == vehicle) {
        return;
    }
    disconnect(_messageConnection);
    _vehicle = vehicle;
    _gpsPosition = {};
    _ekfPosition = {};
    if (!vehicle) {
        return;
    }
    _messageConnection =
        connect(vehicle, &Vehicle::mavlinkMessageReceived, this, [this](const mavlink_message_t& message) {
            if (!_vehicle || message.sysid != _vehicle->id()) {
                return;
            }
            if (message.msgid == MAVLINK_MSG_ID_GPS_RAW_INT) {
                mavlink_gps_raw_int_t fix = {};
                mavlink_msg_gps_raw_int_decode(&message, &fix);
                _gpsPosition = VehicleGPSObservation::fromMessage(fix).position;
            } else if (message.msgid == MAVLINK_MSG_ID_GLOBAL_POSITION_INT &&
                       message.compid == _vehicle->defaultComponentId()) {
                mavlink_global_position_int_t fix = {};
                mavlink_msg_global_position_int_decode(&message, &fix);
                if (fix.lat != 0 || fix.lon != 0) {
                    _ekfPosition = position(fix.lat * 1e-7, fix.lon * 1e-7, fix.alt / 1000.0,
                                            GPSObservation::FixQuality::Extrapolated);
                }
            } else if (message.msgid == MAVLINK_MSG_ID_HIGH_LATENCY) {
                mavlink_high_latency_t fix = {};
                mavlink_msg_high_latency_decode(&message, &fix);
                const auto observation = VehicleGPSObservation::fromMessage(fix);
                _gpsPosition = observation.position;
                _ekfPosition = observation.fusedPosition;
            } else if (message.msgid == MAVLINK_MSG_ID_HIGH_LATENCY2) {
                mavlink_high_latency2_t fix = {};
                mavlink_msg_high_latency2_decode(&message, &fix);
                const auto observation = VehicleGPSObservation::fromMessage(fix);
                _gpsPosition = observation.position;
                _ekfPosition = observation.fusedPosition;
            }
        });
}

GPSObservation VehicleGPSPositionProvider::gpsPosition() const
{
    return _vehicle && !_vehicle->vehicleLinkManager()->communicationLost() && sane(_gpsPosition) ? _gpsPosition
                                                                                                  : GPSObservation();
}

GPSObservation VehicleGPSPositionProvider::ekfPosition() const
{
    return _vehicle && !_vehicle->vehicleLinkManager()->communicationLost() && sane(_ekfPosition) ? _ekfPosition
                                                                                                  : GPSObservation();
}
