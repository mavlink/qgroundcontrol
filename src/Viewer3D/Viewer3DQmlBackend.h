#ifndef Viewer3DQmlBackend_H
#define Viewer3DQmlBackend_H

#include <QObject>

#include "OsmParser.h"
#include "Viewer3DSettings.h"

///     @author Omid Esrafilian <esrafilian.omid@gmail.com>

class Viewer3DSettings;
class Vehicle;

class Viewer3DQmlBackend : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QGeoCoordinate gpsRef READ gpsRef NOTIFY gpsRefChanged)

public:
    explicit Viewer3DQmlBackend(QObject *parent = nullptr);

    void init(OsmParser* osmThr=nullptr);

    QGeoCoordinate gpsRef(){return _gpsRef;}

signals:
    void gpsRefChanged();

private:
    OsmParser *_osmParserThread;

    QGeoCoordinate _gpsRef;
    uint8_t _gpsRefSet;

    Vehicle *_activeVehicle;
    Viewer3DSettings* _viewer3DSettings = nullptr;


protected slots:
    void _gpsRefChangedEvent(QGeoCoordinate newGpsRef, bool isRefSet);
    void _activeVehicleChangedEvent(Vehicle* vehicle);
    void _activeVehicleCoordinateChanged(QGeoCoordinate newCoordinate);
};

#endif // Viewer3DQmlBackend_H
