#include "GPSReceiverDescriptor.h"

#include <algorithm>
#include <array>

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
        .configurableSurveyDuration = true,
    },
    GPSReceiverDescriptor{
        .type = GPSType::ublox,
        .manufacturerId = 4,
        .detectionKey = "blox",
        // Pre-v27 configuration has no NavIC path; GPS also controls QZSS.
        .capabilities = {.recognized = true,
                         .position = true,
                         .rtkBase = true,
                         .constellationMask = 0x1f,
                         .dynamicModel = true,
                         .surveyIn = true},
        .surveyAccuracy = Accuracy::PositionAccuracy,
        .surveyDuration = Duration::ElapsedTime,
        .configurableSurveyDuration = true,
        .fixedBaseAccuracy = true,
    },
    GPSReceiverDescriptor{
        .type = GPSType::unicore,
        .manufacturerId = 5,
        .detectionKey = "unicore",
        .capabilities = {.recognized = true, .position = true, .rtkBase = true, .receiverAveraging = true},
    },
    GPSReceiverDescriptor{
        .type = GPSType::quectel,
        .manufacturerId = 6,
        .detectionKey = "quectel",
        .capabilities =
            {.recognized = true, .position = true, .rtkBase = true, .surveyIn = true, .persistentConfiguration = true},
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

QVariantMap gpsReceiverPresentation(int manufacturer)
{
    const auto* selected = gpsReceiverDescriptorForManufacturer(manufacturer);
    GPSReceiverCapabilities capabilities;
    bool surveyAccuracy = false;
    bool surveyDuration = false;
    bool fixedBaseAccuracy = false;
    for (const auto& descriptor : DESCRIPTORS) {
        if (manufacturer != 0 && &descriptor != selected) {
            continue;
        }
        capabilities.recognized |= descriptor.capabilities.recognized;
        capabilities.rtkBase |= descriptor.capabilities.rtkBase;
        capabilities.surveyIn |= descriptor.capabilities.surveyIn;
        capabilities.receiverAveraging |= descriptor.capabilities.receiverAveraging;
        surveyAccuracy |= descriptor.surveyAccuracy != Accuracy::Unavailable;
        surveyDuration |= descriptor.configurableSurveyDuration;
        fixedBaseAccuracy |= descriptor.fixedBaseAccuracy;
    }
    // "All" combines editable fields, not receiver-specific side effects or live status.
    return {
        {QStringLiteral("recognized"), capabilities.recognized},
        {QStringLiteral("specificReceiver"), selected != nullptr},
        {QStringLiteral("rtkBase"), capabilities.rtkBase},
        {QStringLiteral("surveyIn"), capabilities.surveyIn},
        {QStringLiteral("receiverAveraging"), capabilities.receiverAveraging},
        {QStringLiteral("passive"), selected && selected->capabilities.passive},
        {QStringLiteral("surveyAccuracy"), surveyAccuracy},
        {QStringLiteral("surveyDuration"), surveyDuration},
        {QStringLiteral("fixedBaseAccuracy"), fixedBaseAccuracy},
        {QStringLiteral("observationAccuracyFilter"),
         selected && selected->surveyAccuracy == Accuracy::ObservationFilter},
        {QStringLiteral("acceptedObservationTime"),
         selected && selected->surveyDuration == Duration::AcceptedObservations},
        {QStringLiteral("reportsSurveyDuration"), selected && selected->surveyDuration != Duration::Unavailable},
        {QStringLiteral("persistentConfiguration"), selected && selected->capabilities.persistentConfiguration},
        {QStringLiteral("restartOnConnect"), selected && selected->restartOnConnect},
        {QStringLiteral("surveyMaySavePosition"), selected && selected->surveyMaySavePosition},
    };
}
