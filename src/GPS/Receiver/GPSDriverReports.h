#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <limits>
#include <optional>

#include <QtCore/QMetaType>

#include "../GPSConstellation.h"
#include "GPSEllipsoidPosition.h"
#include "GPSSatelliteUsageReport.h"

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

    // Receipt of this diagnostic update, not a freshness timestamp for every retained field.
    uint64_t timestampUs = 0;
    uint64_t jammingTimestampUs = 0;
    uint64_t spoofingTimestampUs = 0;
    uint64_t rfTimestampUs = 0;
    uint64_t correctionTimestampUs = 0;
    JammingState jamming = JammingState::Unknown;
    SpoofingState spoofing = SpoofingState::Unknown;
    CorrectionUse correctionUse = CorrectionUse::Unknown;
    std::optional<int32_t> noisePerMillisecond = std::nullopt;
    std::optional<uint16_t> automaticGainControl = std::nullopt;
    std::optional<int32_t> jammingIndicator = std::nullopt;
    std::optional<bool> correctionCrcFailed = std::nullopt;

    /// Project independent diagnostic groups at the consumer's monotonic time.
    GPSIntegrityReport freshAt(uint64_t nowUs, std::chrono::microseconds maximumAge = std::chrono::seconds(5)) const
    {
        auto result = *this;
        const auto fresh = [nowUs, maximumAge](uint64_t receipt) {
            return receipt && receipt <= nowUs && maximumAge.count() > 0 &&
                   nowUs - receipt < static_cast<uint64_t>(maximumAge.count());
        };
        if (!fresh(jammingTimestampUs)) {
            result.jamming = JammingState::Unknown;
        }
        if (!fresh(spoofingTimestampUs)) {
            result.spoofing = SpoofingState::Unknown;
        }
        if (!fresh(rfTimestampUs)) {
            result.noisePerMillisecond.reset();
            result.automaticGainControl.reset();
            result.jammingIndicator.reset();
        }
        if (!fresh(correctionTimestampUs)) {
            result.correctionUse = CorrectionUse::Unknown;
            result.correctionCrcFailed.reset();
        }
        return result;
    }
};

struct GPSPositionReport
{
    enum class FixType
    {
        Unknown,
        NoFix,
        Fix2D,
        Fix3D,
        Differential,
        RTKFloat,
        RTKFixed,
        Extrapolated = 8
    };

    static constexpr FixType fixTypeFromValue(int value)
    {
        return (value >= static_cast<int>(FixType::Unknown) && value <= static_cast<int>(FixType::RTKFixed)) ||
                       value == static_cast<int>(FixType::Extrapolated)
                   ? static_cast<FixType>(value)
                   : FixType::Unknown;
    }

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
    GPSIntegrityReport integrity;
};
Q_DECLARE_METATYPE(GPSPositionReport)
Q_DECLARE_METATYPE(GPSPositionReport::FixType)

struct GPSSatelliteReport
{
    struct Satellite
    {
        uint16_t id = 0;
        uint16_t prn = 0;
        std::optional<bool> used = std::nullopt;
        std::optional<float> elevationDegrees = std::nullopt;
        std::optional<float> azimuthDegrees = std::nullopt;
        std::optional<uint8_t> signalStrength = std::nullopt;
        GPSConstellation constellation = GPSConstellation::Unknown;
        uint64_t inViewTimestampUs = 0;
        uint64_t inUseTimestampUs = 0;
    };

    static constexpr uint16_t MAX_SATELLITES = 128;
    // Latest accepted view receipt. Zero means no view coverage, not an explicitly empty view.
    uint64_t timestampUs = 0;
    uint16_t count = 0;
    std::array<Satellite, MAX_SATELLITES> satellites{};
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
