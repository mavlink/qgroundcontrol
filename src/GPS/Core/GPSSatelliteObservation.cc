#include "GPSSatelliteObservation.h"

#include <algorithm>

std::optional<double> GPSSatellite::azimuthDegrees() const
{
    if (normalizedAzimuthDegrees && qIsFinite(*normalizedAzimuthDegrees) && *normalizedAzimuthDegrees >= 0 &&
        *normalizedAzimuthDegrees <= 360) {
        return *normalizedAzimuthDegrees == 360 ? 0 : *normalizedAzimuthDegrees;
    }
    return std::nullopt;
}

int GPSSatelliteObservation::satellitesInViewCount() const
{
    return std::any_of(provenance.cbegin(), provenance.cend(),
                       [](const auto& report) { return report.inViewTimestampUs != 0; })
               ? static_cast<int>(satellites.size())
               : -1;
}

int GPSSatelliteObservation::satellitesInUseCount() const
{
    int count = -1;
    for (const auto& report : provenance) {
        if (report.inUseTimestampUs && report.satellitesUsed) {
            count = std::max(0, count) + *report.satellitesUsed;
        }
    }
    return count;
}
