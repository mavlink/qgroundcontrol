#pragma once

#include <optional>

#include <QtCore/QDateTime>
#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <QtPositioning/QGeoPositionInfo>

#include "GPSFixQuality.h"

struct GPSEllipsoidPosition;
struct GPSNavigationValues;

enum class GPSAltitudeDatum
{
    Unknown = 0,
    MeanSeaLevel = 1,
    Ellipsoid = 2
};

/// Receiver-independent data. Unknown metadata remains absent, never a manufactured zero.
struct GPSObservation
{
    using FixQuality = GPSFixQuality;

    enum class PositionUse
    {
        GroundStation,
        Motion,
        RemoteID,
        Gga
    };

    QGeoPositionInfo position;
    QDateTime receivedAt;
    quint64 monotonicTimestampUs = 0;
    quint64 sessionId = 0;
    QString sourceId;
    FixQuality fixQuality = FixQuality::Unknown;
    // Coordinates can outlive a valid navigation solution.
    std::optional<bool> receiverFixValid = std::nullopt;

    GPSAltitudeDatum altitudeDatum = GPSAltitudeDatum::Unknown;
    std::optional<int> satellitesUsed;
    std::optional<double> horizontalDop;
    std::optional<double> verticalDop;
    std::optional<double> altitudeEllipsoidMeters;

    /// Converts a native receiver solution that arrived at receivedAtUs on the monotonic clock.
    static GPSObservation fromNavigation(const GPSNavigationValues& navigation, quint64 receivedAtUs);
    /// A known antenna position, such as a fixed or surveyed RTK base, which receivers report as a time-only fix.
    static GPSObservation fromSurveyedPosition(const GPSEllipsoidPosition& position, double accuracyMeters,
                                               quint64 receivedAtUs);
    /// Nominal accuracy in meters for receivers that report only DOP (DOP x 5.1 m UERE x 2).
    static double accuracyFromDop(double dop);

    /// The owner separately enforces freshness and session authorization.
    [[nodiscard]] std::optional<GPSObservation> projected(PositionUse use) const;
    bool hasNavigationSolution() const;
    bool usable() const;
    QGeoCoordinate coordinate() const;
    double heading() const;
};
Q_DECLARE_METATYPE(GPSObservation)
