/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "LifetimeChecker.h"
#include "AirspaceRestrictionProvider.h"
#include "AirMapSharedState.h"

#include <QList>
#include <QGeoCoordinate>

#include "airmap/geometry.h"

/**
 * @file AirMapRestrictionManager.h
 * Class to download polygons from AirMap
 */

class AirMapRestrictionManager : public AirspaceRestrictionProvider, public LifetimeChecker
{
    Q_OBJECT
public:
    AirMapRestrictionManager        (AirMapSharedState& shared);
    void setROI                     (const QGeoCoordinate& center, double radiusMeters) override;

signals:
    void error                      (const QString& what, const QString& airmapdMessage, const QString& airmapdDetails);

private:
    static void _addPolygonToList   (const airmap::Geometry::Polygon& polygon, QList<AirspacePolygonRestriction*>& list);
    enum class State {
        Idle,
        RetrieveItems,
    };
    State                   _state = State::Idle;
    AirMapSharedState&      _shared;
};

