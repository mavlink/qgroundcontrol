#include "GPSReceiverDescriptor.h"

#include <algorithm>
#include <array>

using namespace Qt::StringLiterals;

namespace {
using Accuracy = GPSReceiverDescriptor::SurveyAccuracy;
using Duration = GPSReceiverDescriptor::SurveyDuration;

constexpr std::array DESCRIPTORS{
    GPSReceiverDescriptor{
        .type = GPSType::Trimble,
        .name = "Trimble"_L1,
        .manufacturerId = 1,
        .capabilities = {.recognized = true, .rtkBase = true, .surveyIn = true},
        .surveyDuration = Duration::ElapsedTime,
        .configurableSurveyDuration = true,
    },
    GPSReceiverDescriptor{
        .type = GPSType::Septentrio,
        .name = "Septentrio"_L1,
        .manufacturerId = 2,
        .protocol = "SBF"_L1,
        .capabilities = {.recognized = true, .rtkBase = true, .surveyIn = true},
        .surveyDuration = Duration::ElapsedTime,
    },
    GPSReceiverDescriptor{
        .type = GPSType::Femto,
        .name = "Femtomes"_L1,
        .manufacturerId = 3,
        .capabilities = {.recognized = true, .rtkBase = true, .surveyIn = true},
        .surveyDuration = Duration::ElapsedTime,
        // POSAVE ON uses the receiver's averaging policy; the driver does not program a duration.
        .configurableSurveyDuration = false,
    },
    GPSReceiverDescriptor{
        .type = GPSType::UBlox,
        .name = "u-blox"_L1,
        .manufacturerId = 4,
        .protocol = "UBX"_L1,
        .capabilities = {.recognized = true, .rtkBase = true, .surveyIn = true, .compactObservations = true},
        .surveyAccuracy = Accuracy::PositionAccuracy,
        .surveyDuration = Duration::ElapsedTime,
        .configurableSurveyDuration = true,
        .fixedBaseAccuracy = true,
    },
    GPSReceiverDescriptor{
        .type = GPSType::Unicore,
        .name = "Unicore"_L1,
        .manufacturerId = 5,
        .capabilities = {.recognized = true, .rtkBase = true, .receiverAveraging = true},
    },
    GPSReceiverDescriptor{
        .type = GPSType::Quectel,
        .name = "Quectel"_L1,
        .manufacturerId = 6,
        .capabilities = {.recognized = true, .rtkBase = true, .surveyIn = true, .persistentConfiguration = true},
        .surveyAccuracy = Accuracy::ObservationFilter,
        .surveyDuration = Duration::AcceptedObservations,
        .configurableSurveyDuration = true,
        .restartOnConnect = true,
        .surveyMaySavePosition = true,
    },
    GPSReceiverDescriptor{
        .type = GPSType::Passive,
        .name = "passive NMEA"_L1,
        .manufacturerId = 7,
        .protocol = "NMEA"_L1,
        .capabilities = {.recognized = true, .passive = true},
    },
};

/// Every GPSType before automatic has exactly one descriptor, with a nonzero manufacturer ID of its own.
consteval bool registryComplete()
{
    for (int type = 0; type < static_cast<int>(GPSType::Automatic); ++type) {
        if (std::ranges::count(DESCRIPTORS, static_cast<GPSType>(type), &GPSReceiverDescriptor::type) != 1) {
            return false;
        }
    }
    for (const auto& descriptor : DESCRIPTORS) {
        if (descriptor.type == GPSType::Automatic || descriptor.manufacturerId == GPS_AUTOMATIC_MANUFACTURER ||
            std::ranges::count(DESCRIPTORS, descriptor.manufacturerId, &GPSReceiverDescriptor::manufacturerId) != 1) {
            return false;
        }
    }
    return DESCRIPTORS.size() == static_cast<size_t>(GPSType::Automatic);
}

static_assert(registryComplete(), "Each GPSType needs one descriptor with a unique, nonzero manufacturer ID");
}  // namespace

std::span<const GPSReceiverDescriptor> gpsReceiverDescriptors()
{
    return DESCRIPTORS;
}

const GPSReceiverDescriptor* gpsReceiverDescriptor(GPSType type)
{
    const auto entry = std::ranges::find(DESCRIPTORS, type, &GPSReceiverDescriptor::type);
    return entry == DESCRIPTORS.end() ? nullptr : &*entry;
}

QString gpsReceiverName(GPSType type)
{
    const auto* descriptor = gpsReceiverDescriptor(type);
    return descriptor ? QString(descriptor->name) : QString();
}

QString gpsInputProtocolName(GPSType type)
{
    const auto* descriptor = gpsReceiverDescriptor(type);
    if (!descriptor || descriptor->protocol.isEmpty()) {
        return {};
    }
    const QString protocol(descriptor->protocol);
    return type == GPSType::Passive ? protocol : QStringLiteral("%1 (%2)").arg(gpsReceiverName(type), protocol);
}

const GPSReceiverDescriptor* gpsReceiverDescriptorForManufacturer(int manufacturer)
{
    const auto entry = std::ranges::find(DESCRIPTORS, manufacturer, &GPSReceiverDescriptor::manufacturerId);
    return entry == DESCRIPTORS.end() ? nullptr : &*entry;
}

std::optional<GPSType> gpsReceiverTypeForManufacturer(int manufacturer)
{
    if (manufacturer == GPS_AUTOMATIC_MANUFACTURER) {
        return GPSType::Automatic;
    }
    if (const auto* descriptor = gpsReceiverDescriptorForManufacturer(manufacturer)) {
        return descriptor->type;
    }
    return std::nullopt;
}

int gpsReceiverManufacturerForType(GPSType type)
{
    const auto* descriptor = gpsReceiverDescriptor(type);
    return descriptor ? descriptor->manufacturerId : GPS_AUTOMATIC_MANUFACTURER;
}

bool forwardsCorrectionsWithoutBase(GPSType family)
{
    // Passive input comes from a receiver that was not configured here, so it reports no base. A Septentrio receiver
    // starts its corrections with its configuration and reports its base in PVTGeodetic blocks half a second apart,
    // and no recording of a base confirms how those read, so its corrections do not wait for them.
    return family == GPSType::Passive || family == GPSType::Septentrio;
}

GPSReceiverCapabilities gpsReceiverCapabilities(GPSType type)
{
    if (type == GPSType::Automatic) {
        // Detection finds a base family; until then every option some base family supports is accepted.
        GPSReceiverCapabilities combined;
        for (const auto& descriptor : DESCRIPTORS) {
            const auto& capabilities = descriptor.capabilities;
            if (!capabilities.rtkBase) {
                continue;
            }
            combined.recognized = true;
            combined.rtkBase = true;
            combined.surveyIn |= capabilities.surveyIn;
            combined.receiverAveraging |= capabilities.receiverAveraging;
            combined.persistentConfiguration |= capabilities.persistentConfiguration;
            combined.compactObservations |= capabilities.compactObservations;
        }
        return combined;
    }

    const auto* descriptor = gpsReceiverDescriptor(type);
    if (!descriptor) {
        return {};
    }
    return descriptor->capabilities;
}
