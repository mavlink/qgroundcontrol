#pragma once

#include <optional>
#include <span>

#include <QtCore/QLatin1StringView>
#include <QtCore/QObject>
#include <QtCore/QString>

#include "GPSType.h"

/// Software request support, not a guarantee for every physical receiver model or firmware.
struct GPSReceiverCapabilities
{
    bool recognized = false;
    bool rtkBase = false;
    bool surveyIn = false;
    bool receiverAveraging = false;
    bool passive = false;
    bool persistentConfiguration = false;
    bool compactObservations = false;
};

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
    QLatin1StringView name;
    int manufacturerId;
    /// The protocol a passive input decodes for this family, such as UBX; empty when it has none of its own.
    QLatin1StringView protocol = {};
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

/// The family a persisted manufacturer selects, GPSType::automatic for GPS_AUTOMATIC_MANUFACTURER; empty when unknown.
[[nodiscard]] std::optional<GPSType> gpsReceiverTypeForManufacturer(int manufacturer);
/// The persisted manufacturer of @a type, or GPS_AUTOMATIC_MANUFACTURER when it has none.
[[nodiscard]] int gpsReceiverManufacturerForType(GPSType type);

/// What @a type supports, for either role; unknown receiver families support nothing.
[[nodiscard]] GPSReceiverCapabilities gpsReceiverCapabilities(GPSType type);

/// The name of @a type's receiver family as shown to users; empty when no descriptor exists.
[[nodiscard]] QString gpsReceiverName(GPSType type);

/// How the protocol a passive input decodes is shown to users: the family and its protocol, such as "u-blox (UBX)", or
/// "NMEA" for GPSType::passive; empty for a family without a protocol of its own.
[[nodiscard]] QString gpsInputProtocolName(GPSType type);

/// What the receiver settings and status views offer for one receiver family. The combined view of Automatic
/// (GPS_AUTOMATIC_MANUFACTURER) offers the editable fields, flash-save consent and side-effect notes of every family,
/// which apply only if that family is identified, but no family-specific labels or live status.
struct GPSReceiverPresentation
{
    Q_GADGET
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
