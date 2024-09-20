/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtPositioning/QGeoCoordinate>

#include "Viewer3DUtils.h"

///     @author Omid Esrafilian <esrafilian.omid@gmail.com>

class GeoCoordinateType: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QGeoCoordinate gpsRef READ gpsRef WRITE setGpsRef NOTIFY gpsRefChanged)
    Q_PROPERTY(QGeoCoordinate coordinate READ coordinate WRITE setCoordinate NOTIFY coordinateChanged)
    Q_PROPERTY(QVector3D localCoordinate READ localCoordinate NOTIFY localCoordinateChanged)

public:
    explicit GeoCoordinateType(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    void gps_to_local()
    {
        QGeoCoordinate gps_tmp = _coordinate;
        QGeoCoordinate gps_ref_tmp = _gpsRef;

        QVector3D local_pose_tmp = mapGpsToLocalPoint(gps_tmp, gps_ref_tmp);

        if(_localCoordinate != local_pose_tmp) {
            _localCoordinate = local_pose_tmp;
            _localCoordinate.setZ(_coordinate.altitude());
            emit localCoordinateChanged();
        }
    }

    QGeoCoordinate gpsRef() const
    {
        return _gpsRef;
    }
    void setGpsRef(const QGeoCoordinate &newGps_ref)
    {
        if (_gpsRef == newGps_ref) {
            return;
        }
        _gpsRef = newGps_ref;

        gps_to_local();

        emit gpsRefChanged();
    }

    QGeoCoordinate coordinate() const
    {
        return _coordinate;
    }
    void setCoordinate(const QGeoCoordinate& newCoordinate)
    {
        if (_coordinate == newCoordinate) {
            return;
        }
        _coordinate = newCoordinate;

        gps_to_local();
        emit coordinateChanged();
    }

    QVector3D localCoordinate(){return _localCoordinate;}

signals:
    void gpsRefChanged();
    void coordinateChanged();
    void localCoordinateChanged();

protected slots:
    void gpsRefChangedEvent()
    {
        gps_to_local();

        emit gpsRefChanged();
    }

    void coordinateChangedEvent()
    {
        gps_to_local();

        emit coordinateChanged();
    }

private:
    QGeoCoordinate _gpsRef;
    QGeoCoordinate _coordinate;
    QVector3D _localCoordinate;
};
