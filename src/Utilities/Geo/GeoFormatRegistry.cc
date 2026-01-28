#include "GeoFormatRegistry.h"
#include "CSVHelper.h"
#include "GeoJsonHelper.h"
#include "GeoPackageHelper.h"
#include "GeoUtilities.h"
#include "GPXHelper.h"
#include "KMLHelper.h"
#include "OpenAirParser.h"
#include "QGCLoggingCategory.h"
#include "SHPHelper.h"
#include "WKTHelper.h"

#include <QtConcurrent/QtConcurrent>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QLibrary>
#include <QtCore/QProcess>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QUuid>

QGC_LOGGING_CATEGORY(GeoFormatRegistryLog, "Utilities.Geo.GeoFormatRegistry")

namespace GeoFormatRegistry
{

namespace {

static bool s_gdalChecked = false;
static bool s_gdalAvailable = false;
static QString s_gdalVersion;

void checkGDAL()
{
    if (s_gdalChecked) {
        return;
    }
    s_gdalChecked = true;

    // Try to find ogr2ogr in PATH
    QProcess process;
    process.start(QStringLiteral("ogr2ogr"), QStringList() << QStringLiteral("--version"));
    if (process.waitForFinished(1000)) {
        QString output = QString::fromUtf8(process.readAllStandardOutput());
        if (output.contains(QStringLiteral("GDAL"))) {
            s_gdalAvailable = true;
            // Extract version: "GDAL 3.4.1, released 2021/12/27"
            int idx = output.indexOf(QStringLiteral("GDAL"));
            if (idx >= 0) {
                s_gdalVersion = output.mid(idx + 5).split(',').first().trimmed();
            }
            qCDebug(GeoFormatRegistryLog) << "GDAL available:" << s_gdalVersion;
        }
    }

#ifdef Q_OS_LINUX
    // Also try loading libgdal directly
    if (!s_gdalAvailable) {
        QLibrary gdal(QStringLiteral("gdal"));
        if (gdal.load()) {
            s_gdalAvailable = true;
            qCDebug(GeoFormatRegistryLog) << "GDAL library loaded directly";
            gdal.unload();
        }
    }
#endif

    if (!s_gdalAvailable) {
        qCDebug(GeoFormatRegistryLog) << "GDAL not available - using native parsers only";
    }
}

QList<FormatInfo> buildNativeFormats()
{
    QList<FormatInfo> formats;

    formats.append({
        QStringLiteral("KML"),
        QStringLiteral("Keyhole Markup Language"),
        {QStringLiteral("kml")},
        CanReadWriteAll,
        true
    });

    formats.append({
        QStringLiteral("KMZ"),
        QStringLiteral("Compressed KML"),
        {QStringLiteral("kmz")},
        CanRead | CanReadPolygonsWithHoles | CanWritePolygons | CanWritePolylines | CanWritePolygonsWithHoles,
        true
    });

    formats.append({
        QStringLiteral("GeoJSON"),
        QStringLiteral("GeoJSON"),
        {QStringLiteral("geojson"), QStringLiteral("json")},
        CanReadWriteAll,
        true
    });

    formats.append({
        QStringLiteral("GPX"),
        QStringLiteral("GPS Exchange Format"),
        {QStringLiteral("gpx")},
        CanReadWrite | CanReadTracks | CanWriteTracks,
        true
    });

    formats.append({
        QStringLiteral("Shapefile"),
        QStringLiteral("ESRI Shapefile"),
        {QStringLiteral("shp")},
        CanReadWriteAll,
        true
    });

    formats.append({
        QStringLiteral("WKT"),
        QStringLiteral("Well-Known Text"),
        {QStringLiteral("wkt")},
        CanReadWrite,
        true
    });

    formats.append({
        QStringLiteral("OpenAir"),
        QStringLiteral("Airspace definition format"),
        {QStringLiteral("txt"), QStringLiteral("air")},
        CanReadPolygons | CanReadAirspace,
        true
    });

    formats.append({
        QStringLiteral("GeoPackage"),
        QStringLiteral("OGC GeoPackage"),
        {QStringLiteral("gpkg")},
        CanReadWrite,
        true
    });

    formats.append({
        QStringLiteral("CSV"),
        QStringLiteral("Comma-Separated Values"),
        {QStringLiteral("csv")},
        CanReadPoints | CanWritePoints,
        true
    });

    return formats;
}

} // anonymous namespace

// ============================================================================
// Format Detection from Content
// ============================================================================

DetectedFormat detectFormatFromBytes(const QByteArray &data)
{
    DetectedFormat result;

    if (data.size() < 4) {
        return result;
    }

    // Check for ZIP (KMZ, GPKG)
    if (data.startsWith("PK\x03\x04")) {
        // It's a ZIP - check if it contains KML (KMZ) or is GeoPackage
        // KMZ files contain a .kml inside
        // For now, assume KMZ since GeoPackage uses SQLite format
        result.extension = QStringLiteral("kmz");
        result.formatName = QStringLiteral("KMZ (Compressed KML)");
        result.confidence = 70;  // Could also be regular ZIP
        return result;
    }

    // Check for SQLite (GeoPackage)
    if (data.startsWith("SQLite format 3")) {
        result.extension = QStringLiteral("gpkg");
        result.formatName = QStringLiteral("GeoPackage");
        result.confidence = 80;  // Could be regular SQLite
        return result;
    }

    // Check for Shapefile (shp)
    // SHP magic number: 0x0000270a (big endian)
    if (data.size() >= 4) {
        quint32 magic = (static_cast<quint8>(data[0]) << 24) |
                        (static_cast<quint8>(data[1]) << 16) |
                        (static_cast<quint8>(data[2]) << 8) |
                        static_cast<quint8>(data[3]);
        if (magic == 0x0000270a) {
            result.extension = QStringLiteral("shp");
            result.formatName = QStringLiteral("Shapefile");
            result.confidence = 95;
            return result;
        }
    }

    // Text-based formats - convert to string for analysis
    QString text = QString::fromUtf8(data.left(2048));  // Check first 2KB

    // Check for KML
    if (text.contains(QLatin1String("<kml")) || text.contains(QLatin1String("opengis.net/kml"))) {
        result.extension = QStringLiteral("kml");
        result.formatName = QStringLiteral("KML (Keyhole Markup Language)");
        result.confidence = 95;
        return result;
    }

    // Check for GPX
    if (text.contains(QLatin1String("<gpx")) || text.contains(QLatin1String("topografix.com/GPX"))) {
        result.extension = QStringLiteral("gpx");
        result.formatName = QStringLiteral("GPX (GPS Exchange Format)");
        result.confidence = 95;
        return result;
    }

    // Check for GeoJSON
    if (text.contains(QLatin1String("\"type\"")) &&
        (text.contains(QLatin1String("\"FeatureCollection\"")) ||
         text.contains(QLatin1String("\"Feature\"")) ||
         text.contains(QLatin1String("\"Point\"")) ||
         text.contains(QLatin1String("\"LineString\"")) ||
         text.contains(QLatin1String("\"Polygon\"")) ||
         text.contains(QLatin1String("\"geometry\"")))) {
        result.extension = QStringLiteral("geojson");
        result.formatName = QStringLiteral("GeoJSON");
        result.confidence = 90;
        return result;
    }

    // Check for WKT
    if (text.contains(QRegularExpression(QStringLiteral("^\\s*(POINT|LINESTRING|POLYGON|MULTIPOINT|MULTILINESTRING|MULTIPOLYGON|GEOMETRYCOLLECTION)\\s*\\("),
                                         QRegularExpression::CaseInsensitiveOption))) {
        result.extension = QStringLiteral("wkt");
        result.formatName = QStringLiteral("WKT (Well-Known Text)");
        result.confidence = 90;
        return result;
    }

    // Check for OpenAir
    if (text.contains(QRegularExpression(QStringLiteral("^\\s*AC\\s+[A-Z]"), QRegularExpression::MultilineOption)) &&
        text.contains(QRegularExpression(QStringLiteral("^\\s*AN\\s+"), QRegularExpression::MultilineOption))) {
        result.extension = QStringLiteral("txt");  // OpenAir uses .txt or .air
        result.formatName = QStringLiteral("OpenAir (Airspace)");
        result.confidence = 85;
        return result;
    }

    // Check for CSV with coordinates
    // Look for header patterns
    QString lowerText = text.toLower();
    if ((lowerText.contains(QLatin1String("latitude")) || lowerText.contains(QLatin1String("lat"))) &&
        (lowerText.contains(QLatin1String("longitude")) || lowerText.contains(QLatin1String("lon")) || lowerText.contains(QLatin1String("lng")))) {
        result.extension = QStringLiteral("csv");
        result.formatName = QStringLiteral("CSV with coordinates");
        result.confidence = 70;
        return result;
    }

    return result;
}

DetectedFormat detectFormat(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return DetectedFormat();
    }

    QByteArray data = file.read(4096);  // Read first 4KB
    file.close();

    return detectFormatFromBytes(data);
}

QList<FormatInfo> supportedFormats()
{
    QList<FormatInfo> formats = buildNativeFormats();

    // Add GDAL formats if available
    if (isGDALAvailable()) {
        // These could be handled by GDAL if native fails
        formats.append({
            QStringLiteral("GML"),
            QStringLiteral("Geography Markup Language (via GDAL)"),
            {QStringLiteral("gml")},
            CanRead,
            false
        });

        formats.append({
            QStringLiteral("AIXM"),
            QStringLiteral("Aeronautical Information Exchange Model (via GDAL)"),
            {QStringLiteral("xml")},
            CanReadPolygons | CanReadAirspace,
            false
        });

        formats.append({
            QStringLiteral("MapInfo"),
            QStringLiteral("MapInfo TAB (via GDAL)"),
            {QStringLiteral("tab")},
            CanRead,
            false
        });
    }

    return formats;
}

QList<FormatInfo> nativeFormats()
{
    return buildNativeFormats();
}

bool isSupported(const QString &extension)
{
    QString ext = extension.toLower();
    QList<FormatInfo> formats = supportedFormats();

    for (const FormatInfo &format : formats) {
        if (format.extensions.contains(ext)) {
            return true;
        }
    }

    return false;
}

FormatInfo formatInfo(const QString &extension)
{
    QString ext = extension.toLower();
    QList<FormatInfo> formats = supportedFormats();

    for (const FormatInfo &format : formats) {
        if (format.extensions.contains(ext)) {
            return format;
        }
    }

    return FormatInfo();
}

FormatInfo formatInfoForFile(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    return formatInfo(fileInfo.suffix());
}

bool hasCapability(const QString &extension, Capability capability)
{
    FormatInfo info = formatInfo(extension);
    return info.capabilities & capability;
}

bool fileHasCapability(const QString &filePath, Capability capability)
{
    FormatInfo info = formatInfoForFile(filePath);
    return info.capabilities & capability;
}

QString readFileFilter()
{
    QStringList allExtensions;
    QStringList filters;

    QList<FormatInfo> formats = supportedFormats();
    for (const FormatInfo &format : formats) {
        if (format.capabilities & CanRead) {
            QStringList exts;
            for (const QString &ext : format.extensions) {
                exts.append(QStringLiteral("*.%1").arg(ext));
                allExtensions.append(QStringLiteral("*.%1").arg(ext));
            }
            filters.append(QStringLiteral("%1 (%2)").arg(format.name, exts.join(' ')));
        }
    }

    // Add "All Geo Files" at the beginning
    QString allFilter = QStringLiteral("All Geo Files (%1)").arg(allExtensions.join(' '));
    filters.prepend(allFilter);

    return filters.join(QStringLiteral(";;"));
}

QString writeFileFilter()
{
    QStringList filters;

    QList<FormatInfo> formats = supportedFormats();
    for (const FormatInfo &format : formats) {
        if (format.capabilities & CanWrite) {
            QStringList exts;
            for (const QString &ext : format.extensions) {
                exts.append(QStringLiteral("*.%1").arg(ext));
            }
            filters.append(QStringLiteral("%1 (%2)").arg(format.name, exts.join(' ')));
        }
    }

    return filters.join(QStringLiteral(";;"));
}

QStringList readFileFilterList()
{
    QStringList allExtensions;
    QStringList filters;

    QList<FormatInfo> formats = supportedFormats();
    for (const FormatInfo &format : formats) {
        if (format.capabilities & CanRead) {
            for (const QString &ext : format.extensions) {
                allExtensions.append(QStringLiteral("*.%1").arg(ext));
            }
        }
    }

    // Add "All Geo Files" filter first
    filters.append(QStringLiteral("All Geo Files (%1)").arg(allExtensions.join(' ')));

    // Add individual format filters
    for (const FormatInfo &format : formats) {
        if (format.capabilities & CanRead) {
            QStringList exts;
            for (const QString &ext : format.extensions) {
                exts.append(QStringLiteral("*.%1").arg(ext));
            }
            filters.append(QStringLiteral("%1 (%2)").arg(format.name, exts.join(' ')));
        }
    }

    return filters;
}

QStringList writeFileFilterList()
{
    QStringList filters;

    QList<FormatInfo> formats = supportedFormats();
    for (const FormatInfo &format : formats) {
        if (format.capabilities & CanWrite) {
            QStringList exts;
            for (const QString &ext : format.extensions) {
                exts.append(QStringLiteral("*.%1").arg(ext));
            }
            filters.append(QStringLiteral("%1 (%2)").arg(format.name, exts.join(' ')));
        }
    }

    return filters;
}

// ============================================================================
// Shape Detection
// ============================================================================

ShapeType determineShapeType(const QString &filePath, QString &errorString)
{
    errorString.clear();

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        errorString = QObject::tr("File not found: %1").arg(filePath);
        return ShapeType::Error;
    }

    QString ext = fileInfo.suffix().toLower();

    if (ext == QStringLiteral("kml") || ext == QStringLiteral("kmz")) {
        return KMLHelper::determineShapeType(filePath, errorString);
    }
    if (ext == QStringLiteral("geojson") || ext == QStringLiteral("json")) {
        return GeoJsonHelper::determineShapeType(filePath, errorString);
    }
    if (ext == QStringLiteral("gpx")) {
        return GPXHelper::determineShapeType(filePath, errorString);
    }
    if (ext == QStringLiteral("shp")) {
        return SHPHelper::determineShapeType(filePath, errorString);
    }

    // For formats without specific shape type detection, try loading
    LoadResult result = loadFile(filePath);
    if (!result.success) {
        errorString = result.errorString;
        return ShapeType::Error;
    }

    if (!result.polygons.isEmpty() || !result.polygonsWithHoles.isEmpty()) {
        return ShapeType::Polygon;
    }
    if (!result.polylines.isEmpty()) {
        return ShapeType::Polyline;
    }
    if (!result.points.isEmpty()) {
        return ShapeType::Point;
    }

    errorString = QObject::tr("No geometry found in file");
    return ShapeType::Error;
}

int getEntityCount(const QString &filePath, QString &errorString)
{
    errorString.clear();

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        errorString = QObject::tr("File not found: %1").arg(filePath);
        return 0;
    }

    QString ext = fileInfo.suffix().toLower();

    if (ext == QStringLiteral("kml") || ext == QStringLiteral("kmz")) {
        return KMLHelper::getEntityCount(filePath, errorString);
    }
    if (ext == QStringLiteral("geojson") || ext == QStringLiteral("json")) {
        return GeoJsonHelper::getEntityCount(filePath, errorString);
    }
    if (ext == QStringLiteral("gpx")) {
        return GPXHelper::getEntityCount(filePath, errorString);
    }
    if (ext == QStringLiteral("shp")) {
        return SHPHelper::getEntityCount(filePath, errorString);
    }

    // For other formats, load and count
    LoadResult result = loadFile(filePath);
    if (!result.success) {
        errorString = result.errorString;
        return 0;
    }

    return result.totalFeatures();
}

// ============================================================================
// Vertex Filtering
// ============================================================================

void filterVertices(QList<QGeoCoordinate> &vertices, double filterMeters, int minVertices)
{
    if (filterMeters <= 0 || vertices.count() <= minVertices) {
        return;
    }

    int i = 0;
    while (i < (vertices.count() - 1)) {
        if ((vertices.count() > minVertices) && (vertices[i].distanceTo(vertices[i + 1]) < filterMeters)) {
            vertices.removeAt(i + 1);
        } else {
            i++;
        }
    }
}

LoadResult loadFile(const QString &filePath, double filterMeters)
{
    LoadResult result;

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        result.errorString = QObject::tr("File not found: %1").arg(filePath);
        return result;
    }

    QString ext = fileInfo.suffix().toLower();

    // Lambda to apply filtering to a list of coordinate lists
    auto applyFilter = [filterMeters](QList<QList<QGeoCoordinate>> &coordLists, int minVertices) {
        if (filterMeters > 0) {
            for (QList<QGeoCoordinate> &coords : coordLists) {
                filterVertices(coords, filterMeters, minVertices);
            }
        }
    };

    // Try native loaders first
    if (ext == QStringLiteral("kml") || ext == QStringLiteral("kmz")) {
        QStringList errors;
        QString errorString;
        // KMLHelper handles KMZ files transparently (decompresses internally)
        if (!KMLHelper::loadPointsFromFile(filePath, result.points, errorString) && !errorString.isEmpty()) {
            errors.append(errorString);
        }
        errorString.clear();
        if (!KMLHelper::loadPolylinesFromFile(filePath, result.polylines, errorString, filterMeters) && !errorString.isEmpty()) {
            errors.append(errorString);
        }
        errorString.clear();
        if (!KMLHelper::loadPolygonsFromFile(filePath, result.polygons, errorString, filterMeters) && !errorString.isEmpty()) {
            errors.append(errorString);
        }
        result.formatUsed = (ext == QStringLiteral("kmz")) ? QStringLiteral("KMZ") : QStringLiteral("KML");
        result.success = result.totalFeatures() > 0;
        if (!result.success && !errors.isEmpty()) {
            result.errorString = errors.join(QStringLiteral("; "));
        }
        return result;
    }

    if (ext == QStringLiteral("geojson") || ext == QStringLiteral("json")) {
        QStringList errors;
        QString errorString;
        if (!GeoJsonHelper::loadPointsFromFile(filePath, result.points, errorString) && !errorString.isEmpty()) {
            errors.append(errorString);
        }
        errorString.clear();
        if (!GeoJsonHelper::loadPolylinesFromFile(filePath, result.polylines, errorString, filterMeters) && !errorString.isEmpty()) {
            errors.append(errorString);
        }
        errorString.clear();
        if (!GeoJsonHelper::loadPolygonsFromFile(filePath, result.polygons, errorString, filterMeters) && !errorString.isEmpty()) {
            errors.append(errorString);
        }
        result.formatUsed = QStringLiteral("GeoJSON");
        result.success = result.totalFeatures() > 0;
        if (!result.success && !errors.isEmpty()) {
            result.errorString = errors.join(QStringLiteral("; "));
        }
        return result;
    }

    if (ext == QStringLiteral("gpx")) {
        QStringList errors;
        QString errorString;
        if (!GPXHelper::loadPointsFromFile(filePath, result.points, errorString) && !errorString.isEmpty()) {
            errors.append(errorString);
        }
        errorString.clear();
        if (!GPXHelper::loadPolylinesFromFile(filePath, result.polylines, errorString, filterMeters) && !errorString.isEmpty()) {
            errors.append(errorString);
        }
        errorString.clear();
        if (!GPXHelper::loadPolygonsFromFile(filePath, result.polygons, errorString, filterMeters) && !errorString.isEmpty()) {
            errors.append(errorString);
        }
        result.formatUsed = QStringLiteral("GPX");
        result.success = result.totalFeatures() > 0;
        if (!result.success && !errors.isEmpty()) {
            result.errorString = errors.join(QStringLiteral("; "));
        }
        return result;
    }

    if (ext == QStringLiteral("shp")) {
        QStringList errors;
        QString errorString;
        if (!SHPHelper::loadPointsFromFile(filePath, result.points, errorString) && !errorString.isEmpty()) {
            errors.append(errorString);
        }
        errorString.clear();
        if (!SHPHelper::loadPolylinesFromFile(filePath, result.polylines, errorString, filterMeters) && !errorString.isEmpty()) {
            errors.append(errorString);
        }
        errorString.clear();
        if (!SHPHelper::loadPolygonsFromFile(filePath, result.polygons, errorString, filterMeters) && !errorString.isEmpty()) {
            errors.append(errorString);
        }
        result.formatUsed = QStringLiteral("Shapefile");
        result.success = result.totalFeatures() > 0;
        if (!result.success && !errors.isEmpty()) {
            result.errorString = errors.join(QStringLiteral("; "));
        }
        return result;
    }

    if (ext == QStringLiteral("gpkg")) {
        GeoPackageHelper::LoadResult gpkgResult = GeoPackageHelper::loadAllFeatures(filePath);
        result.points = gpkgResult.points;
        result.polylines = gpkgResult.polylines;
        result.polygons = gpkgResult.polygons;
        applyFilter(result.polylines, GeoUtilities::kMinPolylineVertices);
        applyFilter(result.polygons, GeoUtilities::kMinPolygonVertices);
        result.formatUsed = QStringLiteral("GeoPackage");
        result.success = gpkgResult.success;
        result.errorString = gpkgResult.errorString;
        return result;
    }

    if (ext == QStringLiteral("txt") || ext == QStringLiteral("air")) {
        // Try OpenAir format
        OpenAirParser::ParseResult airResult = OpenAirParser::parseFile(filePath);
        if (airResult.success) {
            for (const OpenAirParser::Airspace &airspace : airResult.airspaces) {
                result.polygons.append(airspace.boundary);
            }
            applyFilter(result.polygons, GeoUtilities::kMinPolygonVertices);
            result.formatUsed = QStringLiteral("OpenAir");
            result.success = true;
            return result;
        }
        // If OpenAir fails, don't report error - might be a regular text file
    }

    if (ext == QStringLiteral("wkt")) {
        // Read WKT from file
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = QString::fromUtf8(file.readAll());
            QString errorString;

            // Try different WKT types
            QList<QGeoCoordinate> polygon;
            if (WKTHelper::parsePolygon(content, polygon, errorString)) {
                result.polygons.append(polygon);
            }

            QList<QGeoCoordinate> line;
            if (WKTHelper::parseLineString(content, line, errorString)) {
                result.polylines.append(line);
            }

            QGeoCoordinate point;
            if (WKTHelper::parsePoint(content, point, errorString)) {
                result.points.append(point);
            }

            applyFilter(result.polylines, GeoUtilities::kMinPolylineVertices);
            applyFilter(result.polygons, GeoUtilities::kMinPolygonVertices);
            result.formatUsed = QStringLiteral("WKT");
            result.success = result.totalFeatures() > 0;
            if (!result.success) {
                result.errorString = errorString;
            }
            return result;
        }
    }

    if (ext == QStringLiteral("csv")) {
        QString errorString;
        if (CSVHelper::loadPointsFromFile(filePath, result.points, errorString)) {
            result.formatUsed = QStringLiteral("CSV");
            result.success = !result.points.isEmpty();
        }
        if (!result.success) {
            result.errorString = errorString;
        }
        return result;
    }

    // Try GDAL as fallback for unsupported formats
    if (isGDALAvailable()) {
        qCDebug(GeoFormatRegistryLog) << "No native loader for extension" << ext << "- falling back to GDAL for:" << filePath;
        return loadWithGDAL(filePath);
    }

    result.errorString = QObject::tr("Unsupported format: %1").arg(ext);
    return result;
}

QFuture<LoadResult> loadFileAsync(const QString &filePath, double filterMeters)
{
    // Capture copies of parameters to ensure thread safety
    return QtConcurrent::run([filePath, filterMeters]() {
        return loadFile(filePath, filterMeters);
    });
}

bool loadPoints(const QString &filePath, QList<QGeoCoordinate> &points, QString &errorString)
{
    LoadResult result = loadFile(filePath);
    if (!result.success) {
        errorString = result.errorString;
        return false;
    }
    points = result.points;
    return true;
}

bool loadPolylines(const QString &filePath, QList<QList<QGeoCoordinate>> &polylines, QString &errorString, double filterMeters)
{
    LoadResult result = loadFile(filePath, filterMeters);
    if (!result.success) {
        errorString = result.errorString;
        return false;
    }
    polylines = result.polylines;
    return true;
}

bool loadPolygons(const QString &filePath, QList<QList<QGeoCoordinate>> &polygons, QString &errorString, double filterMeters)
{
    LoadResult result = loadFile(filePath, filterMeters);
    if (!result.success) {
        errorString = result.errorString;
        return false;
    }
    polygons = result.polygons;
    return true;
}

bool loadPolygon(const QString &filePath, QList<QGeoCoordinate> &polygon, QString &errorString, double filterMeters)
{
    QList<QList<QGeoCoordinate>> polygons;
    if (!loadPolygons(filePath, polygons, errorString, filterMeters)) {
        return false;
    }
    if (polygons.isEmpty()) {
        errorString = QObject::tr("No polygons found in file");
        return false;
    }
    polygon = polygons.first();
    return true;
}

bool loadPolyline(const QString &filePath, QList<QGeoCoordinate> &polyline, QString &errorString, double filterMeters)
{
    QList<QList<QGeoCoordinate>> polylines;
    if (!loadPolylines(filePath, polylines, errorString, filterMeters)) {
        return false;
    }
    if (polylines.isEmpty()) {
        errorString = QObject::tr("No polylines found in file");
        return false;
    }
    polyline = polylines.first();
    return true;
}

bool loadPolygonsWithHoles(const QString &filePath, QList<QGeoPolygon> &polygons, QString &errorString)
{
    errorString.clear();
    polygons.clear();

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        errorString = QObject::tr("File not found: %1").arg(filePath);
        return false;
    }

    QString ext = fileInfo.suffix().toLower();

    if (ext == QStringLiteral("kml") || ext == QStringLiteral("kmz")) {
        return KMLHelper::loadPolygonsWithHolesFromFile(filePath, polygons, errorString);
    }
    if (ext == QStringLiteral("geojson") || ext == QStringLiteral("json")) {
        return GeoJsonHelper::loadPolygonsWithHolesFromFile(filePath, polygons, errorString);
    }
    if (ext == QStringLiteral("shp")) {
        return SHPHelper::loadPolygonsWithHolesFromFile(filePath, polygons, errorString);
    }
    if (ext == QStringLiteral("gpx")) {
        errorString = QObject::tr("GPX format does not support polygons with holes");
        return false;
    }

    errorString = QObject::tr("Format does not support polygons with holes: %1").arg(ext);
    return false;
}

bool loadPolygonWithHoles(const QString &filePath, QGeoPolygon &polygon, QString &errorString)
{
    QList<QGeoPolygon> polygons;
    if (!loadPolygonsWithHoles(filePath, polygons, errorString)) {
        return false;
    }
    if (polygons.isEmpty()) {
        errorString = QObject::tr("No polygons found in file");
        return false;
    }
    polygon = polygons.first();
    return true;
}

// ============================================================================
// Save Functions
// ============================================================================

SaveResult savePoint(const QString &filePath, const QGeoCoordinate &point)
{
    QList<QGeoCoordinate> points;
    points.append(point);
    return savePoints(filePath, points);
}

SaveResult savePoints(const QString &filePath, const QList<QGeoCoordinate> &points)
{
    SaveResult result;

    // Validate coordinates before saving
    QString validationError;
    if (!GeoUtilities::validateCoordinates(points, validationError)) {
        result.errorString = QObject::tr("Invalid coordinate data: %1").arg(validationError);
        return result;
    }

    QFileInfo fileInfo(filePath);
    QString ext = fileInfo.suffix().toLower();

    QString errorString;

    if (ext == QStringLiteral("kml")) {
        if (KMLHelper::savePointsToFile(filePath, points, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("KML");
        } else {
            result.errorString = errorString;
        }
    } else if (ext == QStringLiteral("geojson") || ext == QStringLiteral("json")) {
        if (GeoJsonHelper::savePointsToFile(filePath, points, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("GeoJSON");
        } else {
            result.errorString = errorString;
        }
    } else if (ext == QStringLiteral("gpx")) {
        if (GPXHelper::savePointsToFile(filePath, points, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("GPX");
        } else {
            result.errorString = errorString;
        }
    } else if (ext == QStringLiteral("shp")) {
        if (SHPHelper::savePointsToFile(filePath, points, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("Shapefile");
        } else {
            result.errorString = errorString;
        }
    } else if (ext == QStringLiteral("gpkg")) {
        if (GeoPackageHelper::savePoints(filePath, points, QStringLiteral("points"), errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("GeoPackage");
        } else {
            result.errorString = errorString;
        }
    } else if (ext == QStringLiteral("wkt")) {
        if (WKTHelper::savePointsToFile(filePath, points, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("WKT");
        } else {
            result.errorString = errorString;
        }
    } else if (ext == QStringLiteral("csv")) {
        if (CSVHelper::savePointsToFile(filePath, points, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("CSV");
        } else {
            result.errorString = errorString;
        }
    } else {
        result.errorString = QObject::tr("Unsupported format for point save: %1").arg(ext);
    }

    return result;
}

SaveResult savePolyline(const QString &filePath, const QList<QGeoCoordinate> &polyline)
{
    QList<QList<QGeoCoordinate>> polylines;
    polylines.append(polyline);
    return savePolylines(filePath, polylines);
}

SaveResult savePolylines(const QString &filePath, const QList<QList<QGeoCoordinate>> &polylines)
{
    SaveResult result;

    // Validate coordinates before saving
    QString validationError;
    for (int i = 0; i < polylines.size(); ++i) {
        if (!GeoUtilities::validateCoordinates(polylines[i], validationError)) {
            result.errorString = QObject::tr("Polyline %1: %2").arg(i + 1).arg(validationError);
            return result;
        }
    }

    QFileInfo fileInfo(filePath);
    QString ext = fileInfo.suffix().toLower();

    QString errorString;

    if (ext == QStringLiteral("kml")) {
        if (KMLHelper::savePolylinesToFile(filePath, polylines, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("KML");
        } else {
            result.errorString = errorString;
        }
    } else if (ext == QStringLiteral("geojson") || ext == QStringLiteral("json")) {
        if (GeoJsonHelper::savePolylinesToFile(filePath, polylines, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("GeoJSON");
        } else {
            result.errorString = errorString;
        }
    } else if (ext == QStringLiteral("gpx")) {
        if (GPXHelper::savePolylinesToFile(filePath, polylines, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("GPX");
        } else {
            result.errorString = errorString;
        }
    } else if (ext == QStringLiteral("shp")) {
        if (SHPHelper::savePolylinesToFile(filePath, polylines, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("Shapefile");
        } else {
            result.errorString = errorString;
        }
    } else if (ext == QStringLiteral("gpkg")) {
        if (GeoPackageHelper::savePolylines(filePath, polylines, QStringLiteral("polylines"), errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("GeoPackage");
        } else {
            result.errorString = errorString;
        }
    } else if (ext == QStringLiteral("wkt")) {
        if (WKTHelper::savePolylinesToFile(filePath, polylines, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("WKT");
        } else {
            result.errorString = errorString;
        }
    } else {
        result.errorString = QObject::tr("Unsupported format for polyline save: %1").arg(ext);
    }

    return result;
}

SaveResult savePolygon(const QString &filePath, const QList<QGeoCoordinate> &polygon)
{
    QList<QList<QGeoCoordinate>> polygons;
    polygons.append(polygon);
    return savePolygons(filePath, polygons);
}

SaveResult savePolygons(const QString &filePath, const QList<QList<QGeoCoordinate>> &polygons)
{
    SaveResult result;

    // Validate coordinates before saving
    QString validationError;
    for (int i = 0; i < polygons.size(); ++i) {
        if (!GeoUtilities::validateCoordinates(polygons[i], validationError)) {
            result.errorString = QObject::tr("Polygon %1: %2").arg(i + 1).arg(validationError);
            return result;
        }
    }

    QFileInfo fileInfo(filePath);
    QString ext = fileInfo.suffix().toLower();

    QString errorString;

    if (ext == QStringLiteral("kml")) {
        if (KMLHelper::savePolygonsToFile(filePath, polygons, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("KML");
        } else {
            result.errorString = errorString;
        }
    } else if (ext == QStringLiteral("geojson") || ext == QStringLiteral("json")) {
        if (GeoJsonHelper::savePolygonsToFile(filePath, polygons, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("GeoJSON");
        } else {
            result.errorString = errorString;
        }
    } else if (ext == QStringLiteral("gpx")) {
        if (GPXHelper::savePolygonsToFile(filePath, polygons, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("GPX");
        } else {
            result.errorString = errorString;
        }
    } else if (ext == QStringLiteral("shp")) {
        if (SHPHelper::savePolygonsToFile(filePath, polygons, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("Shapefile");
        } else {
            result.errorString = errorString;
        }
    } else if (ext == QStringLiteral("gpkg")) {
        if (GeoPackageHelper::savePolygons(filePath, polygons, QStringLiteral("polygons"), errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("GeoPackage");
        } else {
            result.errorString = errorString;
        }
    } else if (ext == QStringLiteral("wkt")) {
        if (WKTHelper::savePolygonsToFile(filePath, polygons, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("WKT");
        } else {
            result.errorString = errorString;
        }
    } else {
        result.errorString = QObject::tr("Unsupported format for polygon save: %1").arg(ext);
    }

    return result;
}

SaveResult savePolygonWithHoles(const QString &filePath, const QGeoPolygon &polygon)
{
    QList<QGeoPolygon> polygons;
    polygons.append(polygon);
    return savePolygonsWithHoles(filePath, polygons);
}

SaveResult savePolygonsWithHoles(const QString &filePath, const QList<QGeoPolygon> &polygons)
{
    SaveResult result;

    // Validate coordinates before saving
    QString validationError;
    for (int i = 0; i < polygons.size(); ++i) {
        // Validate outer ring
        QList<QGeoCoordinate> outerRing = polygons[i].perimeter();
        if (!GeoUtilities::validateCoordinates(outerRing, validationError)) {
            result.errorString = QObject::tr("Polygon %1 outer ring: %2").arg(i + 1).arg(validationError);
            return result;
        }
        // Validate holes
        for (int h = 0; h < polygons[i].holesCount(); ++h) {
            QList<QGeoCoordinate> hole = polygons[i].holePath(h);
            if (!GeoUtilities::validateCoordinates(hole, validationError)) {
                result.errorString = QObject::tr("Polygon %1 hole %2: %3").arg(i + 1).arg(h + 1).arg(validationError);
                return result;
            }
        }
    }

    QFileInfo fileInfo(filePath);
    QString ext = fileInfo.suffix().toLower();

    QString errorString;

    if (ext == QStringLiteral("kml") || ext == QStringLiteral("kmz")) {
        if (KMLHelper::savePolygonsWithHolesToFile(filePath, polygons, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("KML");
        } else {
            result.errorString = errorString;
        }
    } else if (ext == QStringLiteral("geojson") || ext == QStringLiteral("json")) {
        if (GeoJsonHelper::savePolygonsWithHolesToFile(filePath, polygons, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("GeoJSON");
        } else {
            result.errorString = errorString;
        }
    } else if (ext == QStringLiteral("shp")) {
        if (SHPHelper::savePolygonsWithHolesToFile(filePath, polygons, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("Shapefile");
        } else {
            result.errorString = errorString;
        }
    } else if (ext == QStringLiteral("gpx")) {
        result.errorString = QObject::tr("GPX format does not support polygons with holes");
    } else {
        result.errorString = QObject::tr("Unsupported format for polygon with holes save: %1").arg(ext);
    }

    return result;
}

SaveResult saveTrack(const QString &filePath, const QList<QGeoCoordinate> &track)
{
    SaveResult result;

    // Validate coordinates before saving
    QString validationError;
    if (!GeoUtilities::validateCoordinates(track, validationError)) {
        result.errorString = QObject::tr("Track: %1").arg(validationError);
        return result;
    }

    QFileInfo fileInfo(filePath);
    QString ext = fileInfo.suffix().toLower();

    if (ext == QStringLiteral("gpx")) {
        QString errorString;
        if (GPXHelper::saveTrackToFile(filePath, track, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("GPX");
        } else {
            result.errorString = errorString;
        }
    } else {
        result.errorString = QObject::tr("Tracks are only supported in GPX format. Use savePolyline for other formats.");
    }

    return result;
}

SaveResult saveTracks(const QString &filePath, const QList<QList<QGeoCoordinate>> &tracks)
{
    SaveResult result;

    // Validate coordinates before saving
    QString validationError;
    for (int i = 0; i < tracks.size(); ++i) {
        if (!GeoUtilities::validateCoordinates(tracks[i], validationError)) {
            result.errorString = QObject::tr("Track %1: %2").arg(i + 1).arg(validationError);
            return result;
        }
    }

    QFileInfo fileInfo(filePath);
    QString ext = fileInfo.suffix().toLower();

    if (ext == QStringLiteral("gpx")) {
        QString errorString;
        if (GPXHelper::saveTracksToFile(filePath, tracks, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("GPX");
        } else {
            result.errorString = errorString;
        }
    } else {
        result.errorString = QObject::tr("Tracks are only supported in GPX format. Use savePolylines for other formats.");
    }

    return result;
}

// ============================================================================
// Validation Functions
// ============================================================================

namespace {

// Check which save implementations exist for a format by extension
Capabilities checkSaveImplementations(const QString &ext)
{
    Capabilities caps;

    // These are the formats that have save implementations in the save functions above
    if (ext == QStringLiteral("kml") || ext == QStringLiteral("geojson") ||
        ext == QStringLiteral("json") || ext == QStringLiteral("gpx") ||
        ext == QStringLiteral("shp") || ext == QStringLiteral("gpkg") ||
        ext == QStringLiteral("wkt")) {
        caps = CanWritePoints | CanWritePolylines | CanWritePolygons;
    }
    // KMZ has partial write support (no points)
    else if (ext == QStringLiteral("kmz")) {
        caps = CanWritePolylines | CanWritePolygons;
    }
    // CSV only supports points
    else if (ext == QStringLiteral("csv")) {
        caps = CanWritePoints;
    }

    return caps;
}

// Check which load implementations exist for a format by extension
Capabilities checkLoadImplementations(const QString &ext)
{
    Capabilities caps;

    if (ext == QStringLiteral("kml") || ext == QStringLiteral("kmz") ||
        ext == QStringLiteral("geojson") || ext == QStringLiteral("json") ||
        ext == QStringLiteral("gpx") || ext == QStringLiteral("shp") ||
        ext == QStringLiteral("gpkg") || ext == QStringLiteral("wkt")) {
        caps = CanReadPoints | CanReadPolylines | CanReadPolygons;
    }
    // OpenAir only has polygon/airspace support
    else if (ext == QStringLiteral("txt") || ext == QStringLiteral("air")) {
        caps = CanReadPolygons | CanReadAirspace;
    }
    // CSV only supports points
    else if (ext == QStringLiteral("csv")) {
        caps = CanReadPoints;
    }

    return caps;
}

} // anonymous namespace

QList<ValidationResult> validateCapabilities()
{
    QList<ValidationResult> results;

    QList<FormatInfo> formats = nativeFormats();  // Only validate native formats

    for (const FormatInfo &format : formats) {
        ValidationResult result;
        result.formatName = format.name;

        for (const QString &ext : format.extensions) {
            Capabilities actualLoad = checkLoadImplementations(ext);
            Capabilities actualSave = checkSaveImplementations(ext);
            Capabilities actualCaps = actualLoad | actualSave;

            // Check read capabilities
            if ((format.capabilities & CanReadPoints) && !(actualCaps & CanReadPoints)) {
                result.issues.append(QStringLiteral("Claims CanReadPoints but no implementation for .%1").arg(ext));
            }
            if ((format.capabilities & CanReadPolylines) && !(actualCaps & CanReadPolylines)) {
                result.issues.append(QStringLiteral("Claims CanReadPolylines but no implementation for .%1").arg(ext));
            }
            if ((format.capabilities & CanReadPolygons) && !(actualCaps & CanReadPolygons)) {
                result.issues.append(QStringLiteral("Claims CanReadPolygons but no implementation for .%1").arg(ext));
            }

            // Check write capabilities
            if ((format.capabilities & CanWritePoints) && !(actualCaps & CanWritePoints)) {
                result.issues.append(QStringLiteral("Claims CanWritePoints but no implementation for .%1").arg(ext));
            }
            if ((format.capabilities & CanWritePolylines) && !(actualCaps & CanWritePolylines)) {
                result.issues.append(QStringLiteral("Claims CanWritePolylines but no implementation for .%1").arg(ext));
            }
            if ((format.capabilities & CanWritePolygons) && !(actualCaps & CanWritePolygons)) {
                result.issues.append(QStringLiteral("Claims CanWritePolygons but no implementation for .%1").arg(ext));
            }
        }

        result.valid = result.issues.isEmpty();
        if (!result.valid) {
            qCWarning(GeoFormatRegistryLog) << "Format" << format.name << "has capability mismatches:" << result.issues;
        }
        results.append(result);
    }

    return results;
}

bool allCapabilitiesValid()
{
    QList<ValidationResult> results = validateCapabilities();
    for (const ValidationResult &result : results) {
        if (!result.valid) {
            return false;
        }
    }
    return true;
}

bool isGDALAvailable()
{
    checkGDAL();
    return s_gdalAvailable;
}

QString gdalVersion()
{
    checkGDAL();
    return s_gdalVersion;
}

LoadResult loadWithGDAL(const QString &filePath)
{
    LoadResult result;

    if (!isGDALAvailable()) {
        result.errorString = QObject::tr("GDAL not available");
        return result;
    }

    // Convert to GeoJSON using ogr2ogr, then parse with our GeoJSON loader
    // Use platform-appropriate temp directory with unique filename
    const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString tempFile = QStringLiteral("%1/qgc_gdal_%2.geojson")
                                 .arg(tempDir, QUuid::createUuid().toString(QUuid::WithoutBraces));

    QProcess process;
    process.start(QStringLiteral("ogr2ogr"),
                  QStringList() << QStringLiteral("-f") << QStringLiteral("GeoJSON")
                                << tempFile << filePath);

    if (!process.waitForFinished(30000)) {
        result.errorString = QObject::tr("GDAL conversion timed out");
        QFile::remove(tempFile);  // Clean up on timeout
        return result;
    }

    if (process.exitCode() != 0) {
        result.errorString = QObject::tr("GDAL conversion failed: %1")
                                 .arg(QString::fromUtf8(process.readAllStandardError()));
        QFile::remove(tempFile);  // Clean up on error
        return result;
    }

    // Load the converted GeoJSON with proper error accumulation
    QStringList errors;
    QString errorString;
    if (!GeoJsonHelper::loadPointsFromFile(tempFile, result.points, errorString) && !errorString.isEmpty()) {
        errors.append(errorString);
    }
    errorString.clear();
    if (!GeoJsonHelper::loadPolylinesFromFile(tempFile, result.polylines, errorString, true) && !errorString.isEmpty()) {
        errors.append(errorString);
    }
    errorString.clear();
    if (!GeoJsonHelper::loadPolygonsFromFile(tempFile, result.polygons, errorString, true) && !errorString.isEmpty()) {
        errors.append(errorString);
    }

    // Clean up temp file
    QFile::remove(tempFile);

    result.formatUsed = QStringLiteral("GDAL");
    result.success = result.totalFeatures() > 0;
    if (!result.success && !errors.isEmpty()) {
        result.errorString = errors.join(QStringLiteral("; "));
    }

    return result;
}

QStringList gdalFormats()
{
    if (!isGDALAvailable()) {
        return QStringList();
    }

    // Get GDAL supported formats
    QProcess process;
    process.start(QStringLiteral("ogr2ogr"), QStringList() << QStringLiteral("--formats"));

    if (!process.waitForFinished(5000)) {
        return QStringList();
    }

    QStringList formats;
    QString output = QString::fromUtf8(process.readAllStandardOutput());
    QStringList lines = output.split('\n');

    for (const QString &line : lines) {
        if (line.contains(QStringLiteral("->"))) {
            QString format = line.split(QStringLiteral("->")).first().trimmed();
            format = format.split(QStringLiteral(" ")).first();
            if (!format.isEmpty()) {
                formats.append(format);
            }
        }
    }

    return formats;
}

} // namespace GeoFormatRegistry
