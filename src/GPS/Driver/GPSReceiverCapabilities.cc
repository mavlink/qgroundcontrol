#include "GPSReceiverCapabilities.h"

#include <QtCore/QCoreApplication>

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
    return {};
}
