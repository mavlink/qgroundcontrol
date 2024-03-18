#include "Viewer3DQmlBackend.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "Vehicle.h"

#define GPS_REF_NOT_SET                 0
#define GPS_REF_SET_BY_MAP              1
#define GPS_REF_SET_BY_VEHICLE          2

Viewer3DQmlBackend::Viewer3DQmlBackend(QObject *parent)
    : QObject{parent}
{
    _gpsRefSet = GPS_REF_NOT_SET;
    _activeVehicle = nullptr;
    _viewer3DSettings = qgcApp()->toolbox()->settingsManager()->viewer3DSettings();
}

void Viewer3DQmlBackend::init(OsmParser* osmThr)
{
    _osmParserThread = osmThr;
    _activeVehicleChangedEvent(qgcApp()->toolbox()->multiVehicleManager()->activeVehicle());

    connect(_osmParserThread, &OsmParser::gpsRefChanged, this, &Viewer3DQmlBackend::_gpsRefChangedEvent);
    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::activeVehicleChanged, this, &Viewer3DQmlBackend::_activeVehicleChangedEvent);
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
