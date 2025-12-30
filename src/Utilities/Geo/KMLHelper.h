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
#include <QtCore/QLoggingCategory>
#include <QtPositioning/QGeoCoordinate>

#include "ShapeFileHelper.h"

Q_DECLARE_LOGGING_CATEGORY(KMLHelperLog)

namespace KMLHelper
{
    ShapeFileHelper::ShapeType determineShapeType(const QString &file, QString &errorString);
    bool loadPolygonFromFile(const QString &kmlFile, QList<QGeoCoordinate> &vertices, QString &errorString);
    bool loadPolylineFromFile(const QString &kmlFile, QList<QGeoCoordinate> &coords, QString &errorString);
};
