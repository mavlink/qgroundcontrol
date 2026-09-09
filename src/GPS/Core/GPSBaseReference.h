#pragma once

#include "GPSObservation.h"

/// A surveyed or fixed reference remains valid only for its owning receiver session.
struct GPSBaseReference
{
    GPSObservation observation;
    std::optional<double> accuracyMeters = std::nullopt;
    bool valid = false;

    bool isValid() const { return valid && observation.sessionId != 0 && observation.position.isValid(); }
};
Q_DECLARE_METATYPE(GPSBaseReference)
