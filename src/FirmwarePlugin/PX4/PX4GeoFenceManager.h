/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef PX4GeoFenceManager_H
#define PX4GeoFenceManager_H

#include "GeoFenceManager.h"
#include "QGCMAVLink.h"
#include "FactSystem.h"
#include "PlanManager.h"

class PX4GeoFenceManager : public GeoFenceManager
{
    Q_OBJECT
    
public:
    PX4GeoFenceManager(Vehicle* vehicle);
    ~PX4GeoFenceManager();

    // Overrides from GeoFenceManager
    bool            inProgress              (void) const final;
    void            loadFromVehicle         (void) final;
    void            sendToVehicle           (const QGeoCoordinate& breachReturn, QmlObjectListModel& inclusionPolygons, QmlObjectListModel& exclusionPolygons) final;
    void            removeAll               (void) final;
    bool            polygonSupported        (void) const final { return true; }
    bool            polygonEnabled          (void) const final { return true; }
    bool            breachReturnSupported   (void) const final { return true; }
    bool            circleEnabled           (void) const final { return true; }
    Fact*           circleRadiusFact        (void) const final { return _circleRadiusFact; }
    QVariantList    params                  (void) const final { return _params; }
    QStringList     paramLabels             (void) const final { return _paramLabels; }

private slots:
    void _parametersReady           (void);
    void _sendComplete              (bool error);
    void _planManagerLoadComplete   (bool removeAllRequested);

private:
    PlanManager                     _planManager;
    bool                            _firstParamLoadComplete;
    Fact*                           _circleRadiusFact;
    QVariantList                    _params;
    QStringList                     _paramLabels;
    QList<QList<QGeoCoordinate>>    _sendInclusions;
    QList<QList<QGeoCoordinate>>    _sendExclusions;
};

#endif
