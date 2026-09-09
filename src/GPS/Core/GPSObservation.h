#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QList>
#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <QtPositioning/QGeoPositionInfo>

#include <array>
#include <optional>

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
        NTRIP
    };

    QGeoPositionInfo position;
    QDateTime receivedAt;
    quint64 monotonicTimestampUs = 0;
    quint64 sessionId = 0;
    QString sourceId;
    FixQuality fixQuality = FixQuality::Unknown;
    AltitudeDatum altitudeDatum = AltitudeDatum::Unknown;
    std::optional<int> satellitesUsed;
    std::optional<double> horizontalDop;
    std::optional<double> verticalDop;
    std::optional<double> altitudeEllipsoidMeters;
    // Antenna orientation is distinct from QGeoPositionInfo::Direction (course over ground).
    std::optional<double> trueHeadingDegrees;
    std::optional<double> trueHeadingAccuracyDegrees;
    std::optional<int> jammingState;
    std::optional<int> spoofingState;
    std::optional<int> authenticationState;
    std::optional<int> correctionsProtocol;
    std::optional<int> correctionsUsed;

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
    enum class Constellation
    {
        Unknown,
        GPS,
        GLONASS,
        Galileo,
        BeiDou,
        QZSS,
        SBAS,
        NavIC
    };

    enum class AzimuthEncoding
    {
        Unknown,
        ScaledFullCircleByte,
        DegreesModulo256
    };

    int id = 0;
    int prn = 0;
    Constellation constellation = Constellation::Unknown;
    std::optional<bool> used;
    std::optional<double> elevationDegrees;
    std::optional<int> signalStrength;
    std::optional<double> normalizedAzimuthDegrees;
    std::optional<int> rawAzimuth;
    AzimuthEncoding azimuthEncoding = AzimuthEncoding::Unknown;

    std::optional<double> azimuthDegrees() const;
};

/// Independent original receipts; zero means the field has no accepted report.
struct GPSSatelliteProvenance
{
    GPSSatellite::Constellation constellation = GPSSatellite::Constellation::Unknown;
    quint64 inViewTimestampUs = 0;
    quint64 inUseTimestampUs = 0;
    std::optional<int> satellitesUsed;
};

struct GPSSatelliteObservation
{
    quint64 monotonicTimestampUs = 0;
    quint64 sessionId = 0;
    QList<GPSSatellite> satellites;
    QList<GPSSatelliteProvenance> provenance = {};
    quint64 revision = 0;  // Monotonic publication order assigned by the accepted-observation store.
    QString sourceId = {};
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
    int referenceStationId = 0;
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
    bool movingBase = false;
    bool referencePositionMissing = false;
    bool referenceObservationsMissing = false;
    bool normalized = false;
};
Q_DECLARE_METATYPE(GPSRelativeObservation)
