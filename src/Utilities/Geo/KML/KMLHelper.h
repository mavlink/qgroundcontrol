#pragma once

#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPolygon>

#include "GeoFormatRegistry.h"

Q_DECLARE_LOGGING_CATEGORY(KMLHelperLog)

namespace KMLHelper
{
    GeoFormatRegistry::ShapeType determineShapeType(const QString &file, QString &errorString);

    /// Get the number of geometry entities in the KML file
    int getEntityCount(const QString &kmlFile, QString &errorString);

    /// Load first polygon entity (convenience wrapper)
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolygonFromFile(const QString &kmlFile, QList<QGeoCoordinate> &vertices, QString &errorString,
                             double filterMeters = GeoFormatRegistry::kDefaultVertexFilterMeters);

    /// Load all polygon entities
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolygonsFromFile(const QString &kmlFile, QList<QList<QGeoCoordinate>> &polygons, QString &errorString,
                              double filterMeters = GeoFormatRegistry::kDefaultVertexFilterMeters);

    /// Load first polygon with holes (QGeoPolygon preserves hole information)
    bool loadPolygonWithHolesFromFile(const QString &kmlFile, QGeoPolygon &polygon, QString &errorString);

    /// Load all polygons with holes
    bool loadPolygonsWithHolesFromFile(const QString &kmlFile, QList<QGeoPolygon> &polygons, QString &errorString);

    /// Load first polyline entity (convenience wrapper)
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolylineFromFile(const QString &kmlFile, QList<QGeoCoordinate> &coords, QString &errorString,
                              double filterMeters = GeoFormatRegistry::kDefaultVertexFilterMeters);

    /// Load all polyline entities
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolylinesFromFile(const QString &kmlFile, QList<QList<QGeoCoordinate>> &polylines, QString &errorString,
                               double filterMeters = GeoFormatRegistry::kDefaultVertexFilterMeters);

    /// Load first point entity (convenience wrapper)
    bool loadPointFromFile(const QString &kmlFile, QGeoCoordinate &point, QString &errorString);

    /// Load all point entities
    bool loadPointsFromFile(const QString &kmlFile, QList<QGeoCoordinate> &points, QString &errorString);

    // ========================================================================
    // Save functions
    // ========================================================================

    /// Save a single polygon to file
    bool savePolygonToFile(const QString &kmlFile, const QList<QGeoCoordinate> &vertices, QString &errorString);

    /// Save multiple polygons to file
    bool savePolygonsToFile(const QString &kmlFile, const QList<QList<QGeoCoordinate>> &polygons, QString &errorString);

    /// Save a polygon with holes to file
    bool savePolygonWithHolesToFile(const QString &kmlFile, const QGeoPolygon &polygon, QString &errorString);

    /// Save multiple polygons with holes to file
    bool savePolygonsWithHolesToFile(const QString &kmlFile, const QList<QGeoPolygon> &polygons, QString &errorString);

    /// Save a single polyline to file
    bool savePolylineToFile(const QString &kmlFile, const QList<QGeoCoordinate> &coords, QString &errorString);

    /// Save multiple polylines to file
    bool savePolylinesToFile(const QString &kmlFile, const QList<QList<QGeoCoordinate>> &polylines, QString &errorString);

    /// Save a single point to file
    bool savePointToFile(const QString &kmlFile, const QGeoCoordinate &point, QString &errorString);

    /// Save points to file
    bool savePointsToFile(const QString &kmlFile, const QList<QGeoCoordinate> &points, QString &errorString);
}
