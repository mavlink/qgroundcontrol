/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirspaceRestriction.h"

AirspaceRestriction::AirspaceRestriction(QString advisoryID, QColor color, QColor lineColor, float lineWidth, QObject* parent)
    : QObject(parent)
    , _advisoryID(advisoryID)
    , _color(color)
    , _lineColor(lineColor)
    , _lineWidth(lineWidth)
{
}

AirspacePolygonRestriction::AirspacePolygonRestriction(const QVariantList& polygon, QString advisoryID, QColor color, QColor lineColor, float lineWidth, QObject* parent)
    : AirspaceRestriction(advisoryID, color, lineColor, lineWidth, parent)
    , _polygon(polygon)
{

}

AirspaceCircularRestriction::AirspaceCircularRestriction(const QGeoCoordinate& center, double radius, QString advisoryID, QColor color, QColor lineColor, float lineWidth, QObject* parent)
    : AirspaceRestriction(advisoryID, color, lineColor, lineWidth, parent)
    , _center(center)
    , _radius(radius)
{

}

