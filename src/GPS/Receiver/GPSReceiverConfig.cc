#include "GPSReceiverConfig.h"

#include <cmath>
#include <limits>
#include <numbers>

#include "GPSReceiverCapabilities.h"

GPSReceiverCapabilities gpsReceiverCapabilities(GPSType type, GPSReceiverConfig::Role role)
{
    if (role != GPSReceiverConfig::Role::RTKBase && role != GPSReceiverConfig::Role::Position &&
        role != GPSReceiverConfig::Role::Passive) {
        return {};
    }

    GPSReceiverCapabilities capabilities;
    switch (type) {
        case GPSType::ublox:
            // Pre-v27 configuration has no NavIC path; GPS also controls QZSS.
            capabilities.constellationMask = 0x1f;
            capabilities.dynamicModel = role == GPSReceiverConfig::Role::Position;
            break;
        case GPSType::trimble:
        case GPSType::septentrio:
        case GPSType::femto:
        case GPSType::unicore:
        case GPSType::quectel:
        case GPSType::passive:
            break;
        default:
            return {};
    }
    capabilities.recognized = true;
    capabilities.position = type == GPSType::ublox || type == GPSType::unicore || type == GPSType::quectel;
    capabilities.rtkBase = type != GPSType::passive;
    capabilities.surveyIn = capabilities.rtkBase && type != GPSType::unicore;
    capabilities.receiverAveraging = type == GPSType::unicore;
    capabilities.passive = type == GPSType::passive;
    return capabilities;
}

GPSReceiverConfigError gpsValidateBaseStationConfig(const GPSBaseStationConfig& config)
{
    constexpr double MAX_UNSIGNED_VALUE = (std::numeric_limits<uint32_t>::max)();
    if (config.surveyMode != GPSBaseStationConfig::SurveyMode::AccuracyControlled &&
        config.surveyMode != GPSBaseStationConfig::SurveyMode::ReceiverManaged) {
        return GPSReceiverConfigError::UnsupportedBaseMode;
    }
    if (config.surveyMode == GPSBaseStationConfig::SurveyMode::ReceiverManaged) {
        if (config.useFixedBase || config.receiverAveragingDurationSecs < 1 ||
            config.receiverAveragingDurationSecs > 3600) {
            return GPSReceiverConfigError::InvalidReceiverAveraging;
        }
        return GPSReceiverConfigError::None;
    }
    if (config.useFixedBase) {
        const double altitudeCm = static_cast<double>(config.fixedBaseAltitudeMeters) * 100.0;
        // Match legacy float conversions before checking wire limits.
        const double accuracyUnits = static_cast<double>((config.fixedBaseAccuracyMeters * 1000.0f) * 10.0f);
        if (!std::isfinite(config.fixedBaseLatitude) || std::abs(config.fixedBaseLatitude) > 90.0 ||
            !std::isfinite(config.fixedBaseLongitude) || std::abs(config.fixedBaseLongitude) > 180.0 ||
            !std::isfinite(altitudeCm) || altitudeCm < (std::numeric_limits<int32_t>::min)() ||
            altitudeCm > (std::numeric_limits<int32_t>::max)() || !std::isfinite(accuracyUnits) || accuracyUnits < 0 ||
            accuracyUnits > MAX_UNSIGNED_VALUE) {
            return GPSReceiverConfigError::InvalidFixedBase;
        }
    } else {
        const double accuracyUnits = config.surveyInAccMeters * 10000.0;
        if (!std::isfinite(accuracyUnits) || accuracyUnits < 1 || accuracyUnits > MAX_UNSIGNED_VALUE ||
            config.surveyInDurationSecs < 1 || config.surveyInDurationSecs > (std::numeric_limits<uint32_t>::max)()) {
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
    if (config.allowPersistentChanges && type != GPSType::quectel) {
        return GPSReceiverConfigError::UnsupportedPersistentConfiguration;
    }
    if (config.role == GPSReceiverConfig::Role::RTKBase) {
        if (!config.base.useFixedBase &&
            ((config.base.surveyMode == GPSBaseStationConfig::SurveyMode::ReceiverManaged &&
              !capabilities.receiverAveraging) ||
             (config.base.surveyMode == GPSBaseStationConfig::SurveyMode::AccuracyControlled &&
              !capabilities.surveyIn))) {
            return GPSReceiverConfigError::UnsupportedBaseMode;
        }
        const GPSReceiverConfigError error = gpsValidateBaseStationConfig(config.base);
        if (error != GPSReceiverConfigError::None) {
            return error;
        }
    }
    if (config.role == GPSReceiverConfig::Role::Passive &&
        (config.base.useFixedBase || config.base.surveyInAccMeters != 0 || config.base.surveyInDurationSecs != 0 ||
         config.base.surveyMode != GPSBaseStationConfig::SurveyMode::AccuracyControlled)) {
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
    if (config.headingOffsetRadians.has_value()) {
        if (!capabilities.headingOffset) {
            return GPSReceiverConfigError::UnsupportedHeadingOffset;
        }
        const float offset = *config.headingOffsetRadians;
        if (!std::isfinite(offset) || std::abs(offset) > std::numbers::pi_v<float>) {
            return GPSReceiverConfigError::InvalidHeadingOffset;
        }
    }
    return GPSReceiverConfigError::None;
}
