#pragma once

#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPolygon>

#include "GeoFormatRegistry.h"

Q_DECLARE_LOGGING_CATEGORY(SHPHelperLog)

namespace SHPHelper
{
    GeoFormatRegistry::ShapeType determineShapeType(const QString &file, QString &errorString);

    /// Get the number of entities in the shapefile
    int getEntityCount(const QString &shpFile, QString &errorString);

    /// Load first polygon entity (convenience wrapper)
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolygonFromFile(const QString &shpFile, QList<QGeoCoordinate> &vertices, QString &errorString,
                             double filterMeters = GeoFormatRegistry::kDefaultVertexFilterMeters);

    /// Load all polygon entities
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolygonsFromFile(const QString &shpFile, QList<QList<QGeoCoordinate>> &polygons, QString &errorString,
                              double filterMeters = GeoFormatRegistry::kDefaultVertexFilterMeters);

    /// Load first polygon with holes (QGeoPolygon preserves hole information)
    bool loadPolygonWithHolesFromFile(const QString &shpFile, QGeoPolygon &polygon, QString &errorString);

    /// Load all polygons with holes
    bool loadPolygonsWithHolesFromFile(const QString &shpFile, QList<QGeoPolygon> &polygons, QString &errorString);

    /// Load first polyline entity (convenience wrapper)
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolylineFromFile(const QString &shpFile, QList<QGeoCoordinate> &coords, QString &errorString,
                              double filterMeters = GeoFormatRegistry::kDefaultVertexFilterMeters);

    /// Load all polyline entities
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolylinesFromFile(const QString &shpFile, QList<QList<QGeoCoordinate>> &polylines, QString &errorString,
                               double filterMeters = GeoFormatRegistry::kDefaultVertexFilterMeters);

    /// Load first point entity (convenience wrapper)
    bool loadPointFromFile(const QString &shpFile, QGeoCoordinate &point, QString &errorString);

    /// Load all point entities
    bool loadPointsFromFile(const QString &shpFile, QList<QGeoCoordinate> &points, QString &errorString);

    // ========================================================================
    // Save functions
    // ========================================================================

    /// Save a single polygon to file
    bool savePolygonToFile(const QString &shpFile, const QList<QGeoCoordinate> &vertices, QString &errorString);

    /// Save multiple polygons to file
    bool savePolygonsToFile(const QString &shpFile, const QList<QList<QGeoCoordinate>> &polygons, QString &errorString);

    /// Save a polygon with holes to file
    bool savePolygonWithHolesToFile(const QString &shpFile, const QGeoPolygon &polygon, QString &errorString);

    /// Save multiple polygons with holes to file
    bool savePolygonsWithHolesToFile(const QString &shpFile, const QList<QGeoPolygon> &polygons, QString &errorString);

    /// Save a single polyline to file
    bool savePolylineToFile(const QString &shpFile, const QList<QGeoCoordinate> &coords, QString &errorString);

    /// Save multiple polylines to file
    bool savePolylinesToFile(const QString &shpFile, const QList<QList<QGeoCoordinate>> &polylines, QString &errorString);

    /// Save a single point to file
    bool savePointToFile(const QString &shpFile, const QGeoCoordinate &point, QString &errorString);

    /// Save points to file
    bool savePointsToFile(const QString &shpFile, const QList<QGeoCoordinate> &points, QString &errorString);

    /// Write WGS84 PRJ file for a shapefile
    /// @param shpFile Path to the .shp file (will create .prj with same basename)
    bool writePrjFile(const QString &shpFile, QString &errorString);
}
