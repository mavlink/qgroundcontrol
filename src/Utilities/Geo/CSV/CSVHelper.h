#pragma once

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QLoggingCategory>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPolygon>

Q_DECLARE_LOGGING_CATEGORY(CSVHelperLog)

/// @file CSVHelper.h
/// @brief CSV file support for geographic point data
///
/// CSVHelper provides reading and writing of geographic coordinates in CSV format.
/// It supports common column naming conventions and handles various coordinate formats.
///
/// Supported column names (case-insensitive):
/// - Latitude: lat, latitude, y
/// - Longitude: lon, lng, longitude, x
/// - Altitude: alt, altitude, elevation, z
/// - Name: name, label, id
///
/// Example CSV:
/// @code
/// lat,lon,alt,name
/// 47.6062,-122.3321,100,Seattle
/// 37.7749,-122.4194,50,San Francisco
/// @endcode
///
/// @see GeoFormatRegistry for unified format handling

namespace CSVHelper
{
    /// Result of loading points from CSV
    struct LoadResult {
        bool success = false;
        QString errorString;
        QList<QGeoCoordinate> points;
        QStringList names;  ///< Optional names for each point (parallel to points)
    };

    /// Options for CSV parsing
    struct ParseOptions {
        QChar delimiter = ',';      ///< Field delimiter (comma, semicolon, tab)
        bool hasHeader = true;      ///< First row contains column names
        int latColumn = -1;         ///< Latitude column index (auto-detect if -1)
        int lonColumn = -1;         ///< Longitude column index (auto-detect if -1)
        int altColumn = -1;         ///< Altitude column index (auto-detect if -1)
        int nameColumn = -1;        ///< Name column index (auto-detect if -1)
        int sequenceColumn = -1;    ///< Sequence column index for polylines (auto-detect if -1)
        int lineIdColumn = -1;      ///< Line ID column for multiple polylines (auto-detect if -1)
    };

    // ========================================================================
    // Loading Functions
    // ========================================================================

    /// Load points from a CSV file
    /// @param filePath Path to CSV file
    /// @param[out] points Loaded coordinates
    /// @param[out] errorString Error description if loading fails
    /// @return true on success
    bool loadPointsFromFile(const QString &filePath, QList<QGeoCoordinate> &points, QString &errorString);

    /// Load points from CSV file with options
    /// @param filePath Path to CSV file
    /// @param options Parsing options
    /// @return Load result with points and optional names
    LoadResult loadPointsFromFile(const QString &filePath, const ParseOptions &options = ParseOptions());

    /// Load points from CSV string content
    /// @param content CSV content
    /// @param options Parsing options
    /// @return Load result with points and optional names
    LoadResult loadPointsFromString(const QString &content, const ParseOptions &options = ParseOptions());

    // ========================================================================
    // Polyline Functions
    // ========================================================================

    /// Load a single polyline from CSV file (points ordered by sequence column)
    /// @param filePath Path to CSV file
    /// @param[out] coords Loaded coordinates in sequence order
    /// @param[out] errorString Error description if loading fails
    /// @return true on success
    bool loadPolylineFromFile(const QString &filePath, QList<QGeoCoordinate> &coords, QString &errorString);

    /// Load multiple polylines from CSV file (grouped by line ID column)
    /// @param filePath Path to CSV file
    /// @param[out] polylines Loaded polylines
    /// @param[out] errorString Error description if loading fails
    /// @return true on success
    bool loadPolylinesFromFile(const QString &filePath, QList<QList<QGeoCoordinate>> &polylines, QString &errorString);

    /// Save a polyline to CSV file (with sequence column)
    /// @param filePath Output file path
    /// @param coords Coordinates to save
    /// @param[out] errorString Error description if saving fails
    /// @return true on success
    bool savePolylineToFile(const QString &filePath, const QList<QGeoCoordinate> &coords, QString &errorString);

    /// Save multiple polylines to CSV file (with line ID and sequence columns)
    /// @param filePath Output file path
    /// @param polylines Polylines to save
    /// @param[out] errorString Error description if saving fails
    /// @return true on success
    bool savePolylinesToFile(const QString &filePath, const QList<QList<QGeoCoordinate>> &polylines, QString &errorString);

    // ========================================================================
    // Polygon Functions (Stubs - Not Supported)
    // ========================================================================

    /// @note CSV format does not support polygon geometries
    /// This stub function returns false with a clear error message
    bool loadPolygonFromFile(const QString &filePath, QList<QGeoCoordinate> &vertices, QString &errorString);

    /// @note CSV format does not support polygon geometries
    /// This stub function returns false with a clear error message
    bool loadPolygonsFromFile(const QString &filePath, QList<QList<QGeoCoordinate>> &polygons, QString &errorString);

    /// @note CSV format does not support polygons with holes
    /// This stub function returns false with a clear error message
    bool loadPolygonWithHolesFromFile(const QString &filePath, QGeoPolygon &polygon, QString &errorString);

    /// @note CSV format does not support polygons with holes
    /// This stub function returns false with a clear error message
    bool loadPolygonsWithHolesFromFile(const QString &filePath, QList<QGeoPolygon> &polygons, QString &errorString);

    // ========================================================================
    // Saving Functions
    // ========================================================================

    /// Save points to a CSV file
    /// @param filePath Output file path
    /// @param points Coordinates to save
    /// @param[out] errorString Error description if saving fails
    /// @return true on success
    bool savePointsToFile(const QString &filePath, const QList<QGeoCoordinate> &points, QString &errorString);

    /// Save points to a CSV file with names
    /// @param filePath Output file path
    /// @param points Coordinates to save
    /// @param names Optional names for each point
    /// @param[out] errorString Error description if saving fails
    /// @return true on success
    bool savePointsToFile(const QString &filePath, const QList<QGeoCoordinate> &points,
                          const QStringList &names, QString &errorString);

    // ========================================================================
    // Utility Functions
    // ========================================================================

    /// Detect the delimiter used in a CSV file
    /// @param firstLine First line of the CSV file
    /// @return Detected delimiter (comma, semicolon, or tab)
    QChar detectDelimiter(const QString &firstLine);

    /// Find column indices for coordinate columns based on header names
    /// @param headers List of header column names
    /// @param[out] options ParseOptions with detected column indices
    void detectColumns(const QStringList &headers, ParseOptions &options);
}
