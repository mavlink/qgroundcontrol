#pragma once

#include <optional>

#include <QtPositioning/QGeoPositionInfo>

#include "GPSReceiverReports.h"

struct GPSEllipsoidPosition;
struct GPSNavigationValues;

enum class GPSAltitudeDatum
{
    Unknown = 0,
    MeanSeaLevel = 1,
    Ellipsoid = 2,
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
        Gga,
    };

    QGeoPositionInfo position;
    quint64 monotonicTimestampUs = 0;
    quint64 sessionId = 0;
    FixQuality fixQuality = FixQuality::Unknown;
    // Coordinates can outlive a valid navigation solution.
    std::optional<bool> receiverFixValid = std::nullopt;

    GPSAltitudeDatum altitudeDatum = GPSAltitudeDatum::Unknown;
    std::optional<int> satellitesUsed;
    std::optional<double> horizontalDop;
    std::optional<double> altitudeEllipsoidMeters;

    /// Converts a native receiver solution that arrived at receivedAtUs on the monotonic clock.
    static GPSObservation fromNavigation(const GPSNavigationValues& navigation, quint64 receivedAtUs);
    /// A known antenna position, such as a fixed or surveyed RTK base, which receivers report as a time-only fix.
    static GPSObservation fromSurveyedPosition(const GPSEllipsoidPosition& position, double accuracyMeters,
                                               quint64 receivedAtUs);
    /// Nominal accuracy in meters of a receiver that reports only a dilution of precision: HDOP x 5.1 m UERE for the
    /// horizontal accuracy, VDOP x 5.1 m for the vertical. It is one standard deviation, as the accuracy of a GST
    /// sentence, of the Septentrio driver and of Android is, so the limits of usable() and coordinate() hold for it
    /// as for those: a position is usable up to HDOP 19.6 and keeps its altitude up to VDOP 1.96. Qt's NMEA source
    /// estimates twice as much.
    [[nodiscard]] static double accuracyFromDop(double dop);

    /// The owner separately enforces freshness and session authorization.
    [[nodiscard]] std::optional<GPSObservation> projected(PositionUse use) const;
    [[nodiscard]] bool hasNavigationSolution() const;
    [[nodiscard]] bool usable() const;
    [[nodiscard]] QGeoCoordinate coordinate() const;
    [[nodiscard]] double heading() const;
    /// The position's altitude above mean sea level. A height above the WGS84 ellipsoid, which the altitude or
    /// altitudeEllipsoidMeters may be, is converted with the EGM96 geoid. Empty when the position has no height or
    /// only an altitude of unknown datum.
    [[nodiscard]] std::optional<double> altitudeMslMeters() const;
    /// The position's height above the WGS84 ellipsoid: altitudeEllipsoidMeters or an altitude in that datum, else
    /// an altitude above mean sea level converted with the EGM96 geoid. Empty as for altitudeMslMeters().
    [[nodiscard]] std::optional<double> ellipsoidHeightMeters() const;
    /// How far ellipsoidHeightMeters() is above altitudeMslMeters(): what a receiver that reports both heights has
    /// between them, else the EGM96 geoid at the position. Empty for invalid coordinates.
    [[nodiscard]] std::optional<double> geoidSeparationMeters() const;
};
