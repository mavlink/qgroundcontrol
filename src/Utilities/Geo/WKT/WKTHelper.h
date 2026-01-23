#pragma once

#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QString>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPolygon>

Q_DECLARE_LOGGING_CATEGORY(WKTHelperLog)

/// @file WKTHelper.h
/// @brief Well-Known Text (WKT) format support for geographic shapes
///
/// WKTHelper provides parsing and generation of WKT strings, a standard
/// text format for representing geometric objects. Supports:
/// - POINT / MULTIPOINT
/// - LINESTRING / MULTILINESTRING
/// - POLYGON / MULTIPOLYGON (including holes)
///
/// WKT uses lon/lat order (X Y), with optional Z (altitude) and M (measure).
///
/// @section examples Examples
/// @code
/// // Parse WKT string
/// QList<QGeoCoordinate> vertices;
/// QString error;
/// if (WKTHelper::parsePolygon("POLYGON ((30 10, 40 40, 20 40, 10 20, 30 10))", vertices, error)) {
///     // Use vertices...
/// }
///
/// // Generate WKT string
/// QString wkt = WKTHelper::toWKTPolygon(polygon);
/// @endcode
///
/// @see https://en.wikipedia.org/wiki/Well-known_text_representation_of_geometry
namespace WKTHelper
{
    // ========================================================================
    // Parsing Functions (WKT String -> Coordinates)
    // ========================================================================

    /// Parse a POINT WKT string
    /// @param wkt WKT string (e.g., "POINT (30 10)" or "POINT Z (30 10 100)")
    /// @param[out] coord Resulting coordinate
    /// @param[out] errorString Error description if parsing fails
    /// @return true on success
    bool parsePoint(const QString &wkt, QGeoCoordinate &coord, QString &errorString);

    /// Parse a MULTIPOINT WKT string
    /// @param wkt WKT string (e.g., "MULTIPOINT ((10 40), (40 30))")
    /// @param[out] points Resulting coordinates
    /// @param[out] errorString Error description if parsing fails
    /// @return true on success
    bool parseMultiPoint(const QString &wkt, QList<QGeoCoordinate> &points, QString &errorString);

    /// Parse a LINESTRING WKT string
    /// @param wkt WKT string (e.g., "LINESTRING (30 10, 10 30, 40 40)")
    /// @param[out] coords Resulting coordinates
    /// @param[out] errorString Error description if parsing fails
    /// @return true on success
    bool parseLineString(const QString &wkt, QList<QGeoCoordinate> &coords, QString &errorString);

    /// Parse a MULTILINESTRING WKT string
    /// @param wkt WKT string
    /// @param[out] polylines List of polylines
    /// @param[out] errorString Error description if parsing fails
    /// @return true on success
    bool parseMultiLineString(const QString &wkt, QList<QList<QGeoCoordinate>> &polylines, QString &errorString);

    /// Parse a POLYGON WKT string (outer ring only, ignores holes)
    /// @param wkt WKT string (e.g., "POLYGON ((30 10, 40 40, 20 40, 10 20, 30 10))")
    /// @param[out] vertices Polygon vertices (closing vertex removed)
    /// @param[out] errorString Error description if parsing fails
    /// @return true on success
    bool parsePolygon(const QString &wkt, QList<QGeoCoordinate> &vertices, QString &errorString);

    /// Parse a POLYGON WKT string including holes
    /// @param wkt WKT string
    /// @param[out] polygon QGeoPolygon with outer ring and holes
    /// @param[out] errorString Error description if parsing fails
    /// @return true on success
    bool parsePolygonWithHoles(const QString &wkt, QGeoPolygon &polygon, QString &errorString);

    /// Parse a MULTIPOLYGON WKT string
    /// @param wkt WKT string
    /// @param[out] polygons List of polygons (outer rings only)
    /// @param[out] errorString Error description if parsing fails
    /// @return true on success
    bool parseMultiPolygon(const QString &wkt, QList<QList<QGeoCoordinate>> &polygons, QString &errorString);

    /// Parse a MULTIPOLYGON WKT string including holes
    /// @param wkt WKT string
    /// @param[out] polygons List of QGeoPolygon with holes
    /// @param[out] errorString Error description if parsing fails
    /// @return true on success
    bool parseMultiPolygonWithHoles(const QString &wkt, QList<QGeoPolygon> &polygons, QString &errorString);

    // ========================================================================
    // File Load Functions (WKT File -> Coordinates)
    // ========================================================================

    /// Load a point from a WKT file
    /// @param filePath Input file path
    /// @param[out] coord Resulting coordinate
    /// @param[out] errorString Error description if load fails
    /// @return true on success
    bool loadPointFromFile(const QString &filePath, QGeoCoordinate &coord, QString &errorString);

    /// Load points from a WKT file (POINT or MULTIPOINT)
    /// @param filePath Input file path
    /// @param[out] points Resulting coordinates
    /// @param[out] errorString Error description if load fails
    /// @return true on success
    bool loadPointsFromFile(const QString &filePath, QList<QGeoCoordinate> &points, QString &errorString);

    /// Load a polyline from a WKT file (LINESTRING)
    /// @param filePath Input file path
    /// @param[out] coords Resulting coordinates
    /// @param[out] errorString Error description if load fails
    /// @return true on success
    bool loadPolylineFromFile(const QString &filePath, QList<QGeoCoordinate> &coords, QString &errorString);

    /// Load polylines from a WKT file (LINESTRING or MULTILINESTRING)
    /// @param filePath Input file path
    /// @param[out] polylines Resulting polylines
    /// @param[out] errorString Error description if load fails
    /// @param loadAll If true, loads all polylines; if false, loads first only
    /// @return true on success
    bool loadPolylinesFromFile(const QString &filePath, QList<QList<QGeoCoordinate>> &polylines,
                               QString &errorString, bool loadAll = true);

    /// Load a polygon from a WKT file (POLYGON outer ring)
    /// @param filePath Input file path
    /// @param[out] vertices Polygon vertices
    /// @param[out] errorString Error description if load fails
    /// @return true on success
    bool loadPolygonFromFile(const QString &filePath, QList<QGeoCoordinate> &vertices, QString &errorString);

    /// Load polygons from a WKT file (POLYGON or MULTIPOLYGON)
    /// @param filePath Input file path
    /// @param[out] polygons Resulting polygons
    /// @param[out] errorString Error description if load fails
    /// @param loadAll If true, loads all polygons; if false, loads first only
    /// @return true on success
    bool loadPolygonsFromFile(const QString &filePath, QList<QList<QGeoCoordinate>> &polygons,
                              QString &errorString, bool loadAll = true);

    // ========================================================================
    // Generation Functions (Coordinates -> WKT String)
    // ========================================================================

    /// Generate WKT POINT string
    /// @param coord Coordinate to convert
    /// @param includeAltitude Include Z coordinate if available
    /// @return WKT string
    QString toWKTPoint(const QGeoCoordinate &coord, bool includeAltitude = false);

    /// Generate WKT MULTIPOINT string
    /// @param points Points to convert
    /// @param includeAltitude Include Z coordinates if available
    /// @return WKT string
    QString toWKTMultiPoint(const QList<QGeoCoordinate> &points, bool includeAltitude = false);

    /// Generate WKT LINESTRING string
    /// @param coords Coordinates to convert
    /// @param includeAltitude Include Z coordinates if available
    /// @return WKT string
    QString toWKTLineString(const QList<QGeoCoordinate> &coords, bool includeAltitude = false);

    /// Generate WKT MULTILINESTRING string
    /// @param polylines Polylines to convert
    /// @param includeAltitude Include Z coordinates if available
    /// @return WKT string
    QString toWKTMultiLineString(const QList<QList<QGeoCoordinate>> &polylines, bool includeAltitude = false);

    /// Generate WKT POLYGON string
    /// @param vertices Polygon vertices (will be auto-closed)
    /// @param includeAltitude Include Z coordinates if available
    /// @return WKT string
    QString toWKTPolygon(const QList<QGeoCoordinate> &vertices, bool includeAltitude = false);

    /// Generate WKT POLYGON string with holes
    /// @param polygon QGeoPolygon with holes
    /// @param includeAltitude Include Z coordinates if available
    /// @return WKT string
    QString toWKTPolygon(const QGeoPolygon &polygon, bool includeAltitude = false);

    /// Generate WKT MULTIPOLYGON string
    /// @param polygons List of polygon vertices
    /// @param includeAltitude Include Z coordinates if available
    /// @return WKT string
    QString toWKTMultiPolygon(const QList<QList<QGeoCoordinate>> &polygons, bool includeAltitude = false);

    /// Generate WKT MULTIPOLYGON string with holes
    /// @param polygons List of QGeoPolygon with holes
    /// @param includeAltitude Include Z coordinates if available
    /// @return WKT string
    QString toWKTMultiPolygon(const QList<QGeoPolygon> &polygons, bool includeAltitude = false);

    // ========================================================================
    // File Save Functions (Coordinates -> WKT File)
    // ========================================================================

    /// Save a point to a WKT file
    /// @param filePath Output file path
    /// @param coord Point coordinate
    /// @param[out] errorString Error description if save fails
    /// @param includeAltitude Include Z coordinate if available
    /// @return true on success
    bool savePointToFile(const QString &filePath, const QGeoCoordinate &coord,
                         QString &errorString, bool includeAltitude = false);

    /// Save multiple points to a WKT file as MULTIPOINT
    /// @param filePath Output file path
    /// @param points Point coordinates
    /// @param[out] errorString Error description if save fails
    /// @param includeAltitude Include Z coordinates if available
    /// @return true on success
    bool savePointsToFile(const QString &filePath, const QList<QGeoCoordinate> &points,
                          QString &errorString, bool includeAltitude = false);

    /// Save a polyline to a WKT file as LINESTRING
    /// @param filePath Output file path
    /// @param coords Polyline coordinates
    /// @param[out] errorString Error description if save fails
    /// @param includeAltitude Include Z coordinates if available
    /// @return true on success
    bool savePolylineToFile(const QString &filePath, const QList<QGeoCoordinate> &coords,
                            QString &errorString, bool includeAltitude = false);

    /// Save multiple polylines to a WKT file as MULTILINESTRING
    /// @param filePath Output file path
    /// @param polylines List of polylines
    /// @param[out] errorString Error description if save fails
    /// @param includeAltitude Include Z coordinates if available
    /// @return true on success
    bool savePolylinesToFile(const QString &filePath, const QList<QList<QGeoCoordinate>> &polylines,
                             QString &errorString, bool includeAltitude = false);

    /// Save a polygon to a WKT file
    /// @param filePath Output file path
    /// @param vertices Polygon vertices
    /// @param[out] errorString Error description if save fails
    /// @param includeAltitude Include Z coordinates if available
    /// @return true on success
    bool savePolygonToFile(const QString &filePath, const QList<QGeoCoordinate> &vertices,
                           QString &errorString, bool includeAltitude = false);

    /// Save multiple polygons to a WKT file as MULTIPOLYGON
    /// @param filePath Output file path
    /// @param polygons List of polygon vertex lists
    /// @param[out] errorString Error description if save fails
    /// @param includeAltitude Include Z coordinates if available
    /// @return true on success
    bool savePolygonsToFile(const QString &filePath, const QList<QList<QGeoCoordinate>> &polygons,
                            QString &errorString, bool includeAltitude = false);

    // ========================================================================
    // Utility Functions
    // ========================================================================

    /// Determine the geometry type from a WKT string
    /// @param wkt WKT string
    /// @return Geometry type name (e.g., "POINT", "POLYGON"), or empty if invalid
    QString geometryType(const QString &wkt);

    /// Check if a WKT string includes Z (altitude) values
    /// @param wkt WKT string
    /// @return true if the geometry type includes Z modifier
    bool hasAltitude(const QString &wkt);

} // namespace WKTHelper
