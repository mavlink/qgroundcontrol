#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <limits>
#include <optional>

#include <QtCore/QMetaType>

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

    // Zero means the producer cannot establish the original diagnostic receipt.
    uint64_t timestampUs = 0;
    JammingState jamming = JammingState::Unknown;
    SpoofingState spoofing = SpoofingState::Unknown;
    CorrectionUse correctionUse = CorrectionUse::Unknown;
    std::optional<int32_t> noisePerMillisecond = std::nullopt;
    std::optional<uint16_t> automaticGainControl = std::nullopt;
    std::optional<int32_t> jammingIndicator = std::nullopt;
    std::optional<bool> correctionCrcFailed = std::nullopt;
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
    };

    static constexpr uint16_t MAX_SATELLITES = 128;
    uint64_t timestampUs = 0;
    uint16_t count = 0;
    std::array<Satellite, MAX_SATELLITES> satellites{};
};
Q_DECLARE_METATYPE(GPSSatelliteReport)

struct GPSSurveyReport
{
    double latitudeDegrees = std::numeric_limits<double>::quiet_NaN();
    double longitudeDegrees = std::numeric_limits<double>::quiet_NaN();
    float altitudeEllipsoidMeters = std::numeric_limits<float>::quiet_NaN();
    std::optional<double> meanAccuracyMeters = std::nullopt;
    std::chrono::seconds duration{0};
    bool valid = false;
    bool active = false;
};
