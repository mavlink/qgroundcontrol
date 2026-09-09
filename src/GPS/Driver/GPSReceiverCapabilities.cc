#include "GPSReceiverCapabilities.h"

#include <QtCore/QCoreApplication>

#include <cmath>

#include "GPSDriver.h"
#include "GPSDriverBackend.h"

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
    const std::array<double, 4> values = {static_cast<double>(config.constellationMask),
                                          static_cast<double>(config.dynamicModel),
                                          static_cast<double>(config.outputRateHz), config.headingOffsetDeg};
    for (qsizetype index = 0; index < descriptors.size(); ++index) {
        if (!descriptors[index].accepts(values[index])) {
            return tr("The selected %1 is not supported by this receiver in this mode").arg(descriptors[index].label);
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
    if (kind == QStringLiteral("number")) {
        return true;
    }
    if (std::trunc(value) != value) {
        return false;
    }
    if (kind == QStringLiteral("flags")) {
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
        {QStringLiteral("constellationMask"),
         tr("Constellations"),
         {},
         QStringLiteral("flags"),
         0,
         0,
         static_cast<double>(supportedConstellations),
         {0, 1, 2, 4, 8, 16},
         {tr("Receiver default"), tr("GPS and QZSS"), tr("SBAS"), tr("Galileo"), tr("BeiDou"), tr("GLONASS")},
         constellationSelection},
        {QStringLiteral("dynamicModel"),
         tr("Dynamic model"),
         {},
         QStringLiteral("enum"),
         0,
         0,
         8,
         {0, 2, 3, 4, 5, 6, 7, 8},
         {tr("Default (portable)"), tr("Stationary"), tr("Pedestrian"), tr("Automotive"), tr("Sea"), tr("Airborne 1g"),
          tr("Airborne 2g"), tr("Airborne 4g")},
         baseStation ? Support::Unsupported : dynamicModelSelection},
        {QStringLiteral("outputRateHz"),
         tr("Output rate"),
         QStringLiteral("Hz"),
         QStringLiteral("enum"),
         0,
         0,
         10,
         {0, 1, 2, 5, 10},
         {tr("Receiver default"), QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("5"), QStringLiteral("10")},
         baseStation ? Support::Unsupported : outputRateSelection},
        {QStringLiteral("headingOffsetDeg"),
         tr("Heading offset"),
         QStringLiteral("deg"),
         QStringLiteral("number"),
         5,
         -180,
         180,
         {},
         {},
         baseStation ? Support::Unsupported : headingOffsetSelection},
    };
    result[0].requiredMask = 1;
    for (qsizetype index = result[0].values.size() - 1; index > 0; --index) {
        if ((result[0].values[index] & supportedConstellations) == 0) {
            result[0].values.removeAt(index);
            result[0].labels.removeAt(index);
        }
    }
    return result;
}

QVariantList GPSReceiverCapabilities::settingDescriptors(bool baseStation) const
{
    QVariantList result;
    for (const auto& descriptor : settings(baseStation)) {
        QVariantList values;
        for (int value : descriptor.values) {
            values.append(value);
        }
        result.append(QVariantMap{{QStringLiteral("key"), descriptor.key},
                                  {QStringLiteral("label"), descriptor.label},
                                  {QStringLiteral("units"), descriptor.units},
                                  {QStringLiteral("kind"), descriptor.kind},
                                  {QStringLiteral("defaultValue"), descriptor.defaultValue},
                                  {QStringLiteral("minimum"), descriptor.minimum},
                                  {QStringLiteral("maximum"), descriptor.maximum},
                                  {QStringLiteral("values"), values},
                                  {QStringLiteral("labels"), descriptor.labels},
                                  {QStringLiteral("support"), static_cast<int>(descriptor.support)},
                                  {QStringLiteral("requiredMask"), descriptor.requiredMask},
                                  {QStringLiteral("requiresReconnect"), descriptor.requiresReconnect}});
    }
    return result;
}
