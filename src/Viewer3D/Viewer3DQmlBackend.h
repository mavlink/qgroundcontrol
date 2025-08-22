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
#include <QtQmlIntegration/QtQmlIntegration>

///     @author Omid Esrafilian <esrafilian.omid@gmail.com>

class Viewer3DSettings;
class Vehicle;
class OsmParser;

class Viewer3DQmlBackend : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(QGeoCoordinate gpsRef READ gpsRef NOTIFY gpsRefChanged)

public:
    explicit Viewer3DQmlBackend(QObject *parent = nullptr);

    void init(OsmParser* osmThr=nullptr);

    QGeoCoordinate gpsRef(){return _gpsRef;}

signals:
    void gpsRefChanged();

private:
    OsmParser *_osmParserThread = nullptr;

    QGeoCoordinate _gpsRef;

    enum GpsRefType {
        GPS_REF_NOT_SET = 0,
        GPS_REF_SET_BY_MAP = 1,
        GPS_REF_SET_BY_VEHICLE = 2
    };

    GpsRefType _gpsRefSet = GPS_REF_NOT_SET;

    Vehicle *_activeVehicle = nullptr;
    Viewer3DSettings* _viewer3DSettings = nullptr;


protected slots:
    void _gpsRefChangedEvent(QGeoCoordinate newGpsRef, bool isRefSet);
    void _activeVehicleChangedEvent(Vehicle* vehicle);
    void _activeVehicleCoordinateChanged(QGeoCoordinate newCoordinate);
};
