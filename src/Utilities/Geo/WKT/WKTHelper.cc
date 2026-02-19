#include "WKTHelper.h"
#include "GeoFileIO.h"
#include "GeoUtilities.h"

#include <QtCore/QRegularExpression>
#include <cmath>

Q_LOGGING_CATEGORY(WKTHelperLog, "Utilities.Geo.WKTHelper")

namespace {
    constexpr const char *kFormatName = "WKT";

    // Regex patterns for WKT geometry types
    static const QRegularExpression kGeometryTypeRx(
        QStringLiteral("^\\s*(POINT|MULTIPOINT|LINESTRING|MULTILINESTRING|POLYGON|MULTIPOLYGON)"
                       "\\s*(Z|M|ZM)?\\s*\\("),
        QRegularExpression::CaseInsensitiveOption);

    // Split coordinate string into individual coordinates
    QStringList splitCoordinates(const QString &coordStr)
    {
        return coordStr.split(QLatin1Char(','), Qt::SkipEmptyParts);
    }
} // anonymous namespace

namespace WKTHelper {

// ============================================================================
// Utility Functions
// ============================================================================

QString geometryType(const QString &wkt)
{
    const QRegularExpressionMatch match = kGeometryTypeRx.match(wkt);
    if (match.hasMatch()) {
        return match.captured(1).toUpper();
    }
    return QString();
}

bool hasAltitude(const QString &wkt)
{
    const QRegularExpressionMatch match = kGeometryTypeRx.match(wkt);
    if (match.hasMatch()) {
        const QString modifier = match.captured(2).toUpper();
        return modifier.contains(QLatin1Char('Z'));
    }
    return false;
}

} // namespace WKTHelper

// ============================================================================
// Private Helper Functions (file-local)
// ============================================================================
namespace {

bool parseCoordinateList(const QString &coordStr, QList<QGeoCoordinate> &coords,
                         bool hasZ, QString &errorString)
{
    coords.clear();
    const QStringList coordParts = splitCoordinates(coordStr);

    for (const QString &part : coordParts) {
        const QStringList values = part.trimmed().split(QRegularExpression(QStringLiteral("\\s+")));

        if (values.size() < 2) {
            errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),
                QStringLiteral("Invalid coordinate: %1").arg(part));
            return false;
        }

        bool okX = false, okY = false, okZ = false;
        const double x = values[0].toDouble(&okX);  // longitude
        const double y = values[1].toDouble(&okY);  // latitude

        if (!okX || !okY) {
            errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),
                QStringLiteral("Invalid coordinate values: %1").arg(part));
            return false;
        }

        QGeoCoordinate coord(y, x);  // Note: QGeoCoordinate takes lat, lon

        if (hasZ && values.size() >= 3) {
            const double z = values[2].toDouble(&okZ);
            if (okZ) {
                coord.setAltitude(z);
            }
        }

        coords.append(coord);
    }

    return true;
}

bool parseRing(const QString &ringStr, QList<QGeoCoordinate> &coords,
                          bool hasZ, QString &errorString)
{
    QString inner = ringStr.trimmed();

    // Remove surrounding parentheses if present
    if (inner.startsWith(QLatin1Char('(')) && inner.endsWith(QLatin1Char(')'))) {
        inner = inner.mid(1, inner.length() - 2);
    }

    if (!parseCoordinateList(inner, coords, hasZ, errorString)) {
        return false;
    }

    // Remove closing vertex if present
    GeoUtilities::removeClosingVertex(coords);

    return true;
}

QString coordToString(const QGeoCoordinate &coord, bool includeAltitude)
{
    if (includeAltitude && !std::isnan(coord.altitude())) {
        return QStringLiteral("%1 %2 %3")
            .arg(coord.longitude(), 0, 'f', 8)
            .arg(coord.latitude(), 0, 'f', 8)
            .arg(coord.altitude(), 0, 'f', 2);
    }
    return QStringLiteral("%1 %2")
        .arg(coord.longitude(), 0, 'f', 8)
        .arg(coord.latitude(), 0, 'f', 8);
}

QString coordListToString(const QList<QGeoCoordinate> &coords, bool includeAltitude, bool closeRing)
{
    QStringList parts;
    for (const auto &coord : coords) {
        parts.append(coordToString(coord, includeAltitude));
    }

    // Close the ring if needed (first == last for polygons)
    if (closeRing && !coords.isEmpty()) {
        parts.append(coordToString(coords.first(), includeAltitude));
    }

    return parts.join(QStringLiteral(", "));
}

} // anonymous namespace

namespace WKTHelper {

// ============================================================================
// Parsing Functions
// ============================================================================

bool parsePoint(const QString &wkt, QGeoCoordinate &coord, QString &errorString)
{
    static const QRegularExpression rx(
        QStringLiteral("^\\s*POINT\\s*(Z|M|ZM)?\\s*\\(\\s*([^)]+)\\s*\\)\\s*$"),
        QRegularExpression::CaseInsensitiveOption);

    const QRegularExpressionMatch match = rx.match(wkt);
    if (!match.hasMatch()) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QStringLiteral("Invalid POINT format"));
        return false;
    }

    const bool hasZ = match.captured(1).toUpper().contains(QLatin1Char('Z'));
    QList<QGeoCoordinate> coords;

    if (!parseCoordinateList(match.captured(2), coords, hasZ, errorString)) {
        return false;
    }

    if (coords.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QStringLiteral("No coordinates in POINT"));
        return false;
    }

    coord = coords.first();
    return true;
}

bool parseMultiPoint(const QString &wkt, QList<QGeoCoordinate> &points, QString &errorString)
{
    static const QRegularExpression rx(
        QStringLiteral("^\\s*MULTIPOINT\\s*(Z|M|ZM)?\\s*\\((.+)\\)\\s*$"),
        QRegularExpression::CaseInsensitiveOption);

    const QRegularExpressionMatch match = rx.match(wkt);
    if (!match.hasMatch()) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QStringLiteral("Invalid MULTIPOINT format"));
        return false;
    }

    const bool hasZ = match.captured(1).toUpper().contains(QLatin1Char('Z'));
    QString content = match.captured(2).trimmed();

    points.clear();

    // MULTIPOINT can be in two formats:
    // MULTIPOINT ((10 40), (40 30)) or MULTIPOINT (10 40, 40 30)
    // After outer regex strips MULTIPOINT (), content is "(10 40), (40 30)" or "10 40, 40 30"
    if (content.startsWith(QLatin1Char('('))) {
        // Format with nested parentheses
        static const QRegularExpression pointRx(QStringLiteral("\\(([^)]+)\\)"));
        QRegularExpressionMatchIterator it = pointRx.globalMatch(content);

        while (it.hasNext()) {
            const QRegularExpressionMatch pointMatch = it.next();
            QList<QGeoCoordinate> coords;
            if (!parseCoordinateList(pointMatch.captured(1), coords, hasZ, errorString)) {
                return false;
            }
            if (!coords.isEmpty()) {
                points.append(coords.first());
            }
        }
    } else {
        // Simple format without nested parentheses
        if (!parseCoordinateList(content, points, hasZ, errorString)) {
            return false;
        }
    }

    return true;
}

bool parseLineString(const QString &wkt, QList<QGeoCoordinate> &coords, QString &errorString)
{
    static const QRegularExpression rx(
        QStringLiteral("^\\s*LINESTRING\\s*(Z|M|ZM)?\\s*\\((.+)\\)\\s*$"),
        QRegularExpression::CaseInsensitiveOption);

    const QRegularExpressionMatch match = rx.match(wkt);
    if (!match.hasMatch()) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QStringLiteral("Invalid LINESTRING format"));
        return false;
    }

    const bool hasZ = match.captured(1).toUpper().contains(QLatin1Char('Z'));
    return parseCoordinateList(match.captured(2), coords, hasZ, errorString);
}

bool parseMultiLineString(const QString &wkt, QList<QList<QGeoCoordinate>> &polylines, QString &errorString)
{
    static const QRegularExpression rx(
        QStringLiteral("^\\s*MULTILINESTRING\\s*(Z|M|ZM)?\\s*\\((.+)\\)\\s*$"),
        QRegularExpression::CaseInsensitiveOption);

    const QRegularExpressionMatch match = rx.match(wkt);
    if (!match.hasMatch()) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QStringLiteral("Invalid MULTILINESTRING format"));
        return false;
    }

    const bool hasZ = match.captured(1).toUpper().contains(QLatin1Char('Z'));
    QString content = match.captured(2);

    polylines.clear();

    // Extract each linestring within parentheses
    static const QRegularExpression lineRx(QStringLiteral("\\(([^)]+)\\)"));
    QRegularExpressionMatchIterator it = lineRx.globalMatch(content);

    while (it.hasNext()) {
        const QRegularExpressionMatch lineMatch = it.next();
        QList<QGeoCoordinate> coords;
        if (!parseCoordinateList(lineMatch.captured(1), coords, hasZ, errorString)) {
            return false;
        }
        polylines.append(coords);
    }

    return true;
}

bool parsePolygon(const QString &wkt, QList<QGeoCoordinate> &vertices, QString &errorString)
{
    static const QRegularExpression rx(
        QStringLiteral("^\\s*POLYGON\\s*(Z|M|ZM)?\\s*\\(\\s*\\((.+?)\\)"),
        QRegularExpression::CaseInsensitiveOption);

    const QRegularExpressionMatch match = rx.match(wkt);
    if (!match.hasMatch()) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QStringLiteral("Invalid POLYGON format"));
        return false;
    }

    const bool hasZ = match.captured(1).toUpper().contains(QLatin1Char('Z'));
    return parseRing(match.captured(2), vertices, hasZ, errorString);
}

bool parsePolygonWithHoles(const QString &wkt, QGeoPolygon &polygon, QString &errorString)
{
    static const QRegularExpression rx(
        QStringLiteral("^\\s*POLYGON\\s*(Z|M|ZM)?\\s*\\((.+)\\)\\s*$"),
        QRegularExpression::CaseInsensitiveOption);

    const QRegularExpressionMatch match = rx.match(wkt);
    if (!match.hasMatch()) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QStringLiteral("Invalid POLYGON format"));
        return false;
    }

    const bool hasZ = match.captured(1).toUpper().contains(QLatin1Char('Z'));
    QString content = match.captured(2);

    // Extract all rings
    static const QRegularExpression ringRx(QStringLiteral("\\(([^)]+)\\)"));
    QRegularExpressionMatchIterator it = ringRx.globalMatch(content);

    QList<QList<QGeoCoordinate>> rings;
    while (it.hasNext()) {
        const QRegularExpressionMatch ringMatch = it.next();
        QList<QGeoCoordinate> coords;
        if (!parseRing(ringMatch.captured(1), coords, hasZ, errorString)) {
            return false;
        }
        rings.append(coords);
    }

    if (rings.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QStringLiteral("No rings in POLYGON"));
        return false;
    }

    // First ring is outer boundary
    polygon = QGeoPolygon(rings.first());

    // Remaining rings are holes
    for (int i = 1; i < rings.size(); ++i) {
        polygon.addHole(rings[i]);
    }

    return true;
}

bool parseMultiPolygon(const QString &wkt, QList<QList<QGeoCoordinate>> &polygons, QString &errorString)
{
    static const QRegularExpression rx(
        QStringLiteral("^\\s*MULTIPOLYGON\\s*(Z|M|ZM)?\\s*\\((.+)\\)\\s*$"),
        QRegularExpression::CaseInsensitiveOption);

    const QRegularExpressionMatch match = rx.match(wkt);
    if (!match.hasMatch()) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QStringLiteral("Invalid MULTIPOLYGON format"));
        return false;
    }

    const bool hasZ = match.captured(1).toUpper().contains(QLatin1Char('Z'));
    QString content = match.captured(2);

    polygons.clear();

    // Match each polygon: ((...))
    static const QRegularExpression polyRx(QStringLiteral("\\(\\s*\\(([^)]+)\\)"));
    QRegularExpressionMatchIterator it = polyRx.globalMatch(content);

    while (it.hasNext()) {
        const QRegularExpressionMatch polyMatch = it.next();
        QList<QGeoCoordinate> coords;
        if (!parseRing(polyMatch.captured(1), coords, hasZ, errorString)) {
            return false;
        }
        polygons.append(coords);
    }

    return true;
}

bool parseMultiPolygonWithHoles(const QString &wkt, QList<QGeoPolygon> &polygons, QString &errorString)
{
    static const QRegularExpression rx(
        QStringLiteral("^\\s*MULTIPOLYGON\\s*(Z|M|ZM)?\\s*\\((.+)\\)\\s*$"),
        QRegularExpression::CaseInsensitiveOption);

    const QRegularExpressionMatch match = rx.match(wkt);
    if (!match.hasMatch()) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QStringLiteral("Invalid MULTIPOLYGON format"));
        return false;
    }

    const bool hasZ = match.captured(1).toUpper().contains(QLatin1Char('Z'));
    QString content = match.captured(2);

    polygons.clear();

    // This is complex because we need to match nested parentheses
    // MULTIPOLYGON (((outer1), (hole1)), ((outer2)))
    int depth = 0;
    int polyStart = -1;

    for (int i = 0; i < content.length(); ++i) {
        const QChar c = content[i];
        if (c == QLatin1Char('(')) {
            if (depth == 0) {
                polyStart = i;
            }
            ++depth;
        } else if (c == QLatin1Char(')')) {
            --depth;
            if (depth == 0 && polyStart >= 0) {
                // Extract this polygon
                QString polyStr = content.mid(polyStart, i - polyStart + 1);
                QString fakeWkt = QStringLiteral("POLYGON %1%2")
                    .arg(hasZ ? QStringLiteral("Z ") : QString())
                    .arg(polyStr);

                QGeoPolygon poly;
                if (!parsePolygonWithHoles(fakeWkt, poly, errorString)) {
                    return false;
                }
                polygons.append(poly);
                polyStart = -1;
            }
        }
    }

    return true;
}

// ============================================================================
// Generation Functions
// ============================================================================

QString toWKTPoint(const QGeoCoordinate &coord, bool includeAltitude)
{
    const bool hasZ = includeAltitude && !std::isnan(coord.altitude());
    return QStringLiteral("POINT%1 (%2)")
        .arg(hasZ ? QStringLiteral(" Z") : QString())
        .arg(coordToString(coord, hasZ));
}

QString toWKTMultiPoint(const QList<QGeoCoordinate> &points, bool includeAltitude)
{
    QStringList parts;
    bool hasZ = false;

    if (includeAltitude) {
        for (const auto &p : points) {
            if (!std::isnan(p.altitude())) {
                hasZ = true;
                break;
            }
        }
    }

    for (const auto &p : points) {
        parts.append(QStringLiteral("(%1)").arg(coordToString(p, hasZ)));
    }

    return QStringLiteral("MULTIPOINT%1 (%2)")
        .arg(hasZ ? QStringLiteral(" Z") : QString())
        .arg(parts.join(QStringLiteral(", ")));
}

QString toWKTLineString(const QList<QGeoCoordinate> &coords, bool includeAltitude)
{
    bool hasZ = false;
    if (includeAltitude) {
        for (const auto &c : coords) {
            if (!std::isnan(c.altitude())) {
                hasZ = true;
                break;
            }
        }
    }

    return QStringLiteral("LINESTRING%1 (%2)")
        .arg(hasZ ? QStringLiteral(" Z") : QString())
        .arg(coordListToString(coords, hasZ, false));
}

QString toWKTMultiLineString(const QList<QList<QGeoCoordinate>> &polylines, bool includeAltitude)
{
    bool hasZ = false;
    if (includeAltitude) {
        for (const auto &line : polylines) {
            for (const auto &c : line) {
                if (!std::isnan(c.altitude())) {
                    hasZ = true;
                    break;
                }
            }
            if (hasZ) break;
        }
    }

    QStringList parts;
    for (const auto &line : polylines) {
        parts.append(QStringLiteral("(%1)").arg(coordListToString(line, hasZ, false)));
    }

    return QStringLiteral("MULTILINESTRING%1 (%2)")
        .arg(hasZ ? QStringLiteral(" Z") : QString())
        .arg(parts.join(QStringLiteral(", ")));
}

QString toWKTPolygon(const QList<QGeoCoordinate> &vertices, bool includeAltitude)
{
    bool hasZ = false;
    if (includeAltitude) {
        for (const auto &v : vertices) {
            if (!std::isnan(v.altitude())) {
                hasZ = true;
                break;
            }
        }
    }

    return QStringLiteral("POLYGON%1 ((%2))")
        .arg(hasZ ? QStringLiteral(" Z") : QString())
        .arg(coordListToString(vertices, hasZ, true));  // closeRing = true
}

QString toWKTPolygon(const QGeoPolygon &polygon, bool includeAltitude)
{
    bool hasZ = false;
    if (includeAltitude) {
        for (const auto &v : polygon.perimeter()) {
            if (!std::isnan(v.altitude())) {
                hasZ = true;
                break;
            }
        }
    }

    QStringList rings;
    rings.append(QStringLiteral("(%1)").arg(coordListToString(polygon.perimeter(), hasZ, true)));

    for (int i = 0; i < polygon.holesCount(); ++i) {
        rings.append(QStringLiteral("(%1)").arg(coordListToString(polygon.holePath(i), hasZ, true)));
    }

    return QStringLiteral("POLYGON%1 (%2)")
        .arg(hasZ ? QStringLiteral(" Z") : QString())
        .arg(rings.join(QStringLiteral(", ")));
}

QString toWKTMultiPolygon(const QList<QList<QGeoCoordinate>> &polygons, bool includeAltitude)
{
    bool hasZ = false;
    if (includeAltitude) {
        for (const auto &poly : polygons) {
            for (const auto &v : poly) {
                if (!std::isnan(v.altitude())) {
                    hasZ = true;
                    break;
                }
            }
            if (hasZ) break;
        }
    }

    QStringList parts;
    for (const auto &poly : polygons) {
        parts.append(QStringLiteral("((%1))").arg(coordListToString(poly, hasZ, true)));
    }

    return QStringLiteral("MULTIPOLYGON%1 (%2)")
        .arg(hasZ ? QStringLiteral(" Z") : QString())
        .arg(parts.join(QStringLiteral(", ")));
}

QString toWKTMultiPolygon(const QList<QGeoPolygon> &polygons, bool includeAltitude)
{
    bool hasZ = false;
    if (includeAltitude) {
        for (const auto &poly : polygons) {
            for (const auto &v : poly.perimeter()) {
                if (!std::isnan(v.altitude())) {
                    hasZ = true;
                    break;
                }
            }
            if (hasZ) break;
        }
    }

    QStringList polyParts;
    for (const auto &polygon : polygons) {
        QStringList rings;
        rings.append(QStringLiteral("(%1)").arg(coordListToString(polygon.perimeter(), hasZ, true)));

        for (int i = 0; i < polygon.holesCount(); ++i) {
            rings.append(QStringLiteral("(%1)").arg(coordListToString(polygon.holePath(i), hasZ, true)));
        }

        polyParts.append(QStringLiteral("(%1)").arg(rings.join(QStringLiteral(", "))));
    }

    return QStringLiteral("MULTIPOLYGON%1 (%2)")
        .arg(hasZ ? QStringLiteral(" Z") : QString())
        .arg(polyParts.join(QStringLiteral(", ")));
}

// ============================================================================
// File Load Functions
// ============================================================================

namespace {
    bool readWktFromFile(const QString &filePath, QString &wkt, QString &errorString)
    {
        const auto textResult = GeoFileIO::loadText(filePath, QString::fromLatin1(kFormatName));
        if (!textResult.success) {
            errorString = textResult.error;
            qCWarning(WKTHelperLog) << errorString;
            return false;
        }

        wkt = textResult.content.trimmed();
        if (wkt.isEmpty()) {
            errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),
                QObject::tr("File is empty"));
            qCWarning(WKTHelperLog) << errorString;
            return false;
        }

        return true;
    }
}

bool loadPointFromFile(const QString &filePath, QGeoCoordinate &coord, QString &errorString)
{
    QString wkt;
    if (!readWktFromFile(filePath, wkt, errorString)) {
        return false;
    }

    return parsePoint(wkt, coord, errorString);
}

bool loadPointsFromFile(const QString &filePath, QList<QGeoCoordinate> &points, QString &errorString)
{
    QString wkt;
    if (!readWktFromFile(filePath, wkt, errorString)) {
        return false;
    }

    const QString geomType = geometryType(wkt);
    if (geomType == QStringLiteral("POINT")) {
        QGeoCoordinate coord;
        if (parsePoint(wkt, coord, errorString)) {
            points.clear();
            points.append(coord);
            return true;
        }
        return false;
    } else if (geomType == QStringLiteral("MULTIPOINT")) {
        return parseMultiPoint(wkt, points, errorString);
    }

    errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QObject::tr("File does not contain point data"));
    return false;
}

bool loadPolylineFromFile(const QString &filePath, QList<QGeoCoordinate> &coords, QString &errorString)
{
    QString wkt;
    if (!readWktFromFile(filePath, wkt, errorString)) {
        return false;
    }

    return parseLineString(wkt, coords, errorString);
}

bool loadPolylinesFromFile(const QString &filePath, QList<QList<QGeoCoordinate>> &polylines,
                                       QString &errorString, bool loadAll)
{
    Q_UNUSED(loadAll)

    QString wkt;
    if (!readWktFromFile(filePath, wkt, errorString)) {
        return false;
    }

    const QString geomType = geometryType(wkt);
    if (geomType == QStringLiteral("LINESTRING")) {
        QList<QGeoCoordinate> coords;
        if (parseLineString(wkt, coords, errorString)) {
            polylines.clear();
            polylines.append(coords);
            return true;
        }
        return false;
    } else if (geomType == QStringLiteral("MULTILINESTRING")) {
        return parseMultiLineString(wkt, polylines, errorString);
    }

    errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QObject::tr("File does not contain polyline data"));
    return false;
}

bool loadPolygonFromFile(const QString &filePath, QList<QGeoCoordinate> &vertices, QString &errorString)
{
    QString wkt;
    if (!readWktFromFile(filePath, wkt, errorString)) {
        return false;
    }

    return parsePolygon(wkt, vertices, errorString);
}

bool loadPolygonsFromFile(const QString &filePath, QList<QList<QGeoCoordinate>> &polygons,
                                      QString &errorString, bool loadAll)
{
    Q_UNUSED(loadAll)

    QString wkt;
    if (!readWktFromFile(filePath, wkt, errorString)) {
        return false;
    }

    const QString geomType = geometryType(wkt);
    if (geomType == QStringLiteral("POLYGON")) {
        QList<QGeoCoordinate> vertices;
        if (parsePolygon(wkt, vertices, errorString)) {
            polygons.clear();
            polygons.append(vertices);
            return true;
        }
        return false;
    } else if (geomType == QStringLiteral("MULTIPOLYGON")) {
        return parseMultiPolygon(wkt, polygons, errorString);
    }

    errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QObject::tr("File does not contain polygon data"));
    return false;
}

// ============================================================================
// File Save Functions
// ============================================================================

namespace {
    bool writeWktToFile(const QString &filePath, const QString &wkt, QString &errorString)
    {
        return GeoFileIO::saveText(filePath, wkt + QLatin1Char('\n'),
                                   QString::fromLatin1(kFormatName), errorString);
    }

    bool validateCoordinatesForSave(const QList<QGeoCoordinate> &coords, QString &errorString)
    {
        for (int i = 0; i < coords.size(); ++i) {
            if (!GeoUtilities::isValidCoordinate(coords[i])) {
                errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),
                    QObject::tr("Invalid coordinate at index %1").arg(i));
                return false;
            }
        }
        return true;
    }
}

bool savePointToFile(const QString &filePath, const QGeoCoordinate &coord,
                                 QString &errorString, bool includeAltitude)
{
    if (!GeoUtilities::isValidCoordinate(coord)) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),QObject::tr("Invalid coordinate"));
        return false;
    }

    const QString wkt = toWKTPoint(coord, includeAltitude);
    return writeWktToFile(filePath, wkt, errorString);
}

bool savePointsToFile(const QString &filePath, const QList<QGeoCoordinate> &points,
                                  QString &errorString, bool includeAltitude)
{
    if (points.isEmpty()) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),QObject::tr("No points to save"));
        return false;
    }

    if (!validateCoordinatesForSave(points, errorString)) {
        return false;
    }

    const QString wkt = toWKTMultiPoint(points, includeAltitude);
    return writeWktToFile(filePath, wkt, errorString);
}

bool savePolylineToFile(const QString &filePath, const QList<QGeoCoordinate> &coords,
                                    QString &errorString, bool includeAltitude)
{
    if (coords.size() < GeoUtilities::kMinPolylineVertices) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),
            QObject::tr("Polyline requires at least %1 points").arg(GeoUtilities::kMinPolylineVertices));
        return false;
    }

    if (!validateCoordinatesForSave(coords, errorString)) {
        return false;
    }

    const QString wkt = toWKTLineString(coords, includeAltitude);
    return writeWktToFile(filePath, wkt, errorString);
}

bool savePolylinesToFile(const QString &filePath, const QList<QList<QGeoCoordinate>> &polylines,
                                     QString &errorString, bool includeAltitude)
{
    if (polylines.isEmpty()) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),QObject::tr("No polylines to save"));
        return false;
    }

    for (int i = 0; i < polylines.size(); ++i) {
        if (polylines[i].size() < GeoUtilities::kMinPolylineVertices) {
            errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),
                QObject::tr("Polyline %1 requires at least %2 points").arg(i + 1).arg(GeoUtilities::kMinPolylineVertices));
            return false;
        }
        if (!validateCoordinatesForSave(polylines[i], errorString)) {
            return false;
        }
    }

    const QString wkt = toWKTMultiLineString(polylines, includeAltitude);
    return writeWktToFile(filePath, wkt, errorString);
}

bool savePolygonToFile(const QString &filePath, const QList<QGeoCoordinate> &vertices,
                                   QString &errorString, bool includeAltitude)
{
    if (vertices.size() < GeoUtilities::kMinPolygonVertices) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),
            QObject::tr("Polygon requires at least %1 vertices").arg(GeoUtilities::kMinPolygonVertices));
        return false;
    }

    if (!validateCoordinatesForSave(vertices, errorString)) {
        return false;
    }

    const QString wkt = toWKTPolygon(vertices, includeAltitude);
    return writeWktToFile(filePath, wkt, errorString);
}

bool savePolygonsToFile(const QString &filePath, const QList<QList<QGeoCoordinate>> &polygons,
                                    QString &errorString, bool includeAltitude)
{
    if (polygons.isEmpty()) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),QObject::tr("No polygons to save"));
        return false;
    }

    for (int i = 0; i < polygons.size(); ++i) {
        if (polygons[i].size() < GeoUtilities::kMinPolygonVertices) {
            errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),
                QObject::tr("Polygon %1 requires at least %2 vertices").arg(i + 1).arg(GeoUtilities::kMinPolygonVertices));
            return false;
        }
        if (!validateCoordinatesForSave(polygons[i], errorString)) {
            return false;
        }
    }

    const QString wkt = toWKTMultiPolygon(polygons, includeAltitude);
    return writeWktToFile(filePath, wkt, errorString);
}

} // namespace WKTHelper
