#pragma once

#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtPositioning/QGeoCoordinate>

#include "ShapeFileHelper.h"

Q_DECLARE_LOGGING_CATEGORY(SHPFileHelperLog)

namespace SHPFileHelper
{
    ShapeFileHelper::ShapeType determineShapeType(const QString &file, QString &errorString);
    bool loadPolygonFromFile(const QString &shpFile, QList<QGeoCoordinate> &vertices, QString &errorString);
    bool loadPolylineFromFile(const QString &shpFile, QList<QGeoCoordinate> &coords, QString &errorString);
};
