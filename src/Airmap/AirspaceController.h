/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "AirspaceManagement.h"
#include "QGCMapPolygon.h"

class AirspaceController : public QObject
{
    Q_OBJECT
    
public:
    AirspaceController(QObject* parent = NULL);
    ~AirspaceController() = default;

    Q_PROPERTY(QmlObjectListModel*  polygons    READ polygons       CONSTANT)   ///< List of PolygonAirspaceRestriction objects
    Q_PROPERTY(QmlObjectListModel*  circles     READ circles        CONSTANT)   ///< List of CircularAirspaceRestriction objects
    Q_PROPERTY(QString              weatherIcon READ weatherIcon    NOTIFY weatherChanged)
    Q_PROPERTY(int                  weatherTemp READ weatherTemp    NOTIFY weatherChanged)
    Q_PROPERTY(bool                 hasWeather  READ hasWeather     NOTIFY weatherChanged)

    Q_INVOKABLE void setROI(QGeoCoordinate center, double radius);

    QmlObjectListModel* polygons() { return _manager->polygonRestrictions(); }
    QmlObjectListModel* circles() { return _manager->circularRestrictions(); }

    Q_PROPERTY(QString providerName             READ providerName CONSTANT)

    QString providerName() { return _manager->name(); }

    QString weatherIcon         () { return _weatherIcon; }
    int     weatherTemp         () { return _weatherTemp; }
    bool    hasWeather          () { return _hasWeather;  }

signals:
    void    weatherChanged      ();

private slots:
    void    _weatherUpdate      (bool success, QGeoCoordinate coordinate, WeatherInformation weather);

private:
    AirspaceManager*    _manager;
    QGeoCoordinate      _lastRoiCenter;
    QTime               _weatherTime;
    QString             _weatherIcon;
    int                 _weatherTemp;
    bool                _hasWeather;
};
