#pragma once

#include <QtCore/QMetaType>
#include <QtPositioning/QGeoCoordinate>

#include <chrono>
#include <limits>
#include <optional>
#include <variant>

/// Receiver families QGC can drive using the native protocol drivers.
enum class GPSReceiverType
{
    ublox = 0,
    trimble = 1,
    septentrio = 2,
    femto = 3,
};

enum class GPSConnectionError
{
    None = 0,
    OpenFailed = 1,
    ConfigFailed = 2,
    DeviceError = 3,
};
Q_DECLARE_METATYPE(GPSConnectionError)

struct GPSSurveyInConfig
{
    double accuracyMeters = 0.0;
    std::chrono::seconds minimumDuration{0};
};

struct GPSFixedBaseConfig
{
    QGeoCoordinate coordinate;
    // RTK uses ellipsoid height; QGeoCoordinate altitude denotes height above sea level.
    float altitudeEllipsoidMeters = std::numeric_limits<float>::quiet_NaN();
    float accuracyMeters = 0.0f;
};

/// Configuration used only by the RTK base-station role; callers must supply valid settings for the selected mode.
struct GPSReceiverConfig
{
    std::variant<GPSSurveyInConfig, GPSFixedBaseConfig> base = GPSSurveyInConfig{};
};

/// Survey-in progress, translated from the px4 SurveyInStatus.
struct GPSSurveyInStatus
{
    QGeoCoordinate coordinate;
    float altitudeEllipsoidMeters = std::numeric_limits<float>::quiet_NaN();
    std::optional<double> meanAccuracyMeters = std::nullopt;
    std::chrono::seconds duration{0};
    bool valid = false;
    bool active = false;
};
Q_DECLARE_METATYPE(GPSSurveyInStatus)
