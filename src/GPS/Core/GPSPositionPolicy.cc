#include "GPSPositionPolicy.h"

std::optional<GPSObservation> GPSPositionPolicy::project(const GPSObservation& observation,
                                                         GPSObservation::PositionUse use)
{
    using Use = GPSObservation::PositionUse;
    if (!observation.position.isValid()) {
        return std::nullopt;
    }
    if (use != Use::Diagnostics &&
        (!observation.receiverFixValid.value_or(true) || observation.fixQuality == GPSObservation::FixQuality::NoFix)) {
        return std::nullopt;
    }
    if (use != Use::Diagnostics && use != Use::Gga && !observation.usable()) {
        return std::nullopt;
    }
    GPSObservation accepted = observation;
    switch (use) {
        case Use::Diagnostics:
        case Use::Gga:
            break;
        case Use::GroundStation:
        case Use::Motion:
        case Use::NTRIP:
            accepted.position.setCoordinate(observation.coordinate());
            if (accepted.position.coordinate().type() != QGeoCoordinate::Coordinate3D) {
                accepted.position.removeAttribute(QGeoPositionInfo::VerticalAccuracy);
            }
            break;
        case Use::RemoteID:
            if (observation.altitudeEllipsoidMeters && qIsFinite(*observation.altitudeEllipsoidMeters)) {
                auto coordinate = accepted.position.coordinate();
                coordinate.setAltitude(*observation.altitudeEllipsoidMeters);
                accepted.position.setCoordinate(coordinate);
                accepted.altitudeDatum = GPSAltitudeDatum::Ellipsoid;
            }
            break;
    }
    if (use == Use::Motion) {
        const double course = observation.heading();
        if (qIsFinite(course)) {
            accepted.position.setAttribute(QGeoPositionInfo::Direction, course);
        } else {
            accepted.position.removeAttribute(QGeoPositionInfo::Direction);
            accepted.position.removeAttribute(QGeoPositionInfo::DirectionAccuracy);
        }
    }
    return accepted;
}
