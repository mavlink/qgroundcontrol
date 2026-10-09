#include "GPSReceiverConfig.h"

#include <cmath>
#include <limits>

#include <QtCore/QCoreApplication>

#include "GPSProtocolMath.h"
#include "GPSReceiverDescriptor.h"

GPSReceiverConfigError gpsValidateBaseStationConfig(const GPSBaseStationConfig& config)
{
    if (const auto* averaging = std::get_if<GPSBaseStationConfig::ReceiverAveraging>(&config.mode)) {
        if (averaging->maximumDuration < std::chrono::seconds{1} ||
            averaging->maximumDuration > std::chrono::hours{1}) {
            return GPSReceiverConfigError::InvalidReceiverAveraging;
        }
        return GPSReceiverConfigError::None;
    }
    if (const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&config.mode)) {
        const auto& position = fixed->position;
        const double altitudeCm = position.altitudeMeters * 100.0;
        if (!std::isfinite(position.latitudeDegrees) || std::abs(position.latitudeDegrees) > 90.0 ||
            !std::isfinite(position.longitudeDegrees) || std::abs(position.longitudeDegrees) > 180.0 ||
            !std::isfinite(altitudeCm) || altitudeCm < (std::numeric_limits<int32_t>::min)() ||
            altitudeCm > (std::numeric_limits<int32_t>::max)() ||
            !GPSProtocolMath::tenthMillimeters(fixed->accuracyMeters)) {
            return GPSReceiverConfigError::InvalidFixedBase;
        }
    } else {
        const auto& survey = std::get<GPSBaseStationConfig::SurveyIn>(config.mode);
        const auto accuracyUnits = GPSProtocolMath::tenthMillimeters(survey.accuracyMeters);
        if (!accuracyUnits || *accuracyUnits < 1 || survey.duration < std::chrono::seconds{1} ||
            survey.duration > std::chrono::seconds((std::numeric_limits<uint32_t>::max)())) {
            return GPSReceiverConfigError::InvalidSurveyIn;
        }
    }
    return GPSReceiverConfigError::None;
}

namespace {

/// The options @a config requests, for a family qualified for its role.
GPSReceiverConfigError validateOptions(const GPSReceiverConfig& config, const GPSReceiverCapabilities& capabilities,
                                       bool passive)
{
    if (config.allowPersistentChanges && !capabilities.persistentConfiguration) {
        return GPSReceiverConfigError::UnsupportedPersistentConfiguration;
    }
    if (!passive) {
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
    if (passive && config.base != GPSBaseStationConfig{}) {
        return GPSReceiverConfigError::UnsupportedBaseMode;
    }
    if (!gpsValidBaudRate(config.baudRate, passive)) {
        return GPSReceiverConfigError::InvalidBaudRate;
    }
    return GPSReceiverConfigError::None;
}

}  // namespace

GPSReceiverConfigError gpsValidateReceiverConfig(GPSType type, const GPSReceiverConfig& config)
{
    const GPSReceiverCapabilities capabilities = gpsReceiverCapabilities(type);
    if (!capabilities.recognized) {
        return GPSReceiverConfigError::UnknownReceiver;
    }
    return validateOptions(config, capabilities, type == GPSType::passive);
}

QString gpsReceiverConfigErrorText(GPSReceiverConfigError error)
{
    switch (error) {
        case GPSReceiverConfigError::None:
            return {};
        case GPSReceiverConfigError::UnknownReceiver:
            return QCoreApplication::translate("GPSReceiverConfig", "Unsupported GPS receiver type");
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

GPSReceiverConfig gpsReceiverConfigForDetected(GPSType detected, GPSReceiverConfig config, bool* compactFallback)
{
    const GPSReceiverCapabilities capabilities = gpsReceiverCapabilities(detected);
    config.allowPersistentChanges = config.allowPersistentChanges && capabilities.persistentConfiguration;
    const bool fallback = config.base.compactObservations && !capabilities.compactObservations;
    if (fallback) {
        config.base.compactObservations = false;
    }
    if (compactFallback) {
        *compactFallback = fallback;
    }
    return config;
}

QString gpsDetectedReceiverConfigError(GPSType detected, const GPSReceiverConfig& config)
{
    const GPSReceiverConfigError error = gpsValidateReceiverConfig(detected, config);
    if (error == GPSReceiverConfigError::None) {
        return {};
    }
    const QString name = gpsReceiverName(detected);
    const QString receiver = name.isEmpty() ? QCoreApplication::translate("GPSReceiverConfig", "unknown") : name;
    if (error == GPSReceiverConfigError::UnsupportedBaseMode) {
        const QString mode = std::holds_alternative<GPSBaseStationConfig::ReceiverAveraging>(config.base.mode)
                                 ? QCoreApplication::translate("GPSReceiverConfig", "receiver-managed averaging")
                             : std::holds_alternative<GPSBaseStationConfig::SurveyIn>(config.base.mode)
                                 ? QCoreApplication::translate("GPSReceiverConfig", "survey-in")
                                 : QCoreApplication::translate("GPSReceiverConfig", "a fixed base position");
        //: %1 is a receiver manufacturer, such as Septentrio; %2 is a base mode, such as survey-in
        return QCoreApplication::translate("GPSReceiverConfig", "Detected %1 receiver does not support %2")
            .arg(receiver, mode);
    }
    //: %1 is a receiver manufacturer; %2 explains the unsupported request
    return QCoreApplication::translate("GPSReceiverConfig", "Detected %1 receiver: %2")
        .arg(receiver, gpsReceiverConfigErrorText(error));
}
