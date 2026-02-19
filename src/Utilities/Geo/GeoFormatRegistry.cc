#include "GeoFormatRegistry.h"
#include "CSVHelper.h"
#include "GeoFileIO.h"
#include "GeoJsonHelper.h"
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
#include <QtCore/QRegularExpression>

QGC_LOGGING_CATEGORY(GeoFormatRegistryLog, "Utilities.Geo.GeoFormatRegistry")

namespace GeoFormatRegistry
{

namespace {

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
        CanRead | CanReadPolygonsWithHoles,
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
        {QStringLiteral("air")},
        CanReadPolygons | CanReadAirspace,
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

enum class RegisteredFormat {
    Unknown,
    Kml,
    Kmz,
    GeoJson,
    Gpx,
    Shp,
    OpenAir,
    Wkt,
    Csv
};

RegisteredFormat formatFromExtension(const QString &extension)
{
    const QString ext = extension.toLower();
    if (ext == QStringLiteral("kml")) {
        return RegisteredFormat::Kml;
    }
    if (ext == QStringLiteral("kmz")) {
        return RegisteredFormat::Kmz;
    }
    if (ext == QStringLiteral("geojson") || ext == QStringLiteral("json")) {
        return RegisteredFormat::GeoJson;
    }
    if (ext == QStringLiteral("gpx")) {
        return RegisteredFormat::Gpx;
    }
    if (ext == QStringLiteral("shp")) {
        return RegisteredFormat::Shp;
    }
    if (ext == QStringLiteral("txt") || ext == QStringLiteral("air")) {
        return RegisteredFormat::OpenAir;
    }
    if (ext == QStringLiteral("wkt")) {
        return RegisteredFormat::Wkt;
    }
    if (ext == QStringLiteral("csv")) {
        return RegisteredFormat::Csv;
    }
    return RegisteredFormat::Unknown;
}

QString kmzSaveUnsupportedMessage(const QString &geometryType)
{
    return QObject::tr("KMZ save is not supported for %1 in GeoFormatRegistry. Use .kml instead.")
        .arg(geometryType);
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

    // Check for ZIP (KMZ)
    if (data.startsWith("PK\x03\x04")) {
        result.extension = QStringLiteral("kmz");
        result.formatName = QStringLiteral("KMZ (Compressed KML)");
        result.confidence = 70;  // Could also be regular ZIP
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
    if (text.contains(QRegularExpression(QStringLiteral("^\\s*(POINT|LINESTRING|POLYGON|MULTIPOINT|MULTILINESTRING|MULTIPOLYGON)\\s*\\("),
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

const QList<FormatInfo> &supportedFormats()
{
    static const QList<FormatInfo> formats = buildNativeFormats();
    return formats;
}


bool isSupported(const QString &extension)
{
    const QString ext = extension.toLower();
    const QList<FormatInfo> &formats = supportedFormats();

    for (const FormatInfo &format : formats) {
        if (format.extensions.contains(ext)) {
            return true;
        }
    }

    return false;
}

FormatInfo formatInfo(const QString &extension)
{
    const QString ext = extension.toLower();
    const QList<FormatInfo> &formats = supportedFormats();

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
    const FormatInfo info = formatInfo(extension);
    return info.capabilities.testFlag(capability);
}

bool fileHasCapability(const QString &filePath, Capability capability)
{
    const FormatInfo info = formatInfoForFile(filePath);
    return info.capabilities.testFlag(capability);
}

QString readFileFilter()
{
    return readFileFilterList().join(QStringLiteral(";;"));
}

QString writeFileFilter()
{
    QStringList filters;

    const QList<FormatInfo> &formats = supportedFormats();
    for (const FormatInfo &format : formats) {
        if (format.capabilities.testFlag(CanWrite)) {
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

    const QList<FormatInfo> &formats = supportedFormats();
    for (const FormatInfo &format : formats) {
        if (format.capabilities.testFlag(CanRead)) {
            for (const QString &ext : format.extensions) {
                allExtensions.append(QStringLiteral("*.%1").arg(ext));
            }
        }
    }

    // Add "All Geo Files" filter first
    filters.append(QStringLiteral("All Geo Files (%1)").arg(allExtensions.join(' ')));

    // Add individual format filters
    for (const FormatInfo &format : formats) {
        if (format.capabilities.testFlag(CanRead)) {
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

    const QList<FormatInfo> &formats = supportedFormats();
    for (const FormatInfo &format : formats) {
        if (format.capabilities.testFlag(CanWrite)) {
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

    switch (formatFromExtension(fileInfo.suffix())) {
    case RegisteredFormat::Kml:
    case RegisteredFormat::Kmz:
        return KMLHelper::determineShapeType(filePath, errorString);
    case RegisteredFormat::GeoJson:
        return GeoJsonHelper::determineShapeType(filePath, errorString);
    case RegisteredFormat::Gpx:
        return GPXHelper::determineShapeType(filePath, errorString);
    case RegisteredFormat::Shp:
        return SHPHelper::determineShapeType(filePath, errorString);
    case RegisteredFormat::OpenAir: {
        const OpenAirParser::ParseResult result = OpenAirParser::parseFile(filePath);
        if (!result.success || result.airspaces.isEmpty()) {
            errorString = result.errorString.isEmpty()
                ? QObject::tr("No airspace geometry found in file")
                : result.errorString;
            return ShapeType::Error;
        }
        return ShapeType::Airspace;
    }
    default:
        break;
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

    switch (formatFromExtension(fileInfo.suffix())) {
    case RegisteredFormat::Kml:
    case RegisteredFormat::Kmz:
        return KMLHelper::getEntityCount(filePath, errorString);
    case RegisteredFormat::GeoJson:
        return GeoJsonHelper::getEntityCount(filePath, errorString);
    case RegisteredFormat::Gpx:
        return GPXHelper::getEntityCount(filePath, errorString);
    case RegisteredFormat::Shp:
        return SHPHelper::getEntityCount(filePath, errorString);
    default:
        break;
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

    const QString ext = fileInfo.suffix().toLower();
    const RegisteredFormat format = formatFromExtension(ext);

    // Lambda to apply filtering to a list of coordinate lists
    auto applyFilter = [filterMeters](QList<QList<QGeoCoordinate>> &coordLists, int minVertices) {
        if (filterMeters > 0) {
            for (QList<QGeoCoordinate> &coords : coordLists) {
                filterVertices(coords, filterMeters, minVertices);
            }
        }
    };

    auto loadWithPrimitiveReaders = [&](auto loadPointsFn,
                                        auto loadPolylinesFn,
                                        auto loadPolygonsFn,
                                        const QString &formatUsed) {
        QStringList errors;
        QString errorString;

        if (!loadPointsFn(filePath, result.points, errorString) && !errorString.isEmpty()) {
            errors.append(errorString);
        }

        errorString.clear();
        if (!loadPolylinesFn(filePath, result.polylines, errorString, filterMeters) && !errorString.isEmpty()) {
            errors.append(errorString);
        }

        errorString.clear();
        if (!loadPolygonsFn(filePath, result.polygons, errorString, filterMeters) && !errorString.isEmpty()) {
            errors.append(errorString);
        }

        result.formatUsed = formatUsed;
        result.success = result.totalFeatures() > 0;
        if (!result.success && !errors.isEmpty()) {
            result.errorString = errors.join(QStringLiteral("; "));
        }
        return result;
    };

    // Try native loaders first
    switch (format) {
    case RegisteredFormat::Kml:
    case RegisteredFormat::Kmz:
        // KMLHelper handles KMZ files transparently (decompresses internally)
        return loadWithPrimitiveReaders(KMLHelper::loadPointsFromFile,
                                        KMLHelper::loadPolylinesFromFile,
                                        KMLHelper::loadPolygonsFromFile,
                                        (format == RegisteredFormat::Kmz) ? QStringLiteral("KMZ") : QStringLiteral("KML"));
    case RegisteredFormat::GeoJson:
        return loadWithPrimitiveReaders(GeoJsonHelper::loadPointsFromFile,
                                        GeoJsonHelper::loadPolylinesFromFile,
                                        GeoJsonHelper::loadPolygonsFromFile,
                                        QStringLiteral("GeoJSON"));
    case RegisteredFormat::Gpx:
        return loadWithPrimitiveReaders(GPXHelper::loadPointsFromFile,
                                        GPXHelper::loadPolylinesFromFile,
                                        GPXHelper::loadPolygonsFromFile,
                                        QStringLiteral("GPX"));
    case RegisteredFormat::Shp:
        return loadWithPrimitiveReaders(SHPHelper::loadPointsFromFile,
                                        SHPHelper::loadPolylinesFromFile,
                                        SHPHelper::loadPolygonsFromFile,
                                        QStringLiteral("Shapefile"));
    case RegisteredFormat::OpenAir: {
        // Try OpenAir format
        const OpenAirParser::ParseResult airResult = OpenAirParser::parseFile(filePath);
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
        break;
    }
    case RegisteredFormat::Wkt: {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            if (file.size() > GeoFileIO::kMaxGeoFileSizeBytes) {
                result.errorString = QStringLiteral("WKT file exceeds maximum size limit");
                return result;
            }
            QString content = QString::fromUtf8(file.readAll());
            QString errorString;

            const QString type = WKTHelper::geometryType(content);
            if (type == QStringLiteral("POLYGON") || type == QStringLiteral("MULTIPOLYGON")) {
                QList<QGeoCoordinate> polygon;
                if (WKTHelper::parsePolygon(content, polygon, errorString)) {
                    result.polygons.append(polygon);
                }
            } else if (type == QStringLiteral("LINESTRING") || type == QStringLiteral("MULTILINESTRING")) {
                QList<QGeoCoordinate> line;
                if (WKTHelper::parseLineString(content, line, errorString)) {
                    result.polylines.append(line);
                }
            } else if (type == QStringLiteral("POINT") || type == QStringLiteral("MULTIPOINT")) {
                QGeoCoordinate point;
                if (WKTHelper::parsePoint(content, point, errorString)) {
                    result.points.append(point);
                }
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
        break;
    }
    case RegisteredFormat::Csv: {
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
    case RegisteredFormat::Unknown:
    default:
        break;
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

    const QString ext = fileInfo.suffix().toLower();
    switch (formatFromExtension(ext)) {
    case RegisteredFormat::Kml:
    case RegisteredFormat::Kmz:
        return KMLHelper::loadPolygonsWithHolesFromFile(filePath, polygons, errorString);
    case RegisteredFormat::GeoJson:
        return GeoJsonHelper::loadPolygonsWithHolesFromFile(filePath, polygons, errorString);
    case RegisteredFormat::Shp:
        return SHPHelper::loadPolygonsWithHolesFromFile(filePath, polygons, errorString);
    case RegisteredFormat::Gpx:
        errorString = QObject::tr("GPX format does not support polygons with holes");
        return false;
    default:
        break;
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
    const QString ext = fileInfo.suffix().toLower();
    const RegisteredFormat format = formatFromExtension(ext);

    QString errorString;

    switch (format) {
    case RegisteredFormat::Kmz:
        result.errorString = kmzSaveUnsupportedMessage(QObject::tr("points"));
        break;
    case RegisteredFormat::Kml:
        if (KMLHelper::savePointsToFile(filePath, points, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("KML");
        } else {
            result.errorString = errorString;
        }
        break;
    case RegisteredFormat::GeoJson:
        if (GeoJsonHelper::savePointsToFile(filePath, points, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("GeoJSON");
        } else {
            result.errorString = errorString;
        }
        break;
    case RegisteredFormat::Gpx:
        if (GPXHelper::savePointsToFile(filePath, points, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("GPX");
        } else {
            result.errorString = errorString;
        }
        break;
    case RegisteredFormat::Shp:
        if (SHPHelper::savePointsToFile(filePath, points, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("Shapefile");
        } else {
            result.errorString = errorString;
        }
        break;
    case RegisteredFormat::Wkt:
        if (WKTHelper::savePointsToFile(filePath, points, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("WKT");
        } else {
            result.errorString = errorString;
        }
        break;
    case RegisteredFormat::Csv:
        if (CSVHelper::savePointsToFile(filePath, points, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("CSV");
        } else {
            result.errorString = errorString;
        }
        break;
    default:
        result.errorString = QObject::tr("Unsupported format for point save: %1").arg(ext);
        break;
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
    for (qsizetype i = 0; i < polylines.size(); ++i) {
        if (!GeoUtilities::validateCoordinates(polylines[i], validationError)) {
            result.errorString = QObject::tr("Polyline %1: %2")
                .arg(static_cast<qlonglong>(i + 1))
                .arg(validationError);
            return result;
        }
    }

    QFileInfo fileInfo(filePath);
    const QString ext = fileInfo.suffix().toLower();
    const RegisteredFormat format = formatFromExtension(ext);

    QString errorString;

    switch (format) {
    case RegisteredFormat::Kmz:
        result.errorString = kmzSaveUnsupportedMessage(QObject::tr("polylines"));
        break;
    case RegisteredFormat::Kml:
        if (KMLHelper::savePolylinesToFile(filePath, polylines, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("KML");
        } else {
            result.errorString = errorString;
        }
        break;
    case RegisteredFormat::GeoJson:
        if (GeoJsonHelper::savePolylinesToFile(filePath, polylines, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("GeoJSON");
        } else {
            result.errorString = errorString;
        }
        break;
    case RegisteredFormat::Gpx:
        if (GPXHelper::savePolylinesToFile(filePath, polylines, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("GPX");
        } else {
            result.errorString = errorString;
        }
        break;
    case RegisteredFormat::Shp:
        if (SHPHelper::savePolylinesToFile(filePath, polylines, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("Shapefile");
        } else {
            result.errorString = errorString;
        }
        break;
    case RegisteredFormat::Wkt:
        if (WKTHelper::savePolylinesToFile(filePath, polylines, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("WKT");
        } else {
            result.errorString = errorString;
        }
        break;
    default:
        result.errorString = QObject::tr("Unsupported format for polyline save: %1").arg(ext);
        break;
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
    for (qsizetype i = 0; i < polygons.size(); ++i) {
        if (!GeoUtilities::validateCoordinates(polygons[i], validationError)) {
            result.errorString = QObject::tr("Polygon %1: %2")
                .arg(static_cast<qlonglong>(i + 1))
                .arg(validationError);
            return result;
        }
    }

    QFileInfo fileInfo(filePath);
    const QString ext = fileInfo.suffix().toLower();
    const RegisteredFormat format = formatFromExtension(ext);

    QString errorString;

    switch (format) {
    case RegisteredFormat::Kmz:
        result.errorString = kmzSaveUnsupportedMessage(QObject::tr("polygons"));
        break;
    case RegisteredFormat::Kml:
        if (KMLHelper::savePolygonsToFile(filePath, polygons, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("KML");
        } else {
            result.errorString = errorString;
        }
        break;
    case RegisteredFormat::GeoJson:
        if (GeoJsonHelper::savePolygonsToFile(filePath, polygons, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("GeoJSON");
        } else {
            result.errorString = errorString;
        }
        break;
    case RegisteredFormat::Gpx:
        if (GPXHelper::savePolygonsToFile(filePath, polygons, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("GPX");
        } else {
            result.errorString = errorString;
        }
        break;
    case RegisteredFormat::Shp:
        if (SHPHelper::savePolygonsToFile(filePath, polygons, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("Shapefile");
        } else {
            result.errorString = errorString;
        }
        break;
    case RegisteredFormat::Wkt:
        if (WKTHelper::savePolygonsToFile(filePath, polygons, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("WKT");
        } else {
            result.errorString = errorString;
        }
        break;
    default:
        result.errorString = QObject::tr("Unsupported format for polygon save: %1").arg(ext);
        break;
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
    for (qsizetype i = 0; i < polygons.size(); ++i) {
        // Validate outer ring
        QList<QGeoCoordinate> outerRing = polygons[i].perimeter();
        if (!GeoUtilities::validateCoordinates(outerRing, validationError)) {
            result.errorString = QObject::tr("Polygon %1 outer ring: %2")
                .arg(static_cast<qlonglong>(i + 1))
                .arg(validationError);
            return result;
        }
        // Validate holes
        for (qsizetype h = 0; h < polygons[i].holesCount(); ++h) {
            QList<QGeoCoordinate> hole = polygons[i].holePath(h);
            if (!GeoUtilities::validateCoordinates(hole, validationError)) {
                result.errorString = QObject::tr("Polygon %1 hole %2: %3")
                    .arg(static_cast<qlonglong>(i + 1))
                    .arg(static_cast<qlonglong>(h + 1))
                    .arg(validationError);
                return result;
            }
        }
    }

    QFileInfo fileInfo(filePath);
    const QString ext = fileInfo.suffix().toLower();
    const RegisteredFormat format = formatFromExtension(ext);

    QString errorString;

    switch (format) {
    case RegisteredFormat::Kmz:
        result.errorString = kmzSaveUnsupportedMessage(QObject::tr("polygons with holes"));
        break;
    case RegisteredFormat::Kml:
        if (KMLHelper::savePolygonsWithHolesToFile(filePath, polygons, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("KML");
        } else {
            result.errorString = errorString;
        }
        break;
    case RegisteredFormat::GeoJson:
        if (GeoJsonHelper::savePolygonsWithHolesToFile(filePath, polygons, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("GeoJSON");
        } else {
            result.errorString = errorString;
        }
        break;
    case RegisteredFormat::Shp:
        if (SHPHelper::savePolygonsWithHolesToFile(filePath, polygons, errorString)) {
            result.success = true;
            result.formatUsed = QStringLiteral("Shapefile");
        } else {
            result.errorString = errorString;
        }
        break;
    case RegisteredFormat::Gpx:
        result.errorString = QObject::tr("GPX format does not support polygons with holes");
        break;
    default:
        result.errorString = QObject::tr("Unsupported format for polygon with holes save: %1").arg(ext);
        break;
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
    const QString ext = fileInfo.suffix().toLower();

    if (formatFromExtension(ext) == RegisteredFormat::Gpx) {
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
    const QString ext = fileInfo.suffix().toLower();

    if (formatFromExtension(ext) == RegisteredFormat::Gpx) {
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

QList<ValidationResult> validateCapabilities()
{
    QList<ValidationResult> results;

    const QList<FormatInfo> &formats = supportedFormats();

    for (const FormatInfo &format : formats) {
        ValidationResult result;
        result.formatName = format.name;

        if (format.extensions.isEmpty()) {
            result.issues.append(QStringLiteral("Format has no registered extensions"));
        }

        if (format.capabilities == 0) {
            result.issues.append(QStringLiteral("Format has no declared capabilities"));
        }

        result.valid = result.issues.isEmpty();
        if (!result.valid) {
            qCWarning(GeoFormatRegistryLog) << "Format" << format.name << "has validation issues:" << result.issues;
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

} // namespace GeoFormatRegistry
