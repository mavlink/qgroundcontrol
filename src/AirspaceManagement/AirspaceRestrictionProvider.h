/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

#include "QmlObjectListModel.h"
#include "QGCGeoBoundingCube.h"

#include <QObject>
#include <QList>
#include <QGeoCoordinate>

class AirspacePolygonRestriction;
class AirspaceCircularRestriction;

class AirspaceRestrictionProvider : public QObject {
    Q_OBJECT
public:
    AirspaceRestrictionProvider     (QObject* parent = nullptr);
    ~AirspaceRestrictionProvider    () = default;

    Q_PROPERTY(QmlObjectListModel*  polygons        READ polygons       CONSTANT)
    Q_PROPERTY(QmlObjectListModel*  circles         READ circles        CONSTANT)

    /**
     * Set region of interest that should be queried. When finished, the requestDone() signal will be emmited.
     * @param center Center coordinate for ROI
     * @param radiusMeters Radius in meters around center which is of interest
     */
    virtual void setROI (const QGCGeoBoundingCube& roi, bool reset = false) = 0;

    virtual QmlObjectListModel* polygons        () = 0;     ///< List of AirspacePolygonRestriction objects
    virtual QmlObjectListModel* circles         () = 0;     ///< List of AirspaceCircularRestriction objects
};
