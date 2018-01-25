/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirspaceRestriction.h"

AirspaceRestriction::AirspaceRestriction(QObject* parent)
    : QObject(parent)
{
}

AirspacePolygonRestriction::AirspacePolygonRestriction(const QVariantList& polygon, QObject* parent)
    : AirspaceRestriction(parent)
    , _polygon(polygon)
{

}

AirspaceCircularRestriction::AirspaceCircularRestriction(const QGeoCoordinate& center, double radius, QObject* parent)
    : AirspaceRestriction(parent)
    , _center(center)
    , _radius(radius)
{

}

