#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QList>
#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <QtPositioning/QGeoPositionInfo>

#include <array>
#include <optional>

#include "GPSConstellation.h"

/// Receiver-independent data. Unknown metadata remains absent, never a manufactured zero.
struct GPSObservation
{
    enum class FixQuality
    {
        Unknown,
        NoFix,
        Fix2D,
        Fix3D,
        Differential,
        RTKFloat,
        RTKFixed,
        Extrapolated
    };

    enum class AltitudeDatum
    {
        Unknown,
        MeanSeaLevel,
        Ellipsoid
    };

    enum class PositionUse
    {
        GroundStation,
        Motion,
        RemoteID,
        NTRIP,
        Gga,
        Diagnostics
    };

    QGeoPositionInfo position;
    QDateTime receivedAt;
    quint64 monotonicTimestampUs = 0;
    quint64 sessionId = 0;
    QString sourceId;
    FixQuality fixQuality = FixQuality::Unknown;
    // A receiver can retain coordinates while explicitly declaring its navigation solution invalid.
    std::optional<bool> receiverFixValid = std::nullopt;
    AltitudeDatum altitudeDatum = AltitudeDatum::Unknown;
    std::optional<int> satellitesUsed;
    std::optional<double> speedAccuracyMetersPerSecond;
    quint64 dopTimestampUs = 0;
    quint64 headingTimestampUs = 0;
    quint64 accuracyTimestampUs = 0;
    std::optional<double> horizontalDop;
    std::optional<double> verticalDop;
    std::optional<double> altitudeEllipsoidMeters;
    // Antenna orientation is distinct from QGeoPositionInfo::Direction (course over ground).
    std::optional<double> trueHeadingDegrees;
    std::optional<double> trueHeadingAccuracyDegrees;

    bool usable() const;
    QGeoCoordinate coordinate() const;
    double heading() const;
    /// Apply the consumer's accuracy policy without changing the raw observation.
    QGeoPositionInfo acceptedPosition(PositionUse use) const;
    qint64 ageMilliseconds() const;
    static quint64 monotonicNowUs();
    static qint64 ageMilliseconds(quint64 timestampUs);
};
Q_DECLARE_METATYPE(GPSObservation)

struct GPSSatellite
{
    using Constellation = GPSConstellation;

    int id = 0;
    int prn = 0;
    Constellation constellation = Constellation::Unknown;
    std::optional<bool> used;
    std::optional<double> elevationDegrees;
    std::optional<int> signalStrength;
    std::optional<double> normalizedAzimuthDegrees;

    std::optional<double> azimuthDegrees() const;
};

/// Independent original receipts; zero means the field has no accepted report.
struct GPSSatelliteProvenance
{
    GPSSatellite::Constellation constellation = GPSSatellite::Constellation::Unknown;
    quint64 inViewTimestampUs = 0;
    quint64 inUseTimestampUs = 0;
    std::optional<int> satellitesUsed;
    // Present even for an empty GSA list; independent of visibility reports.
    std::optional<QList<int>> usedSatelliteIds = std::nullopt;
};

struct GPSSatelliteObservation
{
    enum class UpdateMode
    {
        FullSnapshot,       // Omitted constellation/field retires its prior report through this snapshot receipt.
        ConstellationDelta  // Only explicitly supplied view/use receipts replace state; omission preserves it.
    };
    quint64 monotonicTimestampUs = 0;
    quint64 sessionId = 0;
    QList<GPSSatellite> satellites;
    QList<GPSSatelliteProvenance> provenance = {};
    quint64 revision = 0;  // Monotonic publication order assigned by the accepted-observation store.
    QString sourceId = {};
    UpdateMode updateMode = UpdateMode::FullSnapshot;
    int usedCount() const;
    int satellitesInViewCount() const;
    int satellitesInUseCount() const;
};
Q_DECLARE_METATYPE(GPSSatelliteObservation)

struct GPSRelativeObservation
{
    quint64 monotonicTimestampUs = 0;
    quint64 sampleTimestampUs = 0;
    quint64 sessionId = 0;
    quint64 receiverTimeUs = 0;
    std::optional<int> referenceStationId;
    std::array<double, 3> positionNedMeters{};
    std::array<double, 3> accuracyNedMeters{};
    double lengthMeters = 0;
    double lengthAccuracyMeters = 0;
    std::optional<double> headingDegrees;
    std::optional<double> headingAccuracyDegrees;
    bool fixValid = false;
    bool differential = false;
    bool positionValid = false;
    bool carrierFloat = false;
    bool carrierFixed = false;
    std::optional<bool> movingBase;
    std::optional<bool> referencePositionMissing;
    std::optional<bool> referenceObservationsMissing;
    std::optional<bool> normalized;
};
Q_DECLARE_METATYPE(GPSRelativeObservation)
