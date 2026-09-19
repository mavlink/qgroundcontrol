#pragma once

#include <QtCore/QString>

#include "GPSBaseStationConfig.h"
#include "GPSReceiverConfig.h"
#include "GPSType.h"

/// An empty diagnostic means all numeric values fit receiver wire limits.
[[nodiscard]] QString gpsBaseStationConfigError(const GPSBaseStationConfig& config);
/// An empty diagnostic means the receiver supports the role and all supplied settings.
[[nodiscard]] QString gpsReceiverConfigError(GPSType type, const GPSReceiverConfig& config);
