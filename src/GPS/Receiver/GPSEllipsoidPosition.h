#pragma once

#include <limits>

/// WGS84 geodetic position with ellipsoid height, not mean-sea-level altitude.
struct GPSEllipsoidPosition
{
    double latitudeDegrees = std::numeric_limits<double>::quiet_NaN();
    double longitudeDegrees = std::numeric_limits<double>::quiet_NaN();
    float altitudeMeters = std::numeric_limits<float>::quiet_NaN();

    bool operator==(const GPSEllipsoidPosition&) const = default;
};
