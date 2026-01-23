#pragma once

#include <QtCore/QFuture>
#include <QtCore/QLoggingCategory>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPolygon>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(GeoFormatRegistryLog)

/// @file GeoFormatRegistry.h
/// @brief Unified registry for geographic file format support
///
/// GeoFormatRegistry provides a unified interface for loading geographic data
/// from various file formats. It uses native parsers where available and
/// optionally delegates to GDAL for exotic formats.
///
/// Native format support (no external dependencies):
/// - KML/KMZ (Keyhole Markup Language)
/// - GeoJSON
/// - GPX (GPS Exchange Format)
/// - Shapefile (SHP)
/// - WKT (Well-Known Text)
/// - CSV with coordinates
/// - OpenAir (Airspace)
/// - GeoPackage (GPKG)
///
/// Optional GDAL support (runtime detection):
/// - GML, AIXM, and dozens of other formats
///
/// @see KMLHelper, GPXHelper, GeoJsonHelper, SHPHelper

namespace GeoFormatRegistry
{
    // ========================================================================
    // Constants
    // ========================================================================

    /// Default distance threshold for filtering nearby vertices (meters)
    constexpr double kDefaultVertexFilterMeters = 5.0;

    // Note: kMinPolygonVertices and kMinPolylineVertices are defined in GeoUtilities.h
    // Use GeoUtilities::kMinPolygonVertices and GeoUtilities::kMinPolylineVertices

    // ========================================================================
    // Enums and Flags
    // ========================================================================

    /// Format capabilities
    enum Capability {
        CanReadPoints = 0x001,
        CanReadPolylines = 0x002,
        CanReadPolygons = 0x004,
        CanReadPolygonsWithHoles = 0x008,
        CanReadTracks = 0x010,
        CanReadAirspace = 0x020,
        CanWritePoints = 0x040,
        CanWritePolylines = 0x080,
        CanWritePolygons = 0x100,
        CanWritePolygonsWithHoles = 0x200,
        CanWriteTracks = 0x400,

        CanRead = CanReadPoints | CanReadPolylines | CanReadPolygons,
        CanWrite = CanWritePoints | CanWritePolylines | CanWritePolygons,
        CanReadWrite = CanRead | CanWrite,
        CanReadWriteAll = CanReadWrite | CanReadPolygonsWithHoles | CanWritePolygonsWithHoles
    };
    Q_DECLARE_FLAGS(Capabilities, Capability)

    /// Shape type detected in a file
    enum class ShapeType {
        Polygon,
        Polyline,
        Point,
        Error
    };

    // ========================================================================
    // Structs
    // ========================================================================

    /// Information about a supported format
    struct FormatInfo {
        QString name;             ///< Human-readable name
        QString description;      ///< Brief description
        QStringList extensions;   ///< File extensions (without dot)
        Capabilities capabilities;
        bool isNative = true;     ///< True if handled by native code, false if GDAL
    };

    /// Result of loading geographic data
    struct LoadResult {
        bool success = false;
        QString errorString;
        QString formatUsed;

        QList<QGeoCoordinate> points;
        QList<QList<QGeoCoordinate>> polylines;
        QList<QList<QGeoCoordinate>> polygons;
        QList<QGeoPolygon> polygonsWithHoles;

        int totalFeatures() const {
            return points.count() + polylines.count() + polygons.count() + polygonsWithHoles.count();
        }
    };

    /// Result of saving geographic data
    struct SaveResult {
        bool success = false;
        QString errorString;
        QString formatUsed;
    };

    /// Validation result for a single format
    struct ValidationResult {
        QString formatName;
        bool valid = true;
        QStringList issues;
    };

    // ========================================================================
    // Registry Information
    // ========================================================================

    /// Get list of all supported formats
    QList<FormatInfo> supportedFormats();

    /// Get list of native (non-GDAL) formats
    QList<FormatInfo> nativeFormats();

    /// Check if a format is supported
    /// @param extension File extension (without dot)
    /// @return true if format is supported
    bool isSupported(const QString &extension);

    /// Get format info for an extension
    /// @param extension File extension
    /// @return Format info, or invalid info if not supported
    FormatInfo formatInfo(const QString &extension);

    /// Get format info for a file path
    /// @param filePath File path (extension extracted)
    /// @return Format info, or invalid info if not supported
    FormatInfo formatInfoForFile(const QString &filePath);

    /// Check if a format has a specific capability
    /// @param extension File extension (without dot)
    /// @param capability Capability to check
    /// @return true if format has the capability
    bool hasCapability(const QString &extension, Capability capability);

    /// Check if a file's format has a specific capability
    /// @param filePath File path
    /// @param capability Capability to check
    /// @return true if format has the capability
    bool fileHasCapability(const QString &filePath, Capability capability);

    // ========================================================================
    // File Dialog Filters
    // ========================================================================

    /// Get file filter string for all readable formats
    /// @return Filter string for file dialogs (e.g., "All Geo Files (*.kml *.shp ...)")
    QString readFileFilter();

    /// Get file filter string for all writable formats
    QString writeFileFilter();

    /// Get file filter as QStringList for QML file dialogs
    QStringList readFileFilterList();

    /// Get file filter as QStringList for QML file dialogs
    QStringList writeFileFilterList();

    // ========================================================================
    // Format Detection
    // ========================================================================

    /// Detected format from file content
    struct DetectedFormat {
        QString extension;      ///< Detected format extension (e.g., "kml", "geojson")
        QString formatName;     ///< Human-readable format name
        int confidence = 0;     ///< Confidence level 0-100 (100 = certain)
        bool isValid() const { return !extension.isEmpty() && confidence > 0; }
    };

    /// Detect file format from content (magic bytes, headers)
    /// Useful for files with missing or wrong extensions
    /// @param filePath Path to file
    /// @return Detected format info, or invalid if unrecognized
    DetectedFormat detectFormat(const QString &filePath);

    /// Detect file format from raw bytes
    /// @param data First bytes of file (at least 512 bytes recommended)
    /// @return Detected format info, or invalid if unrecognized
    DetectedFormat detectFormatFromBytes(const QByteArray &data);

    // ========================================================================
    // Shape Detection
    // ========================================================================

    /// Determine the primary shape type in a file
    /// @param filePath Path to file
    /// @param[out] errorString Error description if operation fails
    /// @return Shape type, or ShapeType::Error on failure
    ShapeType determineShapeType(const QString &filePath, QString &errorString);

    /// Get the number of geometry entities in a file
    /// @param filePath Path to file
    /// @param[out] errorString Error description if operation fails
    /// @return Number of entities, or 0 on failure
    int getEntityCount(const QString &filePath, QString &errorString);

    // ========================================================================
    // Loading Functions
    // ========================================================================

    /// Load geographic data from any supported format
    /// @param filePath Path to file
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    /// @return Load result with data or error
    LoadResult loadFile(const QString &filePath, double filterMeters = 0);

    /// Load geographic data asynchronously from any supported format
    /// Uses QtConcurrent to run the load operation in a background thread.
    /// @param filePath Path to file
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    /// @return QFuture<LoadResult> - Use QFutureWatcher to monitor completion
    /// @code
    /// QFuture<LoadResult> future = GeoFormatRegistry::loadFileAsync("path/to/file.kml");
    /// QFutureWatcher<LoadResult> *watcher = new QFutureWatcher<LoadResult>();
    /// connect(watcher, &QFutureWatcher<LoadResult>::finished, [=]() {
    ///     LoadResult result = watcher->result();
    ///     // Handle result
    ///     watcher->deleteLater();
    /// });
    /// watcher->setFuture(future);
    /// @endcode
    QFuture<LoadResult> loadFileAsync(const QString &filePath, double filterMeters = 0);

    /// Load points from any supported format
    /// @param filePath Path to file
    /// @param[out] points Loaded points
    /// @param[out] errorString Error description if operation fails
    /// @return true on success
    bool loadPoints(const QString &filePath, QList<QGeoCoordinate> &points, QString &errorString);

    /// Load polylines from any supported format
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolylines(const QString &filePath, QList<QList<QGeoCoordinate>> &polylines,
                       QString &errorString, double filterMeters = 0);

    /// Load polygons from any supported format
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolygons(const QString &filePath, QList<QList<QGeoCoordinate>> &polygons,
                      QString &errorString, double filterMeters = 0);

    /// Load first polygon from file (convenience)
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolygon(const QString &filePath, QList<QGeoCoordinate> &polygon,
                     QString &errorString, double filterMeters = 0);

    /// Load first polyline from file (convenience)
    /// @param filterMeters Filter vertices closer than this distance (0 to disable)
    bool loadPolyline(const QString &filePath, QList<QGeoCoordinate> &polyline,
                      QString &errorString, double filterMeters = 0);

    /// Load polygons with holes from any supported format
    /// @note Not all formats support holes (GPX does not)
    bool loadPolygonsWithHoles(const QString &filePath, QList<QGeoPolygon> &polygons, QString &errorString);

    /// Load first polygon with holes from file (convenience)
    bool loadPolygonWithHoles(const QString &filePath, QGeoPolygon &polygon, QString &errorString);

    // ========================================================================
    // Save Functions
    // ========================================================================

    /// Save a point to a file (format determined by extension)
    /// @param filePath Output file path
    /// @param point Point to save
    /// @return Save result
    SaveResult savePoint(const QString &filePath, const QGeoCoordinate &point);

    /// Save multiple points to a file
    /// @param filePath Output file path
    /// @param points Points to save
    /// @return Save result
    SaveResult savePoints(const QString &filePath, const QList<QGeoCoordinate> &points);

    /// Save a polyline to a file
    /// @param filePath Output file path
    /// @param polyline Polyline coordinates
    /// @return Save result
    SaveResult savePolyline(const QString &filePath, const QList<QGeoCoordinate> &polyline);

    /// Save multiple polylines to a file
    /// @param filePath Output file path
    /// @param polylines List of polylines
    /// @return Save result
    SaveResult savePolylines(const QString &filePath, const QList<QList<QGeoCoordinate>> &polylines);

    /// Save a polygon to a file
    /// @param filePath Output file path
    /// @param polygon Polygon vertices
    /// @return Save result
    SaveResult savePolygon(const QString &filePath, const QList<QGeoCoordinate> &polygon);

    /// Save multiple polygons to a file
    /// @param filePath Output file path
    /// @param polygons List of polygon vertex lists
    /// @return Save result
    SaveResult savePolygons(const QString &filePath, const QList<QList<QGeoCoordinate>> &polygons);

    /// Save a polygon with holes to a file
    /// @note Not all formats support holes (GPX does not)
    SaveResult savePolygonWithHoles(const QString &filePath, const QGeoPolygon &polygon);

    /// Save multiple polygons with holes to a file
    SaveResult savePolygonsWithHoles(const QString &filePath, const QList<QGeoPolygon> &polygons);

    /// Save a track to a file (GPX format only)
    /// @note Tracks are ordered point sequences typically recorded from GPS logs.
    ///       Only GPX format supports tracks; other formats will return an error.
    /// @param filePath Output file path (must be .gpx)
    /// @param track Track coordinates
    /// @return Save result
    SaveResult saveTrack(const QString &filePath, const QList<QGeoCoordinate> &track);

    /// Save multiple tracks to a file (GPX format only)
    /// @param filePath Output file path (must be .gpx)
    /// @param tracks List of track coordinate lists
    /// @return Save result
    SaveResult saveTracks(const QString &filePath, const QList<QList<QGeoCoordinate>> &tracks);

    // ========================================================================
    // Vertex Filtering
    // ========================================================================

    /// Filter vertices that are closer than filterMeters apart
    /// @param vertices Coordinate list to filter in-place
    /// @param filterMeters Remove vertices closer than this distance (0 to disable)
    /// @param minVertices Minimum number of vertices to keep
    void filterVertices(QList<QGeoCoordinate> &vertices, double filterMeters, int minVertices);

    // ========================================================================
    // Validation
    // ========================================================================

    /// Validate that all registered formats implement their declared capabilities
    /// @return List of validation results, one per format
    QList<ValidationResult> validateCapabilities();

    /// Check if all formats pass validation
    /// @return true if all formats are valid
    bool allCapabilitiesValid();

    // ========================================================================
    // GDAL Integration
    // ========================================================================

    /// Check if GDAL is available at runtime
    /// @return true if GDAL/OGR can be used
    bool isGDALAvailable();

    /// Get GDAL version string (empty if not available)
    QString gdalVersion();

    /// Load file using GDAL (if available)
    /// @param filePath Path to file
    /// @return Load result
    LoadResult loadWithGDAL(const QString &filePath);

    /// Get additional formats available via GDAL
    QStringList gdalFormats();
}

Q_DECLARE_OPERATORS_FOR_FLAGS(GeoFormatRegistry::Capabilities)

/// QML singleton wrapper for GeoFormatRegistry namespace
/// Provides file dialog filters, format information, and capability checking to QML
class GeoFormatRegistryQml : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // File dialog filter properties
    Q_PROPERTY(QStringList fileDialogFilters READ fileDialogFilters CONSTANT)
    Q_PROPERTY(QStringList fileDialogKMLFilters READ fileDialogKMLFilters CONSTANT)
    Q_PROPERTY(QStringList fileDialogWriteFilters READ fileDialogWriteFilters CONSTANT)

public:
    explicit GeoFormatRegistryQml(QObject *parent = nullptr) : QObject(parent) {}

    /// Shape types that can be detected in files
    enum ShapeType {
        Polygon = static_cast<int>(GeoFormatRegistry::ShapeType::Polygon),
        Polyline = static_cast<int>(GeoFormatRegistry::ShapeType::Polyline),
        Point = static_cast<int>(GeoFormatRegistry::ShapeType::Point),
        Error = static_cast<int>(GeoFormatRegistry::ShapeType::Error)
    };
    Q_ENUM(ShapeType)

    /// Format capabilities
    enum Capability {
        CanReadPoints = GeoFormatRegistry::CanReadPoints,
        CanReadPolylines = GeoFormatRegistry::CanReadPolylines,
        CanReadPolygons = GeoFormatRegistry::CanReadPolygons,
        CanReadPolygonsWithHoles = GeoFormatRegistry::CanReadPolygonsWithHoles,
        CanReadTracks = GeoFormatRegistry::CanReadTracks,
        CanWritePoints = GeoFormatRegistry::CanWritePoints,
        CanWritePolylines = GeoFormatRegistry::CanWritePolylines,
        CanWritePolygons = GeoFormatRegistry::CanWritePolygons,
        CanWriteTracks = GeoFormatRegistry::CanWriteTracks
    };
    Q_ENUM(Capability)

    // ========================================================================
    // File Dialog Filters
    // ========================================================================

    /// File filter list for all supported read file dialogs
    static QStringList fileDialogFilters() { return GeoFormatRegistry::readFileFilterList(); }

    /// File filter list for KML file dialogs only
    static QStringList fileDialogKMLFilters() {
        return QStringList(tr("KML Files (*.kml)"));
    }

    /// File filter list for all supported write file dialogs
    static QStringList fileDialogWriteFilters() { return GeoFormatRegistry::writeFileFilterList(); }

    // ========================================================================
    // Capability Checking
    // ========================================================================

    /// Check if a format supports a specific capability
    /// @param extension File extension without dot (e.g., "kml", "gpx")
    /// @param capability Capability to check
    Q_INVOKABLE static bool hasCapability(const QString &extension, Capability capability) {
        return GeoFormatRegistry::hasCapability(extension, static_cast<GeoFormatRegistry::Capability>(capability));
    }

    /// Check if a file's format supports a specific capability
    /// @param filePath Full file path
    /// @param capability Capability to check
    Q_INVOKABLE static bool fileHasCapability(const QString &filePath, Capability capability) {
        return GeoFormatRegistry::fileHasCapability(filePath, static_cast<GeoFormatRegistry::Capability>(capability));
    }

    /// Check if a format is supported
    /// @param extension File extension without dot
    Q_INVOKABLE static bool isSupported(const QString &extension) {
        return GeoFormatRegistry::isSupported(extension);
    }

    // ========================================================================
    // Format Information
    // ========================================================================

    /// Get human-readable name for a format
    /// @param extension File extension without dot
    Q_INVOKABLE static QString formatName(const QString &extension) {
        return GeoFormatRegistry::formatInfo(extension).name;
    }

    /// Get description for a format
    /// @param extension File extension without dot
    Q_INVOKABLE static QString formatDescription(const QString &extension) {
        return GeoFormatRegistry::formatInfo(extension).description;
    }
};
