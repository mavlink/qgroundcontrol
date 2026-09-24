#include "GPSSatelliteObservation.h"

#include <algorithm>

int GPSSatelliteObservation::satellitesInViewCount() const
{
    int count = -1;
    for (const auto& system : constellations) {
        if (system.view.receivedAtUs) {
            count = std::max(0, count) + system.view.count;
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
