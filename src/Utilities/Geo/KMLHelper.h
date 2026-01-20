#pragma once

#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtPositioning/QGeoCoordinate>

#include "ShapeFileHelper.h"

Q_DECLARE_LOGGING_CATEGORY(KMLHelperLog)

namespace KMLHelper
{
    ShapeFileHelper::ShapeType determineShapeType(const QString &file, QString &errorString);

    /// Get the number of geometry entities in the KML file
    int getEntityCount(const QString &kmlFile, QString &errorString);

    /// Load first polygon entity (convenience wrapper)
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolygonFromFile(const QString &kmlFile, QList<QGeoCoordinate> &vertices, QString &errorString,
                             double filterMeters = ShapeFileHelper::kDefaultVertexFilterMeters);

    /// Load all polygon entities
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolygonsFromFile(const QString &kmlFile, QList<QList<QGeoCoordinate>> &polygons, QString &errorString,
                              double filterMeters = ShapeFileHelper::kDefaultVertexFilterMeters);

    /// Load first polyline entity (convenience wrapper)
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolylineFromFile(const QString &kmlFile, QList<QGeoCoordinate> &coords, QString &errorString,
                              double filterMeters = ShapeFileHelper::kDefaultVertexFilterMeters);

    /// Load all polyline entities
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolylinesFromFile(const QString &kmlFile, QList<QList<QGeoCoordinate>> &polylines, QString &errorString,
                               double filterMeters = ShapeFileHelper::kDefaultVertexFilterMeters);

    /// Load all point entities
    bool loadPointsFromFile(const QString &kmlFile, QList<QGeoCoordinate> &points, QString &errorString);
}
