#pragma once

#include <optional>

#include "GPSObservation.h"

namespace GPSPositionPolicy {
/// Projects receiver data; the owner separately enforces freshness and session.
std::optional<GPSObservation> project(const GPSObservation& observation, GPSObservation::PositionUse use);
}  // namespace GPSPositionPolicy
