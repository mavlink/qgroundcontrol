#include "GPSReceiverPresentation.h"

#include <QtCore/QHash>

#include "GPSReceiverDescriptor.h"

namespace {
GPSReceiverPresentation buildPresentation(int manufacturer)
{
    using Accuracy = GPSReceiverDescriptor::SurveyAccuracy;
    using Duration = GPSReceiverDescriptor::SurveyDuration;

    const bool automatic = manufacturer == GPS_AUTOMATIC_MANUFACTURER;
    const auto* selected = gpsReceiverDescriptorForManufacturer(manufacturer);
    GPSReceiverPresentation presentation;
    const auto include = [&presentation](const GPSReceiverDescriptor& descriptor) {
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
    };
    if (automatic) {
        for (const auto& descriptor : gpsReceiverDescriptors()) {
            include(descriptor);
        }
    } else if (selected) {
        include(*selected);
    }
    // Labels and live status describe one family, which Automatic only knows once connected.
    presentation.specificReceiver = selected != nullptr;
    presentation.automatic = automatic;
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
        for (const auto& descriptor : gpsReceiverDescriptors()) {
            result.insert(descriptor.manufacturerId, buildPresentation(descriptor.manufacturerId));
        }
        return result;
    }();
    static const GPSReceiverPresentation unknown = buildPresentation(-1);
    const auto presentation = presentations.constFind(manufacturer);
    return presentation != presentations.cend() ? *presentation : unknown;
}
