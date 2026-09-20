#pragma once

#include <span>
#include <string_view>

#include <QtCore/QVariantMap>

#include "GPSReceiverCapabilities.h"
#include "GPSType.h"

/// Persisted manufacturer IDs are distinct from GPSType values; zero means the combined settings view.
struct GPSReceiverDescriptor
{
    enum class SurveyAccuracy
    {
        Unavailable,
        PositionAccuracy,
        ObservationFilter,
    };

    enum class SurveyDuration
    {
        Unavailable,
        ElapsedTime,
        AcceptedObservations,
    };

    GPSType type;
    int manufacturerId;
    std::string_view detectionKey;
    GPSReceiverCapabilities capabilities;
    SurveyAccuracy surveyAccuracy = SurveyAccuracy::Unavailable;
    SurveyDuration surveyDuration = SurveyDuration::Unavailable;
    bool configurableSurveyDuration = false;
    bool fixedBaseAccuracy = false;
    bool restartOnConnect = false;
    bool surveyMaySavePosition = false;
};

[[nodiscard]] std::span<const GPSReceiverDescriptor> gpsReceiverDescriptors();
[[nodiscard]] const GPSReceiverDescriptor* gpsReceiverDescriptor(GPSType type);
[[nodiscard]] const GPSReceiverDescriptor* gpsReceiverDescriptorForManufacturer(int manufacturer);
[[nodiscard]] QVariantMap gpsReceiverPresentation(int manufacturer);
