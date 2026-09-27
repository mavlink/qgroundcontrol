#include "GPSReceiverConfig.h"

#include <cmath>
#include <limits>

#include <QtCore/QCoreApplication>

#include "GPSReceiverCapabilities.h"
#include "GPSReceiverDescriptor.h"

GPSReceiverCapabilities gpsReceiverCapabilities(GPSType type, GPSReceiverConfig::Role role)
{
    if (role != GPSReceiverConfig::Role::RTKBase && role != GPSReceiverConfig::Role::Passive) {
        return {};
    }

    const auto* descriptor = gpsReceiverDescriptor(type);
    if (!descriptor) {
        return {};
    }
    return descriptor->capabilities;
}

GPSReceiverConfigError gpsValidateBaseStationConfig(const GPSBaseStationConfig& config)
{
    constexpr double MAX_UNSIGNED_VALUE = (std::numeric_limits<uint32_t>::max)();
    if (const auto* averaging = std::get_if<GPSBaseStationConfig::ReceiverAveraging>(&config.mode)) {
        if (averaging->maximumDurationSecs < 1 || averaging->maximumDurationSecs > 3600) {
            return GPSReceiverConfigError::InvalidReceiverAveraging;
        }
        return GPSReceiverConfigError::None;
    }
    if (const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&config.mode)) {
        const auto& position = fixed->position;
        const double altitudeCm = static_cast<double>(position.altitudeMeters) * 100.0;
        // Match legacy float conversions before checking wire limits.
        const double accuracyUnits = static_cast<double>((fixed->accuracyMeters * 1000.0f) * 10.0f);
        if (!std::isfinite(position.latitudeDegrees) || std::abs(position.latitudeDegrees) > 90.0 ||
            !std::isfinite(position.longitudeDegrees) || std::abs(position.longitudeDegrees) > 180.0 ||
            !std::isfinite(altitudeCm) || altitudeCm < (std::numeric_limits<int32_t>::min)() ||
            altitudeCm > (std::numeric_limits<int32_t>::max)() || !std::isfinite(accuracyUnits) || accuracyUnits < 0 ||
            accuracyUnits > MAX_UNSIGNED_VALUE) {
            return GPSReceiverConfigError::InvalidFixedBase;
        }
    } else {
        const auto& survey = std::get<GPSBaseStationConfig::SurveyIn>(config.mode);
        const double accuracyUnits = survey.accuracyMeters * 10000.0;
        if (!std::isfinite(accuracyUnits) || accuracyUnits < 1 || accuracyUnits > MAX_UNSIGNED_VALUE ||
            survey.durationSecs < 1 || survey.durationSecs > (std::numeric_limits<uint32_t>::max)()) {
            return GPSReceiverConfigError::InvalidSurveyIn;
        }
    }
    return GPSReceiverConfigError::None;
}

GPSReceiverConfigError gpsValidateReceiverConfig(GPSType type, const GPSReceiverConfig& config)
{
    if (config.role != GPSReceiverConfig::Role::RTKBase && config.role != GPSReceiverConfig::Role::Passive) {
        return GPSReceiverConfigError::InvalidRole;
    }
    const GPSReceiverCapabilities capabilities = gpsReceiverCapabilities(type, config.role);
    if (!capabilities.recognized) {
        return GPSReceiverConfigError::UnknownReceiver;
    }
    if ((config.role == GPSReceiverConfig::Role::RTKBase && !capabilities.rtkBase) ||
        (config.role == GPSReceiverConfig::Role::Passive && !capabilities.passive)) {
        return GPSReceiverConfigError::UnsupportedRole;
    }
    return gpsValidateReceiverPhysicalConfig(config, capabilities);
}

GPSReceiverConfigError gpsValidateReceiverPhysicalConfig(const GPSReceiverConfig& config,
                                                         const GPSReceiverCapabilities& capabilities)
{
    if (config.allowPersistentChanges && !capabilities.persistentConfiguration) {
        return GPSReceiverConfigError::UnsupportedPersistentConfiguration;
    }
    if (config.role == GPSReceiverConfig::Role::RTKBase) {
        if ((std::holds_alternative<GPSBaseStationConfig::ReceiverAveraging>(config.base.mode) &&
             !capabilities.receiverAveraging) ||
            (std::holds_alternative<GPSBaseStationConfig::SurveyIn>(config.base.mode) && !capabilities.surveyIn)) {
            return GPSReceiverConfigError::UnsupportedBaseMode;
        }
        if (config.base.compactObservations && !capabilities.compactObservations) {
            return GPSReceiverConfigError::UnsupportedCompactObservations;
        }
        const GPSReceiverConfigError error = gpsValidateBaseStationConfig(config.base);
        if (error != GPSReceiverConfigError::None) {
            return error;
        }
    }
    if (config.role == GPSReceiverConfig::Role::Passive && config.base != GPSBaseStationConfig{}) {
        return GPSReceiverConfigError::UnsupportedBaseMode;
    }
    if ((config.baudRate != 0 && (config.baudRate < 1200 || config.baudRate > 4000000)) ||
        (config.role == GPSReceiverConfig::Role::Passive && config.baudRate == 0)) {
        return GPSReceiverConfigError::InvalidBaudRate;
    }
    return GPSReceiverConfigError::None;
}

QString gpsReceiverConfigErrorText(GPSReceiverConfigError error)
{
    switch (error) {
        case GPSReceiverConfigError::None:
            return {};
        case GPSReceiverConfigError::UnknownReceiver:
            return QCoreApplication::translate("GPSReceiverConfig", "Unsupported GPS receiver type");
        case GPSReceiverConfigError::InvalidRole:
            return QCoreApplication::translate("GPSReceiverConfig", "Unsupported GPS receiver role");
        case GPSReceiverConfigError::UnsupportedRole:
            return QCoreApplication::translate("GPSReceiverConfig",
                                               "This receiver does not support the requested role");
        case GPSReceiverConfigError::InvalidFixedBase:
            return QCoreApplication::translate("GPSReceiverConfig", "Enter a valid fixed base position and accuracy");
        case GPSReceiverConfigError::InvalidSurveyIn:
            return QCoreApplication::translate("GPSReceiverConfig", "Enter a valid survey-in accuracy and duration");
        case GPSReceiverConfigError::UnsupportedBaseMode:
            return QCoreApplication::translate("GPSReceiverConfig",
                                               "This receiver does not support the selected base mode");
        case GPSReceiverConfigError::InvalidReceiverAveraging:
            return QCoreApplication::translate("GPSReceiverConfig",
                                               "Enter a receiver averaging duration between 1 and 3600 seconds");
        case GPSReceiverConfigError::InvalidBaudRate:
            return QCoreApplication::translate(
                "GPSReceiverConfig", "Select a valid serial baud rate; passive input requires an explicit rate");
        case GPSReceiverConfigError::UnsupportedPersistentConfiguration:
            return QCoreApplication::translate("GPSReceiverConfig",
                                               "This driver does not support persistent receiver configuration");
        case GPSReceiverConfigError::UnsupportedCompactObservations:
            return QCoreApplication::translate("GPSReceiverConfig",
                                               "This receiver cannot send compact (MSM4) RTCM corrections");
    }
    return QCoreApplication::translate("GPSReceiverConfig", "Invalid GPS receiver configuration");
}

QString gpsReceiverConfigError(GPSType type, const GPSReceiverConfig& config)
{
    return gpsReceiverConfigErrorText(gpsValidateReceiverConfig(type, config));
}
