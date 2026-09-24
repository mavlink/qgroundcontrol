#include "VehicleGPSFactGroup.h"

#include <QtPositioning/QGeoCoordinate>

#include "GPSSourceHealth.h"
#include "MAVLinkLib.h"
#include "QGCGeo.h"
#include "QGCLoggingCategory.h"
#include "QtRuntimeScheduler.h"
#include "Vehicle.h"
#include "development/mavlink_msg_gnss_integrity.h"

namespace {

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

}  // namespace

VehicleGPSFactGroup::VehicleGPSFactGroup(QObject* parent, RuntimeScheduler* scheduler, ReceiverIndex receiver)
    : FactGroup(1000, ":/json/Vehicle/GPSFact.json", parent)
    , _receiver(receiver)
    , _scheduler(scheduler ? scheduler : new QtRuntimeScheduler(this))
    , _positionHealth(new GPSSourceHealth(this, _scheduler))
{
    _addFact(&_latFact);
    _addFact(&_lonFact);
    _addFact(&_mgrsFact);
    _addFact(&_hdopFact);
    _addFact(&_vdopFact);
    _addFact(&_horizontalAccuracyFact);
    _addFact(&_verticalAccuracyFact);
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
    _hdopFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _vdopFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _horizontalAccuracyFact.setRawValue(qQNaN());
    _verticalAccuracyFact.setRawValue(qQNaN());
    _courseOverGroundFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _yawFact.setRawValue(qQNaN());
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
        if (_receiver == ReceiverIndex::Primary) {
            _handleGpsRaw(message);
        }
        break;
    case MAVLINK_MSG_ID_GPS2_RAW:
        if (_receiver == ReceiverIndex::Secondary) {
            _handleGpsRaw(message);
        }
        break;
    case MAVLINK_MSG_ID_HIGH_LATENCY:
        if (_receiver == ReceiverIndex::Primary) {
            _handleHighLatency(message);
        }
        break;
    case MAVLINK_MSG_ID_HIGH_LATENCY2:
        if (_receiver == ReceiverIndex::Primary) {
            _handleHighLatency2(message);
        }
        break;
    case MAVLINK_MSG_ID_GNSS_INTEGRITY:
        _handleGnssIntegrity(message);
        break;
    default:
        break;
    }
}

std::optional<GPSObservation> VehicleGPSFactGroup::acceptedObservation() const
{
    return _positionHealth->acceptedObservation(GPSObservation::PositionUse::Gga);
}

void VehicleGPSFactGroup::_updateGpsObservation(GPSObservation observation, int fixType, int satellitesVisible,
                                                double yawValue)
{
    observation.receivedAt = QDateTime::currentDateTimeUtc();
    observation.monotonicTimestampUs = _scheduler->nowUs();
    observation.position.setTimestamp(observation.receivedAt);
    observation.altitudeDatum = GPSAltitudeDatum::MeanSeaLevel;
    observation.fixQuality = fixQuality(fixType);
    observation.receiverFixValid = observation.fixQuality != GPSObservation::FixQuality::NoFix;
    _positionHealth->updateObservation(observation);

    const auto coordinate = observation.position.coordinate();
    lat()->setRawValue(coordinate.latitude());
    lon()->setRawValue(coordinate.longitude());
    mgrs()->setRawValue(coordinate.isValid() ? QGCGeo::convertGeoToMGRS(coordinate) : QString());
    count()->setRawValue(satellitesVisible == UINT8_MAX ? 0 : satellitesVisible);
    hdop()->setRawValue(observation.horizontalDop.value_or(qQNaN()));
    vdop()->setRawValue(observation.verticalDop.value_or(qQNaN()));
    horizontalAccuracy()->setRawValue(observation.position.attribute(QGeoPositionInfo::HorizontalAccuracy));
    verticalAccuracy()->setRawValue(observation.position.attribute(QGeoPositionInfo::VerticalAccuracy));
    courseOverGround()->setRawValue(observation.position.attribute(QGeoPositionInfo::Direction));
    yaw()->setRawValue(yawValue);
    lock()->setRawValue(fixType);
    _setTelemetryAvailable(true);
}

void VehicleGPSFactGroup::_handleGpsRaw(const mavlink_message_t& message)
{
    const auto update = [this](const auto& raw) {
        GPSObservation observation;
        observation.position.setCoordinate(QGeoCoordinate(raw.lat * 1e-7, raw.lon * 1e-7, raw.alt / 1000.0));
        if (raw.eph != UINT16_MAX) {
            observation.horizontalDop = raw.eph / 100.0;
        }
        if (raw.epv != UINT16_MAX) {
            observation.verticalDop = raw.epv / 100.0;
        }
        if (raw.h_acc != 0) {
            observation.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, raw.h_acc / 1000.0);
        }
        if (raw.v_acc != 0) {
            observation.position.setAttribute(QGeoPositionInfo::VerticalAccuracy, raw.v_acc / 1000.0);
        }
        if (raw.cog < 36000) {
            observation.position.setAttribute(QGeoPositionInfo::Direction, raw.cog / 100.0);
        }
        const double yawValue = raw.yaw > 0 && raw.yaw <= 36000 ? (raw.yaw % 36000) / 100.0 : qQNaN();
        _updateGpsObservation(observation, raw.fix_type, raw.satellites_visible, yawValue);
    };
    if (message.msgid == MAVLINK_MSG_ID_GPS_RAW_INT) {
        mavlink_gps_raw_int_t raw{};
        mavlink_msg_gps_raw_int_decode(&message, &raw);
        update(raw);
    } else {
        mavlink_gps2_raw_t raw{};
        mavlink_msg_gps2_raw_decode(&message, &raw);
        update(raw);
    }
}

void VehicleGPSFactGroup::_handleHighLatency(const mavlink_message_t &message)
{
    mavlink_high_latency_t highLatency{};
    mavlink_msg_high_latency_decode(&message, &highLatency);

    GPSObservation observation;
    observation.position.setCoordinate(
        QGeoCoordinate(highLatency.latitude * 1e-7, highLatency.longitude * 1e-7, highLatency.altitude_amsl));
    _updateGpsObservation(observation, highLatency.gps_fix_type, highLatency.gps_nsat);
}

void VehicleGPSFactGroup::_handleHighLatency2(const mavlink_message_t &message)
{
    mavlink_high_latency2_t highLatency2{};
    mavlink_msg_high_latency2_decode(&message, &highLatency2);

    GPSObservation observation;
    observation.position.setCoordinate(
        QGeoCoordinate(highLatency2.latitude * 1e-7, highLatency2.longitude * 1e-7, highLatency2.altitude));
    if (highLatency2.eph != UINT8_MAX) {
        observation.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, highLatency2.eph / 10.0);
    }
    if (highLatency2.epv != UINT8_MAX) {
        observation.position.setAttribute(QGeoPositionInfo::VerticalAccuracy, highLatency2.epv / 10.0);
    }
    // HIGH_LATENCY2 reports an estimated global position, not a raw GPS fix.
    _updateGpsObservation(observation, GPS_FIX_TYPE_NO_GPS, UINT8_MAX);
}

void VehicleGPSFactGroup::_handleGnssIntegrity(const mavlink_message_t& message)
{
    mavlink_gnss_integrity_t gnssIntegrity;
    mavlink_msg_gnss_integrity_decode(&message, &gnssIntegrity);

    if (gnssIntegrity.id != static_cast<uint8_t>(_receiver)) {
        return;
    }

    const quint64 receiptUs = _scheduler->nowUs();
    systemErrors()->setRawValue         (gnssIntegrity.system_errors);
    spoofingState()->setRawValue        (gnssIntegrity.spoofing_state);
    jammingState()->setRawValue         (gnssIntegrity.jamming_state);
    authenticationState()->setRawValue  (gnssIntegrity.authentication_state);
    correctionsQuality()->setRawValue   (gnssIntegrity.corrections_quality);
    systemQuality()->setRawValue        (gnssIntegrity.system_status_summary);
    gnssSignalQuality()->setRawValue    (gnssIntegrity.gnss_signal_quality);
    postProcessingQuality()->setRawValue(gnssIntegrity.post_processing_quality);

    _gnssIntegrityTimestampUs = receiptUs;
    emit gnssIntegrityReceived();
}
