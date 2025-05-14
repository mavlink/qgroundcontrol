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

Q_DECLARE_LOGGING_CATEGORY(SHPFileHelperLog)

namespace SHPFileHelper
{
    ShapeFileHelper::ShapeType determineShapeType(const QString &shpFile, QString &errorString);
    bool loadPolygonFromFile(const QString &shpFile, QList<QGeoCoordinate> &vertices, QString &errorString);
    bool loadPolylineFromFile(const QString &shpFile, QList<QGeoCoordinate> &coords, QString &errorString);
};
