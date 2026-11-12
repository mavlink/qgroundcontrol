#pragma once

#include <optional>
#include <span>

#include <QtCore/QLatin1StringView>
#include <QtCore/QString>

#include "GPSReceiverConfig.h"
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
    /// The constellations whose observations a base can leave out of its corrections; none where a base always sends
    /// the family's own set.
    GPSCorrectionConstellations correctionConstellations{};
};

/// The persisted manufacturer that detects the receiver family on every connect (GPSType::Automatic). Its settings view
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

/// The family a persisted manufacturer selects, GPSType::Automatic for GPS_AUTOMATIC_MANUFACTURER; empty when unknown.
[[nodiscard]] std::optional<GPSType> gpsReceiverTypeForManufacturer(int manufacturer);
/// The persisted manufacturer of @a type, or GPS_AUTOMATIC_MANUFACTURER when it has none.
[[nodiscard]] int gpsReceiverManufacturerForType(GPSType type);

/// What @a type supports, for either role; unknown receiver families support nothing.
[[nodiscard]] GPSReceiverCapabilities gpsReceiverCapabilities(GPSType type);

/// Whether @a family's RTCM3 is forwarded whatever base status it publishes, instead of only while that is Valid.
[[nodiscard]] bool forwardsCorrectionsWithoutBase(GPSType family);

/// The name of @a type's receiver family as shown to users; empty when no descriptor exists.
[[nodiscard]] QString gpsReceiverName(GPSType type);

/// How the protocol a passive input decodes is shown to users: the family and its protocol, such as "u-blox (UBX)", or
/// "NMEA" for GPSType::Passive; empty for a family without a protocol of its own.
[[nodiscard]] QString gpsInputProtocolName(GPSType type);
