#pragma once

#include <chrono>
#include <cstdint>
#include <limits>
#include <optional>

#include <QtCore/QObject>

#include "GPSReceiverConfig.h"
#include "MonotonicClock.h"

namespace GPSFixQualities {
Q_NAMESPACE

enum class GPSFixQuality
{
    Unknown = 0,
    NoFix = 1,
    Fix2D = 2,
    Fix3D = 3,
    Differential = 4,
    RTKFloat = 5,
    RTKFixed = 6,
    Extrapolated = 8,
};
Q_ENUM_NS(GPSFixQuality)

}  // namespace GPSFixQualities

using GPSFixQuality = GPSFixQualities::GPSFixQuality;

[[nodiscard]] constexpr GPSFixQuality gpsFixQualityFromValue(int value)
{
    return (value >= static_cast<int>(GPSFixQuality::Unknown) && value <= static_cast<int>(GPSFixQuality::RTKFixed)) ||
                   value == static_cast<int>(GPSFixQuality::Extrapolated)
               ? static_cast<GPSFixQuality>(value)
               : GPSFixQuality::Unknown;
}

enum class GPSConstellation
{
    Unknown,
    GPS,
    GLONASS,
    Galileo,
    BeiDou,
    QZSS,
    SBAS,
    NavIC
};

struct GPSIntegrityReport
{
    enum class JammingState
    {
        Unknown,
        Ok,
        Warning,
        Critical
    };
    enum class SpoofingState
    {
        Unknown,
        None,
        Indicated,
        Multiple
    };
    /// Ordered by severity, so the worst of several reports is the greatest.
    enum class AntennaState
    {
        Unknown,
        Ok,
        Open,
        Short
    };

    [[nodiscard]] static constexpr JammingState jammingStateFromValue(int value)
    {
        return value >= static_cast<int>(JammingState::Unknown) && value <= static_cast<int>(JammingState::Critical)
                   ? static_cast<JammingState>(value)
                   : JammingState::Unknown;
    }

    [[nodiscard]] static constexpr SpoofingState spoofingStateFromValue(int value)
    {
        return value >= static_cast<int>(SpoofingState::Unknown) && value <= static_cast<int>(SpoofingState::Multiple)
                   ? static_cast<SpoofingState>(value)
                   : SpoofingState::Unknown;
    }

    struct Jamming
    {
        uint64_t timestampUs = 0;
        JammingState state = JammingState::Unknown;
    };

    struct Spoofing
    {
        uint64_t timestampUs = 0;
        SpoofingState state = SpoofingState::Unknown;
    };

    struct Antenna
    {
        uint64_t timestampUs = 0;
        AntennaState state = AntennaState::Unknown;
    };

    // Receipt of this update; the retained diagnostic groups have independent receipts.
    uint64_t timestampUs = 0;
    Jamming jamming{};
    Spoofing spoofing{};
    Antenna antenna{};
    /// Receipt of the latest report that the receiver dropped output its connection could not carry; zero when none.
    /// Consumers decide how long it stays relevant.
    uint64_t outputOverflowUs = 0;

    /// Receipt age at which a diagnostic group reverts to unknown.
    static constexpr std::chrono::microseconds DIAGNOSTIC_MAX_AGE = std::chrono::seconds(5);

    /// Project independent diagnostic groups at the consumer's monotonic time.
    GPSIntegrityReport freshAt(uint64_t nowUs, std::chrono::microseconds maximumAge = DIAGNOSTIC_MAX_AGE) const
    {
        auto result = *this;
        if (!MonotonicClock::fresh(jamming.timestampUs, nowUs, maximumAge)) {
            result.jamming.state = JammingState::Unknown;
        }
        if (!MonotonicClock::fresh(spoofing.timestampUs, nowUs, maximumAge)) {
            result.spoofing.state = SpoofingState::Unknown;
        }
        if (!MonotonicClock::fresh(antenna.timestampUs, nowUs, maximumAge)) {
            result.antenna.state = AntennaState::Unknown;
        }
        return result;
    }
};

struct GPSNavigationValues
{
    using FixType = GPSFixQuality;

    uint64_t timestampUs = 0;
    uint64_t utcTimeUs = 0;
    FixType fixType = FixType::Unknown;
    double latitudeDegrees = std::numeric_limits<double>::quiet_NaN();
    double longitudeDegrees = std::numeric_limits<double>::quiet_NaN();
    double altitudeMslMeters = std::numeric_limits<double>::quiet_NaN();
    double altitudeEllipsoidMeters = std::numeric_limits<double>::quiet_NaN();
    float horizontalAccuracyMeters = std::numeric_limits<float>::quiet_NaN();
    float verticalAccuracyMeters = std::numeric_limits<float>::quiet_NaN();
    float horizontalDop = std::numeric_limits<float>::quiet_NaN();
    float verticalDop = std::numeric_limits<float>::quiet_NaN();
    // Unavailable when the producer rejects its velocity solution, even with a valid position fix.
    float speedMetersPerSecond = std::numeric_limits<float>::quiet_NaN();
    float courseRadians = std::numeric_limits<float>::quiet_NaN();
    std::optional<uint8_t> satellitesUsed = std::nullopt;
};

struct GPSPositionReport
{
    using FixType = GPSNavigationValues::FixType;

    GPSNavigationValues navigation{};
    GPSIntegrityReport integrity{};
};

struct GPSSatelliteReport
{
    // Latest accepted view receipt. Zero plus absent inView means no view coverage.
    uint64_t timestampUs = 0;
    std::optional<int> inView = std::nullopt;
    std::optional<int> used = std::nullopt;

    bool operator==(const GPSSatelliteReport&) const = default;
};

struct GPSSurveyReport
{
    /// Receipt of this status; a status a family buffered keeps its original receipt.
    uint64_t timestampUs = 0;
    GPSEllipsoidPosition position{};
    std::optional<double> meanAccuracyMeters = std::nullopt;
    std::chrono::seconds duration{0};
    bool valid = false;
    bool active = false;
};
