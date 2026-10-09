#include "VehicleGPSFactGroup.h"

#include <algorithm>
#include <cmath>

#include <QtCore/QDateTime>
#include <QtPositioning/QGeoCoordinate>

#include "MAVLinkLib.h"
#include "QGCGeo.h"
#include "QGCLoggingCategory.h"
#include "RuntimeScheduler.h"
#include "Vehicle.h"
#include "development/mavlink_msg_gnss_integrity.h"

VehicleGPSFactGroup::VehicleGPSFactGroup(QObject* parent, RuntimeScheduler* scheduler, ReceiverIndex receiver)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/GPSFact.json"), parent)
    , _receiver(receiver)
    , _scheduler(RuntimeScheduler::orDefault(scheduler, this))
    , _rtkStatusExpiry(_scheduler, this)
    , _integrityExpiry(_scheduler, this)
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
    _addFact(&_rtkBaselineFact);
    _addFact(&_rtkRateFact);
    _addFact(&_rtkSatellitesFact);

    (void) connect(&_systemErrorsFact, &Fact::rawValueChanged, this, &VehicleGPSFactGroup::systemErrorTextChanged);
    for (Fact* fact : {&_spoofingStateFact, &_jammingStateFact, &_authenticationStateFact}) {
        (void) connect(fact, &Fact::rawValueChanged, this, &VehicleGPSFactGroup::resilienceChanged);
    }

    _latFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _lonFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _mgrsFact.setRawValue("");
    _hdopFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _vdopFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _horizontalAccuracyFact.setRawValue(qQNaN());
    _verticalAccuracyFact.setRawValue(qQNaN());
    _courseOverGroundFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _yawFact.setRawValue(qQNaN());
    _clearIntegrity();
    _clearRtkStatus();
}

void VehicleGPSFactGroup::handleMessage(Vehicle* vehicle, const mavlink_message_t& message)
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
    case MAVLINK_MSG_ID_GPS_RTK:
        if (_receiver == ReceiverIndex::Primary) {
            _handleGpsRtk(message);
        }
        break;
    case MAVLINK_MSG_ID_GPS2_RTK:
        if (_receiver == ReceiverIndex::Secondary) {
            _handleGpsRtk(message);
        }
        break;
    default:
        break;
    }
}

void VehicleGPSFactGroup::_updateFix(int fixType, int satellitesVisible, double hdopValue, double vdopValue,
                                     double courseValue, double yawValue)
{
    count()->setRawValue(satellitesVisible == UINT8_MAX ? 0 : satellitesVisible);
    hdop()->setRawValue(hdopValue);
    vdop()->setRawValue(vdopValue);
    courseOverGround()->setRawValue(courseValue);
    yaw()->setRawValue(yawValue);
    lock()->setRawValue(fixType);
}

void VehicleGPSFactGroup::_reportPosition(QGeoPositionInfo position, int fixType)
{
    position.setTimestamp(QDateTime::currentDateTimeUtc());
    const auto coordinate = position.coordinate();
    lat()->setRawValue(coordinate.latitude());
    lon()->setRawValue(coordinate.longitude());
    mgrs()->setRawValue(coordinate.isValid() ? QGCGeo::convertGeoToMGRS(coordinate) : QString());
    horizontalAccuracy()->setRawValue(position.attribute(QGeoPositionInfo::HorizontalAccuracy));
    verticalAccuracy()->setRawValue(position.attribute(QGeoPositionInfo::VerticalAccuracy));
    _setTelemetryAvailable(true);
    emit positionReported(position, fixType);
}

void VehicleGPSFactGroup::_handleGpsRaw(const mavlink_message_t& message)
{
    const auto update = [this](const auto& raw) {
        QGeoPositionInfo position;
        position.setCoordinate(QGeoCoordinate(raw.lat * 1e-7, raw.lon * 1e-7, raw.alt / 1000.0));
        if (raw.h_acc != 0) {
            position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, raw.h_acc / 1000.0);
        }
        if (raw.v_acc != 0) {
            position.setAttribute(QGeoPositionInfo::VerticalAccuracy, raw.v_acc / 1000.0);
        }
        if (raw.cog < 36000) {
            position.setAttribute(QGeoPositionInfo::Direction, raw.cog / 100.0);
        }
        const double hdopValue = raw.eph == UINT16_MAX ? qQNaN() : raw.eph / 100.0;
        const double vdopValue = raw.epv == UINT16_MAX ? qQNaN() : raw.epv / 100.0;
        const double yawValue = raw.yaw > 0 && raw.yaw <= 36000 ? (raw.yaw % 36000) / 100.0 : qQNaN();
        _updateFix(raw.fix_type, raw.satellites_visible, hdopValue, vdopValue,
                   position.attribute(QGeoPositionInfo::Direction), yawValue);
        _reportPosition(position, raw.fix_type);
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

void VehicleGPSFactGroup::_handleHighLatency(const mavlink_message_t& message)
{
    mavlink_high_latency_t highLatency{};
    mavlink_msg_high_latency_decode(&message, &highLatency);

    QGeoPositionInfo position;
    position.setCoordinate(
        QGeoCoordinate(highLatency.latitude * 1e-7, highLatency.longitude * 1e-7, highLatency.altitude_amsl));
    _updateFix(highLatency.gps_fix_type, highLatency.gps_nsat, qQNaN(), qQNaN(), qQNaN(), qQNaN());
    _reportPosition(position, highLatency.gps_fix_type);
}

void VehicleGPSFactGroup::_handleHighLatency2(const mavlink_message_t& message)
{
    mavlink_high_latency2_t highLatency2{};
    mavlink_msg_high_latency2_decode(&message, &highLatency2);

    QGeoPositionInfo position;
    position.setCoordinate(
        QGeoCoordinate(highLatency2.latitude * 1e-7, highLatency2.longitude * 1e-7, highLatency2.altitude));
    if (highLatency2.eph != UINT8_MAX) {
        position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, highLatency2.eph / 10.0);
    }
    if (highLatency2.epv != UINT8_MAX) {
        position.setAttribute(QGeoPositionInfo::VerticalAccuracy, highLatency2.epv / 10.0);
    }
    // HIGH_LATENCY2 reports an estimated global position and no receiver state, so the fix facts keep their values
    // and the report does not count as a GPS fix.
    _reportPosition(position, GPS_FIX_TYPE_NO_GPS);
}

void VehicleGPSFactGroup::_handleGnssIntegrity(const mavlink_message_t& message)
{
    mavlink_gnss_integrity_t gnssIntegrity;
    mavlink_msg_gnss_integrity_decode(&message, &gnssIntegrity);

    if (gnssIntegrity.id != static_cast<uint8_t>(_receiver)) {
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
    (void) _integrityExpiry.schedule(GNSS_INTEGRITY_TIMEOUT, [this]() { _clearIntegrity(); });
}

void VehicleGPSFactGroup::_clearIntegrity()
{
    _systemErrorsFact.setRawValue(0);
    for (Fact* fact : {&_spoofingStateFact, &_jammingStateFact, &_authenticationStateFact, &_correctionsQualityFact,
                       &_systemQualityFact, &_gnssSignalQualityFact, &_postProcessingQualityFact}) {
        fact->setRawValue(NOT_REPORTED);
    }
}

void VehicleGPSFactGroup::_handleGpsRtk(const mavlink_message_t& message)
{
    const auto apply = [this](const auto& rtk) {
        // Baseline length is independent of whether the components are ECEF or NED.
        const double a = rtk.baseline_a_mm;
        const double b = rtk.baseline_b_mm;
        const double c = rtk.baseline_c_mm;
        rtkBaseline()->setRawValue(std::sqrt(a * a + b * b + c * c) / 1000.0);
        rtkRate()->setRawValue(rtk.rtk_rate);
        rtkSatellites()->setRawValue(rtk.nsats);
    };
    if (message.msgid == MAVLINK_MSG_ID_GPS2_RTK) {
        mavlink_gps2_rtk_t rtk;
        mavlink_msg_gps2_rtk_decode(&message, &rtk);
        apply(rtk);
    } else {
        mavlink_gps_rtk_t rtk;
        mavlink_msg_gps_rtk_decode(&message, &rtk);
        apply(rtk);
    }
    (void) _rtkStatusExpiry.schedule(RTK_STATUS_TIMEOUT, [this]() { _clearRtkStatus(); });
}

void VehicleGPSFactGroup::_clearRtkStatus()
{
    _rtkBaselineFact.setRawValue(qQNaN());
    _rtkRateFact.setRawValue(qQNaN());
    _rtkSatellitesFact.setRawValue(-1);
}

QString VehicleGPSFactGroup::systemErrorText() const
{
    // MAVLink GPS_SYSTEM_ERROR_FLAGS.
    static constexpr const char* ERROR_NAMES[] = {
        QT_TR_NOOP("Incoming correction"),
        QT_TR_NOOP("Configuration"),
        QT_TR_NOOP("Software"),
        QT_TR_NOOP("Antenna"),
        QT_TR_NOOP("Event congestion"),
        QT_TR_NOOP("CPU overload"),
        QT_TR_NOOP("Output congestion"),
    };
    const quint32 errors = _systemErrorsFact.rawValue().toUInt();
    QStringList names;
    quint32 known = 0;
    for (quint32 bit = 0; bit < std::size(ERROR_NAMES); ++bit) {
        known |= 1u << bit;
        if (errors & (1u << bit)) {
            names.append(tr(ERROR_NAMES[bit]));
        }
    }
    if (errors & ~known) {
        names.append(tr("Other (0x%1)").arg(errors & ~known, 0, 16));
    }
    return names.join(QStringLiteral(", "));
}

bool VehicleGPSFactGroup::jammingReported() const
{
    return reported(_jammingStateFact.rawValue().toInt());
}

bool VehicleGPSFactGroup::spoofingReported() const
{
    return reported(_spoofingStateFact.rawValue().toInt());
}

bool VehicleGPSFactGroup::authenticationReported() const
{
    return reported(_authenticationStateFact.rawValue().toInt());
}

int VehicleGPSFactGroup::interferenceState() const
{
    const int spoofing = _spoofingStateFact.rawValue().toInt();
    const int jamming = _jammingStateFact.rawValue().toInt();
    return (std::max) (reported(spoofing) ? spoofing : 0, reported(jamming) ? jamming : 0);
}

int VehicleGPSFactGroup::authenticationSeverity() const
{
    switch (static_cast<AuthenticationState>(_authenticationStateFact.rawValue().toInt())) {
        case AuthenticationState::Disabled:
            return 1;
        case AuthenticationState::Initializing:
            return 2;
        case AuthenticationState::Ok:
            return 3;
        case AuthenticationState::Error:
            return 4;
        default:
            return 0;
    }
}
