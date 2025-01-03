/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Viewer3DQmlBackend.h"
#include "SettingsManager.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "Viewer3DSettings.h"
#include "OsmParser.h"

#define GPS_REF_NOT_SET                 0
#define GPS_REF_SET_BY_MAP              1
#define GPS_REF_SET_BY_VEHICLE          2

Viewer3DQmlBackend::Viewer3DQmlBackend(QObject *parent)
    : QObject{parent}
{
    _gpsRefSet = GPS_REF_NOT_SET;
    _activeVehicle = nullptr;
    _viewer3DSettings = SettingsManager::instance()->viewer3DSettings();
}

void Viewer3DQmlBackend::init(OsmParser* osmThr)
{
    _osmParserThread = osmThr;
    _activeVehicleChangedEvent(MultiVehicleManager::instance()->activeVehicle());

    connect(_osmParserThread, &OsmParser::gpsRefChanged, this, &Viewer3DQmlBackend::_gpsRefChangedEvent);
    connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this, &Viewer3DQmlBackend::_activeVehicleChangedEvent);
}

void Viewer3DQmlBackend::_activeVehicleChangedEvent(Vehicle *vehicle)
{
    if(_activeVehicle){
        disconnect(_activeVehicle, &Vehicle::coordinateChanged, this, &Viewer3DQmlBackend::_activeVehicleCoordinateChanged);
    }

    _activeVehicle = vehicle;
    if(!_activeVehicle){ // means that all the vehicle have been disconnected
        if(_gpsRefSet == GPS_REF_SET_BY_VEHICLE){
            _gpsRefSet = GPS_REF_NOT_SET;
        }
    }else{
        _activeVehicleCoordinateChanged(_activeVehicle->coordinate());
        connect(_activeVehicle, &Vehicle::coordinateChanged, this, &Viewer3DQmlBackend::_activeVehicleCoordinateChanged);
    }
}

void Viewer3DQmlBackend::_activeVehicleCoordinateChanged(QGeoCoordinate newCoordinate)
{
    if(_gpsRefSet == GPS_REF_NOT_SET){
        if(newCoordinate.latitude() && newCoordinate.longitude()){
            _gpsRef = newCoordinate;
            _gpsRef.setAltitude(0);
            _gpsRefSet = GPS_REF_SET_BY_VEHICLE;
            emit gpsRefChanged();

            qDebug() << "3D viewer gps reference set by vehicles:" << _gpsRef.latitude() << _gpsRef.longitude() << _gpsRef.altitude();
        }
    }
}

void Viewer3DQmlBackend::_gpsRefChangedEvent(QGeoCoordinate newGpsRef, bool isRefSet)
{
    if(isRefSet){
        _gpsRef = newGpsRef;
        _gpsRefSet = GPS_REF_SET_BY_MAP;
        emit gpsRefChanged();
        qDebug() << "3D viewer gps reference set by osm map:" << _gpsRef.latitude() << _gpsRef.longitude() << _gpsRef.altitude();
    }else{
        _gpsRefSet = GPS_REF_NOT_SET;
    }
}
