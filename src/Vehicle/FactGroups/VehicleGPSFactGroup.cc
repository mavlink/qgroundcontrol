#include "VehicleGPSFactGroup.h"
#include "Vehicle.h"
#include "QGCGeo.h"
#include "QGCLoggingCategory.h"
#include "development/mavlink_msg_gnss_integrity.h"

#include <QtPositioning/QGeoCoordinate>

VehicleGPSFactGroup::VehicleGPSFactGroup(QObject *parent)
    : FactGroup(1000, ":/json/Vehicle/GPSFact.json", parent)
{
    _addFact(&_latFact);
    _addFact(&_lonFact);
    _addFact(&_mgrsFact);
    _addFact(&_altitudeMSLFact);
    _addFact(&_altitudeEllipsoidFact);
    _addFact(&_hdopFact);
    _addFact(&_vdopFact);
    _addFact(&_hAccFact);
    _addFact(&_vAccFact);
    _addFact(&_groundSpeedFact);
    _addFact(&_velAccFact);
    _addFact(&_hdgAccFact);
    _addFact(&_courseOverGroundFact);
    _addFact(&_yawFact);
    _addFact(&_lockFact);
    _addFact(&_countFact);
    _addFact(&_systemErrorsFact);
    _addFact(&_spoofingStateFact);
    _addFact(&_jammingStateFact);
    _addFact(&_authenticationStateFact);
    _addFact(&_correctionsQualityFact);
    _addFact(&_systemQualityFact);
    _addFact(&_gnssSignalQualityFact);
    _addFact(&_postProcessingQualityFact);

    _latFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _lonFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _mgrsFact.setRawValue("");
    _altitudeMSLFact.setRawValue(std::numeric_limits<double>::quiet_NaN());
    _altitudeEllipsoidFact.setRawValue(std::numeric_limits<double>::quiet_NaN());
    _hdopFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _vdopFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _hAccFact.setRawValue(std::numeric_limits<double>::quiet_NaN());
    _vAccFact.setRawValue(std::numeric_limits<double>::quiet_NaN());
    _groundSpeedFact.setRawValue(std::numeric_limits<double>::quiet_NaN());
    _velAccFact.setRawValue(std::numeric_limits<double>::quiet_NaN());
    _hdgAccFact.setRawValue(std::numeric_limits<double>::quiet_NaN());
    _courseOverGroundFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _yawFact.setRawValue(std::numeric_limits<double>::quiet_NaN());
    _spoofingStateFact.setRawValue(255);
    _jammingStateFact.setRawValue(255);
    _authenticationStateFact.setRawValue(255);
    _correctionsQualityFact.setRawValue(255);
    _systemQualityFact.setRawValue(255);
    _gnssSignalQualityFact.setRawValue(255);
    _postProcessingQualityFact.setRawValue(255);
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

    lat()->setRawValue(gpsRawInt.lat * 1e-7);
    lon()->setRawValue(gpsRawInt.lon * 1e-7);
    mgrs()->setRawValue(QGCGeo::convertGeoToMGRS(QGeoCoordinate(gpsRawInt.lat * 1e-7, gpsRawInt.lon * 1e-7)));
    altitudeMSL()->setRawValue(gpsRawInt.alt / 1000.0);
    altitudeEllipsoid()->setRawValue(gpsRawInt.alt_ellipsoid / 1000.0);
    count()->setRawValue((gpsRawInt.satellites_visible == 255) ? 0 : gpsRawInt.satellites_visible);
    hdop()->setRawValue((gpsRawInt.eph == UINT16_MAX) ? qQNaN() : (gpsRawInt.eph / 100.0));
    vdop()->setRawValue((gpsRawInt.epv == UINT16_MAX) ? qQNaN() : (gpsRawInt.epv / 100.0));
    hAcc()->setRawValue((gpsRawInt.h_acc == 0) ? qQNaN() : (gpsRawInt.h_acc / 1000.0));
    vAcc()->setRawValue((gpsRawInt.v_acc == 0) ? qQNaN() : (gpsRawInt.v_acc / 1000.0));
    groundSpeed()->setRawValue((gpsRawInt.vel == UINT16_MAX) ? qQNaN() : (gpsRawInt.vel / 100.0));
    velAcc()->setRawValue((gpsRawInt.vel_acc == 0) ? qQNaN() : (gpsRawInt.vel_acc / 1000.0));
    hdgAcc()->setRawValue((gpsRawInt.hdg_acc == 0) ? qQNaN() : (gpsRawInt.hdg_acc / 1e5));
    courseOverGround()->setRawValue((gpsRawInt.cog == UINT16_MAX) ? qQNaN() : (gpsRawInt.cog / 100.0));
    yaw()->setRawValue((gpsRawInt.yaw == UINT16_MAX) ? qQNaN() : (gpsRawInt.yaw / 100.0));
    lock()->setRawValue(gpsRawInt.fix_type);

    _setTelemetryAvailable(true);
}

void VehicleGPSFactGroup::_handleHighLatency(const mavlink_message_t &message)
{
    mavlink_high_latency_t highLatency{};
    mavlink_msg_high_latency_decode(&message, &highLatency);

    lat()->setRawValue(highLatency.latitude * 1e-7);
    lon()->setRawValue(highLatency.longitude * 1e-7);
    mgrs()->setRawValue(QGCGeo::convertGeoToMGRS(QGeoCoordinate(highLatency.latitude * 1e-7, highLatency.longitude * 1e-7, highLatency.altitude_amsl)));
    altitudeMSL()->setRawValue(static_cast<double>(highLatency.altitude_amsl));
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
    altitudeMSL()->setRawValue(static_cast<double>(highLatency2.altitude));
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

    systemErrors()->setRawValue         (gnssIntegrity.system_errors);
    spoofingState()->setRawValue        (gnssIntegrity.spoofing_state);
    jammingState()->setRawValue         (gnssIntegrity.jamming_state);
    authenticationState()->setRawValue  (gnssIntegrity.authentication_state);
    correctionsQuality()->setRawValue   (gnssIntegrity.corrections_quality);
    systemQuality()->setRawValue        (gnssIntegrity.system_status_summary);
    gnssSignalQuality()->setRawValue    (gnssIntegrity.gnss_signal_quality);
    postProcessingQuality()->setRawValue(gnssIntegrity.post_processing_quality);

    emit gnssIntegrityReceived();
}
