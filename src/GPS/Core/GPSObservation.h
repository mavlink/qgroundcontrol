#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <QtPositioning/QGeoPositionInfo>

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

    QGeoPositionInfo position;
    QDateTime receivedAt;
    quint64 monotonicTimestampUs = 0;
    quint64 sessionId = 0;
    QString sourceId;
    FixQuality fixQuality = FixQuality::Unknown;
    // A receiver can retain coordinates while explicitly declaring its navigation solution invalid.
    std::optional<bool> receiverFixValid = std::nullopt;

    bool usable() const;
    QGeoCoordinate coordinate() const;
    double heading() const;
    /// Apply the ground-station accuracy policy without changing the raw observation.
    QGeoPositionInfo acceptedPosition() const;
};
Q_DECLARE_METATYPE(GPSObservation)
