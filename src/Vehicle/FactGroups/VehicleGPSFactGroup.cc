#include "VehicleGPSFactGroup.h"

#include <QtCore/QPointer>
#include <QtPositioning/QGeoCoordinate>

#include "QGCGeo.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"
#include "VehicleGPSObservation.h"
#include "development/mavlink_msg_gnss_integrity.h"

VehicleGPSFactGroup::VehicleGPSFactGroup(QObject* parent)
    : GPSPositionFactGroup(parent)
{
    _addFact(systemErrors());
    _addFact(spoofingState());
    _addFact(jammingState());
    _addFact(authenticationState());
    _addFact(correctionsQuality());
    _addFact(systemQuality());
    _addFact(gnssSignalQuality());
    _addFact(postProcessingQuality());
}

void VehicleGPSFactGroup::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
{
    Q_UNUSED(vehicle);

    switch (message.msgid) {
    case MAVLINK_MSG_ID_GPS_RAW_INT:
        _handleGpsRawInt(message);
        break;
    case MAVLINK_MSG_ID_HIGH_LATENCY:
        _handleHighLatency(message);
        break;
    case MAVLINK_MSG_ID_HIGH_LATENCY2:
        _handleHighLatency2(message);
        break;
    case MAVLINK_MSG_ID_GNSS_INTEGRITY:
        _handleGnssIntegrity(message);
        break;
    default:
        break;
    }
}

void VehicleGPSFactGroup::_handleGpsRawInt(const mavlink_message_t &message)
{
    mavlink_gps_raw_int_t gpsRawInt{};
    mavlink_msg_gps_raw_int_decode(&message, &gpsRawInt);

    const auto observation = VehicleGPSObservation::fromMessage(gpsRawInt);
    updatePosition(observation.position, observation.satellitesVisible, observation.fixType);
}

void VehicleGPSFactGroup::_handleHighLatency(const mavlink_message_t &message)
{
    mavlink_high_latency_t highLatency{};
    mavlink_msg_high_latency_decode(&message, &highLatency);

    lat()->setRawValue(highLatency.latitude * 1e-7);
    lon()->setRawValue(highLatency.longitude * 1e-7);
    mgrs()->setRawValue(QGCGeo::convertGeoToMGRS(QGeoCoordinate(highLatency.latitude * 1e-7, highLatency.longitude * 1e-7, highLatency.altitude_amsl)));
    count()->setRawValue(0);

    _setTelemetryAvailable(true);
}

void VehicleGPSFactGroup::_handleHighLatency2(const mavlink_message_t &message)
{
    mavlink_high_latency2_t highLatency2{};
    mavlink_msg_high_latency2_decode(&message, &highLatency2);

    lat()->setRawValue(highLatency2.latitude * 1e-7);
    lon()->setRawValue(highLatency2.longitude * 1e-7);
    mgrs()->setRawValue(QGCGeo::convertGeoToMGRS(QGeoCoordinate(highLatency2.latitude * 1e-7, highLatency2.longitude * 1e-7, highLatency2.altitude)));
    count()->setRawValue(0);
    hdop()->setRawValue((highLatency2.eph == UINT8_MAX) ? qQNaN() : (highLatency2.eph / 10.0));
    vdop()->setRawValue((highLatency2.epv == UINT8_MAX) ? qQNaN() : (highLatency2.epv / 10.0));

    _setTelemetryAvailable(true);
}

void VehicleGPSFactGroup::_handleGnssIntegrity(const mavlink_message_t& message)
{
    mavlink_gnss_integrity_t gnssIntegrity;
    mavlink_msg_gnss_integrity_decode(&message, &gnssIntegrity);

    if (gnssIntegrity.id != _gnssIntegrityId) {
        return;
    }

    GPSIntegrityObservation observation;
    observation.monotonicTimestampUs = GPSObservation::monotonicNowUs();
    observation.systemErrors = gnssIntegrity.system_errors;
    if (gnssIntegrity.spoofing_state != UINT8_MAX) {
        observation.spoofingState = gnssIntegrity.spoofing_state;
    }
    if (gnssIntegrity.jamming_state != UINT8_MAX) {
        observation.jammingState = gnssIntegrity.jamming_state;
    }
    if (gnssIntegrity.authentication_state != UINT8_MAX) {
        observation.authenticationState = gnssIntegrity.authentication_state;
    }
    if (gnssIntegrity.corrections_quality != UINT8_MAX) {
        observation.correctionsQuality = gnssIntegrity.corrections_quality;
    }
    if (gnssIntegrity.system_status_summary != UINT8_MAX) {
        observation.systemQuality = gnssIntegrity.system_status_summary;
    }
    if (gnssIntegrity.gnss_signal_quality != UINT8_MAX) {
        observation.gnssSignalQuality = gnssIntegrity.gnss_signal_quality;
    }
    if (gnssIntegrity.post_processing_quality != UINT8_MAX) {
        observation.postProcessingQuality = gnssIntegrity.post_processing_quality;
    }
    const QPointer<VehicleGPSFactGroup> guard(this);
    integrity()->update(observation);
    if (!guard) {
        return;
    }

    emit gnssIntegrityReceived();
}
