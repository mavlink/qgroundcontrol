#pragma once

#include <cstdint>

#include "GPSReceiverConfig.h"
#include "GPSType.h"

/// Software request support, not a guarantee for every physical receiver model or firmware.
struct GPSReceiverCapabilities
{
    bool recognized = false;
    bool position = false;
    bool rtkBase = false;
    uint32_t constellationMask = 0;
    bool dynamicModel = false;
    bool headingOffset = false;
};

/// Unknown receiver families or roles return no capabilities; known families may not support the requested role.
[[nodiscard]] GPSReceiverCapabilities gpsReceiverCapabilities(GPSType type, GPSReceiverConfig::Role role);
