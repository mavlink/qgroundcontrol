#include "GPSObservation.h"

#include <algorithm>
#include <cmath>

#include <QtCore/QDateTime>
#include <QtCore/QTimeZone>
#include <QtCore/QtMath>

#include "GPSReceiverConfig.h"
#include "GPSReceiverReports.h"
#include "QGCGeoid.h"

namespace {

/// The height of mean sea level above the ellipsoid at @a coordinate; empty for an invalid coordinate.
std::optional<double> egm96Separation(const QGeoCoordinate& coordinate)
{
    const double separation = QGCGeo::geoidSeparation(coordinate.latitude(), coordinate.longitude());
    return qIsFinite(separation) ? std::optional<double>(separation) : std::nullopt;
}

/// The altitude of @a observation when its datum is @a datum.
std::optional<double> altitudeIn(const GPSObservation& observation, GPSAltitudeDatum datum)
{
    const double altitude = observation.position.coordinate().altitude();
    return observation.altitudeDatum == datum && qIsFinite(altitude) ? std::optional<double>(altitude) : std::nullopt;
}

/// The height above the ellipsoid that @a observation has without a conversion.
std::optional<double> reportedEllipsoidHeight(const GPSObservation& observation)
{
    const std::optional<double> height = observation.altitudeEllipsoidMeters;
    return height && qIsFinite(*height) ? height : altitudeIn(observation, GPSAltitudeDatum::Ellipsoid);
}

}  // namespace

GPSObservation GPSObservation::fromNavigation(const GPSNavigationValues& navigation, quint64 receivedAtUs)
{
    GPSObservation observation;
    observation.monotonicTimestampUs = receivedAtUs;
    observation.fixQuality = navigation.fixType;
    observation.receiverFixValid = navigation.fixType != FixQuality::Unknown && navigation.fixType != FixQuality::NoFix;
    if (qIsFinite(navigation.latitudeDegrees) && qIsFinite(navigation.longitudeDegrees)) {
        QGeoCoordinate coordinate(navigation.latitudeDegrees, navigation.longitudeDegrees);
        if (qIsFinite(navigation.altitudeMslMeters)) {
            coordinate.setAltitude(navigation.altitudeMslMeters);
            observation.altitudeDatum = GPSAltitudeDatum::MeanSeaLevel;
        } else if (qIsFinite(navigation.altitudeEllipsoidMeters)) {
            coordinate.setAltitude(navigation.altitudeEllipsoidMeters);
            observation.altitudeDatum = GPSAltitudeDatum::Ellipsoid;
        }
        const QDateTime timestamp =
            navigation.utcTimeUs
                ? QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(navigation.utcTimeUs / 1000), QTimeZone::UTC)
                : QDateTime::currentDateTimeUtc();
        observation.position = QGeoPositionInfo(coordinate, timestamp);
    }
    const auto setAttribute = [&observation](QGeoPositionInfo::Attribute attribute, double value, bool allowZero) {
        if (qIsFinite(value) && (value > 0 || (allowZero && value == 0))) {
            observation.position.setAttribute(attribute, value);
        }
    };
    // Receivers without a metric estimate, such as NMEA without GST, fall back to DOP.
    const auto accuracy = [](float meters, float dop) {
        return qIsFinite(meters) && meters > 0 ? meters : (qIsFinite(dop) && dop > 0 ? accuracyFromDop(dop) : qQNaN());
    };
    setAttribute(QGeoPositionInfo::HorizontalAccuracy,
                 accuracy(navigation.horizontalAccuracyMeters, navigation.horizontalDop), false);
    setAttribute(QGeoPositionInfo::VerticalAccuracy,
                 accuracy(navigation.verticalAccuracyMeters, navigation.verticalDop), false);
    setAttribute(QGeoPositionInfo::GroundSpeed, navigation.speedMetersPerSecond, true);
    if (qIsFinite(navigation.courseRadians)) {
        const double degrees = std::fmod(qRadiansToDegrees(static_cast<double>(navigation.courseRadians)), 360.0);
        observation.position.setAttribute(QGeoPositionInfo::Direction, degrees < 0 ? degrees + 360.0 : degrees);
    }
    if (qIsFinite(navigation.altitudeEllipsoidMeters)) {
        observation.altitudeEllipsoidMeters = navigation.altitudeEllipsoidMeters;
    }
    if (navigation.satellitesUsed) {
        observation.satellitesUsed = static_cast<int>(*navigation.satellitesUsed);
    }
    if (qIsFinite(navigation.horizontalDop) && navigation.horizontalDop > 0) {
        observation.horizontalDop = navigation.horizontalDop;
    }
    return observation;
}

GPSObservation GPSObservation::fromSurveyedPosition(const GPSEllipsoidPosition& position, double accuracyMeters,
                                                    quint64 receivedAtUs)
{
    // A declared position with no stated accuracy is still a precise, operator-supplied position.
    constexpr double MINIMUM_ACCURACY_METERS = 0.01;
    GPSObservation observation;
    observation.monotonicTimestampUs = receivedAtUs;
    observation.fixQuality = FixQuality::Fix3D;
    observation.receiverFixValid = true;
    if (qIsFinite(position.latitudeDegrees) && qIsFinite(position.longitudeDegrees) && qIsFinite(accuracyMeters)) {
        const double accuracy = (std::max) (accuracyMeters, MINIMUM_ACCURACY_METERS);
        const bool hasHeight = qIsFinite(position.altitudeMeters);
        QGeoCoordinate coordinate(position.latitudeDegrees, position.longitudeDegrees);
        if (hasHeight) {
            // A base is positioned by its height above the ellipsoid, as accurately as horizontally.
            coordinate.setAltitude(position.altitudeMeters);
            observation.altitudeDatum = GPSAltitudeDatum::Ellipsoid;
        }
        observation.position = QGeoPositionInfo(coordinate, QDateTime::currentDateTimeUtc());
        observation.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, accuracy);
        if (hasHeight) {
            observation.position.setAttribute(QGeoPositionInfo::VerticalAccuracy, accuracy);
        }
    }
    if (qIsFinite(position.altitudeMeters)) {
        observation.altitudeEllipsoidMeters = position.altitudeMeters;
    }
    return observation;
}

double GPSObservation::accuracyFromDop(double dop)
{
    constexpr double USER_EQUIVALENT_RANGE_ERROR_METERS = 5.1;
    constexpr double CONFIDENCE_SCALE = 2.0;
    return dop * USER_EQUIVALENT_RANGE_ERROR_METERS * CONFIDENCE_SCALE;
}

std::optional<GPSObservation> GPSObservation::projected(PositionUse use) const
{
    if (!hasNavigationSolution() || (use != PositionUse::Gga && !usable())) {
        return std::nullopt;
    }
    GPSObservation accepted = *this;
    switch (use) {
        case PositionUse::Gga:
        case PositionUse::Motion:
            break;
        case PositionUse::GroundStation: {
            QGeoCoordinate groundStation = coordinate();
            if (groundStation.type() != QGeoCoordinate::Coordinate3D) {
                // An altitude that is not accurate enough is withheld in either datum.
                accepted.position.removeAttribute(QGeoPositionInfo::VerticalAccuracy);
                accepted.altitudeEllipsoidMeters.reset();
            } else if (const auto altitudeMsl = altitudeMslMeters()) {
                groundStation.setAltitude(*altitudeMsl);
                accepted.altitudeDatum = GPSAltitudeDatum::MeanSeaLevel;
            }
            accepted.position.setCoordinate(groundStation);
            break;
        }
        case PositionUse::RemoteID:
            if (const auto ellipsoidHeight = ellipsoidHeightMeters()) {
                auto ellipsoidCoordinate = accepted.position.coordinate();
                ellipsoidCoordinate.setAltitude(*ellipsoidHeight);
                accepted.position.setCoordinate(ellipsoidCoordinate);
                accepted.altitudeDatum = GPSAltitudeDatum::Ellipsoid;
            } else if (!qIsFinite(accepted.position.coordinate().altitude())) {
                auto horizontalCoordinate = accepted.position.coordinate();
                horizontalCoordinate.setAltitude(qQNaN());
                accepted.position.setCoordinate(horizontalCoordinate);
                accepted.position.removeAttribute(QGeoPositionInfo::VerticalAccuracy);
                accepted.altitudeDatum = GPSAltitudeDatum::Unknown;
            }
            // Otherwise the altitude is of unknown datum, which the consumer checks before calling it geodetic.
            break;
    }
    if (use == PositionUse::Motion) {
        const double course = heading();
        if (qIsFinite(course)) {
            accepted.position.setAttribute(QGeoPositionInfo::Direction, course);
        } else {
            accepted.position.removeAttribute(QGeoPositionInfo::Direction);
            accepted.position.removeAttribute(QGeoPositionInfo::DirectionAccuracy);
        }
    }
    return accepted;
}

bool GPSObservation::hasNavigationSolution() const
{
    return position.isValid() && receiverFixValid.value_or(true) && fixQuality != FixQuality::NoFix;
}

bool GPSObservation::usable() const
{
    constexpr double MAXIMUM_HORIZONTAL_ACCURACY_METERS = 100.0;
    const double accuracy = position.attribute(QGeoPositionInfo::HorizontalAccuracy);
    return hasNavigationSolution() && position.hasAttribute(QGeoPositionInfo::HorizontalAccuracy) &&
           qIsFinite(accuracy) && accuracy > 0 && accuracy <= MAXIMUM_HORIZONTAL_ACCURACY_METERS;
}

QGeoCoordinate GPSObservation::coordinate() const
{
    constexpr double MAXIMUM_VERTICAL_ACCURACY_METERS = 10.0;
    if (!usable()) {
        return {};
    }
    QGeoCoordinate coordinate(position.coordinate().latitude(), position.coordinate().longitude());
    const double accuracy = position.attribute(QGeoPositionInfo::VerticalAccuracy);
    if (position.hasAttribute(QGeoPositionInfo::VerticalAccuracy) && qIsFinite(accuracy) && accuracy > 0 &&
        accuracy <= MAXIMUM_VERTICAL_ACCURACY_METERS && qIsFinite(position.coordinate().altitude())) {
        coordinate.setAltitude(position.coordinate().altitude());
    }
    return coordinate;
}

std::optional<double> GPSObservation::altitudeMslMeters() const
{
    if (const auto altitude = altitudeIn(*this, GPSAltitudeDatum::MeanSeaLevel)) {
        return altitude;
    }
    const std::optional<double> ellipsoidHeight = reportedEllipsoidHeight(*this);
    const std::optional<double> separation = egm96Separation(position.coordinate());
    return ellipsoidHeight && separation ? std::optional<double>(*ellipsoidHeight - *separation) : std::nullopt;
}

std::optional<double> GPSObservation::ellipsoidHeightMeters() const
{
    if (const auto ellipsoidHeight = reportedEllipsoidHeight(*this)) {
        return ellipsoidHeight;
    }
    const std::optional<double> altitude = altitudeIn(*this, GPSAltitudeDatum::MeanSeaLevel);
    const std::optional<double> separation = egm96Separation(position.coordinate());
    return altitude && separation ? std::optional<double>(*altitude + *separation) : std::nullopt;
}

std::optional<double> GPSObservation::geoidSeparationMeters() const
{
    const std::optional<double> altitude = altitudeIn(*this, GPSAltitudeDatum::MeanSeaLevel);
    const std::optional<double> ellipsoidHeight = reportedEllipsoidHeight(*this);
    if (altitude && ellipsoidHeight) {
        return *ellipsoidHeight - *altitude;
    }
    return egm96Separation(position.coordinate());
}

double GPSObservation::heading() const
{
    constexpr double MAXIMUM_DIRECTION_ACCURACY_DEGREES = 30.0;
    constexpr double MINIMUM_GROUND_SPEED_METERS_PER_SECOND = 0.5;
    const double direction = position.attribute(QGeoPositionInfo::Direction);
    const double speed = position.attribute(QGeoPositionInfo::GroundSpeed);
    const double accuracy = position.attribute(QGeoPositionInfo::DirectionAccuracy);
    const bool accuracyAcceptable =
        !position.hasAttribute(QGeoPositionInfo::DirectionAccuracy) ||
        (qIsFinite(accuracy) && accuracy >= 0 && accuracy <= MAXIMUM_DIRECTION_ACCURACY_DEGREES);
    // Course over ground is unreliable when nearly stationary.
    if (!usable() || !position.hasAttribute(QGeoPositionInfo::Direction) || !qIsFinite(direction) || direction < 0 ||
        direction > 360 || !position.hasAttribute(QGeoPositionInfo::GroundSpeed) || !qIsFinite(speed) ||
        speed < MINIMUM_GROUND_SPEED_METERS_PER_SECOND || !accuracyAcceptable) {
        return qQNaN();
    }
    return direction == 360 ? 0 : direction;
}
