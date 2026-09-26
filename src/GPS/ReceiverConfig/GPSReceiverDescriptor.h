#pragma once

#include <span>
#include <string_view>

#include <QtCore/QObject>

#include "GPSReceiverCapabilities.h"
#include "GPSType.h"

/// The persisted manufacturer that detects the receiver family on every connect (GPSType::automatic). Its settings view
/// combines the editable fields of every family.
inline constexpr int GPS_AUTOMATIC_MANUFACTURER = 0;

/// Persisted manufacturer IDs are distinct from GPSType values.
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
    /// Shown to users, such as in "Detected Septentrio receiver".
    std::string_view name;
    int manufacturerId;
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

/// What the receiver settings and status views offer for one receiver family. The combined view of Automatic
/// (GPS_AUTOMATIC_MANUFACTURER) offers the editable fields, flash-save consent and side-effect notes of every family,
/// which apply only if that family is identified, but no family-specific labels or live status.
struct GPSReceiverPresentation
{
    Q_GADGET
    Q_PROPERTY(bool recognized MEMBER recognized FINAL)
    Q_PROPERTY(bool specificReceiver MEMBER specificReceiver FINAL)
    Q_PROPERTY(bool automatic MEMBER automatic FINAL)
    Q_PROPERTY(bool rtkBase MEMBER rtkBase FINAL)
    Q_PROPERTY(bool surveyIn MEMBER surveyIn FINAL)
    Q_PROPERTY(bool receiverAveraging MEMBER receiverAveraging FINAL)
    Q_PROPERTY(bool compactObservations MEMBER compactObservations FINAL)
    Q_PROPERTY(bool passive MEMBER passive FINAL)
    Q_PROPERTY(bool surveyAccuracy MEMBER surveyAccuracy FINAL)
    Q_PROPERTY(bool surveyDuration MEMBER surveyDuration FINAL)
    Q_PROPERTY(bool fixedBaseAccuracy MEMBER fixedBaseAccuracy FINAL)
    Q_PROPERTY(bool observationAccuracyFilter MEMBER observationAccuracyFilter FINAL)
    Q_PROPERTY(bool acceptedObservationTime MEMBER acceptedObservationTime FINAL)
    Q_PROPERTY(bool reportsSurveyDuration MEMBER reportsSurveyDuration FINAL)
    Q_PROPERTY(bool persistentConfiguration MEMBER persistentConfiguration FINAL)
    Q_PROPERTY(bool restartOnConnect MEMBER restartOnConnect FINAL)
    Q_PROPERTY(bool surveyMaySavePosition MEMBER surveyMaySavePosition FINAL)

public:
    bool recognized = false;
    bool specificReceiver = false;
    /// The family is detected when connecting.
    bool automatic = false;
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
