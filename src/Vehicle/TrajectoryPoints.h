/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPath>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(TrajectoryPointsLog)

class Vehicle;

class TrajectoryPoints : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(QList<QGeoCoordinate> path READ path NOTIFY pathChanged)

public:
    explicit TrajectoryPoints(Vehicle *vehicle, QObject *parent = nullptr);
    ~TrajectoryPoints();

    /// The simplified trajectory of the vehicle
    const QList<QGeoCoordinate> &path() const { return _path.path(); }

    /// Begin recording trajectory points (clears any existing path)
    void start();

    /// Stop recording trajectory points
    void stop();

public slots:
    /// Clear the current path and reset state
    void clear();

signals:
    /// Emitted whenever the path is changed (added or updated coordinate)
    void pathChanged(const QGeoPath &path);

private slots:
    /// Slot connected to Vehicle::coordinateChanged
    void _vehicleCoordinateChanged(QGeoCoordinate coordinate);

private:
    Vehicle *_vehicle = nullptr;
    QGeoPath _path;                 ///< Accumulated trajectory (graphics‑friendly)
    QGeoCoordinate _lastPoint;      ///< Cached last point for distance/azimuth checks
    double _lastAzimuth = qQNaN();

    static constexpr double kDistanceTolerance = 2.0;   ///< metres
    static constexpr double kAzimuthTolerance = 1.5;    ///< degrees
};
