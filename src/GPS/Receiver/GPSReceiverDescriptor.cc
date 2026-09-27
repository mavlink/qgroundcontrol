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
        .manufacturerId = 1,
        .detectionKey = "trimble",
        .capabilities = {.recognized = true, .rtkBase = true, .surveyIn = true},
        .surveyDuration = Duration::ElapsedTime,
        .configurableSurveyDuration = true,
    },
    GPSReceiverDescriptor{
        .type = GPSType::septentrio,
        .manufacturerId = 2,
        .detectionKey = "septentrio",
        .capabilities = {.recognized = true, .rtkBase = true, .surveyIn = true},
        .surveyDuration = Duration::ElapsedTime,
    },
    GPSReceiverDescriptor{
        .type = GPSType::femto,
        .manufacturerId = 3,
        .detectionKey = "femtomes",
        .capabilities = {.recognized = true, .rtkBase = true, .surveyIn = true},
        .surveyDuration = Duration::ElapsedTime,
        // POSAVE ON uses the receiver's averaging policy; the driver does not program a duration.
        .configurableSurveyDuration = false,
    },
    GPSReceiverDescriptor{
        .type = GPSType::ublox,
        .manufacturerId = 4,
        .detectionKey = "blox",
        .capabilities = {.recognized = true, .rtkBase = true, .surveyIn = true, .compactObservations = true},
        .surveyAccuracy = Accuracy::PositionAccuracy,
        .surveyDuration = Duration::ElapsedTime,
        .configurableSurveyDuration = true,
        .fixedBaseAccuracy = true,
    },
    GPSReceiverDescriptor{
        .type = GPSType::unicore,
        .manufacturerId = 5,
        .detectionKey = "unicore",
        .capabilities = {.recognized = true, .rtkBase = true, .receiverAveraging = true},
    },
    GPSReceiverDescriptor{
        .type = GPSType::quectel,
        .manufacturerId = 6,
        .detectionKey = "quectel",
        .capabilities = {.recognized = true, .rtkBase = true, .surveyIn = true, .persistentConfiguration = true},
        .surveyAccuracy = Accuracy::ObservationFilter,
        .surveyDuration = Duration::AcceptedObservations,
        .configurableSurveyDuration = true,
        .restartOnConnect = true,
        .surveyMaySavePosition = true,
    },
    GPSReceiverDescriptor{
        .type = GPSType::passive,
        .manufacturerId = 7,
        .detectionKey = "passive",
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
        if (manufacturer != 0 && &descriptor != selected) {
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
    }
    // "All" combines editable fields, not receiver-specific side effects or live status.
    presentation.specificReceiver = selected != nullptr;
    presentation.passive = selected && selected->capabilities.passive;
    presentation.observationAccuracyFilter = selected && selected->surveyAccuracy == Accuracy::ObservationFilter;
    presentation.acceptedObservationTime = selected && selected->surveyDuration == Duration::AcceptedObservations;
    presentation.reportsSurveyDuration = selected && selected->surveyDuration != Duration::Unavailable;
    presentation.persistentConfiguration = selected && selected->capabilities.persistentConfiguration;
    presentation.restartOnConnect = selected && selected->restartOnConnect;
    presentation.surveyMaySavePosition = selected && selected->surveyMaySavePosition;
    return presentation;
}
}  // namespace

const GPSReceiverPresentation& gpsReceiverPresentation(int manufacturer)
{
    // Immutable after first use, so QML bindings see one value per receiver family.
    static const QHash<int, GPSReceiverPresentation> presentations = [] {
        QHash<int, GPSReceiverPresentation> result{{0, buildPresentation(0)}};
        for (const auto& descriptor : DESCRIPTORS) {
            result.insert(descriptor.manufacturerId, buildPresentation(descriptor.manufacturerId));
        }
        return result;
    }();
    static const GPSReceiverPresentation unknown = buildPresentation(-1);
    const auto presentation = presentations.constFind(manufacturer);
    return presentation != presentations.cend() ? *presentation : unknown;
}
