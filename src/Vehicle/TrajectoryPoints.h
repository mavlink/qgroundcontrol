/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtPositioning/QGeoCoordinate>
#include <QtCore/QObject>
#include <QtCore/QVariantList>
class Vehicle;

enum class PositionSrc{
  eSrc_GlobalPosition = 0,
  eSrc_GPSRaw
};

class TrajectoryPoints : public QObject
{
    Q_OBJECT

public:
    TrajectoryPoints(Vehicle* vehicle, QObject* parent = nullptr);

    Q_INVOKABLE QVariantList list(void) const { return _points; }
    Q_INVOKABLE QVariantList gpsList(void) const {return _gpsPoints;}

    void start  (void);
    void stop   (void);

public slots:
    void clear  (void);

signals:
    void pointAdded     (QGeoCoordinate coordinate);
    void gpsPointAdded  (QGeoCoordinate coordinate);
    void updateLastPoint(QGeoCoordinate coordinate);
    void gpsUpdateLastPoint(QGeoCoordinate coordinate);
    void pointsCleared  (void);

private slots:
    void _vehicleCoordinateChanged(QGeoCoordinate coordinate, PositionSrc src);

private:
    Vehicle*        _vehicle;
    QVariantList    _points,
                    _gpsPoints;
    QGeoCoordinate  _lastPoint,
                    _gpsLastPoit;
    double          _lastAzimuth,
                    _gpsLastAzimuth;

    static constexpr double _distanceTolerance = 2.0;
    static constexpr double _azimuthTolerance = 1.5;
};
