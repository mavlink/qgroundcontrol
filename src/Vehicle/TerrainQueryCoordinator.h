#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtPositioning/QGeoCoordinate>

class TerrainAtCoordinateQuery;
class Vehicle;

/// \brief Coordinates the terrain-query workflows attached to a Vehicle:
///
///   1. DO_SET_HOME: query terrain altitude for the target coord, then send
///      MAV_CMD_DO_SET_HOME with a sanity-checked AMSL altitude.
///   2. Altitude-above-terrain polling: on Vehicle coordinate updates, query terrain
///      and update the vehicle's altitudeAboveTerr fact, throttled by time/distance.
///
/// Each workflow owns a single outstanding TerrainAtCoordinateQuery; starting a new
/// request while one is in-flight disconnects the previous one (auto-deleted by
/// TerrainQuery). Callbacks access the Vehicle to send commands or write facts.
///
class TerrainQueryCoordinator : public QObject
{
    Q_OBJECT

public:
    explicit TerrainQueryCoordinator(Vehicle* vehicle);

    /// DO_SET_HOME terrain-backed flow. Starts a terrain query; when it resolves,
    /// sends MAV_CMD_DO_SET_HOME if the coordinate and resolved altitude are valid.
    void doSetHomeWithTerrain(const QGeoCoordinate& coord);

    /// Periodic altitude-above-terrain query. Connected to Vehicle::coordinateChanged.
    /// Throttled by time (500 ms min) and distance/altitude deltas (2 m / 0.5 m).
    void updateAltAboveTerrain();

private slots:
    void _doSetHomeTerrainReceived(bool success, QList<double> heights);
    void _altitudeAboveTerrainReceived(bool success, QList<double> heights);

private:
    Vehicle* _vehicle = nullptr;

    // QPointer: TerrainAtCoordinateQuery self-deletes after its signal (autoDelete=true).
    QPointer<TerrainAtCoordinateQuery> _doSetHomeQuery;
    QGeoCoordinate                     _doSetHomeCoordinate;

    QElapsedTimer                      _altQueryTimer;
    QPointer<TerrainAtCoordinateQuery> _altQuery;
    QGeoCoordinate                     _altLastCoord;
    float                              _altLastRelAlt = qQNaN();
};
