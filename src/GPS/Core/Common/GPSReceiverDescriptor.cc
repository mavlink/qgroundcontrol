#include "GPSReceiverDescriptor.h"

#include <algorithm>
#include <array>

#include <QtCore/QHash>

using namespace Qt::StringLiterals;

namespace {
using Accuracy = GPSReceiverDescriptor::SurveyAccuracy;
using Duration = GPSReceiverDescriptor::SurveyDuration;

constexpr std::array DESCRIPTORS{
    GPSReceiverDescriptor{
        .type = GPSType::trimble,
        .name = "Trimble"_L1,
        .manufacturerId = 1,
        .capabilities = {.recognized = true, .rtkBase = true, .surveyIn = true},
        .surveyDuration = Duration::ElapsedTime,
        .configurableSurveyDuration = true,
    },
    GPSReceiverDescriptor{
        .type = GPSType::septentrio,
        .name = "Septentrio"_L1,
        .manufacturerId = 2,
        .protocol = "SBF"_L1,
        .capabilities = {.recognized = true, .rtkBase = true, .surveyIn = true},
        .surveyDuration = Duration::ElapsedTime,
    },
    GPSReceiverDescriptor{
        .type = GPSType::femto,
        .name = "Femtomes"_L1,
        .manufacturerId = 3,
        .capabilities = {.recognized = true, .rtkBase = true, .surveyIn = true},
        .surveyDuration = Duration::ElapsedTime,
        // POSAVE ON uses the receiver's averaging policy; the driver does not program a duration.
        .configurableSurveyDuration = false,
    },
    GPSReceiverDescriptor{
        .type = GPSType::ublox,
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
        .type = GPSType::unicore,
        .name = "Unicore"_L1,
        .manufacturerId = 5,
        .capabilities = {.recognized = true, .rtkBase = true, .receiverAveraging = true},
    },
    GPSReceiverDescriptor{
        .type = GPSType::quectel,
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
        .type = GPSType::passive,
        .name = "passive NMEA"_L1,
        .manufacturerId = 7,
        .protocol = "NMEA"_L1,
        .capabilities = {.recognized = true, .passive = true},
    },
};

/// Every GPSType before automatic has exactly one descriptor, with a nonzero manufacturer ID of its own.
consteval bool registryComplete()
{
    for (int type = 0; type < static_cast<int>(GPSType::automatic); ++type) {
        if (std::ranges::count(DESCRIPTORS, static_cast<GPSType>(type), &GPSReceiverDescriptor::type) != 1) {
            return false;
        }
    }
    for (const auto& descriptor : DESCRIPTORS) {
        if (descriptor.type == GPSType::automatic || descriptor.manufacturerId == GPS_AUTOMATIC_MANUFACTURER ||
            std::ranges::count(DESCRIPTORS, descriptor.manufacturerId, &GPSReceiverDescriptor::manufacturerId) != 1) {
            return false;
        }
    }
    return DESCRIPTORS.size() == static_cast<size_t>(GPSType::automatic);
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
    return type == GPSType::passive ? protocol : QStringLiteral("%1 (%2)").arg(gpsReceiverName(type), protocol);
}

const GPSReceiverDescriptor* gpsReceiverDescriptorForManufacturer(int manufacturer)
{
    const auto entry = std::ranges::find(DESCRIPTORS, manufacturer, &GPSReceiverDescriptor::manufacturerId);
    return entry == DESCRIPTORS.end() ? nullptr : &*entry;
}

std::optional<GPSType> gpsReceiverTypeForManufacturer(int manufacturer)
{
    if (manufacturer == GPS_AUTOMATIC_MANUFACTURER) {
        return GPSType::automatic;
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

GPSReceiverCapabilities gpsReceiverCapabilities(GPSType type)
{
    if (type == GPSType::automatic) {
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

namespace {
GPSReceiverPresentation buildPresentation(int manufacturer)
{
    const auto* selected = gpsReceiverDescriptorForManufacturer(manufacturer);
    GPSReceiverPresentation presentation;
    for (const auto& descriptor : DESCRIPTORS) {
        if (manufacturer != GPS_AUTOMATIC_MANUFACTURER && &descriptor != selected) {
            continue;
        }
        presentation.rtkBase |= descriptor.capabilities.rtkBase;
        presentation.surveyIn |= descriptor.capabilities.surveyIn;
        presentation.receiverAveraging |= descriptor.capabilities.receiverAveraging;
        presentation.compactObservations |= descriptor.capabilities.compactObservations;
        presentation.surveyAccuracy |= descriptor.surveyAccuracy != Accuracy::Unavailable;
        presentation.surveyDuration |= descriptor.configurableSurveyDuration;
        presentation.fixedBaseAccuracy |= descriptor.fixedBaseAccuracy;
        // Automatic offers these for whichever family it identifies: the consent a connection may pass and the notes
        // on that family's side effects.
        presentation.persistentConfiguration |= descriptor.capabilities.persistentConfiguration;
        presentation.restartOnConnect |= descriptor.restartOnConnect;
        presentation.surveyMaySavePosition |= descriptor.surveyMaySavePosition;
    }
    // Labels and live status describe one family, which Automatic only knows once connected.
    presentation.specificReceiver = selected != nullptr;
    presentation.automatic = manufacturer == GPS_AUTOMATIC_MANUFACTURER;
    presentation.passive = selected && selected->capabilities.passive;
    presentation.observationAccuracyFilter = selected && selected->surveyAccuracy == Accuracy::ObservationFilter;
    presentation.acceptedObservationTime = selected && selected->surveyDuration == Duration::AcceptedObservations;
    presentation.reportsSurveyDuration = selected && selected->surveyDuration != Duration::Unavailable;
    return presentation;
}
}  // namespace

const GPSReceiverPresentation& gpsReceiverPresentation(int manufacturer)
{
    // Immutable after first use, so QML bindings see one value per receiver family.
    static const QHash<int, GPSReceiverPresentation> presentations = [] {
        QHash<int, GPSReceiverPresentation> result{
            {GPS_AUTOMATIC_MANUFACTURER, buildPresentation(GPS_AUTOMATIC_MANUFACTURER)}};
        for (const auto& descriptor : DESCRIPTORS) {
            result.insert(descriptor.manufacturerId, buildPresentation(descriptor.manufacturerId));
        }
        return result;
    }();
    static const GPSReceiverPresentation unknown = buildPresentation(-1);
    const auto presentation = presentations.constFind(manufacturer);
    return presentation != presentations.cend() ? *presentation : unknown;
}
