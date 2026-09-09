#include "GPSBaseReferenceSave.h"

#include <QtCore/QCoreApplication>

#include <cmath>

GPSBaseReferenceSave::Result GPSBaseReferenceSave::prepare(const GPSBaseReference& reference, quint64 currentSession,
                                                           const GPSReceiverConfig& settings, quint64 nowUs)
{
    const auto tr = [](const char* text) { return QCoreApplication::translate("GPSBaseReferenceSave", text); };
    if (!reference.isValid() || !currentSession || reference.observation.sessionId != currentSession ||
        !reference.observation.monotonicTimestampUs || reference.observation.monotonicTimestampUs > nowUs) {
        return {std::nullopt, tr("No valid base reference is available for the current receiver session")};
    }
    if (settings.role != GPSReceiverConfig::Role::RTKBase || !settings.base.useFixedBase) {
        return {std::nullopt, tr("Select fixed base mode before saving the base reference")};
    }
    const auto coordinate = reference.observation.position.coordinate();
    std::optional<double> ellipsoidAltitude;
    if (reference.observation.altitudeDatum == GPSObservation::AltitudeDatum::Ellipsoid) {
        ellipsoidAltitude = coordinate.altitude();
    } else if (reference.observation.altitudeDatum == GPSObservation::AltitudeDatum::MeanSeaLevel) {
        ellipsoidAltitude = reference.observation.altitudeEllipsoidMeters;
    }
    if (!ellipsoidAltitude || !std::isfinite(*ellipsoidAltitude)) {
        return {std::nullopt, tr("The receiver has not supplied a known WGS84 ellipsoid altitude")};
    }
    GPSReceiverConfig configuration = settings;
    configuration.base.fixedBaseLatitude = coordinate.latitude();
    configuration.base.fixedBaseLongitude = coordinate.longitude();
    configuration.base.fixedBaseAltitudeMeters = static_cast<float>(*ellipsoidAltitude);
    if (reference.accuracyMeters) {
        configuration.base.fixedBaseAccuracyMeters = static_cast<float>(*reference.accuracyMeters);
    }
    const auto error = configuration.validationError();
    return error.isEmpty() ? Result{configuration, {}} : Result{std::nullopt, error};
}
