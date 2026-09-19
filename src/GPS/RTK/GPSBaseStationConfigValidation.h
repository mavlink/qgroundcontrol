#pragma once

#include <QtCore/QString>

#include "GPSBaseStationConfig.h"
#include "GPSReceiverConfig.h"
#include "GPSType.h"

/// An empty diagnostic means all numeric values fit receiver wire limits.
[[nodiscard]] QString gpsBaseStationConfigError(const GPSBaseStationConfig& config);
[[nodiscard]] QString gpsReceiverConfigError(GPSType type, const GPSReceiverConfig& config);
