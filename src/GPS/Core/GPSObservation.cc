#include "GPSObservation.h"

bool GPSObservation::usable() const
{
    const double accuracy = position.attribute(QGeoPositionInfo::HorizontalAccuracy);
    return receiverFixValid.value_or(true) && fixQuality != FixQuality::NoFix && position.isValid() &&
           qIsFinite(accuracy) && accuracy > 0 && accuracy <= 100;
}

QGeoCoordinate GPSObservation::coordinate() const
{
    if (!usable()) {
        return {};
    }
    QGeoCoordinate coordinate(position.coordinate().latitude(), position.coordinate().longitude());
    const double accuracy = position.attribute(QGeoPositionInfo::VerticalAccuracy);
    if (qIsFinite(accuracy) && accuracy > 0 && accuracy <= 10 && qIsFinite(position.coordinate().altitude())) {
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
    if (!usable() || !qIsFinite(direction) || direction < 0 || direction > 360 || !qIsFinite(speed) || speed < 0.5 ||
        !accuracyAcceptable) {
        return qQNaN();
    }
    return direction == 360 ? 0 : direction;
}

QGeoPositionInfo GPSObservation::acceptedPosition() const
{
    if (!usable()) {
        return {};
    }
    QGeoPositionInfo accepted = position;
    accepted.setCoordinate(coordinate());
    if (accepted.coordinate().type() != QGeoCoordinate::Coordinate3D) {
        accepted.removeAttribute(QGeoPositionInfo::VerticalAccuracy);
    }
    return accepted;
}
