#pragma once

#include <QtCore/QString>

#include "GPSBaseStationConfig.h"

/// An empty diagnostic means all numeric values fit receiver wire limits.
[[nodiscard]] QString gpsBaseStationConfigError(const GPSBaseStationConfig& config);
