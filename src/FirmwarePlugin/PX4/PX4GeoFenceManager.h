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

class PX4GeoFenceManager : public GeoFenceManager
{
    Q_OBJECT
    
public:
    PX4GeoFenceManager(Vehicle* vehicle);
    ~PX4GeoFenceManager();

    // Overrides from GeoFenceManager
    bool            circleEnabled       (void) const { return true; }
    Fact*           circleRadiusFact    (void) const { return _circleRadiusFact; }
    QVariantList    params              (void) const final { return _params; }
    QStringList     paramLabels         (void) const final { return _paramLabels; }

private slots:
    void _parametersReady(void);
    
private:
    bool            _firstParamLoadComplete;
    Fact*           _circleRadiusFact;
    QVariantList    _params;
    QStringList     _paramLabels;
};

#endif
