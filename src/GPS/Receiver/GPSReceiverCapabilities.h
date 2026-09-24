#pragma once

#include <cstdint>

#include "GPSReceiverConfig.h"
#include "GPSType.h"

/// Software request support, not a guarantee for every physical receiver model or firmware.
struct GPSReceiverCapabilities
{
    bool recognized = false;
    bool rtkBase = false;
    bool surveyIn = false;
    bool receiverAveraging = false;
    bool passive = false;
    bool persistentConfiguration = false;
    bool compactObservations = false;
};

/// Unknown receiver families or roles return no capabilities; known families may not support the requested role.
[[nodiscard]] GPSReceiverCapabilities gpsReceiverCapabilities(GPSType type, GPSReceiverConfig::Role role);
