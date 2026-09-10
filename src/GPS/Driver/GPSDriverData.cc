#include "GPSDriverData.h"

#include <QtCore/QTimeZone>
#include <QtCore/QtMath>

#include <algorithm>
#include <cmath>
#include <limits>

#include "GPSPositionReport.h"
#include "GPSRelativeReport.h"
#include "GPSSatelliteReport.h"
#include "QGCLoggingCategory.h"

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

bool freshMetadata(uint64_t timestamp, uint64_t receipt)
{
    // Zero preserves callers that supply a complete, uncached report without per-field receipts.
    return timestamp == 0 || (timestamp <= receipt && receipt - timestamp <= 5000000);
}

QGeoPositionInfo positionInfo(const GPSPositionReport& fix, const GPSExecutionContext& context)
{
    QGeoCoordinate coordinate(fix.latitude_deg, fix.longitude_deg);
    if (!coordinate.isValid()) {
        qCDebug(GPSDriverDataLog) << "Rejected receiver fix: invalid coordinate"
                                  << "latitude:" << fix.latitude_deg << "longitude:" << fix.longitude_deg;
        return {};
    }
    const bool altitudeValid = fix.fix_type >= GPSPositionReport::FIX_TYPE_3D && qIsFinite(fix.altitude_msl_m);
    if (altitudeValid) {
        coordinate.setAltitude(fix.altitude_msl_m);
    }
    // Some receivers have a position before UTC is available. Use local reception time in that case.
    const QDateTime timestamp =
        fix.time_utc_usec > 0
            ? QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(fix.time_utc_usec / 1000), QTimeZone::UTC)
            : QDateTime::fromMSecsSinceEpoch((context.utcNowUs() + 500) / 1000, QTimeZone::UTC);
    if (!timestamp.isValid()) {
        qCDebug(GPSDriverDataLog) << "Rejected receiver fix: invalid UTC timestamp"
                                  << "timeUtcUs:" << fix.time_utc_usec;
    }
    QGeoPositionInfo position(coordinate, timestamp);
    if (freshMetadata(fix.accuracy_timestamp, context.nowUs()) && qIsFinite(fix.eph) && fix.eph > 0) {
        position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, fix.eph);
    }
    if (freshMetadata(fix.accuracy_timestamp, context.nowUs()) && altitudeValid && qIsFinite(fix.epv) && fix.epv > 0) {
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

void GPSDriverData::initialize(GPSPositionReport& fix)
{
    fix = {};
    fix.latitude_deg = qQNaN();
    fix.longitude_deg = qQNaN();
    fix.altitude_msl_m = qQNaN();
    fix.altitude_ellipsoid_m = qQNaN();
    fix.heading = qQNaN();
    fix.heading_accuracy = qQNaN();
    fix.s_variance_m_s = qQNaN();
    fix.hdop = qQNaN();
    fix.vdop = qQNaN();
    fix.satellites_used = std::numeric_limits<uint8_t>::max();
}

GPSObservation GPSDriverData::position(const GPSPositionReport& fix, const GPSExecutionContext& context)
{
    GPSObservation result;
    result.position = positionInfo(fix, context);
    result.receiverFixValid =
        fix.fix_type >= GPSPositionReport::FIX_TYPE_2D && fix.fix_type <= GPSPositionReport::FIX_TYPE_RTK_FIXED;
    result.altitudeDatum = GPSObservation::AltitudeDatum::MeanSeaLevel;
    result.monotonicTimestampUs = fix.timestamp != 0 ? fix.timestamp : context.nowUs();
    const auto now = context.nowUs();
    const qint64 age = now >= result.monotonicTimestampUs ? (now - result.monotonicTimestampUs) / 1000 : -1;
    result.receivedAt =
        age >= 0 ? QDateTime::fromMSecsSinceEpoch((context.utcNowUs() + 500) / 1000, QTimeZone::UTC).addMSecs(-age)
                 : QDateTime();
    switch (fix.fix_type) {
        case GPSPositionReport::FIX_TYPE_NONE:
            result.fixQuality = GPSObservation::FixQuality::NoFix;
            break;
        case GPSPositionReport::FIX_TYPE_2D:
            result.fixQuality = GPSObservation::FixQuality::Fix2D;
            break;
        case GPSPositionReport::FIX_TYPE_3D:
            result.fixQuality = GPSObservation::FixQuality::Fix3D;
            break;
        case GPSPositionReport::FIX_TYPE_RTCM_CODE_DIFFERENTIAL:
            result.fixQuality = GPSObservation::FixQuality::Differential;
            break;
        case GPSPositionReport::FIX_TYPE_RTK_FLOAT:
            result.fixQuality = GPSObservation::FixQuality::RTKFloat;
            break;
        case GPSPositionReport::FIX_TYPE_RTK_FIXED:
            result.fixQuality = GPSObservation::FixQuality::RTKFixed;
            break;
        case GPSPositionReport::FIX_TYPE_EXTRAPOLATED:
            result.fixQuality = GPSObservation::FixQuality::Extrapolated;
            break;
        default:
            break;
    }
    if (fix.satellites_used != std::numeric_limits<uint8_t>::max()) {
        result.satellitesUsed = fix.satellites_used;
    }
    if (fix.vel_ned_valid && qIsFinite(fix.s_variance_m_s) && fix.s_variance_m_s >= 0) {
        result.speedAccuracyMetersPerSecond = fix.s_variance_m_s;
    }
    result.dopTimestampUs = fix.dop_timestamp;
    result.headingTimestampUs = fix.heading_timestamp;
    result.accuracyTimestampUs = fix.accuracy_timestamp;
    if (freshMetadata(fix.dop_timestamp, now)) {
        result.horizontalDop = positive(fix.hdop);
        result.verticalDop = positive(fix.vdop);
    }
    if (fix.fix_type >= GPSPositionReport::FIX_TYPE_3D && fix.fix_type <= GPSPositionReport::FIX_TYPE_RTK_FIXED &&
        qIsFinite(fix.altitude_ellipsoid_m)) {
        result.altitudeEllipsoidMeters = fix.altitude_ellipsoid_m;
    }
    if (freshMetadata(fix.heading_timestamp, now) && qIsFinite(fix.heading)) {
        double degrees = std::fmod(qRadiansToDegrees(static_cast<double>(fix.heading)), 360.0);
        result.trueHeadingDegrees = degrees < 0 ? degrees + 360.0 : degrees;
        if (qIsFinite(fix.heading_accuracy) && fix.heading_accuracy >= 0) {
            result.trueHeadingAccuracyDegrees = qRadiansToDegrees(static_cast<double>(fix.heading_accuracy));
        }
    }
    result.integrity.provenance = GPSIntegrityProvenance{
        .jammingTimestampUs = fix.jamming_state_timestamp,
        .spoofingTimestampUs = fix.spoofing_state_timestamp,
        .authenticationTimestampUs = fix.authentication_state_timestamp,
        .correctionsTimestampUs = fix.corrections_timestamp,
        .rfTimestampUs = fix.rf_timestamp,
    };
    result.integrity.noisePerMillisecond = fix.noise_per_ms;
    result.integrity.automaticGainControl = fix.automatic_gain_control;
    result.integrity.jammingIndicator = fix.jamming_indicator;
    result.integrity.correctionsCrcFailed = fix.corrections_crc_failed;
    result.integrity.jammingState = knownState(fix.jamming_state);
    result.integrity.spoofingState = knownState(fix.spoofing_state);
    result.integrity.authenticationState = knownState(fix.authentication_state);
    result.integrity.correctionsProtocol = knownState(fix.corrections_protocol);
    result.integrity.correctionsUsed = knownState(fix.corrections_msg_used);
    return result;
}

GPSSatelliteObservation GPSDriverData::satellites(const GPSSatelliteReport& report, const GPSExecutionContext& context)
{
    GPSSatelliteObservation result;
    result.monotonicTimestampUs = report.timestamp != 0 ? report.timestamp : context.nowUs();
    if (report.constellation) {
        result.updateMode = GPSSatelliteObservation::UpdateMode::ConstellationDelta;
        result.provenance.append({*report.constellation, result.monotonicTimestampUs, 0, std::nullopt});
    }
    const auto count = std::min<size_t>(report.count, report.entries.size());
    for (size_t index = 0; index < count; ++index) {
        const auto& entry = report.entries[index];
        if (!entry.id) {
            continue;
        }
        GPSSatellite satellite;
        satellite.id = entry.id;
        satellite.prn = entry.prn;
        satellite.constellation = entry.constellation;
        satellite.used = entry.used;
        satellite.elevationDegrees = entry.elevation;
        satellite.signalStrength = entry.signal;
        satellite.normalizedAzimuthDegrees = entry.azimuth;
        result.satellites.append(satellite);
    }
    return result;
}

GPSSatelliteObservation GPSDriverData::satellites(const GPSSatelliteUsageReport& report,
                                                  const GPSExecutionContext& context)
{
    GPSSatelliteObservation result;
    result.monotonicTimestampUs = report.timestamp ? report.timestamp : context.nowUs();
    result.updateMode = GPSSatelliteObservation::UpdateMode::ConstellationDelta;
    result.provenance.append({GPSConstellation::Unknown, 0, result.monotonicTimestampUs, report.usedCount});
    return result;
}

GPSRelativeObservation GPSDriverData::relativePosition(const GPSRelativeReport& report,
                                                       const GPSExecutionContext& context)
{
    GPSRelativeObservation result;
    result.monotonicTimestampUs = report.timestamp != 0 ? report.timestamp : context.nowUs();
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
