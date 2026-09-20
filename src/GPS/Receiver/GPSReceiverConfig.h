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
        /// Only receivers with a verified base-to-rover transition support this role.
        Position,
        /// Receive NMEA/RTCM without issuing receiver configuration commands.
        Passive,
    };

    Role role = Role::RTKBase;
    GPSBaseStationConfig base{};
    /// Zero retains receiver defaults. Bits: GPS=1, SBAS=2, Galileo=4, BeiDou=8, GLONASS=16.
    uint32_t constellationMask = 0;
    /// UBX models 0 and 2..8; an explicit zero is distinct from an absent request.
    std::optional<int> dynamicModel{};
    /// No currently supported role accepts heading offsets.
    std::optional<float> headingOffsetRadians{};
    /// Zero selects managed-driver baud detection; passive serial input requires an explicit rate.
    uint32_t baudRate = 0;
    /// Per-connection consent to persistent receiver settings and the required restart.
    bool allowPersistentChanges = false;
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
    UnsupportedBaseMode,
    InvalidReceiverAveraging,
    InvalidBaudRate,
    UnsupportedPersistentConfiguration,
};

/// Check the selected base mode against the existing receiver wire-unit limits.
[[nodiscard]] GPSReceiverConfigError gpsValidateBaseStationConfig(const GPSBaseStationConfig& config);

/// Precedence: valid role, recognized receiver, supported role, RTK base, constellations, dynamic model, heading
/// offset. For each optional request, unsupported takes precedence over an invalid value.
[[nodiscard]] GPSReceiverConfigError gpsValidateReceiverConfig(GPSType type, const GPSReceiverConfig& config);
