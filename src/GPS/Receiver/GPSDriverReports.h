#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <limits>
#include <optional>

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
