#include "GPSReceiverCapabilities.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QLatin1StringView>

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

#include "GPSProtocolFeatures.h"
#include "GPSReceiverConfig.h"

namespace {
struct GPSReceiverFamily
{
    GPSType type;
    QLatin1StringView name;
    std::array<QLatin1StringView, 3> aliases;
    int manufacturerId;
    GPSReceiverCapabilities::Support baseSupport;
    GPSReceiverCapabilities::Support nmeaSupport;
    GPSReceiverCapabilities::Support correctionInput;
    GPSReceiverCapabilities::Support constellationSelection = GPSReceiverCapabilities::Support::Unsupported;
    GPSReceiverCapabilities::Support dynamicModelSelection = GPSReceiverCapabilities::Support::Unsupported;
    GPSReceiverCapabilities::Support outputRateSelection = GPSReceiverCapabilities::Support::Unsupported;
    GPSReceiverCapabilities::Support headingOffsetSelection = GPSReceiverCapabilities::Support::Unsupported;
};

using Support = GPSReceiverCapabilities::Support;
constexpr std::array<GPSReceiverFamily,
                     QGC_GPS_ENABLE_UBX + QGC_GPS_ENABLE_ASHTECH + QGC_GPS_ENABLE_SBF + QGC_GPS_ENABLE_FEMTO>
    families = {
#if QGC_GPS_ENABLE_UBX
        GPSReceiverFamily{GPSType::u_blox,
                          QLatin1StringView("U-blox"),
                          {QLatin1StringView("blox"), QLatin1StringView("ubx"), QLatin1StringView("u-blox")},
                          4,
                          Support::Unknown,
                          Support::Supported,
                          Support::Unknown,
                          Support::Unknown,
                          Support::Supported,
                          Support::Unknown,
                          Support::Unsupported},
#endif
#if QGC_GPS_ENABLE_ASHTECH
        GPSReceiverFamily{
            GPSType::trimble,
            QLatin1StringView("Trimble"),
            {QLatin1StringView("trimble"), QLatin1StringView("ashtech"), QLatin1StringView("spectra")},
            1,
            Support::Supported,
            Support::Unsupported,
            Support::Unknown,
        },
#endif
#if QGC_GPS_ENABLE_SBF
        GPSReceiverFamily{GPSType::septentrio,
                          QLatin1StringView("Septentrio"),
                          {QLatin1StringView("septentrio"), QLatin1StringView("sbf"), QLatin1StringView()},
                          2,
                          Support::Supported,
                          Support::Unsupported,
                          Support::Supported,
                          Support::Unsupported,
                          Support::Unsupported,
                          Support::Unsupported,
                          Support::Supported},
#endif
#if QGC_GPS_ENABLE_FEMTO
        GPSReceiverFamily{
            GPSType::femto,
            QLatin1StringView("Femtomes"),
            {QLatin1StringView("femtomes"), QLatin1StringView("femto"), QLatin1StringView()},
            3,
            Support::Supported,
            Support::Unsupported,
            Support::Unknown,
        },
#endif
};

QString configurationError(const GPSReceiverConfig& config)
{
    const auto tr = [](const char* text) { return QCoreApplication::translate("GPSReceiverConfig", text); };
    if (config.role != GPSReceiverConfig::Role::RTKBase && config.role != GPSReceiverConfig::Role::Position) {
        return tr("Select a valid receiver role");
    }
    if ((config.outputProtocol != GPSReceiverConfig::OutputProtocol::Native &&
         config.outputProtocol != GPSReceiverConfig::OutputProtocol::NMEA) ||
        (config.outputProtocol == GPSReceiverConfig::OutputProtocol::NMEA &&
         config.role != GPSReceiverConfig::Role::Position)) {
        return tr("Select a valid receiver output protocol");
    }
    if (!std::isfinite(config.headingOffsetDeg)) {
        return tr("Enter a finite receiver heading offset");
    }
    if (config.role != GPSReceiverConfig::Role::RTKBase) {
        return {};
    }
    if (config.base.useFixedBase) {
        if (!std::isfinite(config.base.fixedBaseLatitude) || std::abs(config.base.fixedBaseLatitude) > 90.0 ||
            !std::isfinite(config.base.fixedBaseLongitude) || std::abs(config.base.fixedBaseLongitude) > 180.0 ||
            !std::isfinite(config.base.fixedBaseAltitudeMeters) ||
            std::abs(static_cast<double>(config.base.fixedBaseAltitudeMeters) * 100.0) >
                (std::numeric_limits<int32_t>::max)() ||
            !std::isfinite(config.base.fixedBaseAccuracyMeters) || config.base.fixedBaseAccuracyMeters < 0.0f ||
            static_cast<double>(config.base.fixedBaseAccuracyMeters * 1000.0f * 10.0f) >
                (std::numeric_limits<uint32_t>::max)()) {
            return tr("Enter a valid fixed base position and accuracy");
        }
    } else if (!std::isfinite(config.base.surveyInAccMeters) || config.base.surveyInAccMeters <= 0.0 ||
               config.base.surveyInAccMeters * 10000.0 > (std::numeric_limits<uint32_t>::max)() ||
               config.base.surveyInDurationSecs <= 0) {
        return tr("Enter a valid survey-in accuracy and duration");
    }
    return {};
}
}  // namespace

GPSReceiverCapabilities GPSReceiverCapabilities::forType(GPSType type)
{
    GPSReceiverCapabilities result;
    result.type = type;
    for (const GPSReceiverFamily& family : families) {
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
    for (const GPSReceiverFamily& family : families) {
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
    if (const QString error = configurationError(config); !error.isEmpty()) {
        return error;
    }
    if (config.role == GPSReceiverConfig::Role::RTKBase && rtkBase == Support::Unsupported) {
        return tr("This receiver does not support RTK base mode");
    }
    if (config.role == GPSReceiverConfig::Role::Position && nativePosition == Support::Unsupported) {
        return tr("This receiver does not support position mode");
    }
    if (config.outputProtocol == GPSReceiverConfig::OutputProtocol::NMEA && nmeaOutput == Support::Unsupported) {
        return tr("This receiver does not support configuring NMEA output in the selected mode");
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
