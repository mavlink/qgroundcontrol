#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QLoggingCategory>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPolygon>

Q_DECLARE_LOGGING_CATEGORY(WKBHelperLog)

/// @file WKBHelper.h
/// @brief Well-Known Binary (WKB) format support for geographic shapes
///
/// WKB is the binary counterpart to WKT, used in spatial databases like
/// PostGIS and GeoPackage. This helper supports standard WKB as well as
/// Extended WKB (EWKB) with SRID prefix and ISO WKB with Z/M flags.
///
/// Geometry types supported:
/// - Point / MultiPoint
/// - LineString / MultiLineString
/// - Polygon / MultiPolygon
/// - GeometryCollection
///
/// @see https://en.wikipedia.org/wiki/Well-known_text_representation_of_geometry#Well-known_binary
/// @see GeoPackage specification: http://www.geopackage.org/spec/

namespace WKBHelper
{
    /// WKB geometry type codes
    enum GeometryType {
        wkbPoint = 1,
        wkbLineString = 2,
        wkbPolygon = 3,
        wkbMultiPoint = 4,
        wkbMultiLineString = 5,
        wkbMultiPolygon = 6,
        wkbGeometryCollection = 7,

        // ISO SQL/MM Part 3 additions (2D + Z/M modifiers)
        wkbPointZ = 1001,
        wkbLineStringZ = 1002,
        wkbPolygonZ = 1003,
        wkbMultiPointZ = 1004,
        wkbMultiLineStringZ = 1005,
        wkbMultiPolygonZ = 1006,
        wkbGeometryCollectionZ = 1007,

        wkbPointM = 2001,
        wkbLineStringM = 2002,
        wkbPolygonM = 2003,

        wkbPointZM = 3001,
        wkbLineStringZM = 3002,
        wkbPolygonZM = 3003,
    };

    /// Byte order indicators
    enum ByteOrder {
        wkbXDR = 0,  ///< Big endian
        wkbNDR = 1   ///< Little endian (most common)
    };

    // ========================================================================
    // Parsing Functions (WKB -> Coordinates)
    // ========================================================================

    /// Parse a WKB POINT
    /// @param wkb WKB binary data
    /// @param[out] coord Resulting coordinate
    /// @param[out] errorString Error description if parsing fails
    /// @return true on success
    bool parsePoint(const QByteArray &wkb, QGeoCoordinate &coord, QString &errorString);

    /// Parse a WKB MULTIPOINT
    /// @param wkb WKB binary data
    /// @param[out] points Resulting coordinates
    /// @param[out] errorString Error description if parsing fails
    /// @return true on success
    bool parseMultiPoint(const QByteArray &wkb, QList<QGeoCoordinate> &points, QString &errorString);

    /// Parse a WKB LINESTRING
    /// @param wkb WKB binary data
    /// @param[out] coords Resulting coordinates
    /// @param[out] errorString Error description if parsing fails
    /// @return true on success
    bool parseLineString(const QByteArray &wkb, QList<QGeoCoordinate> &coords, QString &errorString);

    /// Parse a WKB MULTILINESTRING
    /// @param wkb WKB binary data
    /// @param[out] polylines List of polylines
    /// @param[out] errorString Error description if parsing fails
    /// @return true on success
    bool parseMultiLineString(const QByteArray &wkb, QList<QList<QGeoCoordinate>> &polylines, QString &errorString);

    /// Parse a WKB POLYGON (outer ring only)
    /// @param wkb WKB binary data
    /// @param[out] vertices Polygon vertices
    /// @param[out] errorString Error description if parsing fails
    /// @return true on success
    bool parsePolygon(const QByteArray &wkb, QList<QGeoCoordinate> &vertices, QString &errorString);

    /// Parse a WKB POLYGON with holes
    /// @param wkb WKB binary data
    /// @param[out] polygon QGeoPolygon with outer ring and holes
    /// @param[out] errorString Error description if parsing fails
    /// @return true on success
    bool parsePolygonWithHoles(const QByteArray &wkb, QGeoPolygon &polygon, QString &errorString);

    /// Parse a WKB MULTIPOLYGON
    /// @param wkb WKB binary data
    /// @param[out] polygons List of polygons (outer rings only)
    /// @param[out] errorString Error description if parsing fails
    /// @return true on success
    bool parseMultiPolygon(const QByteArray &wkb, QList<QList<QGeoCoordinate>> &polygons, QString &errorString);

    /// Parse any WKB geometry and return points, lines, and polygons
    /// @param wkb WKB binary data
    /// @param[out] points Extracted points
    /// @param[out] polylines Extracted polylines
    /// @param[out] polygons Extracted polygons
    /// @param[out] errorString Error description if parsing fails
    /// @return true on success
    bool parseGeometry(const QByteArray &wkb,
                       QList<QGeoCoordinate> &points,
                       QList<QList<QGeoCoordinate>> &polylines,
                       QList<QList<QGeoCoordinate>> &polygons,
                       QString &errorString);

    // ========================================================================
    // Generation Functions (Coordinates -> WKB)
    // ========================================================================

    /// Generate WKB POINT
    /// @param coord Coordinate to convert
    /// @param includeZ Include Z coordinate
    /// @return WKB binary data
    QByteArray toWKBPoint(const QGeoCoordinate &coord, bool includeZ = false);

    /// Generate WKB LINESTRING
    /// @param coords Coordinates to convert
    /// @param includeZ Include Z coordinates
    /// @return WKB binary data
    QByteArray toWKBLineString(const QList<QGeoCoordinate> &coords, bool includeZ = false);

    /// Generate WKB POLYGON
    /// @param vertices Polygon vertices (will be auto-closed)
    /// @param includeZ Include Z coordinates
    /// @return WKB binary data
    QByteArray toWKBPolygon(const QList<QGeoCoordinate> &vertices, bool includeZ = false);

    /// Generate WKB MULTIPOINT
    /// @param points Points to convert
    /// @param includeZ Include Z coordinates
    /// @return WKB binary data
    QByteArray toWKBMultiPoint(const QList<QGeoCoordinate> &points, bool includeZ = false);

    /// Generate WKB MULTILINESTRING
    /// @param polylines Polylines to convert
    /// @param includeZ Include Z coordinates
    /// @return WKB binary data
    QByteArray toWKBMultiLineString(const QList<QList<QGeoCoordinate>> &polylines, bool includeZ = false);

    /// Generate WKB MULTIPOLYGON
    /// @param polygons Polygons to convert
    /// @param includeZ Include Z coordinates
    /// @return WKB binary data
    QByteArray toWKBMultiPolygon(const QList<QList<QGeoCoordinate>> &polygons, bool includeZ = false);

    // ========================================================================
    // Utility Functions
    // ========================================================================

    /// Get geometry type from WKB header
    /// @param wkb WKB binary data
    /// @return Geometry type code, or -1 if invalid
    int geometryType(const QByteArray &wkb);

    /// Get geometry type name from code
    /// @param type Geometry type code
    /// @return Type name (e.g., "Point", "Polygon")
    QString geometryTypeName(int type);

    /// Check if geometry type has Z values
    /// @param type Geometry type code
    /// @return true if Z values are present
    bool hasZ(int type);

    /// Check if geometry type has M values
    /// @param type Geometry type code
    /// @return true if M values are present
    bool hasM(int type);

    /// Get base geometry type (strip Z/M modifiers)
    /// @param type Geometry type code
    /// @return Base type (1-7)
    int baseType(int type);

    // ========================================================================
    // GeoPackage Binary Geometry (GPB) Support
    // ========================================================================

    /// Parse GeoPackage binary geometry header
    /// @param gpb GeoPackage binary data
    /// @param[out] srid Spatial Reference ID
    /// @param[out] wkbOffset Offset to WKB data within gpb
    /// @param[out] errorString Error description if parsing fails
    /// @return true on success
    bool parseGeoPackageHeader(const QByteArray &gpb, int &srid, int &wkbOffset, QString &errorString);

    /// Create GeoPackage binary geometry from WKB
    /// @param wkb WKB binary data
    /// @param srid Spatial Reference ID (4326 for WGS84)
    /// @return GeoPackage binary data
    QByteArray toGeoPackageBinary(const QByteArray &wkb, int srid = 4326);
}
