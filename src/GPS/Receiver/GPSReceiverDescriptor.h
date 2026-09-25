#pragma once

#include <span>
#include <string_view>

#include <QtCore/QObject>

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

/// What the receiver settings and status views offer for one receiver family. The combined view (0) offers the
/// editable fields of every family, but no family-specific side effects or live status.
struct GPSReceiverPresentation
{
    Q_GADGET
    Q_PROPERTY(bool recognized MEMBER recognized)
    Q_PROPERTY(bool specificReceiver MEMBER specificReceiver)
    Q_PROPERTY(bool rtkBase MEMBER rtkBase)
    Q_PROPERTY(bool surveyIn MEMBER surveyIn)
    Q_PROPERTY(bool receiverAveraging MEMBER receiverAveraging)
    Q_PROPERTY(bool compactObservations MEMBER compactObservations)
    Q_PROPERTY(bool passive MEMBER passive)
    Q_PROPERTY(bool surveyAccuracy MEMBER surveyAccuracy)
    Q_PROPERTY(bool surveyDuration MEMBER surveyDuration)
    Q_PROPERTY(bool fixedBaseAccuracy MEMBER fixedBaseAccuracy)
    Q_PROPERTY(bool observationAccuracyFilter MEMBER observationAccuracyFilter)
    Q_PROPERTY(bool acceptedObservationTime MEMBER acceptedObservationTime)
    Q_PROPERTY(bool reportsSurveyDuration MEMBER reportsSurveyDuration)
    Q_PROPERTY(bool persistentConfiguration MEMBER persistentConfiguration)
    Q_PROPERTY(bool restartOnConnect MEMBER restartOnConnect)
    Q_PROPERTY(bool surveyMaySavePosition MEMBER surveyMaySavePosition)

public:
    bool recognized = false;
    bool specificReceiver = false;
    bool rtkBase = false;
    bool surveyIn = false;
    bool receiverAveraging = false;
    bool compactObservations = false;
    bool passive = false;
    bool surveyAccuracy = false;
    bool surveyDuration = false;
    bool fixedBaseAccuracy = false;
    bool observationAccuracyFilter = false;
    bool acceptedObservationTime = false;
    bool reportsSurveyDuration = false;
    bool persistentConfiguration = false;
    bool restartOnConnect = false;
    bool surveyMaySavePosition = false;

    bool operator==(const GPSReceiverPresentation&) const = default;
};

[[nodiscard]] const GPSReceiverPresentation& gpsReceiverPresentation(int manufacturer);
