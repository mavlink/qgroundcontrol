#include "GPSObservation.h"

#include <algorithm>
#include <cmath>

#include <QtCore/QTimeZone>
#include <QtCore/QtMath>

#include "GPSDriverReports.h"
#include "GPSEllipsoidPosition.h"

GPSObservation GPSObservation::fromNavigation(const GPSNavigationValues& navigation, quint64 receivedAtUs)
{
    GPSObservation observation;
    observation.receivedAt = QDateTime::currentDateTimeUtc();
    observation.monotonicTimestampUs = receivedAtUs;
    observation.fixQuality = navigation.fixType;
    observation.receiverFixValid = navigation.fixType != FixQuality::Unknown && navigation.fixType != FixQuality::NoFix;
    if (qIsFinite(navigation.latitudeDegrees) && qIsFinite(navigation.longitudeDegrees)) {
        QGeoCoordinate coordinate(navigation.latitudeDegrees, navigation.longitudeDegrees);
        if (qIsFinite(navigation.altitudeMslMeters)) {
            coordinate.setAltitude(navigation.altitudeMslMeters);
            observation.altitudeDatum = GPSAltitudeDatum::MeanSeaLevel;
        }
        const QDateTime timestamp =
            navigation.utcTimeUs
                ? QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(navigation.utcTimeUs / 1000), QTimeZone::UTC)
                : observation.receivedAt;
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
    if (qIsFinite(navigation.verticalDop) && navigation.verticalDop > 0) {
        observation.verticalDop = navigation.verticalDop;
    }
    observation.sourceId = QStringLiteral("RTK receiver");
    return observation;
}

GPSObservation GPSObservation::fromSurveyedPosition(const GPSEllipsoidPosition& position, double accuracyMeters,
                                                    quint64 receivedAtUs)
{
    // A declared position with no stated accuracy is still a precise, operator-supplied position.
    constexpr double MINIMUM_ACCURACY_METERS = 0.01;
    GPSObservation observation;
    observation.receivedAt = QDateTime::currentDateTimeUtc();
    observation.monotonicTimestampUs = receivedAtUs;
    observation.fixQuality = FixQuality::Fix3D;
    observation.receiverFixValid = true;
    observation.sourceId = QStringLiteral("RTK base");
    if (qIsFinite(position.latitudeDegrees) && qIsFinite(position.longitudeDegrees) && qIsFinite(accuracyMeters)) {
        // Only ellipsoid height is known, so the ground-station coordinate stays two-dimensional.
        observation.position = QGeoPositionInfo(QGeoCoordinate(position.latitudeDegrees, position.longitudeDegrees),
                                                observation.receivedAt);
        observation.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy,
                                          std::max(accuracyMeters, MINIMUM_ACCURACY_METERS));
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
        case PositionUse::GroundStation:
            accepted.position.setCoordinate(coordinate());
            if (accepted.position.coordinate().type() != QGeoCoordinate::Coordinate3D) {
                accepted.position.removeAttribute(QGeoPositionInfo::VerticalAccuracy);
            }
            break;
        case PositionUse::RemoteID:
            if (altitudeEllipsoidMeters && qIsFinite(*altitudeEllipsoidMeters)) {
                auto ellipsoidCoordinate = accepted.position.coordinate();
                ellipsoidCoordinate.setAltitude(*altitudeEllipsoidMeters);
                accepted.position.setCoordinate(ellipsoidCoordinate);
                accepted.altitudeDatum = GPSAltitudeDatum::Ellipsoid;
            } else if (altitudeDatum != GPSAltitudeDatum::Ellipsoid ||
                       !qIsFinite(accepted.position.coordinate().altitude())) {
                auto horizontalCoordinate = accepted.position.coordinate();
                horizontalCoordinate.setAltitude(qQNaN());
                accepted.position.setCoordinate(horizontalCoordinate);
                accepted.position.removeAttribute(QGeoPositionInfo::VerticalAccuracy);
                accepted.altitudeDatum = GPSAltitudeDatum::Unknown;
            }
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
    const double accuracy = position.attribute(QGeoPositionInfo::HorizontalAccuracy);
    return hasNavigationSolution() && position.hasAttribute(QGeoPositionInfo::HorizontalAccuracy) &&
           qIsFinite(accuracy) && accuracy > 0 && accuracy <= 100;
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
    // Course over ground is unreliable when nearly stationary.
    if (!usable() || !position.hasAttribute(QGeoPositionInfo::Direction) || !qIsFinite(direction) || direction < 0 ||
        direction > 360 || !position.hasAttribute(QGeoPositionInfo::GroundSpeed) || !qIsFinite(speed) || speed < 0.5 ||
        !accuracyAcceptable) {
        return qQNaN();
    }
    return direction == 360 ? 0 : direction;
}
