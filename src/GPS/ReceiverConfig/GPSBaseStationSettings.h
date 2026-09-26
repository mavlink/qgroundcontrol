#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <utility>

#include <QtCore/QString>

#include "GPSEllipsoidPosition.h"
#include "GPSReceiverConfig.h"
#include "GPSType.h"

struct GPSSurveyReport;

/// Base-station values as the application's settings store them.
struct GPSBaseStationSettings
{
    /// The saved base-mode values; any other value is rejected.
    enum class Mode
    {
        SurveyIn = 0,
        Fixed = 1,
        ReceiverAveraging = 2,
    };

    Mode mode = Mode::SurveyIn;
    GPSEllipsoidPosition fixedPosition{};
    float fixedAccuracyMeters = 0.f;
    double surveyInAccuracyMeters = 0.;
    std::chrono::seconds surveyInMinimumDuration{0};
    std::chrono::seconds averagingMaximumDuration{0};
    bool compactObservations = false;
};

/// A receiver session's base-station mode and the antenna position it reports.
struct GPSBaseStationState
{
    /// Base mode of a configured base; empty for a passive receiver.
    std::optional<GPSBaseStationSettings::Mode> mode;
    /// Antenna position and its accuracy in meters once known; the receiver then reports only a time fix.
    std::optional<std::pair<GPSEllipsoidPosition, double>> position;
    /// Assumed accuracy of a survey-in result that reports none.
    std::optional<double> surveyAccuracyLimitMeters;

    /// A surveying base adopts a valid, located result and otherwise has no position.
    void applySurvey(const GPSSurveyReport& report);
};

/// Builds the receiver request for one connection. @a error, when given, receives the translated diagnostic, which is
/// empty when the receiver supports the request; the request is returned either way.
[[nodiscard]] GPSReceiverConfig gpsReceiverConfigFor(const GPSBaseStationSettings& settings, GPSType type,
                                                     uint32_t baudRate, bool allowPersistentChanges, QString* error);

/// The state a session starts from; a fixed base knows its position before the receiver reports.
[[nodiscard]] GPSBaseStationState gpsBaseStationStateFor(const GPSReceiverConfig& config);
