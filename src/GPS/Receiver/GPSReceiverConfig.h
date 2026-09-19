#pragma once

#include <cstdint>
#include <optional>

#include "GPSBaseStationConfig.h"
#include "GPSType.h"

/// Receiver requests supported by the wrapper; absent options retain wrapper defaults.
struct GPSReceiverConfig
{
    enum class Role
    {
        RTKBase,
        /// Currently supported only by u-blox; other backends cannot safely leave base mode.
        Position,
    };

    Role role = Role::RTKBase;
    GPSBaseStationConfig base{};
    /// Zero retains receiver defaults. Bits: GPS=1, SBAS=2, Galileo=4, BeiDou=8, GLONASS=16.
    uint32_t constellationMask = 0;
    /// UBX models 0 and 2..8; an explicit zero is distinct from an absent request.
    std::optional<int> dynamicModel{};
    /// No currently supported role accepts heading offsets.
    std::optional<float> headingOffsetRadians{};
};

enum class GPSReceiverConfigError
{
    None,
    UnknownReceiver,
    InvalidRole,
    UnsupportedRole,
    InvalidSurveyIn,
    InvalidFixedBase,
    UnsupportedConstellations,
    InvalidConstellations,
    UnsupportedDynamicModel,
    InvalidDynamicModel,
    UnsupportedHeadingOffset,
    InvalidHeadingOffset,
};

/// Check the selected base mode against the existing receiver wire-unit limits.
[[nodiscard]] GPSReceiverConfigError gpsValidateBaseStationConfig(const GPSBaseStationConfig& config);

/// Precedence: valid role, recognized receiver, supported role, RTK base, constellations, dynamic model, heading
/// offset. For each optional request, unsupported takes precedence over an invalid value.
[[nodiscard]] GPSReceiverConfigError gpsValidateReceiverConfig(GPSType type, const GPSReceiverConfig& config);
