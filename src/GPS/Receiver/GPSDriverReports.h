#pragma once

#include <chrono>
#include <cstdint>
#include <limits>
#include <optional>

#include <QtCore/QMetaType>

#include "GPSEllipsoidPosition.h"
#include "GPSFixQuality.h"
#include "MonotonicClock.h"

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
    enum class CorrectionUse
    {
        Unknown,
        NotUsed,
        Used
    };
    enum class CorrectionProtocol
    {
        Unknown,
        RTCM3,
        SPARTN,
        HAS,
        PMP,
        QZSSL6,
    };

    static constexpr JammingState jammingStateFromValue(int value)
    {
        return value >= static_cast<int>(JammingState::Unknown) && value <= static_cast<int>(JammingState::Critical)
                   ? static_cast<JammingState>(value)
                   : JammingState::Unknown;
    }

    static constexpr SpoofingState spoofingStateFromValue(int value)
    {
        return value >= static_cast<int>(SpoofingState::Unknown) && value <= static_cast<int>(SpoofingState::Multiple)
                   ? static_cast<SpoofingState>(value)
                   : SpoofingState::Unknown;
    }

    static constexpr CorrectionUse correctionUseFromValue(int value)
    {
        return value >= static_cast<int>(CorrectionUse::Unknown) && value <= static_cast<int>(CorrectionUse::Used)
                   ? static_cast<CorrectionUse>(value)
                   : CorrectionUse::Unknown;
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

    struct RF
    {
        uint64_t timestampUs = 0;
        std::optional<int32_t> noisePerMillisecond = std::nullopt;
        std::optional<uint16_t> automaticGainControl = std::nullopt;
        std::optional<int32_t> jammingIndicator = std::nullopt;
    };

    struct Corrections
    {
        uint64_t timestampUs = 0;
        CorrectionUse use = CorrectionUse::Unknown;
        std::optional<bool> crcFailed = std::nullopt;
        CorrectionProtocol protocol = CorrectionProtocol::Unknown;
    };

    // Receipt of this update; the retained diagnostic groups have independent receipts.
    uint64_t timestampUs = 0;
    Jamming jamming{};
    Spoofing spoofing{};
    RF rf{};
    Corrections corrections{};

    /// Receipt age at which a diagnostic group reverts to unknown.
    static constexpr std::chrono::microseconds DIAGNOSTIC_MAX_AGE = std::chrono::seconds(5);

    /// Project independent diagnostic groups at the consumer's monotonic time.
    GPSIntegrityReport freshAt(uint64_t nowUs, std::chrono::microseconds maximumAge = DIAGNOSTIC_MAX_AGE) const
    {
        auto result = *this;
        const auto fresh = [nowUs, maximumAge](uint64_t receipt) {
            return MonotonicClock::remaining(receipt, nowUs, maximumAge) > std::chrono::microseconds::zero();
        };
        if (!fresh(jamming.timestampUs)) {
            result.jamming.state = JammingState::Unknown;
        }
        if (!fresh(spoofing.timestampUs)) {
            result.spoofing.state = SpoofingState::Unknown;
        }
        if (!fresh(rf.timestampUs)) {
            result.rf = RF{.timestampUs = rf.timestampUs};
        }
        if (!fresh(corrections.timestampUs)) {
            result.corrections = Corrections{.timestampUs = corrections.timestampUs};
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
    float headingRadians = std::numeric_limits<float>::quiet_NaN();
    float headingAccuracyRadians = std::numeric_limits<float>::quiet_NaN();
    std::optional<uint8_t> satellitesUsed = std::nullopt;
};

struct GPSPositionReport
{
    using FixType = GPSNavigationValues::FixType;

    GPSNavigationValues navigation{};
    GPSIntegrityReport integrity{};
};
Q_DECLARE_METATYPE(GPSPositionReport)
Q_DECLARE_METATYPE(GPSPositionReport::FixType)

struct GPSSatelliteReport
{
    // Latest accepted view receipt. Zero plus absent inView means no view coverage.
    uint64_t timestampUs = 0;
    std::optional<int> inView = std::nullopt;
    std::optional<int> used = std::nullopt;

    bool operator==(const GPSSatelliteReport&) const = default;
};
Q_DECLARE_METATYPE(GPSSatelliteReport)

struct GPSSurveyReport
{
    GPSEllipsoidPosition position{};
    std::optional<double> meanAccuracyMeters = std::nullopt;
    std::chrono::seconds duration{0};
    bool valid = false;
    bool active = false;
};
Q_DECLARE_METATYPE(GPSSurveyReport)
