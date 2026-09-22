#include "GPSReceiverConfig.h"

#include <cmath>
#include <limits>

#include "GPSReceiverCapabilities.h"
#include "GPSReceiverDescriptor.h"

GPSReceiverCapabilities gpsReceiverCapabilities(GPSType type, GPSReceiverConfig::Role role)
{
    if (role != GPSReceiverConfig::Role::RTKBase && role != GPSReceiverConfig::Role::Position &&
        role != GPSReceiverConfig::Role::Passive) {
        return {};
    }

    const auto* descriptor = gpsReceiverDescriptor(type);
    if (!descriptor) {
        return {};
    }
    auto capabilities = descriptor->capabilities;
    capabilities.dynamicModel &= role == GPSReceiverConfig::Role::Position;
    return capabilities;
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
    if (config.role != GPSReceiverConfig::Role::RTKBase && config.role != GPSReceiverConfig::Role::Position &&
        config.role != GPSReceiverConfig::Role::Passive) {
        return GPSReceiverConfigError::InvalidRole;
    }
    const GPSReceiverCapabilities capabilities = gpsReceiverCapabilities(type, config.role);
    if (!capabilities.recognized) {
        return GPSReceiverConfigError::UnknownReceiver;
    }
    if ((config.role == GPSReceiverConfig::Role::Position && !capabilities.position) ||
        (config.role == GPSReceiverConfig::Role::RTKBase && !capabilities.rtkBase) ||
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
    if (config.constellationMask != 0) {
        if (capabilities.constellationMask == 0) {
            return GPSReceiverConfigError::UnsupportedConstellations;
        }
        if ((config.constellationMask & ~capabilities.constellationMask) != 0) {
            return GPSReceiverConfigError::InvalidConstellations;
        }
    }
    if (config.dynamicModel.has_value()) {
        if (!capabilities.dynamicModel) {
            return GPSReceiverConfigError::UnsupportedDynamicModel;
        }
        const int model = *config.dynamicModel;
        if (model != 0 && (model < 2 || model > 8)) {
            return GPSReceiverConfigError::InvalidDynamicModel;
        }
    }
    return GPSReceiverConfigError::None;
}
