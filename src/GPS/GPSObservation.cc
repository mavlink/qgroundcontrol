#include "GPSObservation.h"

#include <algorithm>
#include <chrono>

bool GPSObservation::usable() const
{
    const double accuracy = position.attribute(QGeoPositionInfo::HorizontalAccuracy);
    return fixQuality != FixQuality::NoFix && position.isValid() &&
           position.hasAttribute(QGeoPositionInfo::HorizontalAccuracy) && qIsFinite(accuracy) && accuracy > 0 &&
           accuracy <= 100;
}

QGeoCoordinate GPSObservation::coordinate() const
{
    if (!usable()) {
        return {};
    }
    QGeoCoordinate coordinate(position.coordinate().latitude(), position.coordinate().longitude());
    const double accuracy = position.attribute(QGeoPositionInfo::VerticalAccuracy);
    if (position.hasAttribute(QGeoPositionInfo::VerticalAccuracy) && qIsFinite(accuracy) && accuracy > 0 &&
        accuracy <= 10 && qIsFinite(position.coordinate().altitude())) {
        coordinate.setAltitude(position.coordinate().altitude());
    }
    return coordinate;
}

double GPSObservation::heading() const
{
    const double direction = position.attribute(QGeoPositionInfo::Direction);
    const double speed = position.attribute(QGeoPositionInfo::GroundSpeed);
    const double accuracy = position.attribute(QGeoPositionInfo::DirectionAccuracy);
    const bool accuracyAcceptable = !position.hasAttribute(QGeoPositionInfo::DirectionAccuracy) ||
                                    (qIsFinite(accuracy) && accuracy >= 0 && accuracy <= 30);
    // Both decoders report course over ground, which is unreliable when nearly stationary.
    if (!usable() || !position.hasAttribute(QGeoPositionInfo::Direction) || !qIsFinite(direction) || direction < 0 ||
        direction > 360 || !position.hasAttribute(QGeoPositionInfo::GroundSpeed) || !qIsFinite(speed) || speed < 0.5 ||
        !accuracyAcceptable) {
        return qQNaN();
    }
    return direction == 360 ? 0 : direction;
}

QGeoPositionInfo GPSObservation::acceptedPosition(PositionUse use) const
{
    if (!usable()) {
        return {};
    }
    QGeoPositionInfo accepted = position;
    switch (use) {
        case PositionUse::GroundStation:
        case PositionUse::Motion:
        case PositionUse::NTRIP:
            accepted.setCoordinate(coordinate());
            if (accepted.coordinate().type() != QGeoCoordinate::Coordinate3D) {
                accepted.removeAttribute(QGeoPositionInfo::VerticalAccuracy);
            }
            break;
        case PositionUse::RemoteID:
            // Operator reports may use a measured altitude even when its accuracy is unknown.
            // Preserve legacy source altitude when no measured ellipsoid altitude is available.
            if (altitudeEllipsoidMeters && qIsFinite(*altitudeEllipsoidMeters)) {
                QGeoCoordinate coordinate = accepted.coordinate();
                coordinate.setAltitude(*altitudeEllipsoidMeters);
                accepted.setCoordinate(coordinate);
            }
            break;
    }
    if (use == PositionUse::Motion) {
        const double course = heading();
        if (qIsFinite(course)) {
            accepted.setAttribute(QGeoPositionInfo::Direction, course);
        } else {
            accepted.removeAttribute(QGeoPositionInfo::Direction);
            accepted.removeAttribute(QGeoPositionInfo::DirectionAccuracy);
        }
    }
    return accepted;
}

quint64 GPSObservation::monotonicNowUs()
{
    return static_cast<quint64>(
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

qint64 GPSObservation::ageMilliseconds(quint64 timestampUs)
{
    if (timestampUs == 0) {
        return 0;
    }
    const quint64 now = monotonicNowUs();
    return timestampUs > now ? -1 : static_cast<qint64>((now - timestampUs) / 1000);
}

qint64 GPSObservation::ageMilliseconds() const
{
    return ageMilliseconds(monotonicTimestampUs);
}

int GPSSatelliteObservation::usedCount() const
{
    return static_cast<int>(std::count_if(satellites.cbegin(), satellites.cend(),
                                          [](const GPSSatellite& satellite) { return satellite.used.value_or(false); }));
}

std::optional<double> GPSSatellite::azimuthDegrees() const
{
    if (normalizedAzimuthDegrees && qIsFinite(*normalizedAzimuthDegrees) && *normalizedAzimuthDegrees >= 0 &&
        *normalizedAzimuthDegrees <= 360) {
        return *normalizedAzimuthDegrees == 360 ? 0 : *normalizedAzimuthDegrees;
    }
    if (!rawAzimuth || *rawAzimuth < 0 || *rawAzimuth > 255 ||
        azimuthEncoding != AzimuthEncoding::ScaledFullCircleByte) {
        return std::nullopt;
    }
    return *rawAzimuth == 255 ? 0.0 : *rawAzimuth * 360.0 / 255.0;
}
