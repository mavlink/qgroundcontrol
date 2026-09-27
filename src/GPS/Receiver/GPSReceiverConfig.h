#pragma once

#include <cstdint>

#include <QtCore/QString>

#include "GPSBaseStationConfig.h"
#include "GPSType.h"

struct GPSReceiverCapabilities;

/// Receiver requests supported by the wrapper; absent options retain wrapper defaults.
struct GPSReceiverConfig
{
    enum class Role
    {
        RTKBase,
        /// Receive NMEA/RTCM without issuing receiver configuration commands.
        Passive,
    };

    Role role = Role::RTKBase;
    GPSBaseStationConfig base{};
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
    UnsupportedBaseMode,
    InvalidReceiverAveraging,
    InvalidBaudRate,
    UnsupportedPersistentConfiguration,
    UnsupportedCompactObservations,
};

/// Check the selected base mode against the existing receiver wire-unit limits.
[[nodiscard]] GPSReceiverConfigError gpsValidateBaseStationConfig(const GPSBaseStationConfig& config);

/// Physical option validation only; does not qualify a receiver family for a public role.
[[nodiscard]] GPSReceiverConfigError gpsValidateReceiverPhysicalConfig(const GPSReceiverConfig& config,
                                                                       const GPSReceiverCapabilities& capabilities);

/// Precedence: valid role, recognized receiver, supported role, persistent permission, base mode, baud rate.
[[nodiscard]] GPSReceiverConfigError gpsValidateReceiverConfig(GPSType type, const GPSReceiverConfig& config);

/// Translated diagnostic; empty for GPSReceiverConfigError::None.
[[nodiscard]] QString gpsReceiverConfigErrorText(GPSReceiverConfigError error);
/// An empty diagnostic means the receiver supports the role and all supplied settings.
[[nodiscard]] QString gpsReceiverConfigError(GPSType type, const GPSReceiverConfig& config);
