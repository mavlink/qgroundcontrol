#pragma once

#include <QtCore/QString>

#include "GPSReceiverConfig.h"

/// Stable identities shared by validation, command reporting, and presentation.
enum class GPSReceiverSetting
{
    Unknown,
    ConstellationMask,
    DynamicModel,
    OutputRateHz,
    HeadingOffsetDeg,
};

namespace GPSReceiverSettings {
inline QString key(GPSReceiverSetting setting)
{
    switch (setting) {
        case GPSReceiverSetting::Unknown:
            return {};
        case GPSReceiverSetting::ConstellationMask:
            return QStringLiteral("constellationMask");
        case GPSReceiverSetting::DynamicModel:
            return QStringLiteral("dynamicModel");
        case GPSReceiverSetting::OutputRateHz:
            return QStringLiteral("outputRateHz");
        case GPSReceiverSetting::HeadingOffsetDeg:
            return QStringLiteral("headingOffsetDeg");
    }
    return {};
}

inline double value(GPSReceiverSetting setting, const GPSReceiverConfig& config)
{
    switch (setting) {
        case GPSReceiverSetting::Unknown:
            return qQNaN();
        case GPSReceiverSetting::ConstellationMask:
            return config.constellationMask;
        case GPSReceiverSetting::DynamicModel:
            return config.dynamicModel;
        case GPSReceiverSetting::OutputRateHz:
            return config.outputRateHz;
        case GPSReceiverSetting::HeadingOffsetDeg:
            return config.headingOffsetDeg;
    }
    return qQNaN();
}
}  // namespace GPSReceiverSettings
