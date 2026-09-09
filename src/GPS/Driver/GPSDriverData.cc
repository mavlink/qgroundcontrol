#include "GPSDriverData.h"

#include <QtCore/QTimeZone>
#include <QtCore/QtMath>

#include <algorithm>
#include <cmath>
#include <limits>

#include "QGCLoggingCategory.h"
#include "satellite_info.h"
#include "sensor_gnss_relative.h"
#include "sensor_gps.h"

QGC_LOGGING_CATEGORY(GPSDriverDataLog, "GPS.Driver.GPSDriverData")

namespace {
std::optional<double> positive(double value)
{
    return qIsFinite(value) && value > 0 ? std::optional<double>(value) : std::nullopt;
}

std::optional<int> knownState(int value)
{
    return value > 0 ? std::optional<int>(value) : std::nullopt;
}

QGeoPositionInfo positionInfo(const sensor_gps_s& fix)
{
    QGeoCoordinate coordinate(fix.latitude_deg, fix.longitude_deg);
    if (!coordinate.isValid()) {
        qCDebug(GPSDriverDataLog) << "Rejected receiver fix: invalid coordinate"
                                  << "latitude:" << fix.latitude_deg << "longitude:" << fix.longitude_deg;
        return {};
    }
    const bool altitudeValid = fix.fix_type >= sensor_gps_s::FIX_TYPE_3D && qIsFinite(fix.altitude_msl_m);
    if (altitudeValid) {
        coordinate.setAltitude(fix.altitude_msl_m);
    }
    // Some receivers have a position before UTC is available. Use local reception time in that case.
    const QDateTime timestamp =
        fix.time_utc_usec > 0
            ? QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(fix.time_utc_usec / 1000), QTimeZone::UTC)
            : QDateTime::currentDateTimeUtc();
    if (!timestamp.isValid()) {
        qCDebug(GPSDriverDataLog) << "Rejected receiver fix: invalid UTC timestamp"
                                  << "timeUtcUs:" << fix.time_utc_usec;
    }
    QGeoPositionInfo position(coordinate, timestamp);
    if (qIsFinite(fix.eph) && fix.eph > 0) {
        position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, fix.eph);
    }
    if (altitudeValid && qIsFinite(fix.epv) && fix.epv > 0) {
        position.setAttribute(QGeoPositionInfo::VerticalAccuracy, fix.epv);
    }
    if (fix.vel_ned_valid) {
        if (qIsFinite(fix.vel_m_s) && fix.vel_m_s >= 0) {
            position.setAttribute(QGeoPositionInfo::GroundSpeed, fix.vel_m_s);
        }
        if (qIsFinite(fix.vel_d_m_s)) {
            position.setAttribute(QGeoPositionInfo::VerticalSpeed, -fix.vel_d_m_s);
        }
        if (qIsFinite(fix.cog_rad)) {
            const double degrees = std::fmod(qRadiansToDegrees(static_cast<double>(fix.cog_rad)), 360.0);
            position.setAttribute(QGeoPositionInfo::Direction, degrees < 0 ? degrees + 360.0 : degrees);
            if (qIsFinite(fix.c_variance_rad) && fix.c_variance_rad > 0) {
                position.setAttribute(QGeoPositionInfo::DirectionAccuracy, qRadiansToDegrees(fix.c_variance_rad));
            }
        }
    }
    return position;
}

}  // namespace

void GPSDriverData::initialize(sensor_gps_s& fix)
{
    fix = {};
    fix.latitude_deg = qQNaN();
    fix.longitude_deg = qQNaN();
    fix.altitude_msl_m = qQNaN();
    fix.altitude_ellipsoid_m = qQNaN();
    fix.heading = qQNaN();
    fix.heading_accuracy = qQNaN();
    fix.hdop = qQNaN();
    fix.vdop = qQNaN();
    fix.satellites_used = std::numeric_limits<uint8_t>::max();
}

GPSObservation GPSDriverData::position(const sensor_gps_s& fix)
{
    GPSObservation result;
    result.position = positionInfo(fix);
    result.receiverFixValid =
        fix.fix_type >= sensor_gps_s::FIX_TYPE_2D && fix.fix_type <= sensor_gps_s::FIX_TYPE_RTK_FIXED;
    result.altitudeDatum = GPSObservation::AltitudeDatum::MeanSeaLevel;
    result.monotonicTimestampUs = fix.timestamp != 0 ? fix.timestamp : GPSObservation::monotonicNowUs();
    const qint64 age = result.ageMilliseconds();
    result.receivedAt = age >= 0 ? QDateTime::currentDateTimeUtc().addMSecs(-age) : QDateTime();
    switch (fix.fix_type) {
        case sensor_gps_s::FIX_TYPE_NONE:
            result.fixQuality = GPSObservation::FixQuality::NoFix;
            break;
        case sensor_gps_s::FIX_TYPE_2D:
            result.fixQuality = GPSObservation::FixQuality::Fix2D;
            break;
        case sensor_gps_s::FIX_TYPE_3D:
            result.fixQuality = GPSObservation::FixQuality::Fix3D;
            break;
        case sensor_gps_s::FIX_TYPE_RTCM_CODE_DIFFERENTIAL:
            result.fixQuality = GPSObservation::FixQuality::Differential;
            break;
        case sensor_gps_s::FIX_TYPE_RTK_FLOAT:
            result.fixQuality = GPSObservation::FixQuality::RTKFloat;
            break;
        case sensor_gps_s::FIX_TYPE_RTK_FIXED:
            result.fixQuality = GPSObservation::FixQuality::RTKFixed;
            break;
        case sensor_gps_s::FIX_TYPE_EXTRAPOLATED:
            result.fixQuality = GPSObservation::FixQuality::Extrapolated;
            break;
        default:
            break;
    }
    if (fix.satellites_used != std::numeric_limits<uint8_t>::max()) {
        result.satellitesUsed = fix.satellites_used;
    }
    result.horizontalDop = positive(fix.hdop);
    result.verticalDop = positive(fix.vdop);
    if (fix.fix_type >= sensor_gps_s::FIX_TYPE_3D && fix.fix_type <= sensor_gps_s::FIX_TYPE_RTK_FIXED &&
        qIsFinite(fix.altitude_ellipsoid_m)) {
        result.altitudeEllipsoidMeters = fix.altitude_ellipsoid_m;
    }
    if (qIsFinite(fix.heading)) {
        double degrees = std::fmod(qRadiansToDegrees(static_cast<double>(fix.heading)), 360.0);
        result.trueHeadingDegrees = degrees < 0 ? degrees + 360.0 : degrees;
        if (qIsFinite(fix.heading_accuracy) && fix.heading_accuracy >= 0) {
            result.trueHeadingAccuracyDegrees = qRadiansToDegrees(static_cast<double>(fix.heading_accuracy));
        }
    }
    result.integrityProvenance = GPSIntegrityProvenance{
        .jammingTimestampUs = fix.jamming_state_timestamp,
        .spoofingTimestampUs = fix.spoofing_state_timestamp,
        .authenticationTimestampUs = fix.authentication_state_timestamp,
        .correctionsTimestampUs = fix.corrections_timestamp,
    };
    result.jammingState = knownState(fix.jamming_state);
    result.spoofingState = knownState(fix.spoofing_state);
    result.authenticationState = knownState(fix.authentication_state);
    result.correctionsProtocol = knownState(fix.corrections_protocol);
    result.correctionsUsed = knownState(fix.corrections_msg_used);
    return result;
}

GPSSatelliteObservation GPSDriverData::satellites(const satellite_info_s& report, std::optional<GPSType> type)
{
    GPSSatelliteObservation result;
    result.monotonicTimestampUs = report.timestamp != 0 ? report.timestamp : GPSObservation::monotonicNowUs();
    const int count = std::min(report.count, satellite_info_s::SAT_INFO_MAX_SATELLITES);
    result.satellites.reserve(count);
    for (int index = 0; index < count; ++index) {
        GPSSatellite satellite;
        satellite.id = report.svid[index];
        satellite.prn = report.prn[index];
        satellite.used = report.used[index] != 0;
        satellite.elevationDegrees = report.elevation[index];
        satellite.signalStrength = report.snr[index];
        if (type != GPSType::septentrio) {
            satellite.rawAzimuth = report.azimuth[index];
        }
        if (type == GPSType::u_blox) {
            satellite.azimuthEncoding = GPSSatellite::AzimuthEncoding::ScaledFullCircleByte;
        } else if (type == GPSType::trimble || type == GPSType::femto) {
            // These native drivers narrow a degree value to uint8_t before this boundary.
            satellite.azimuthEncoding = GPSSatellite::AzimuthEncoding::DegreesModulo256;
        }
        result.satellites.append(satellite);
    }
    return result;
}

GPSRelativeObservation GPSDriverData::relativePosition(const sensor_gnss_relative_s& report)
{
    GPSRelativeObservation result;
    result.monotonicTimestampUs = report.timestamp != 0 ? report.timestamp : GPSObservation::monotonicNowUs();
    result.sampleTimestampUs = report.timestamp_sample;
    result.receiverTimeUs = report.time_utc_usec;
    result.referenceStationId = report.reference_station_id;
    for (size_t index = 0; index < result.positionNedMeters.size(); ++index) {
        result.positionNedMeters[index] = report.position[index];
        result.accuracyNedMeters[index] = report.position_accuracy[index];
    }
    result.lengthMeters = report.position_length;
    result.lengthAccuracyMeters = report.accuracy_length;
    if (report.heading_valid && qIsFinite(report.heading) && qIsFinite(report.heading_accuracy) &&
        report.heading_accuracy >= 0) {
        double degrees = std::fmod(qRadiansToDegrees(static_cast<double>(report.heading)), 360.0);
        result.headingDegrees = degrees < 0 ? degrees + 360.0 : degrees;
        result.headingAccuracyDegrees = qRadiansToDegrees(static_cast<double>(report.heading_accuracy));
    }
    result.fixValid = report.gnss_fix_ok;
    result.differential = report.differential_solution;
    result.positionValid = report.relative_position_valid;
    result.carrierFloat = report.carrier_solution_floating;
    result.carrierFixed = report.carrier_solution_fixed;
    result.movingBase = report.moving_base_mode;
    result.referencePositionMissing = report.reference_position_miss;
    result.referenceObservationsMissing = report.reference_observations_miss;
    result.normalized = report.relative_position_normalized;
    return result;
}
