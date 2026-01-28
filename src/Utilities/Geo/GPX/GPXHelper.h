#pragma once

#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPolygon>

#include "GeoFormatRegistry.h"

Q_DECLARE_LOGGING_CATEGORY(GPXHelperLog)

/// Helper functions for reading and writing GPX (GPS Exchange Format) files.
///
/// GPX Element Mapping:
/// - Waypoints (wpt) → Points
/// - Routes (rte/rtept) → Polylines (planned paths)
/// - Tracks (trk/trkseg/trkpt) → Polylines (recorded GPS logs)
/// - Polygons: Treated as closed routes (first point repeated at end)
///
/// GPX 1.1 Specification: https://www.topografix.com/GPX/1/1/
namespace GPXHelper
{
    /// Determine the primary shape type in the GPX file
    /// Priority: Polygon (closed route) > Polyline (route/track) > Point (waypoint)
    GeoFormatRegistry::ShapeType determineShapeType(const QString &filePath, QString &errorString);

    /// Get the number of geometry entities in the GPX file
    /// Counts: waypoints + routes + track segments
    int getEntityCount(const QString &filePath, QString &errorString);

    // ========================================================================
    // Load functions
    // ========================================================================

    /// Load first polygon entity (closed route or track)
    /// A polygon is detected when the first and last coordinates are identical or very close
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolygonFromFile(const QString &filePath, QList<QGeoCoordinate> &vertices, QString &errorString,
                             double filterMeters = GeoFormatRegistry::kDefaultVertexFilterMeters);

    /// Load all polygon entities (closed routes and tracks)
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolygonsFromFile(const QString &filePath, QList<QList<QGeoCoordinate>> &polygons, QString &errorString,
                              double filterMeters = GeoFormatRegistry::kDefaultVertexFilterMeters);

    /// @note GPX format does not support polygons with holes
    /// This stub function returns false with a clear error message
    bool loadPolygonWithHolesFromFile(const QString &filePath, QGeoPolygon &polygon, QString &errorString);

    /// @note GPX format does not support polygons with holes
    /// This stub function returns false with a clear error message
    bool loadPolygonsWithHolesFromFile(const QString &filePath, QList<QGeoPolygon> &polygons, QString &errorString);

    /// Load first polyline entity (route or track segment)
    /// Routes take priority over tracks
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolylineFromFile(const QString &filePath, QList<QGeoCoordinate> &coords, QString &errorString,
                              double filterMeters = GeoFormatRegistry::kDefaultVertexFilterMeters);

    /// Load all polyline entities (all routes and track segments)
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolylinesFromFile(const QString &filePath, QList<QList<QGeoCoordinate>> &polylines, QString &errorString,
                               double filterMeters = GeoFormatRegistry::kDefaultVertexFilterMeters);

    /// Load first waypoint entity (convenience wrapper)
    bool loadPointFromFile(const QString &filePath, QGeoCoordinate &point, QString &errorString);

    /// Load all waypoint entities
    bool loadPointsFromFile(const QString &filePath, QList<QGeoCoordinate> &points, QString &errorString);

    // ========================================================================
    // Save functions
    // ========================================================================

    /// Save a single polygon to file as a closed route
    /// The first point is automatically repeated at the end to close the polygon
    bool savePolygonToFile(const QString &filePath, const QList<QGeoCoordinate> &vertices, QString &errorString);

    /// Save multiple polygons to file as closed routes
    bool savePolygonsToFile(const QString &filePath, const QList<QList<QGeoCoordinate>> &polygons, QString &errorString);

    /// Save a single polyline to file as a route
    bool savePolylineToFile(const QString &filePath, const QList<QGeoCoordinate> &coords, QString &errorString);

    /// Save multiple polylines to file as routes
    bool savePolylinesToFile(const QString &filePath, const QList<QList<QGeoCoordinate>> &polylines, QString &errorString);

    /// Save a single waypoint to file
    bool savePointToFile(const QString &filePath, const QGeoCoordinate &point, QString &errorString);

    /// Save waypoints to file
    bool savePointsToFile(const QString &filePath, const QList<QGeoCoordinate> &points, QString &errorString);

    /// Save a track (recorded GPS log) to file
    /// @param coords Track points with timestamps (altitude used, time not preserved)
    bool saveTrackToFile(const QString &filePath, const QList<QGeoCoordinate> &coords, QString &errorString);

    /// Save multiple tracks to file
    bool saveTracksToFile(const QString &filePath, const QList<QList<QGeoCoordinate>> &tracks, QString &errorString);
}
