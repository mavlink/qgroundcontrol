#pragma once

#include <chrono>
#include <cstdint>
#include <limits>
#include <variant>

#include <QtCore/QString>

#include "GPSType.h"

/// WGS84 geodetic position with ellipsoid height, not mean-sea-level altitude.
struct GPSEllipsoidPosition
{
    double latitudeDegrees = std::numeric_limits<double>::quiet_NaN();
    double longitudeDegrees = std::numeric_limits<double>::quiet_NaN();
    double altitudeMeters = std::numeric_limits<double>::quiet_NaN();

    bool operator==(const GPSEllipsoidPosition&) const = default;
};

/// Configuration used only by the RTK base-station role.
struct GPSBaseStationConfig
{
    struct SurveyIn
    {
        double accuracyMeters = 0.0;
        std::chrono::seconds duration{0};
        bool operator==(const SurveyIn&) const = default;
    };

    struct Fixed
    {
        GPSEllipsoidPosition position{};
        float accuracyMeters = 0.0f;
        bool operator==(const Fixed&) const = default;
    };

    struct ReceiverAveraging
    {
        /// Maximum receiver-managed time, not a minimum duration or an accuracy guarantee.
        std::chrono::seconds maximumDuration{60};
        bool operator==(const ReceiverAveraging&) const = default;
    };

    using Mode = std::variant<SurveyIn, Fixed, ReceiverAveraging>;
    Mode mode = SurveyIn{};
    /// MSM4 instead of MSM7 observations: about a third less correction bandwidth, without Doppler.
    bool compactObservations = false;

    bool operator==(const GPSBaseStationConfig&) const = default;
};

/// One connection's receiver request. GPSType::passive receives NMEA/RTCM without issuing receiver configuration
/// commands; every other type configures an RTK base.
struct GPSReceiverConfig
{
    GPSBaseStationConfig base{};
    /// Zero lets a configurable receiver detect the rate; passive serial input requires an explicit rate.
    uint32_t baudRate = 0;
    /// Per-connection consent to persistent receiver settings and the required restart.
    bool allowPersistentChanges = false;
};

enum class GPSReceiverConfigError
{
    None,
    UnknownReceiver,
    InvalidSurveyIn,
    InvalidFixedBase,
    UnsupportedBaseMode,
    InvalidReceiverAveraging,
    InvalidBaudRate,
    UnsupportedPersistentConfiguration,
    UnsupportedCompactObservations,
};

/// An explicit serial rate within the receivers' range, or zero for a configurable receiver to detect the rate.
/// A passive receiver is never configured, so it needs its existing rate.
[[nodiscard]] constexpr bool gpsValidBaudRate(uint32_t baudRate, bool passive)
{
    return baudRate == 0 ? !passive : baudRate >= 1200 && baudRate <= 4000000;
}

/// Check the selected base mode against the existing receiver wire-unit limits.
[[nodiscard]] GPSReceiverConfigError gpsValidateBaseStationConfig(const GPSBaseStationConfig& config);

/// Precedence: recognized receiver, persistent permission, base mode, baud rate.
[[nodiscard]] GPSReceiverConfigError gpsValidateReceiverConfig(GPSType type, const GPSReceiverConfig& config);

/// Translated diagnostic; empty for GPSReceiverConfigError::None.
[[nodiscard]] QString gpsReceiverConfigErrorText(GPSReceiverConfigError error);
/// An empty diagnostic means the receiver supports the role and all supplied settings.
[[nodiscard]] QString gpsReceiverConfigError(GPSType type, const GPSReceiverConfig& config);

/// Fits a GPSType::automatic request to the family detection found. Consent to persistent changes is a permission, so
/// it lapses where the family has no persistent configuration; a compact-observation request lapses where the family
/// has no compact option, which then sends its standard observations, and @a compactFallback, when given, reports that.
/// Any other option the family lacks is kept, for gpsDetectedReceiverConfigError() to report.
[[nodiscard]] GPSReceiverConfig gpsReceiverConfigForDetected(GPSType detected, GPSReceiverConfig config,
                                                             bool* compactFallback = nullptr);

/// Why the @a detected family cannot perform @a config, naming the family and the option, such as "Detected
/// Septentrio receiver does not support receiver-managed averaging"; empty when it can.
[[nodiscard]] QString gpsDetectedReceiverConfigError(GPSType detected, const GPSReceiverConfig& config);
