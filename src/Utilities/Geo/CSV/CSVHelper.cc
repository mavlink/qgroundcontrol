#include "CSVHelper.h"
#include "GeoFileIO.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QRegularExpression>
#include <QtPositioning/QGeoPolygon>

QGC_LOGGING_CATEGORY(CSVHelperLog, "Utilities.Geo.CSVHelper")

namespace CSVHelper
{

constexpr const char *kFormatName = "CSV";

namespace {

QStringList parseLine(const QString &line, QChar delimiter)
{
    QStringList fields;
    QString field;
    bool inQuotes = false;

    for (int i = 0; i < line.length(); ++i) {
        QChar c = line[i];

        if (c == '"') {
            if (inQuotes && i + 1 < line.length() && line[i + 1] == '"') {
                // Escaped quote
                field += '"';
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (c == delimiter && !inQuotes) {
            fields.append(field.trimmed());
            field.clear();
        } else {
            field += c;
        }
    }
    fields.append(field.trimmed());

    return fields;
}

bool isLatitudeHeader(const QString &header)
{
    QString h = header.toLower().trimmed();
    return h == "lat" || h == "latitude" || h == "y";
}

bool isLongitudeHeader(const QString &header)
{
    QString h = header.toLower().trimmed();
    return h == "lon" || h == "lng" || h == "longitude" || h == "x";
}

bool isAltitudeHeader(const QString &header)
{
    QString h = header.toLower().trimmed();
    return h == "alt" || h == "altitude" || h == "elevation" || h == "z" || h == "elev";
}

bool isNameHeader(const QString &header)
{
    QString h = header.toLower().trimmed();
    return h == "name" || h == "label" || h == "id" || h == "title" || h == "description";
}

bool isSequenceHeader(const QString &header)
{
    QString h = header.toLower().trimmed();
    return h == "seq" || h == "sequence" || h == "order" || h == "index" || h == "point_order";
}

bool isLineIdHeader(const QString &header)
{
    QString h = header.toLower().trimmed();
    return h == "line_id" || h == "lineid" || h == "polyline_id" || h == "path_id" || h == "track_id" || h == "group";
}

} // anonymous namespace

QChar detectDelimiter(const QString &firstLine)
{
    // Count occurrences of common delimiters
    int commas = firstLine.count(',');
    int semicolons = firstLine.count(';');
    int tabs = firstLine.count('\t');

    if (tabs > commas && tabs > semicolons) {
        return '\t';
    }
    if (semicolons > commas) {
        return ';';
    }
    return ',';
}

void detectColumns(const QStringList &headers, ParseOptions &options)
{
    for (int i = 0; i < headers.count(); ++i) {
        const QString &header = headers[i];

        if (options.latColumn < 0 && isLatitudeHeader(header)) {
            options.latColumn = i;
        } else if (options.lonColumn < 0 && isLongitudeHeader(header)) {
            options.lonColumn = i;
        } else if (options.altColumn < 0 && isAltitudeHeader(header)) {
            options.altColumn = i;
        } else if (options.nameColumn < 0 && isNameHeader(header)) {
            options.nameColumn = i;
        } else if (options.sequenceColumn < 0 && isSequenceHeader(header)) {
            options.sequenceColumn = i;
        } else if (options.lineIdColumn < 0 && isLineIdHeader(header)) {
            options.lineIdColumn = i;
        }
    }
}

LoadResult loadPointsFromString(const QString &content, const ParseOptions &options)
{
    LoadResult result;
    ParseOptions opts = options;

    QStringList lines = content.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
    if (lines.isEmpty()) {
        result.errorString = GeoFileIO::formatLoadError(QLatin1String(kFormatName), QObject::tr("File is empty"));
        return result;
    }

    // Detect delimiter if not specified
    QChar delimiter = opts.delimiter;
    if (delimiter == ',') {
        delimiter = detectDelimiter(lines.first());
    }

    int startLine = 0;

    // Process header row
    if (opts.hasHeader && !lines.isEmpty()) {
        QStringList headers = parseLine(lines.first(), delimiter);
        detectColumns(headers, opts);
        startLine = 1;
    }

    // If columns still not detected, try common patterns
    if (opts.latColumn < 0 || opts.lonColumn < 0) {
        // Assume lat,lon order or lon,lat based on first data row
        if (startLine < lines.count()) {
            QStringList firstRow = parseLine(lines[startLine], delimiter);
            if (firstRow.count() >= 2) {
                // Default to lat,lon (column 0, 1)
                opts.latColumn = 0;
                opts.lonColumn = 1;
                if (firstRow.count() >= 3) {
                    opts.altColumn = 2;
                }
            }
        }
    }

    if (opts.latColumn < 0 || opts.lonColumn < 0) {
        result.errorString = GeoFileIO::formatLoadError(QLatin1String(kFormatName), QObject::tr("Cannot detect latitude/longitude columns"));
        return result;
    }

    // Parse data rows
    for (int i = startLine; i < lines.count(); ++i) {
        QStringList fields = parseLine(lines[i], delimiter);

        if (fields.count() <= qMax(opts.latColumn, opts.lonColumn)) {
            qCWarning(CSVHelperLog) << "Skipping row" << i << "- insufficient columns";
            continue;
        }

        bool latOk, lonOk;
        double lat = fields[opts.latColumn].toDouble(&latOk);
        double lon = fields[opts.lonColumn].toDouble(&lonOk);

        if (!latOk || !lonOk) {
            qCWarning(CSVHelperLog) << "Skipping row" << i << "- invalid coordinates";
            continue;
        }

        // Validate coordinate ranges
        if (lat < -90 || lat > 90 || lon < -180 || lon > 180) {
            qCWarning(CSVHelperLog) << "Skipping row" << i << "- coordinates out of range";
            continue;
        }

        QGeoCoordinate coord(lat, lon);

        // Parse altitude if available
        if (opts.altColumn >= 0 && opts.altColumn < fields.count()) {
            bool altOk;
            double alt = fields[opts.altColumn].toDouble(&altOk);
            if (altOk) {
                coord.setAltitude(alt);
            }
        }

        result.points.append(coord);

        // Parse name if available
        if (opts.nameColumn >= 0 && opts.nameColumn < fields.count()) {
            result.names.append(fields[opts.nameColumn]);
        } else {
            result.names.append(QString());
        }
    }

    if (result.points.isEmpty()) {
        result.errorString = GeoFileIO::formatNoEntitiesError(QLatin1String(kFormatName), QObject::tr("coordinates"));
        return result;
    }

    result.success = true;
    qCDebug(CSVHelperLog) << "Loaded" << result.points.count() << "points from CSV";

    return result;
}

LoadResult loadPointsFromFile(const QString &filePath, const ParseOptions &options)
{
    LoadResult result;

    const GeoFileIO::TextResult textResult = GeoFileIO::loadText(filePath, QLatin1String(kFormatName));
    if (!textResult.success) {
        result.errorString = textResult.error;
        return result;
    }

    return loadPointsFromString(textResult.content, options);
}

bool loadPointsFromFile(const QString &filePath, QList<QGeoCoordinate> &points, QString &errorString)
{
    LoadResult result = loadPointsFromFile(filePath, ParseOptions());
    points = result.points;
    errorString = result.errorString;
    return result.success;
}

bool savePointsToFile(const QString &filePath, const QList<QGeoCoordinate> &points, QString &errorString)
{
    return savePointsToFile(filePath, points, QStringList(), errorString);
}

bool savePointsToFile(const QString &filePath, const QList<QGeoCoordinate> &points,
                      const QStringList &names, QString &errorString)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        errorString = GeoFileIO::formatSaveError(QLatin1String(kFormatName), file.errorString());
        return false;
    }

    QTextStream out(&file);

    // Determine if we have altitudes
    bool hasAltitude = false;
    for (const QGeoCoordinate &coord : points) {
        if (!qIsNaN(coord.altitude())) {
            hasAltitude = true;
            break;
        }
    }

    // Determine if we have names
    bool hasNames = !names.isEmpty();
    for (const QString &name : names) {
        if (!name.isEmpty()) {
            hasNames = true;
            break;
        }
    }

    // Write header
    out << "latitude,longitude";
    if (hasAltitude) {
        out << ",altitude";
    }
    if (hasNames) {
        out << ",name";
    }
    out << "\n";

    // Write data rows
    for (int i = 0; i < points.count(); ++i) {
        const QGeoCoordinate &coord = points[i];

        out << QString::number(coord.latitude(), 'f', 8) << ","
            << QString::number(coord.longitude(), 'f', 8);

        if (hasAltitude) {
            if (!qIsNaN(coord.altitude())) {
                out << "," << QString::number(coord.altitude(), 'f', 2);
            } else {
                out << ",";
            }
        }

        if (hasNames) {
            QString name = (i < names.count()) ? names[i] : QString();
            // Quote names containing commas or quotes
            if (name.contains(',') || name.contains('"')) {
                name = "\"" + name.replace("\"", "\"\"") + "\"";
            }
            out << "," << name;
        }

        out << "\n";
    }

    file.close();
    qCDebug(CSVHelperLog) << "Saved" << points.count() << "points to" << filePath;

    return true;
}

bool loadPolylineFromFile(const QString &filePath, QList<QGeoCoordinate> &coords, QString &errorString)
{
    QList<QList<QGeoCoordinate>> polylines;
    if (!loadPolylinesFromFile(filePath, polylines, errorString)) {
        return false;
    }
    if (polylines.isEmpty()) {
        errorString = GeoFileIO::formatNoEntitiesError(QLatin1String(kFormatName), QObject::tr("polylines"));
        return false;
    }
    coords = polylines.first();
    return true;
}

bool loadPolylinesFromFile(const QString &filePath, QList<QList<QGeoCoordinate>> &polylines, QString &errorString)
{
    const GeoFileIO::TextResult textResult = GeoFileIO::loadText(filePath, QLatin1String(kFormatName));
    if (!textResult.success) {
        errorString = textResult.error;
        return false;
    }

    QStringList lines = textResult.content.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
    if (lines.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QLatin1String(kFormatName), QObject::tr("File is empty"));
        return false;
    }

    ParseOptions opts;
    QChar delimiter = detectDelimiter(lines.first());

    int startLine = 0;
    if (opts.hasHeader && !lines.isEmpty()) {
        QStringList headers = parseLine(lines.first(), delimiter);
        detectColumns(headers, opts);
        startLine = 1;
    }

    if (opts.latColumn < 0 || opts.lonColumn < 0) {
        // Default to lat,lon order
        opts.latColumn = 0;
        opts.lonColumn = 1;
    }

    // Structure to hold points with sequence and line ID
    struct PointData {
        QGeoCoordinate coord;
        int sequence = 0;
        QString lineId;
    };

    QList<PointData> allPoints;

    for (int i = startLine; i < lines.count(); ++i) {
        QStringList fields = parseLine(lines[i], delimiter);

        if (fields.count() <= qMax(opts.latColumn, opts.lonColumn)) {
            continue;
        }

        bool latOk, lonOk;
        double lat = fields[opts.latColumn].toDouble(&latOk);
        double lon = fields[opts.lonColumn].toDouble(&lonOk);

        if (!latOk || !lonOk || lat < -90 || lat > 90 || lon < -180 || lon > 180) {
            continue;
        }

        PointData pd;
        pd.coord = QGeoCoordinate(lat, lon);

        if (opts.altColumn >= 0 && opts.altColumn < fields.count()) {
            bool altOk;
            double alt = fields[opts.altColumn].toDouble(&altOk);
            if (altOk) {
                pd.coord.setAltitude(alt);
            }
        }

        if (opts.sequenceColumn >= 0 && opts.sequenceColumn < fields.count()) {
            pd.sequence = fields[opts.sequenceColumn].toInt();
        } else {
            pd.sequence = i - startLine;  // Use row number as sequence
        }

        if (opts.lineIdColumn >= 0 && opts.lineIdColumn < fields.count()) {
            pd.lineId = fields[opts.lineIdColumn];
        }

        allPoints.append(pd);
    }

    if (allPoints.isEmpty()) {
        errorString = GeoFileIO::formatNoEntitiesError(QLatin1String(kFormatName), QObject::tr("coordinates"));
        return false;
    }

    // Group by line ID and sort by sequence
    QMap<QString, QList<PointData>> lineGroups;
    for (const PointData &pd : allPoints) {
        lineGroups[pd.lineId].append(pd);
    }

    for (auto it = lineGroups.begin(); it != lineGroups.end(); ++it) {
        QList<PointData> &group = it.value();
        std::sort(group.begin(), group.end(), [](const PointData &a, const PointData &b) {
            return a.sequence < b.sequence;
        });

        QList<QGeoCoordinate> polyline;
        for (const PointData &pd : group) {
            polyline.append(pd.coord);
        }

        if (polyline.count() >= 2) {
            polylines.append(polyline);
        }
    }

    if (polylines.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QLatin1String(kFormatName), QObject::tr("No valid polylines found (need at least 2 points per line)"));
        return false;
    }

    qCDebug(CSVHelperLog) << "Loaded" << polylines.count() << "polylines from CSV";
    return true;
}

bool savePolylineToFile(const QString &filePath, const QList<QGeoCoordinate> &coords, QString &errorString)
{
    QList<QList<QGeoCoordinate>> polylines;
    polylines.append(coords);
    return savePolylinesToFile(filePath, polylines, errorString);
}

bool savePolylinesToFile(const QString &filePath, const QList<QList<QGeoCoordinate>> &polylines, QString &errorString)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        errorString = GeoFileIO::formatSaveError(QLatin1String(kFormatName), file.errorString());
        return false;
    }

    QTextStream out(&file);

    // Determine if we have altitudes
    bool hasAltitude = false;
    for (const QList<QGeoCoordinate> &polyline : polylines) {
        for (const QGeoCoordinate &coord : polyline) {
            if (!qIsNaN(coord.altitude())) {
                hasAltitude = true;
                break;
            }
        }
        if (hasAltitude) break;
    }

    // Write header
    out << "line_id,sequence,latitude,longitude";
    if (hasAltitude) {
        out << ",altitude";
    }
    out << "\n";

    // Write data rows
    for (int lineIdx = 0; lineIdx < polylines.count(); ++lineIdx) {
        const QList<QGeoCoordinate> &polyline = polylines[lineIdx];

        for (int seq = 0; seq < polyline.count(); ++seq) {
            const QGeoCoordinate &coord = polyline[seq];

            out << lineIdx << ","
                << seq << ","
                << QString::number(coord.latitude(), 'f', 8) << ","
                << QString::number(coord.longitude(), 'f', 8);

            if (hasAltitude) {
                if (!qIsNaN(coord.altitude())) {
                    out << "," << QString::number(coord.altitude(), 'f', 2);
                } else {
                    out << ",";
                }
            }

            out << "\n";
        }
    }

    file.close();

    int totalPoints = 0;
    for (const QList<QGeoCoordinate> &p : polylines) {
        totalPoints += p.count();
    }
    qCDebug(CSVHelperLog) << "Saved" << polylines.count() << "polylines (" << totalPoints << "points) to" << filePath;

    return true;
}

bool loadPolygonFromFile(const QString &filePath, QList<QGeoCoordinate> &vertices, QString &errorString)
{
    Q_UNUSED(filePath);
    Q_UNUSED(vertices);
    errorString = GeoFileIO::formatLoadError(QLatin1String(kFormatName),
        QObject::tr("CSV format does not support polygon geometries. Use KML, GeoJSON, Shapefile, or GPX instead."));
    return false;
}

bool loadPolygonsFromFile(const QString &filePath, QList<QList<QGeoCoordinate>> &polygons, QString &errorString)
{
    Q_UNUSED(filePath);
    Q_UNUSED(polygons);
    errorString = GeoFileIO::formatLoadError(QLatin1String(kFormatName),
        QObject::tr("CSV format does not support polygon geometries. Use KML, GeoJSON, Shapefile, or GPX instead."));
    return false;
}

bool loadPolygonWithHolesFromFile(const QString &filePath, QGeoPolygon &polygon, QString &errorString)
{
    Q_UNUSED(filePath);
    Q_UNUSED(polygon);
    errorString = GeoFileIO::formatLoadError(QLatin1String(kFormatName),
        QObject::tr("CSV format does not support polygons with holes. Use KML, GeoJSON, or Shapefile instead."));
    return false;
}

bool loadPolygonsWithHolesFromFile(const QString &filePath, QList<QGeoPolygon> &polygons, QString &errorString)
{
    Q_UNUSED(filePath);
    Q_UNUSED(polygons);
    errorString = GeoFileIO::formatLoadError(QLatin1String(kFormatName),
        QObject::tr("CSV format does not support polygons with holes. Use KML, GeoJSON, or Shapefile instead."));
    return false;
}

} // namespace CSVHelper
