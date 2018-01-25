/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

/**
 * @class AirspaceRestrictionProvider
 * Base class that queries for airspace restrictions
 */

#include <QObject>
#include "AirspaceRestriction.h"

class AirspaceRestrictionProvider : public QObject {
    Q_OBJECT
public:
    AirspaceRestrictionProvider     (QObject* parent = NULL);
    ~AirspaceRestrictionProvider    () = default;

    /**
     * Set region of interest that should be queried. When finished, the requestDone() signal will be emmited.
     * @param center Center coordinate for ROI
     * @param radiusMeters Radius in meters around center which is of interest
     */
    virtual void setROI (const QGeoCoordinate& center, double radiusMeters) = 0;

    const QList<AirspacePolygonRestriction*>&   polygons() const { return _polygonList; }
    const QList<AirspaceCircularRestriction*>&  circles () const { return _circleList;  }

signals:
    void requestDone    (bool success);

protected:
    QList<AirspacePolygonRestriction*>  _polygonList;
    QList<AirspaceCircularRestriction*> _circleList;
};
