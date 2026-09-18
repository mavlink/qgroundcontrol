#include "GPSObservation.h"

std::optional<GPSObservation> GPSObservation::projected(PositionUse use) const
{
    if (!position.isValid() || !receiverFixValid.value_or(true) || fixQuality == FixQuality::NoFix ||
        (use != PositionUse::Gga && !usable())) {
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

bool GPSObservation::usable() const
{
    const double accuracy = position.attribute(QGeoPositionInfo::HorizontalAccuracy);
    return receiverFixValid.value_or(true) && fixQuality != FixQuality::NoFix && position.isValid() &&
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
    // Course over ground is unreliable when nearly stationary.
    if (!usable() || !position.hasAttribute(QGeoPositionInfo::Direction) || !qIsFinite(direction) || direction < 0 ||
        direction > 360 || !position.hasAttribute(QGeoPositionInfo::GroundSpeed) || !qIsFinite(speed) || speed < 0.5 ||
        !accuracyAcceptable) {
        return qQNaN();
    }
    return direction == 360 ? 0 : direction;
}
