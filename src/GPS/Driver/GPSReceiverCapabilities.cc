#include "GPSReceiverCapabilities.h"

#include <QtCore/QCoreApplication>

#include <array>

#include "GPSDriver.h"

namespace {
struct ReceiverFamily
{
    GPSType type;
    QLatin1StringView name;
    std::array<QLatin1StringView, 3> aliases;
    int manufacturerId;
    GPSReceiverCapabilities::Support baseSupport;
    GPSReceiverCapabilities::Support nmeaSupport;
};

using Support = GPSReceiverCapabilities::Support;
constexpr std::array families{
    ReceiverFamily{GPSType::u_blox,
                   QLatin1StringView("U-blox"),
                   {QLatin1StringView("blox"), QLatin1StringView("ubx"), QLatin1StringView("u-blox")},
                   4,
                   Support::Unknown,
                   Support::Supported},
    ReceiverFamily{GPSType::trimble,
                   QLatin1StringView("Trimble"),
                   {QLatin1StringView("trimble"), QLatin1StringView("ashtech"), QLatin1StringView("spectra")},
                   1,
                   Support::Supported,
                   Support::Unsupported},
    ReceiverFamily{GPSType::septentrio,
                   QLatin1StringView("Septentrio"),
                   {QLatin1StringView("septentrio"), QLatin1StringView("sbf"), QLatin1StringView()},
                   2,
                   Support::Supported,
                   Support::Unsupported},
    ReceiverFamily{GPSType::femto,
                   QLatin1StringView("Femtomes"),
                   {QLatin1StringView("femtomes"), QLatin1StringView("femto"), QLatin1StringView()},
                   3,
                   Support::Supported,
                   Support::Unsupported},
};
}  // namespace

GPSReceiverCapabilities GPSReceiverCapabilities::forType(GPSType type)
{
    GPSReceiverCapabilities result;
    result.type = type;
    for (const ReceiverFamily& family : families) {
        if (family.type == type) {
            result.name = family.name;
            result.manufacturerId = family.manufacturerId;
            result.nativePosition = Support::Supported;
            result.rtkBase = family.baseSupport;
            result.nmeaOutput = family.nmeaSupport;
            break;
        }
    }
    return result;
}

std::optional<GPSType> GPSReceiverCapabilities::typeForName(QStringView name)
{
    for (const ReceiverFamily& family : families) {
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
