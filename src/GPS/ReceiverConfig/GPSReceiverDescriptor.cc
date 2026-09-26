#include "GPSReceiverDescriptor.h"

#include <algorithm>
#include <array>

#include <QtCore/QHash>

namespace {
using Accuracy = GPSReceiverDescriptor::SurveyAccuracy;
using Duration = GPSReceiverDescriptor::SurveyDuration;

constexpr std::array DESCRIPTORS{
    GPSReceiverDescriptor{
        .type = GPSType::trimble,
        .name = "Trimble",
        .manufacturerId = 1,
        .capabilities = {.recognized = true, .rtkBase = true, .surveyIn = true},
        .surveyDuration = Duration::ElapsedTime,
        .configurableSurveyDuration = true,
    },
    GPSReceiverDescriptor{
        .type = GPSType::septentrio,
        .name = "Septentrio",
        .manufacturerId = 2,
        .capabilities = {.recognized = true, .rtkBase = true, .surveyIn = true},
        .surveyDuration = Duration::ElapsedTime,
    },
    GPSReceiverDescriptor{
        .type = GPSType::femto,
        .name = "Femtomes",
        .manufacturerId = 3,
        .capabilities = {.recognized = true, .rtkBase = true, .surveyIn = true},
        .surveyDuration = Duration::ElapsedTime,
        // POSAVE ON uses the receiver's averaging policy; the driver does not program a duration.
        .configurableSurveyDuration = false,
    },
    GPSReceiverDescriptor{
        .type = GPSType::ublox,
        .name = "u-blox",
        .manufacturerId = 4,
        .capabilities = {.recognized = true, .rtkBase = true, .surveyIn = true, .compactObservations = true},
        .surveyAccuracy = Accuracy::PositionAccuracy,
        .surveyDuration = Duration::ElapsedTime,
        .configurableSurveyDuration = true,
        .fixedBaseAccuracy = true,
    },
    GPSReceiverDescriptor{
        .type = GPSType::unicore,
        .name = "Unicore",
        .manufacturerId = 5,
        .capabilities = {.recognized = true, .rtkBase = true, .receiverAveraging = true},
    },
    GPSReceiverDescriptor{
        .type = GPSType::quectel,
        .name = "Quectel",
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
        .name = "passive NMEA",
        .manufacturerId = 7,
        .capabilities = {.recognized = true, .passive = true},
    },
};
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

const GPSReceiverDescriptor* gpsReceiverDescriptorForManufacturer(int manufacturer)
{
    const auto entry = std::ranges::find(DESCRIPTORS, manufacturer, &GPSReceiverDescriptor::manufacturerId);
    return entry == DESCRIPTORS.end() ? nullptr : &*entry;
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
        presentation.recognized |= descriptor.capabilities.recognized;
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
