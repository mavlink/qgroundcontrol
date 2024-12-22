/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QList>
#include <QtPositioning/QGeoCoordinate>

#include "ShapeFileHelper.h"

namespace KMLHelper
{
    ShapeFileHelper::ShapeType determineShapeType(const QString &kmlFile, QString &errorString);
    bool loadPolygonFromFile(const QString &kmlFile, QList<QGeoCoordinate> &vertices, QString &errorString);
    bool loadPolylineFromFile(const QString &kmlFile, QList<QGeoCoordinate> &coords, QString &errorString);
};
