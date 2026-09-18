#pragma once

#include <optional>

#include <QtCore/QDateTime>
#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <QtPositioning/QGeoPositionInfo>

#include "GPSAltitudeDatum.h"

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
    quint64 dopTimestampUs = 0;
    quint64 accuracyTimestampUs = 0;
    std::optional<double> horizontalDop;
    std::optional<double> verticalDop;
    std::optional<double> altitudeEllipsoidMeters;

    /// The owner separately enforces freshness and session authorization.
    [[nodiscard]] std::optional<GPSObservation> projected(PositionUse use) const;
    bool usable() const;
    QGeoCoordinate coordinate() const;
    double heading() const;
};
Q_DECLARE_METATYPE(GPSObservation)
