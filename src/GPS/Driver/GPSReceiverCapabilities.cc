#include "GPSReceiverCapabilities.h"

#include <QtCore/QCoreApplication>

#include <cmath>

#include "GPSDriverBackend.h"
#include "GPSReceiverConfig.h"

GPSReceiverCapabilities GPSReceiverCapabilities::forType(GPSType type)
{
    GPSReceiverCapabilities result;
    result.type = type;
    for (const GPSDriverFamily& family : gpsDriverFamilies()) {
        if (family.type == type) {
            result.name = family.name;
            result.manufacturerId = family.manufacturerId;
            result.nativePosition = Support::Supported;
            result.rtkBase = family.baseSupport;
            result.nmeaOutput = family.nmeaSupport;
            result.correctionInput = family.correctionInput;
            result.constellationSelection = family.constellationSelection;
            result.dynamicModelSelection = family.dynamicModelSelection;
            result.outputRateSelection = family.outputRateSelection;
            result.headingOffsetSelection = family.headingOffsetSelection;
            break;
        }
    }
    return result;
}

std::optional<GPSType> GPSReceiverCapabilities::typeForName(QStringView name)
{
    for (const GPSDriverFamily& family : gpsDriverFamilies()) {
        for (const QLatin1StringView alias : family.aliases) {
            if (!alias.isEmpty() && name.contains(alias, Qt::CaseInsensitive)) {
                return family.type;
            }
        }
    }
    return std::nullopt;
}

QString GPSReceiverCapabilities::validationError(const GPSReceiverConfig& config) const
{
    const auto tr = [](const char* text) { return QCoreApplication::translate("GPSReceiverCapabilities", text); };
    if (!recognized()) {
        return tr("Select a supported receiver type");
    }
    switch (config.role) {
        case GPSReceiverConfig::Role::RTKBase:
            if (rtkBase == Support::Unsupported) {
                return tr("This receiver does not support RTK base mode");
            }
            break;
        case GPSReceiverConfig::Role::Position:
            if (nativePosition == Support::Unsupported) {
                return tr("This receiver does not support position mode");
            }
            break;
        default:
            return tr("Select a valid receiver role");
    }
    switch (config.outputProtocol) {
        case GPSReceiverConfig::OutputProtocol::Native:
            break;
        case GPSReceiverConfig::OutputProtocol::NMEA:
            if (config.role != GPSReceiverConfig::Role::Position || nmeaOutput == Support::Unsupported) {
                return tr("This receiver does not support configuring NMEA output in the selected mode");
            }
            break;
        default:
            return tr("Select a valid receiver output protocol");
    }
    const auto descriptors = settings(config.role == GPSReceiverConfig::Role::RTKBase);
    for (const auto& descriptor : descriptors) {
        if (!descriptor.accepts(GPSReceiverSettings::value(descriptor.id, config))) {
            return tr("The selected %1 is not supported by this receiver in this mode").arg(descriptor.label);
        }
    }
    return {};
}

bool GPSReceiverCapabilities::SettingDescriptor::accepts(double value) const
{
    if (!std::isfinite(value) || value < minimum || value > maximum) {
        return false;
    }
    if (value == defaultValue) {
        return true;
    }
    if (support == Support::Unsupported) {
        return false;
    }
    if (kind == Kind::Number) {
        return true;
    }
    if (std::trunc(value) != value) {
        return false;
    }
    if (kind == Kind::Flags) {
        int mask = 0;
        for (int flag : values) {
            mask |= flag;
        }
        return (static_cast<int>(value) & ~mask) == 0 && (static_cast<int>(value) & requiredMask) == requiredMask;
    }
    return values.contains(static_cast<int>(value));
}

QList<GPSReceiverCapabilities::SettingDescriptor> GPSReceiverCapabilities::settings(bool baseStation) const
{
    const auto tr = [](const char* text) { return QCoreApplication::translate("GPSReceiverCapabilities", text); };
    QList<SettingDescriptor> result = {
        {GPSReceiverSetting::ConstellationMask,
         tr("Constellations"),
         {},
         SettingDescriptor::Kind::Flags,
         0,
         0,
         static_cast<double>(supportedConstellations),
         {0, 1, 2, 4, 8, 16},
         {tr("Receiver default"), tr("GPS and QZSS"), tr("SBAS"), tr("Galileo"), tr("BeiDou"), tr("GLONASS")},
         constellationSelection},
        {GPSReceiverSetting::DynamicModel,
         tr("Dynamic model"),
         {},
         SettingDescriptor::Kind::Enum,
         0,
         0,
         8,
         {0, 2, 3, 4, 5, 6, 7, 8},
         {tr("Default (portable)"), tr("Stationary"), tr("Pedestrian"), tr("Automotive"), tr("Sea"), tr("Airborne 1g"),
          tr("Airborne 2g"), tr("Airborne 4g")},
         baseStation ? Support::Unsupported : dynamicModelSelection},
        {GPSReceiverSetting::OutputRateHz,
         tr("Output rate"),
         QStringLiteral("Hz"),
         SettingDescriptor::Kind::Enum,
         0,
         0,
         10,
         {0, 1, 2, 5, 10},
         {tr("Receiver default"), QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("5"), QStringLiteral("10")},
         baseStation ? Support::Unsupported : outputRateSelection},
        {GPSReceiverSetting::HeadingOffsetDeg,
         tr("Heading offset"),
         QStringLiteral("deg"),
         SettingDescriptor::Kind::Number,
         5,
         -180,
         180,
         {},
         {},
         baseStation ? Support::Unsupported : headingOffsetSelection},
    };
    for (auto& descriptor : result) {
        if (descriptor.id != GPSReceiverSetting::ConstellationMask) {
            continue;
        }
        descriptor.requiredMask = 1;
        for (qsizetype index = descriptor.values.size() - 1; index > 0; --index) {
            if ((descriptor.values[index] & supportedConstellations) == 0) {
                descriptor.values.removeAt(index);
                descriptor.labels.removeAt(index);
            }
        }
    }
    return result;
}
