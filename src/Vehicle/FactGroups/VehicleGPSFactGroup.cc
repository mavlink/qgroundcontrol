#include "VehicleGPSFactGroup.h"

#include <QtPositioning/QGeoCoordinate>

#include "MAVLinkLib.h"
#include "MonotonicClock.h"
#include "QGCGeo.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"
#include "development/mavlink_msg_gnss_integrity.h"

namespace {

template <typename Message>
GPSObservation rawObservation(const Message& message, const QString& sourceId, quint64 receiptUs)
{
    GPSObservation observation;
    observation.receivedAt = QDateTime::currentDateTimeUtc();
    observation.monotonicTimestampUs = receiptUs;
    observation.sourceId = sourceId;
    observation.receiverFixValid = message.fix_type >= GPS_FIX_TYPE_2D_FIX && message.fix_type <= GPS_FIX_TYPE_PPP;
    switch (message.fix_type) {
        case GPS_FIX_TYPE_NO_GPS:
        case GPS_FIX_TYPE_NO_FIX:
            observation.fixQuality = GPSObservation::FixQuality::NoFix;
            break;
        case GPS_FIX_TYPE_2D_FIX:
            observation.fixQuality = GPSObservation::FixQuality::Fix2D;
            break;
        case GPS_FIX_TYPE_3D_FIX:
        case GPS_FIX_TYPE_STATIC:
        case GPS_FIX_TYPE_PPP:
            observation.fixQuality = GPSObservation::FixQuality::Fix3D;
            break;
        case GPS_FIX_TYPE_DGPS:
            observation.fixQuality = GPSObservation::FixQuality::Differential;
            break;
        case GPS_FIX_TYPE_RTK_FLOAT:
            observation.fixQuality = GPSObservation::FixQuality::RTKFloat;
            break;
        case GPS_FIX_TYPE_RTK_FIXED:
            observation.fixQuality = GPSObservation::FixQuality::RTKFixed;
            break;
        default:
            break;
    }
    QGeoCoordinate coordinate(message.lat * 1e-7, message.lon * 1e-7);
    if (*observation.receiverFixValid && message.fix_type >= GPS_FIX_TYPE_3D_FIX && message.alt != INT32_MAX &&
        message.alt != INT32_MIN) {
        coordinate.setAltitude(message.alt / 1000.0);
        observation.altitudeDatum = GPSAltitudeDatum::MeanSeaLevel;
    }
    // time_usec may be boot-relative, not UTC.
    observation.position = QGeoPositionInfo(coordinate, observation.receivedAt);
    if (message.h_acc > 0 && message.h_acc != UINT32_MAX) {
        observation.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, message.h_acc / 1000.0);
        observation.accuracyTimestampUs = receiptUs;
    }
    if (message.v_acc > 0 && message.v_acc != UINT32_MAX) {
        observation.position.setAttribute(QGeoPositionInfo::VerticalAccuracy, message.v_acc / 1000.0);
        observation.accuracyTimestampUs = receiptUs;
    }
    return observation;
}

}  // namespace

VehicleGPSFactGroup::VehicleGPSFactGroup(QObject* parent)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/GPSFact.json"), parent)
{
    _addFact(&_latFact);
    _addFact(&_lonFact);
    _addFact(&_mgrsFact);
    _addFact(&_hdopFact);
    _addFact(&_vdopFact);
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

    _latFact.setRawValue(qQNaN());
    _lonFact.setRawValue(qQNaN());
    _mgrsFact.setRawValue(QString());
    _hdopFact.setRawValue(qQNaN());
    _vdopFact.setRawValue(qQNaN());
    _courseOverGroundFact.setRawValue(qQNaN());
    _yawFact.setRawValue(std::numeric_limits<int16_t>::quiet_NaN());
    _lockFact.setRawValue(0);
    _countFact.setRawValue(0);
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
    switch (message.msgid) {
    case MAVLINK_MSG_ID_GPS_RAW_INT:
        _updateObservation(vehicle, message);
        _handleGpsRawInt(message);
        break;
    case MAVLINK_MSG_ID_HIGH_LATENCY:
        if (!vehicle || (message.sysid == vehicle->id() && message.compid == vehicle->defaultComponentId())) {
            invalidateObservation();
        }
        _handleHighLatency(message);
        break;
    case MAVLINK_MSG_ID_HIGH_LATENCY2:
        if (!vehicle || (message.sysid == vehicle->id() && message.compid == vehicle->defaultComponentId())) {
            invalidateObservation();
        }
        _handleHighLatency2(message);
        break;
    case MAVLINK_MSG_ID_GNSS_INTEGRITY:
        _handleGnssIntegrity(message);
        break;
    default:
        break;
    }
}

void VehicleGPSFactGroup::_updateObservation(Vehicle* vehicle, const mavlink_message_t& message)
{
    if (vehicle && (message.sysid != vehicle->id() || message.compid != vehicle->defaultComponentId())) {
        return;
    }
    const auto receiptUs = MonotonicClock::nowUs();
    if (message.msgid == MAVLINK_MSG_ID_GPS_RAW_INT) {
        mavlink_gps_raw_int_t raw{};
        mavlink_msg_gps_raw_int_decode(&message, &raw);
        _observation = rawObservation(raw, QStringLiteral("VehicleGPS"), receiptUs);
    } else if (message.msgid == MAVLINK_MSG_ID_GPS2_RAW) {
        mavlink_gps2_raw_t raw{};
        mavlink_msg_gps2_raw_decode(&message, &raw);
        _observation = rawObservation(raw, QStringLiteral("VehicleGPS2"), receiptUs);
    }
}

void VehicleGPSFactGroup::_handleGpsRawInt(const mavlink_message_t &message)
{
    mavlink_gps_raw_int_t gpsRawInt{};
    mavlink_msg_gps_raw_int_decode(&message, &gpsRawInt);

    lat()->setRawValue(gpsRawInt.lat * 1e-7);
    lon()->setRawValue(gpsRawInt.lon * 1e-7);
    mgrs()->setRawValue(QGCGeo::convertGeoToMGRS(QGeoCoordinate(gpsRawInt.lat * 1e-7, gpsRawInt.lon * 1e-7)));
    count()->setRawValue((gpsRawInt.satellites_visible == 255) ? 0 : gpsRawInt.satellites_visible);
    hdop()->setRawValue((gpsRawInt.eph == UINT16_MAX) ? qQNaN() : (gpsRawInt.eph / 100.0));
    vdop()->setRawValue((gpsRawInt.epv == UINT16_MAX) ? qQNaN() : (gpsRawInt.epv / 100.0));
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
