#include "VehicleGPSFactGroup.h"
#include "Vehicle.h"
#include "QGCGeo.h"
#include "QGCLoggingCategory.h"
#include "development/mavlink_msg_gnss_integrity.h"

#include <QtCore/QtMath>
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
    _addFact(&_rtkHealthFact);
    _addFact(&_rtkRateFact);
    _addFact(&_rtkNumSatsFact);
    _addFact(&_rtkBaselineFact);
    _addFact(&_rtkAccuracyFact);
    _addFact(&_rtkIARFact);

    _latFact.setRawValue(qQNaN());
    _lonFact.setRawValue(qQNaN());
    _mgrsFact.setRawValue("");
    _altitudeMSLFact.setRawValue(qQNaN());
    _altitudeEllipsoidFact.setRawValue(qQNaN());
    _hdopFact.setRawValue(qQNaN());
    _vdopFact.setRawValue(qQNaN());
    _hAccFact.setRawValue(qQNaN());
    _vAccFact.setRawValue(qQNaN());
    _groundSpeedFact.setRawValue(qQNaN());
    _velAccFact.setRawValue(qQNaN());
    _hdgAccFact.setRawValue(qQNaN());
    _courseOverGroundFact.setRawValue(qQNaN());
    _yawFact.setRawValue(qQNaN());
    _spoofingStateFact.setRawValue(255);
    _jammingStateFact.setRawValue(255);
    _authenticationStateFact.setRawValue(255);
    _correctionsQualityFact.setRawValue(255);
    _systemQualityFact.setRawValue(255);
    _gnssSignalQualityFact.setRawValue(255);
    _postProcessingQualityFact.setRawValue(255);
    _rtkBaselineFact.setRawValue(qQNaN());
    _rtkAccuracyFact.setRawValue(qQNaN());
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
    case MAVLINK_MSG_ID_GPS_RTK:
        _handleGpsRtk(message);
        break;
    default:
        break;
    }
}

VehicleGPSFactGroup::GPSQuality VehicleGPSFactGroup::quality() const
{
    const int fixType = _lockFact.rawValue().toInt();
    const int sats = _countFact.rawValue().toInt();
    const double hdopVal = _hdopFact.rawValue().toDouble();

    if (fixType < static_cast<int>(GPSFixType::Fix2D))
        return GPSQuality::QualityNone;

    if (fixType >= static_cast<int>(GPSFixType::FixRTKFixed))
        return GPSQuality::QualityExcellent;

    if (fixType >= static_cast<int>(GPSFixType::FixRTKFloat))
        return sats >= 10 ? GPSQuality::QualityGood : GPSQuality::QualityFair;

    // 3D/DGPS fix — score based on HDOP and satellite count
    if (fixType >= static_cast<int>(GPSFixType::Fix3D)) {
        const bool goodHdop = !qIsNaN(hdopVal) && hdopVal < 2.0;
        const bool goodSats = sats >= 12;
        if (goodHdop && goodSats) return GPSQuality::QualityGood;
        if (goodHdop || goodSats) return GPSQuality::QualityFair;
        return GPSQuality::QualityPoor;
    }

    return GPSQuality::QualityPoor;
}

template<typename T>
void VehicleGPSFactGroup::_applyGpsRawFields(const T &raw)
{
    const double latDeg = QGCMAVLink::mavlinkLatLonToDouble(raw.lat);
    const double lonDeg = QGCMAVLink::mavlinkLatLonToDouble(raw.lon);

    lat()->setRawValue(latDeg);
    lon()->setRawValue(lonDeg);
    mgrs()->setRawValue(QGCGeo::convertGeoToMGRS(QGeoCoordinate(latDeg, lonDeg)));
    altitudeMSL()->setRawValue(QGCMAVLink::mavlinkMmToMeters(raw.alt));
    altitudeEllipsoid()->setRawValue(QGCMAVLink::mavlinkMmToMeters(raw.alt_ellipsoid));
    count()->setRawValue((raw.satellites_visible == kInvalidSatelliteCount) ? 0 : raw.satellites_visible);
    hdop()->setRawValue((raw.eph == UINT16_MAX) ? qQNaN() : (raw.eph / 100.0));
    vdop()->setRawValue((raw.epv == UINT16_MAX) ? qQNaN() : (raw.epv / 100.0));
    hAcc()->setRawValue((raw.h_acc == 0) ? qQNaN() : (raw.h_acc / 1000.0));
    vAcc()->setRawValue((raw.v_acc == 0) ? qQNaN() : (raw.v_acc / 1000.0));
    groundSpeed()->setRawValue((raw.vel == UINT16_MAX) ? qQNaN() : (raw.vel / 100.0));
    velAcc()->setRawValue((raw.vel_acc == 0) ? qQNaN() : (raw.vel_acc / 1000.0));
    hdgAcc()->setRawValue((raw.hdg_acc == 0) ? qQNaN() : (raw.hdg_acc / 1e5));
    courseOverGround()->setRawValue((raw.cog == UINT16_MAX) ? qQNaN() : (raw.cog / 100.0));
    yaw()->setRawValue((raw.yaw == UINT16_MAX) ? qQNaN() : (raw.yaw / 100.0));
    lock()->setRawValue(raw.fix_type);

    _setTelemetryAvailable(true);
    emit qualityChanged();
}

// Explicit instantiations for translation units that call _applyGpsRawFields
template void VehicleGPSFactGroup::_applyGpsRawFields(const mavlink_gps_raw_int_t &);
template void VehicleGPSFactGroup::_applyGpsRawFields(const mavlink_gps2_raw_t &);

void VehicleGPSFactGroup::_handleGpsRawInt(const mavlink_message_t &message)
{
    mavlink_gps_raw_int_t gpsRawInt{};
    mavlink_msg_gps_raw_int_decode(&message, &gpsRawInt);
    _applyGpsRawFields(gpsRawInt);
}

void VehicleGPSFactGroup::_handleHighLatency(const mavlink_message_t &message)
{
    mavlink_high_latency_t highLatency{};
    mavlink_msg_high_latency_decode(&message, &highLatency);

    const double latDeg = QGCMAVLink::mavlinkLatLonToDouble(highLatency.latitude);
    const double lonDeg = QGCMAVLink::mavlinkLatLonToDouble(highLatency.longitude);
    lat()->setRawValue(latDeg);
    lon()->setRawValue(lonDeg);
    mgrs()->setRawValue(QGCGeo::convertGeoToMGRS(QGeoCoordinate(latDeg, lonDeg, highLatency.altitude_amsl)));
    altitudeMSL()->setRawValue(static_cast<double>(highLatency.altitude_amsl));
    lock()->setRawValue(highLatency.gps_fix_type);
    count()->setRawValue(0);

    _setTelemetryAvailable(true);
}

void VehicleGPSFactGroup::_handleHighLatency2(const mavlink_message_t &message)
{
    mavlink_high_latency2_t highLatency2{};
    mavlink_msg_high_latency2_decode(&message, &highLatency2);

    const double latDeg = QGCMAVLink::mavlinkLatLonToDouble(highLatency2.latitude);
    const double lonDeg = QGCMAVLink::mavlinkLatLonToDouble(highLatency2.longitude);
    lat()->setRawValue(latDeg);
    lon()->setRawValue(lonDeg);
    mgrs()->setRawValue(QGCGeo::convertGeoToMGRS(QGeoCoordinate(latDeg, lonDeg, highLatency2.altitude)));
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

void VehicleGPSFactGroup::_handleGpsRtk(const mavlink_message_t &message)
{
    // GPS_RTK and GPS2_RTK share the same logical field set but are distinct
    // MAVLink types. Decode into each type's own struct and route the fields
    // through a single local path — safe across dialect versions that may
    // diverge field layouts in the future.
    uint8_t  receiverId      = 0;
    uint8_t  rtkHealthVal    = 0;
    uint8_t  rtkRateVal      = 0;
    uint8_t  nsats           = 0;
    uint32_t iarHypotheses   = 0;
    uint32_t accuracyRaw     = 0;
    int32_t  baselineA       = 0;
    int32_t  baselineB       = 0;
    int32_t  baselineC       = 0;

    if (message.msgid == MAVLINK_MSG_ID_GPS_RTK) {
        mavlink_gps_rtk_t gpsRtk{};
        mavlink_msg_gps_rtk_decode(&message, &gpsRtk);
        receiverId    = gpsRtk.rtk_receiver_id;
        rtkHealthVal  = gpsRtk.rtk_health;
        rtkRateVal    = gpsRtk.rtk_rate;
        nsats         = gpsRtk.nsats;
        iarHypotheses = gpsRtk.iar_num_hypotheses;
        accuracyRaw   = gpsRtk.accuracy;
        baselineA     = gpsRtk.baseline_a_mm;
        baselineB     = gpsRtk.baseline_b_mm;
        baselineC     = gpsRtk.baseline_c_mm;
    } else {
        mavlink_gps2_rtk_t gps2Rtk{};
        mavlink_msg_gps2_rtk_decode(&message, &gps2Rtk);
        receiverId    = gps2Rtk.rtk_receiver_id;
        rtkHealthVal  = gps2Rtk.rtk_health;
        rtkRateVal    = gps2Rtk.rtk_rate;
        nsats         = gps2Rtk.nsats;
        iarHypotheses = gps2Rtk.iar_num_hypotheses;
        accuracyRaw   = gps2Rtk.accuracy;
        baselineA     = gps2Rtk.baseline_a_mm;
        baselineB     = gps2Rtk.baseline_b_mm;
        baselineC     = gps2Rtk.baseline_c_mm;
    }

    if (receiverId != _rtkReceiverId) {
        return;
    }

    rtkHealth()->setRawValue(rtkHealthVal);
    rtkRate()->setRawValue(rtkRateVal);
    rtkNumSats()->setRawValue(nsats);
    rtkIAR()->setRawValue(iarHypotheses);
    rtkAccuracy()->setRawValue(accuracyRaw / 1000.0);

    // Compute 3D baseline length from NED or ECEF components
    const double a = baselineA / 1000.0;
    const double b = baselineB / 1000.0;
    const double c = baselineC / 1000.0;
    rtkBaseline()->setRawValue(qSqrt(a * a + b * b + c * c));
}
