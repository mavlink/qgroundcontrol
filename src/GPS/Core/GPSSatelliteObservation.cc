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
    int count = -1;
    for (const auto& system : constellations) {
        if (system.view.receivedAtUs) {
            count = std::max(0, count) + static_cast<int>(system.view.satellites.size());
        }
    }
    return count;
}

int GPSSatelliteObservation::satellitesInUseCount() const
{
    int count = -1;
    for (const auto& system : constellations) {
        if (system.usage.receivedAtUs && system.usage.count) {
            count = std::max(0, count) + *system.usage.count;
        }
    }
    return count;
}
