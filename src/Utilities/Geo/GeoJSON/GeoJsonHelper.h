#pragma once

#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPolygon>

#include "GeoFormatRegistry.h"

class QJsonValue;

Q_DECLARE_LOGGING_CATEGORY(GeoJsonHelperLog)

namespace GeoJsonHelper
{
    GeoFormatRegistry::ShapeType determineShapeType(const QString &filePath, QString &errorString);

    /// Get the number of geometry entities in the GeoJSON file
    int getEntityCount(const QString &filePath, QString &errorString);

    // ========================================================================
    // Load functions
    // ========================================================================

    /// Load first polygon entity (convenience wrapper)
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolygonFromFile(const QString &filePath, QList<QGeoCoordinate> &vertices, QString &errorString,
                             double filterMeters = GeoFormatRegistry::kDefaultVertexFilterMeters);

    /// Load all polygon entities
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolygonsFromFile(const QString &filePath, QList<QList<QGeoCoordinate>> &polygons, QString &errorString,
                              double filterMeters = GeoFormatRegistry::kDefaultVertexFilterMeters);

    /// Load first polygon with holes (QGeoPolygon preserves hole information)
    bool loadPolygonWithHolesFromFile(const QString &filePath, QGeoPolygon &polygon, QString &errorString);

    /// Load all polygons with holes
    bool loadPolygonsWithHolesFromFile(const QString &filePath, QList<QGeoPolygon> &polygons, QString &errorString);

    /// Load first polyline entity (convenience wrapper)
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolylineFromFile(const QString &filePath, QList<QGeoCoordinate> &coords, QString &errorString,
                              double filterMeters = GeoFormatRegistry::kDefaultVertexFilterMeters);

    /// Load all polyline entities
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolylinesFromFile(const QString &filePath, QList<QList<QGeoCoordinate>> &polylines, QString &errorString,
                               double filterMeters = GeoFormatRegistry::kDefaultVertexFilterMeters);

    /// Load first point entity (convenience wrapper)
    bool loadPointFromFile(const QString &filePath, QGeoCoordinate &point, QString &errorString);

    /// Load all point entities
    bool loadPointsFromFile(const QString &filePath, QList<QGeoCoordinate> &points, QString &errorString);

    // ========================================================================
    // Save functions
    // ========================================================================

    /// Save a single polygon to file
    bool savePolygonToFile(const QString &filePath, const QList<QGeoCoordinate> &vertices, QString &errorString);

    /// Save multiple polygons to file (as FeatureCollection)
    bool savePolygonsToFile(const QString &filePath, const QList<QList<QGeoCoordinate>> &polygons, QString &errorString);

    /// Save a polygon with holes to file
    bool savePolygonWithHolesToFile(const QString &filePath, const QGeoPolygon &polygon, QString &errorString);

    /// Save multiple polygons with holes to file (as FeatureCollection)
    bool savePolygonsWithHolesToFile(const QString &filePath, const QList<QGeoPolygon> &polygons, QString &errorString);

    /// Save a single polyline to file
    bool savePolylineToFile(const QString &filePath, const QList<QGeoCoordinate> &coords, QString &errorString);

    /// Save multiple polylines to file (as FeatureCollection)
    bool savePolylinesToFile(const QString &filePath, const QList<QList<QGeoCoordinate>> &polylines, QString &errorString);

    /// Save a single point to file
    bool savePointToFile(const QString &filePath, const QGeoCoordinate &point, QString &errorString);

    /// Save points to file
    bool savePointsToFile(const QString &filePath, const QList<QGeoCoordinate> &points, QString &errorString);

    // ========================================================================
    // Coordinate helpers (for use with raw JSON)
    // ========================================================================

    /// Loads a QGeoCoordinate from GeoJSON format [lon, lat, alt]
    /// @return false: validation failed
    bool loadGeoJsonCoordinate(const QJsonValue &jsonValue, ///< json value to load from
                               bool altitudeRequired,       ///< true: altitude must be specified
                               QGeoCoordinate &coordinate,  ///< returned QGeoCoordinate
                               QString &errorString);       ///< returned error string if load failure

    /// Saves a QGeoCoordinate to GeoJSON format [lon, lat, alt]
    void saveGeoJsonCoordinate(const QGeoCoordinate &coordinate,    ///< QGeoCoordinate to save
                               bool writeAltitude,                  ///< true: write altitude to json
                               QJsonValue &jsonValue);              ///< json value to save to

    /// Loads a list of QGeoCoordinates from GeoJSON format
    bool loadGeoJsonCoordinateArray(const QJsonValue &jsonValue,
                                    bool altitudeRequired,
                                    QList<QGeoCoordinate> &coordinates,
                                    QString &errorString);

    /// Saves a list of QGeoCoordinates to GeoJSON format
    void saveGeoJsonCoordinateArray(const QList<QGeoCoordinate> &coordinates,
                                    bool writeAltitude,
                                    QJsonValue &jsonValue);

    /// Create a GeoJSON ring array from coordinates (closes ring if needed)
    /// @param coords Polygon ring coordinates
    /// @return QJsonArray of [lon, lat, alt] arrays with closing vertex
    QJsonArray coordinatesToJsonRing(const QList<QGeoCoordinate> &coords);
}
