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

    Q_PROPERTY(QmlObjectListModel*  polygons    READ polygons   CONSTANT)   ///< List of PolygonAirspaceRestriction objects
    Q_PROPERTY(QmlObjectListModel*  circles     READ circles    CONSTANT)   ///< List of CircularAirspaceRestriction objects

    Q_INVOKABLE void setROI(QGeoCoordinate center, double radius) { _manager->setROI(center, radius); }

    QmlObjectListModel* polygons() { return _manager->polygonRestrictions(); }
    QmlObjectListModel* circles() { return _manager->circularRestrictions(); }

    Q_PROPERTY(QString providerName             READ providerName CONSTANT)

    QString providerName() { return _manager->name(); }
private:
    AirspaceManager*      _manager;
};
