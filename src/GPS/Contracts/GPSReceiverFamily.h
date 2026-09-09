#pragma once

#include <QtCore/QLatin1StringView>

#include <array>
#include <span>

#include "GPSReceiverCapabilities.h"

struct GPSReceiverFamily
{
    GPSType type;
    QLatin1StringView name;
    std::array<QLatin1StringView, 3> aliases;
    int manufacturerId;
    GPSReceiverCapabilities::Support baseSupport;
    GPSReceiverCapabilities::Support nmeaSupport;
    GPSReceiverCapabilities::Support correctionInput;
    GPSReceiverCapabilities::Support constellationSelection = GPSReceiverCapabilities::Support::Unsupported;
    GPSReceiverCapabilities::Support dynamicModelSelection = GPSReceiverCapabilities::Support::Unsupported;
    GPSReceiverCapabilities::Support outputRateSelection = GPSReceiverCapabilities::Support::Unsupported;
    GPSReceiverCapabilities::Support headingOffsetSelection = GPSReceiverCapabilities::Support::Unsupported;
};

std::span<const GPSReceiverFamily> gpsReceiverFamilies();
