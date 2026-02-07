#pragma once

#include <QtCore/QList>
#include <QtPositioning/QGeoCoordinate>

#include "ShapeFileHelper.h"


namespace SHPFileHelper
{
    ShapeFileHelper::ShapeType determineShapeType(const QString &file, QString &errorString);

    /// Get the number of entities in the shapefile
    int getEntityCount(const QString &shpFile, QString &errorString);

    /// Load first polygon entity (convenience wrapper)
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolygonFromFile(const QString &shpFile, QList<QGeoCoordinate> &vertices, QString &errorString,
                             double filterMeters = ShapeFileHelper::kDefaultVertexFilterMeters);

    /// Load all polygon entities
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolygonsFromFile(const QString &shpFile, QList<QList<QGeoCoordinate>> &polygons, QString &errorString,
                              double filterMeters = ShapeFileHelper::kDefaultVertexFilterMeters);

    /// Load first polyline entity (convenience wrapper)
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolylineFromFile(const QString &shpFile, QList<QGeoCoordinate> &coords, QString &errorString,
                              double filterMeters = ShapeFileHelper::kDefaultVertexFilterMeters);

    /// Load all polyline entities
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolylinesFromFile(const QString &shpFile, QList<QList<QGeoCoordinate>> &polylines, QString &errorString,
                               double filterMeters = ShapeFileHelper::kDefaultVertexFilterMeters);

    /// Load all point entities
    bool loadPointsFromFile(const QString &shpFile, QList<QGeoCoordinate> &points, QString &errorString);
}
