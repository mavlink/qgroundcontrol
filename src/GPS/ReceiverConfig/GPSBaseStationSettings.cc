#include "GPSBaseStationSettings.h"

#include <cmath>
#include <limits>

#include <QtCore/QCoreApplication>

#include "GPSDriverReports.h"
#include "GPSReceiverCapabilities.h"

void GPSBaseStationState::applySurvey(const GPSSurveyReport& report)
{
    if (!mode || *mode == GPSBaseStationSettings::Mode::Fixed) {
        return;
    }
    const bool located =
        std::isfinite(report.position.latitudeDegrees) && std::isfinite(report.position.longitudeDegrees);
    const double accuracy = report.meanAccuracyMeters.value_or(
        surveyAccuracyLimitMeters.value_or(std::numeric_limits<double>::quiet_NaN()));
    if (report.valid && located && std::isfinite(accuracy)) {
        position = std::pair(report.position, accuracy);
    } else {
        position.reset();
    }
}

GPSReceiverConfig gpsReceiverConfigFor(const GPSBaseStationSettings& settings, GPSType type, uint32_t baudRate,
                                       bool allowPersistentChanges, QString* error)
{
    GPSReceiverConfig config{.baudRate = baudRate, .allowPersistentChanges = allowPersistentChanges};
    const auto diagnosed = [&config, error](QString diagnostic) {
        if (error) {
            *error = std::move(diagnostic);
        }
        return config;
    };
    if (type == GPSType::passive) {
        config.role = GPSReceiverConfig::Role::Passive;
        return diagnosed(gpsReceiverConfigError(type, config));
    }
    switch (settings.mode) {
        case GPSBaseStationSettings::Mode::Fixed:
            config.base.mode = GPSBaseStationConfig::Fixed{
                .position = settings.fixedPosition,
                .accuracyMeters = settings.fixedAccuracyMeters,
            };
            break;
        case GPSBaseStationSettings::Mode::SurveyIn:
            config.base.mode = GPSBaseStationConfig::SurveyIn{
                .accuracyMeters = settings.surveyInAccuracyMeters,
                .duration = settings.surveyInMinimumDuration,
            };
            break;
        case GPSBaseStationSettings::Mode::ReceiverAveraging:
            config.base.mode =
                GPSBaseStationConfig::ReceiverAveraging{.maximumDuration = settings.averagingMaximumDuration};
            break;
        default:
            return diagnosed(QCoreApplication::translate("GPSBaseStationSettings", "Select a supported base mode."));
    }
    // The option is hidden for receivers that cannot send MSM4, so it never blocks their connection.
    config.base.compactObservations =
        settings.compactObservations && gpsReceiverCapabilities(type, config.role).compactObservations;
    return diagnosed(gpsReceiverConfigError(type, config));
}

GPSBaseStationState gpsBaseStationStateFor(const GPSReceiverConfig& config)
{
    GPSBaseStationState state;
    if (config.role == GPSReceiverConfig::Role::Passive) {
        return state;
    }
    if (const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&config.base.mode)) {
        state.mode = GPSBaseStationSettings::Mode::Fixed;
        state.position = std::pair(fixed->position, static_cast<double>(fixed->accuracyMeters));
    } else if (const auto* survey = std::get_if<GPSBaseStationConfig::SurveyIn>(&config.base.mode)) {
        state.mode = GPSBaseStationSettings::Mode::SurveyIn;
        state.surveyAccuracyLimitMeters = survey->accuracyMeters;
    } else {
        state.mode = GPSBaseStationSettings::Mode::ReceiverAveraging;
    }
    return state;
}
