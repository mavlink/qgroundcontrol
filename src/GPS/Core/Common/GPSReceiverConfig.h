#pragma once

#include <chrono>
#include <cstdint>
#include <limits>
#include <variant>

#include <QtCore/QFlags>
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

/// A constellation besides GPS whose observations a base sends in its corrections. Callers persist these values, so
/// they must not change.
enum class GPSCorrectionConstellation : uint8_t
{
    GLONASS = 1 << 0,
    Galileo = 1 << 1,
    BeiDou = 1 << 2,
    QZSS = 1 << 3,
};
Q_DECLARE_FLAGS(GPSCorrectionConstellations, GPSCorrectionConstellation)
Q_DECLARE_OPERATORS_FOR_FLAGS(GPSCorrectionConstellations)

inline constexpr GPSCorrectionConstellations GPS_ALL_CORRECTION_CONSTELLATIONS =
    GPSCorrectionConstellation::GLONASS | GPSCorrectionConstellation::Galileo | GPSCorrectionConstellation::BeiDou |
    GPSCorrectionConstellation::QZSS;

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
        /// Where the antenna's reference point is, which is what a base announces in RTCM 1005 and 1006. The caller
        /// adds the height an antenna has above a surveyed mark.
        GPSEllipsoidPosition position{};
        float accuracyMeters = 0.0f;
        bool operator==(const Fixed&) const = default;
    };

    struct ReceiverAveraging
    {
        /// The limits of maximumDuration.
        static constexpr std::chrono::seconds MIN_DURATION{1};
        static constexpr std::chrono::seconds MAX_DURATION = std::chrono::hours{1};

        /// Maximum receiver-managed time, not a minimum duration or an accuracy guarantee.
        std::chrono::seconds maximumDuration{60};
        bool operator==(const ReceiverAveraging&) const = default;
    };

    using Mode = std::variant<SurveyIn, Fixed, ReceiverAveraging>;
    Mode mode = SurveyIn{};
    /// MSM4 instead of MSM7 observations: about a third less correction bandwidth, without Doppler.
    bool compactObservations = false;
    /// The constellations besides GPS whose observations the corrections carry; those of GPS always are. It selects
    /// the messages a base sends, not the satellites its receiver tracks. A family sends those of them it has messages
    /// for (GPSReceiverCapabilities::correctionConstellations names the ones it can leave out).
    GPSCorrectionConstellations correctionConstellations = GPS_ALL_CORRECTION_CONSTELLATIONS;

    /// The constellations of GPS_ALL_CORRECTION_CONSTELLATIONS whose observations are left out.
    [[nodiscard]] constexpr GPSCorrectionConstellations omittedConstellations() const
    {
        return GPS_ALL_CORRECTION_CONSTELLATIONS & ~correctionConstellations;
    }

    bool operator==(const GPSBaseStationConfig&) const = default;
};

/// One connection's receiver request. GPSType::Passive receives NMEA/RTCM without issuing receiver configuration
/// commands; every other type configures an RTK base.
struct GPSReceiverConfig
{
    GPSBaseStationConfig base{};
    /// Zero finds the receiver's rate: a configurable receiver is probed for it, passive serial input listened for.
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
    UnsupportedCorrectionConstellations,
};

/// The range of explicit serial rates a receiver connection accepts.
inline constexpr uint32_t GPS_MIN_BAUD_RATE = 1200;
inline constexpr uint32_t GPS_MAX_BAUD_RATE = 4000000;

/// An explicit serial rate within the receivers' range. Zero, which asks for the receiver's rate to be found, is not
/// one.
[[nodiscard]] constexpr bool gpsValidExplicitBaudRate(uint32_t baudRate)
{
    return baudRate >= GPS_MIN_BAUD_RATE && baudRate <= GPS_MAX_BAUD_RATE;
}

/// Check the selected base mode against the existing receiver wire-unit limits.
[[nodiscard]] GPSReceiverConfigError gpsValidateBaseStationConfig(const GPSBaseStationConfig& config);

/// Precedence: recognized receiver, persistent permission, base mode, baud rate.
[[nodiscard]] GPSReceiverConfigError gpsValidateReceiverConfig(GPSType type, const GPSReceiverConfig& config);

/// Translated diagnostic; empty for GPSReceiverConfigError::None.
[[nodiscard]] QString gpsReceiverConfigErrorText(GPSReceiverConfigError error);
/// An empty diagnostic means the receiver supports the role and all supplied settings.
[[nodiscard]] QString gpsReceiverConfigError(GPSType type, const GPSReceiverConfig& config);

/// Fits a GPSType::Automatic request to the family detection found. Consent to persistent changes is a permission, so
/// it lapses where the family has no persistent configuration; a compact-observation request lapses where the family
/// has no compact option, which then sends its standard observations, and @a compactFallback, when given, reports that.
/// A request that leaves constellations out of the corrections lapses likewise where the family always sends its own
/// set, which @a constellationFallback reports. Any other option the family lacks is kept, for
/// gpsDetectedReceiverConfigError() to report.
[[nodiscard]] GPSReceiverConfig gpsReceiverConfigForDetected(GPSType detected, GPSReceiverConfig config,
                                                             bool* compactFallback = nullptr,
                                                             bool* constellationFallback = nullptr);

/// Why the @a detected family cannot perform @a config, naming the family and the option, such as "Detected
/// Septentrio receiver does not support receiver-managed averaging"; empty when it can.
[[nodiscard]] QString gpsDetectedReceiverConfigError(GPSType detected, const GPSReceiverConfig& config);
