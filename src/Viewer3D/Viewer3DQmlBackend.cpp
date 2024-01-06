#include "Viewer3DQmlBackend.h"
#include <QQmlEngine>
#include <QQmlComponent>
#include <QUrl>

Viewer3DQmlBackend::Viewer3DQmlBackend(QObject *parent)
    : QObject{parent}
{
}

void Viewer3DQmlBackend::init(Viewer3DSettings *viewerSettingThr, OsmParser* osmThr)
{
    _altitudeBias = viewerSettingThr->altitudeBias()->rawValue().toFloat();
    _osmFilePath = viewerSettingThr->osmFilePath()->rawValue().toString();

    _osmParserThread = osmThr;
    connect(_osmParserThread, &OsmParser::gpsRefChanged, this, &Viewer3DQmlBackend::_gpsRefChangedEvent);
}

void Viewer3DQmlBackend::setGpsRef(const QGeoCoordinate &gpsRef)
{
    if(_gpsRef == gpsRef){
        return;
    }

    _gpsRef = gpsRef;
    emit gpsRefChanged();
}

void Viewer3DQmlBackend::_gpsRefChangedEvent(QGeoCoordinate newGpsRef)
{   
    _gpsRef = newGpsRef;

    emit gpsRefChanged();

    qDebug() << "3D map gps reference:" << _gpsRef.latitude() << _gpsRef.longitude() << _gpsRef.altitude();
}
