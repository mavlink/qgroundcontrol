#pragma once

#include <chrono>
#include <limits>
#include <optional>

#include <QtCore/QMetaType>
#include <QtPositioning/QGeoCoordinate>

#include "GPSAltitudeDatum.h"

/// Survey-in progress, translated from the px4 SurveyInStatus.
struct GPSSurveyInStatus
{
    QGeoCoordinate coordinate;
    float altitudeEllipsoidMeters = std::numeric_limits<float>::quiet_NaN();
    std::optional<double> meanAccuracyMeters = std::nullopt;
    std::chrono::seconds duration{0};
    GPSAltitudeDatum altitudeDatum = GPSAltitudeDatum::Unknown;
    quint64 sessionId = 0;
    quint64 monotonicTimestampUs = 0;
    bool valid = false;
    bool active = false;
};
Q_DECLARE_METATYPE(GPSSurveyInStatus)
