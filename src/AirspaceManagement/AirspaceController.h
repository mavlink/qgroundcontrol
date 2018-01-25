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
#include "AirmapWeatherInformation.h"

class AirspaceController : public QObject
{
    Q_OBJECT
public:
    AirspaceController(QObject* parent = NULL);
    ~AirspaceController() = default;

    Q_PROPERTY(QmlObjectListModel*          polygons        READ polygons       CONSTANT)   ///< List of AirspacePolygonRestriction objects
    Q_PROPERTY(QmlObjectListModel*          circles         READ circles        CONSTANT)   ///< List of AirspaceCircularRestriction objects
    Q_PROPERTY(QString                      providerName    READ providerName   CONSTANT)
    Q_PROPERTY(AirspaceWeatherInfoProvider* weatherInfo     READ weatherInfo    CONSTANT)

    Q_INVOKABLE void setROI                 (QGeoCoordinate center, double radius);

    QmlObjectListModel*             polygons    () { return _manager->polygonRestrictions();  }
    QmlObjectListModel*             circles     () { return _manager->circularRestrictions(); }
    QString                         providerName() { return _manager->name(); }
    AirspaceWeatherInfoProvider*    weatherInfo () { return _manager->weatherInfo(); }

private:
    AirspaceManager*    _manager;
};
