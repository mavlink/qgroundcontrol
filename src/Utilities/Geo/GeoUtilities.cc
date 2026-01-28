#include "GeoUtilities.h"
#include "QGCGeo.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QObject>
#include <QtCore/QRegularExpression>
#include <QtGui/QVector3D>
#include <algorithm>
#include <cmath>
#include <random>

QGC_LOGGING_CATEGORY(GeoUtilitiesLog, "Utilities.Geo.GeoUtilities")

namespace GeoUtilities
{

// ============================================================================
// Bounding Box Utilities
// ============================================================================

QGeoRectangle boundingBox(const QList<QGeoCoordinate> &coords)
{
    if (coords.isEmpty()) {
        return QGeoRectangle();
    }

    QGeoRectangle box(coords.first(), coords.first());
    for (int i = 1; i < coords.size(); ++i) {
        expandBoundingBox(box, coords[i]);
    }
    return box;
}

QGeoRectangle boundingBox(const QList<QList<QGeoCoordinate>> &coordLists)
{
    QGeoRectangle box;
    for (const auto &coords : coordLists) {
        for (const auto &coord : coords) {
            expandBoundingBox(box, coord);
        }
    }
    return box;
}

void expandBoundingBox(QGeoRectangle &box, const QGeoCoordinate &coord)
{
    if (!coord.isValid()) {
        return;
    }

    if (!box.isValid()) {
        box = QGeoRectangle(coord, coord);
        return;
    }

    // QGeoRectangle::extendRectangle is available in Qt 6
    box.extendRectangle(coord);
}

void expandBoundingBox(QGeoRectangle &box, const QGeoRectangle &other)
{
    if (!other.isValid()) {
        return;
    }

    if (!box.isValid()) {
        box = other;
        return;
    }

    box = box.united(other);
}

// ============================================================================
// Coordinate Normalization
// ============================================================================

double normalizeLongitude(double longitude)
{
    // Wrap to [-180, 180]
    while (longitude > 180.0) {
        longitude -= 360.0;
    }
    while (longitude < -180.0) {
        longitude += 360.0;
    }
    return longitude;
}

double normalizeLatitude(double latitude)
{
    // Clamp to [-90, 90] - latitude cannot wrap
    if (latitude > 90.0) {
        return 90.0;
    }
    if (latitude < -90.0) {
        return -90.0;
    }
    return latitude;
}

QGeoCoordinate normalizeCoordinate(const QGeoCoordinate &coord)
{
    const double lat = coord.latitude();
    const double lon = coord.longitude();

    // Only skip if coordinate has NaN values (truly uninitialized)
    if (std::isnan(lat) || std::isnan(lon)) {
        return coord;
    }

    QGeoCoordinate normalized(
        normalizeLatitude(lat),
        normalizeLongitude(lon)
    );

    if (!std::isnan(coord.altitude())) {
        normalized.setAltitude(coord.altitude());
    }

    return normalized;
}

void normalizeCoordinates(QList<QGeoCoordinate> &coords)
{
    for (int i = 0; i < coords.size(); ++i) {
        coords[i] = normalizeCoordinate(coords[i]);
    }
}

void validateAndNormalizeCoordinates(QList<QGeoCoordinate> &coords,
                                     const ValidationWarningCallback &warningCallback)
{
    for (int i = 0; i < coords.size(); ++i) {
        QGeoCoordinate &coord = coords[i];
        const double lat = coord.latitude();
        const double lon = coord.longitude();

        bool needsNormalization = false;

        if (!std::isnan(lat) && (lat < -90.0 || lat > 90.0)) {
            if (warningCallback) {
                warningCallback(i, QStringLiteral("latitude"), lat);
            }
            needsNormalization = true;
        }

        if (!std::isnan(lon) && (lon < -180.0 || lon > 180.0)) {
            if (warningCallback) {
                warningCallback(i, QStringLiteral("longitude"), lon);
            }
            needsNormalization = true;
        }

        if (needsNormalization) {
            coord = normalizeCoordinate(coord);
        }

        // Check altitude (warn but don't modify)
        if (!std::isnan(coord.altitude()) && !isValidAltitude(coord.altitude())) {
            if (warningCallback) {
                warningCallback(i, QStringLiteral("altitude"), coord.altitude());
            }
        }
    }
}

bool removeClosingVertex(QList<QGeoCoordinate> &coords)
{
    if (coords.size() < 4) {
        return false;  // Need at least 4 points to have a meaningful closed polygon
    }

    const QGeoCoordinate &first = coords.first();
    const QGeoCoordinate &last = coords.last();

    // Check if first and last are the same (exact match or within threshold)
    if (first.latitude() == last.latitude() && first.longitude() == last.longitude()) {
        coords.removeLast();
        return true;
    }

    // Also check fuzzy compare for floating point precision issues
    if (qFuzzyCompare(first.latitude(), last.latitude()) &&
        qFuzzyCompare(first.longitude(), last.longitude())) {
        coords.removeLast();
        return true;
    }

    return false;
}

bool ensureClosingVertex(QList<QGeoCoordinate> &coords, double thresholdMeters)
{
    if (coords.size() < 3) {
        return false;  // Not a valid polygon
    }

    const QGeoCoordinate &first = coords.first();
    const QGeoCoordinate &last = coords.last();

    // Check if already closed (exact match)
    if (first.latitude() == last.latitude() && first.longitude() == last.longitude()) {
        return false;
    }

    // Check fuzzy compare for floating point precision
    if (qFuzzyCompare(first.latitude(), last.latitude()) &&
        qFuzzyCompare(first.longitude(), last.longitude())) {
        return false;
    }

    // Check distance threshold
    if (first.distanceTo(last) < thresholdMeters) {
        return false;  // Close enough, considered already closed
    }

    // Add closing vertex
    coords.append(first);
    return true;
}

bool ensureClockwise(QList<QGeoCoordinate> &coords)
{
    if (coords.size() < 3) {
        return false;
    }

    if (!isClockwise(coords)) {
        reverseVertices(coords);
        return true;
    }

    return false;
}

// ============================================================================
// Altitude Validation
// ============================================================================

bool isValidAltitude(double altitude, double minAltitude, double maxAltitude)
{
    if (std::isnan(altitude)) {
        return true;  // No altitude is valid
    }
    return altitude >= minAltitude && altitude <= maxAltitude;
}

bool isValidAltitude(const QGeoCoordinate &coord, double minAltitude, double maxAltitude)
{
    return isValidAltitude(coord.altitude(), minAltitude, maxAltitude);
}

bool validateAltitudes(const QList<QGeoCoordinate> &coords, QString &errorString,
                       double minAltitude, double maxAltitude)
{
    for (int i = 0; i < coords.size(); ++i) {
        const double alt = coords[i].altitude();
        if (!std::isnan(alt) && (alt < minAltitude || alt > maxAltitude)) {
            errorString = QObject::tr("Altitude %1m at index %2 is outside valid range [%3, %4]")
                .arg(alt, 0, 'f', 1)
                .arg(i)
                .arg(minAltitude, 0, 'f', 0)
                .arg(maxAltitude, 0, 'f', 0);
            return false;
        }
    }
    return true;
}

// ============================================================================
// Polygon Validation
// ============================================================================

namespace {
    // Check if two line segments intersect (2D)
    // Segments: (p1, p2) and (p3, p4)
    bool segmentsIntersect(const QGeoCoordinate &p1, const QGeoCoordinate &p2,
                           const QGeoCoordinate &p3, const QGeoCoordinate &p4)
    {
        // Using cross product method
        auto cross = [](double ax, double ay, double bx, double by) {
            return ax * by - ay * bx;
        };

        const double x1 = p1.longitude(), y1 = p1.latitude();
        const double x2 = p2.longitude(), y2 = p2.latitude();
        const double x3 = p3.longitude(), y3 = p3.latitude();
        const double x4 = p4.longitude(), y4 = p4.latitude();

        const double d1 = cross(x4 - x3, y4 - y3, x1 - x3, y1 - y3);
        const double d2 = cross(x4 - x3, y4 - y3, x2 - x3, y2 - y3);
        const double d3 = cross(x2 - x1, y2 - y1, x3 - x1, y3 - y1);
        const double d4 = cross(x2 - x1, y2 - y1, x4 - x1, y4 - y1);

        // Check if segments straddle each other
        if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) &&
            ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0))) {
            return true;
        }

        // Check for collinear cases (endpoints on the other segment)
        constexpr double epsilon = 1e-10;
        if (std::abs(d1) < epsilon || std::abs(d2) < epsilon ||
            std::abs(d3) < epsilon || std::abs(d4) < epsilon) {
            // Collinear - check if segments overlap
            auto onSegment = [](double px, double py, double qx, double qy, double rx, double ry) {
                return qx <= std::max(px, rx) && qx >= std::min(px, rx) &&
                       qy <= std::max(py, ry) && qy >= std::min(py, ry);
            };

            if (std::abs(d1) < epsilon && onSegment(x3, y3, x1, y1, x4, y4)) return true;
            if (std::abs(d2) < epsilon && onSegment(x3, y3, x2, y2, x4, y4)) return true;
            if (std::abs(d3) < epsilon && onSegment(x1, y1, x3, y3, x2, y2)) return true;
            if (std::abs(d4) < epsilon && onSegment(x1, y1, x4, y4, x2, y2)) return true;
        }

        return false;
    }
}

bool isSelfIntersecting(const QList<QGeoCoordinate> &vertices)
{
    const int n = static_cast<int>(vertices.size());
    if (n < 4) {
        return false;  // Triangle or less cannot self-intersect
    }

    // Check each pair of non-adjacent edges
    for (int i = 0; i < n; ++i) {
        const int i2 = (i + 1) % n;

        for (int j = i + 2; j < n; ++j) {
            // Skip adjacent edges
            if (i == 0 && j == n - 1) {
                continue;  // First and last edge share a vertex
            }

            const int j2 = (j + 1) % n;

            if (segmentsIntersect(vertices[i], vertices[i2], vertices[j], vertices[j2])) {
                return true;
            }
        }
    }

    return false;
}

bool isClockwise(const QList<QGeoCoordinate> &vertices)
{
    if (vertices.size() < 3) {
        return true;  // Degenerate
    }

    // Calculate signed area using Shoelace formula
    double sum = 0.0;
    const int n = static_cast<int>(vertices.size());

    for (int i = 0; i < n; ++i) {
        const int j = (i + 1) % n;
        sum += (vertices[j].longitude() - vertices[i].longitude()) *
               (vertices[j].latitude() + vertices[i].latitude());
    }

    // Positive sum = clockwise (in lat/lon coordinate system)
    return sum > 0.0;
}

void reverseVertices(QList<QGeoCoordinate> &vertices)
{
    std::reverse(vertices.begin(), vertices.end());
}

bool validatePolygon(const QList<QGeoCoordinate> &vertices, QString &errorString)
{
    if (vertices.size() < 3) {
        errorString = QObject::tr("Polygon must have at least 3 vertices, found %1").arg(vertices.size());
        return false;
    }

    // Check for invalid coordinates
    for (int i = 0; i < vertices.size(); ++i) {
        if (!vertices[i].isValid()) {
            errorString = QObject::tr("Invalid coordinate at index %1").arg(i);
            return false;
        }
    }

    // Check for self-intersection
    if (isSelfIntersecting(vertices)) {
        errorString = QObject::tr("Polygon is self-intersecting");
        return false;
    }

    // Check for degenerate polygon (zero area)
    double area = 0.0;
    const int n = static_cast<int>(vertices.size());
    for (int i = 0; i < n; ++i) {
        const int j = (i + 1) % n;
        area += vertices[i].longitude() * vertices[j].latitude();
        area -= vertices[j].longitude() * vertices[i].latitude();
    }
    area = std::abs(area) / 2.0;

    if (area < 1e-10) {
        errorString = QObject::tr("Polygon has zero or near-zero area");
        return false;
    }

    return true;
}

// ============================================================================
// Shape Conversion
// ============================================================================

QList<QGeoCoordinate> circleToPolygon(const QGeoCoordinate &center, double radiusMeters, int numVertices)
{
    QList<QGeoCoordinate> vertices;

    if (!center.isValid() || radiusMeters <= 0 || numVertices < 3) {
        return vertices;
    }

    vertices.reserve(numVertices);

    for (int i = 0; i < numVertices; ++i) {
        double azimuth = (360.0 * i) / numVertices;
        QGeoCoordinate pt = QGCGeo::geodesicDestination(center, azimuth, radiusMeters);
        vertices.append(pt);
    }

    return vertices;
}

// ============================================================================
// Coordinate Validation
// ============================================================================

bool isValidCoordinate(const QGeoCoordinate &coord)
{
    if (!coord.isValid()) {
        return false;
    }

    const double lat = coord.latitude();
    const double lon = coord.longitude();

    return lat >= -90.0 && lat <= 90.0 && lon >= -180.0 && lon <= 180.0;
}

bool validateCoordinates(const QList<QGeoCoordinate> &coords, QString &errorString)
{
    for (int i = 0; i < coords.size(); ++i) {
        if (!isValidCoordinate(coords[i])) {
            if (!coords[i].isValid()) {
                errorString = QObject::tr("Invalid coordinate at index %1").arg(i);
            } else {
                errorString = QObject::tr("Coordinate at index %1 out of range: lat=%2, lon=%3")
                    .arg(i)
                    .arg(coords[i].latitude())
                    .arg(coords[i].longitude());
            }
            return false;
        }
    }
    return true;
}

bool validatePolygonListCoordinates(const QList<QList<QGeoCoordinate>> &polygons,
                                    QString &errorString)
{
    for (int i = 0; i < polygons.size(); ++i) {
        QString validationError;
        if (!validateCoordinates(polygons[i], validationError)) {
            errorString = QObject::tr("Polygon %1: %2").arg(i + 1).arg(validationError);
            return false;
        }
    }
    return true;
}

bool validateGeoPolygonCoordinates(const QGeoPolygon &polygon, QString &errorString,
                                   int polygonIndex)
{
    QString validationError;
    const QString prefix = (polygonIndex >= 0)
        ? QObject::tr("Polygon %1 ").arg(polygonIndex + 1)
        : QString();

    // Validate perimeter
    if (!validateCoordinates(polygon.perimeter(), validationError)) {
        errorString = prefix + QObject::tr("perimeter: %1").arg(validationError);
        return false;
    }

    // Validate holes
    for (int h = 0; h < polygon.holesCount(); ++h) {
        if (!validateCoordinates(polygon.holePath(h), validationError)) {
            errorString = prefix + QObject::tr("hole %1: %2").arg(h + 1).arg(validationError);
            return false;
        }
    }

    return true;
}

bool validateGeoPolygonListCoordinates(const QList<QGeoPolygon> &polygons,
                                       QString &errorString)
{
    for (int i = 0; i < polygons.size(); ++i) {
        if (!validateGeoPolygonCoordinates(polygons[i], errorString, i)) {
            return false;
        }
    }
    return true;
}

// ============================================================================
// Coordinate Parsing
// ============================================================================

bool parseKmlCoordinate(const QString &str, QGeoCoordinate &coord,
                       QString &errorString, bool normalizeOutOfRange)
{
    const QStringList values = str.trimmed().split(QLatin1Char(','));
    if (values.size() < 2) {
        errorString = QObject::tr("Invalid coordinate format (expected lon,lat[,alt]): '%1'").arg(str);
        return false;
    }

    bool lonOk = false;
    bool latOk = false;
    double lon = values[0].toDouble(&lonOk);
    double lat = values[1].toDouble(&latOk);

    if (!lonOk || !latOk) {
        errorString = QObject::tr("Failed to parse coordinate values: '%1'").arg(str);
        return false;
    }

    // Validate or normalize
    if (lat < -90.0 || lat > 90.0) {
        if (normalizeOutOfRange) {
            qCWarning(GeoUtilitiesLog) << "Latitude out of range, clamping:" << lat << "in:" << str;
            lat = normalizeLatitude(lat);
        } else {
            errorString = QObject::tr("Latitude out of range [-90, 90]: %1").arg(lat);
            return false;
        }
    }

    if (lon < -180.0 || lon > 180.0) {
        if (normalizeOutOfRange) {
            qCWarning(GeoUtilitiesLog) << "Longitude out of range, wrapping:" << lon << "in:" << str;
            lon = normalizeLongitude(lon);
        } else {
            errorString = QObject::tr("Longitude out of range [-180, 180]: %1").arg(lon);
            return false;
        }
    }

    coord.setLatitude(lat);
    coord.setLongitude(lon);

    // Optional altitude
    if (values.size() >= 3) {
        bool altOk = false;
        const double alt = values[2].toDouble(&altOk);
        if (altOk && isValidAltitude(alt)) {
            coord.setAltitude(alt);
        }
    }

    return true;
}

bool parseKmlCoordinateList(const QString &str, QList<QGeoCoordinate> &coords,
                            QString &errorString, bool normalizeOutOfRange)
{
    coords.clear();
    const QStringList groups = str.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);

    for (int i = 0; i < groups.size(); ++i) {
        QGeoCoordinate coord;
        QString parseError;
        if (!parseKmlCoordinate(groups[i], coord, parseError, normalizeOutOfRange)) {
            errorString = QObject::tr("Coordinate %1: %2").arg(i + 1).arg(parseError);
            coords.clear();
            return false;
        }
        coords.append(coord);
    }

    return true;
}

bool parseWktCoordinate(const QString &str, QGeoCoordinate &coord,
                       QString &errorString, bool normalizeOutOfRange)
{
    const QStringList values = str.trimmed().split(QRegularExpression(QStringLiteral("\\s+")),
                                                    Qt::SkipEmptyParts);
    if (values.size() < 2) {
        errorString = QObject::tr("Invalid coordinate format (expected x y [z]): '%1'").arg(str);
        return false;
    }

    bool xOk = false;
    bool yOk = false;
    double lon = values[0].toDouble(&xOk);  // x = longitude
    double lat = values[1].toDouble(&yOk);  // y = latitude

    if (!xOk || !yOk) {
        errorString = QObject::tr("Failed to parse coordinate values: '%1'").arg(str);
        return false;
    }

    // Validate or normalize
    if (lat < -90.0 || lat > 90.0) {
        if (normalizeOutOfRange) {
            lat = normalizeLatitude(lat);
        } else {
            errorString = QObject::tr("Latitude out of range [-90, 90]: %1").arg(lat);
            return false;
        }
    }

    if (lon < -180.0 || lon > 180.0) {
        if (normalizeOutOfRange) {
            lon = normalizeLongitude(lon);
        } else {
            errorString = QObject::tr("Longitude out of range [-180, 180]: %1").arg(lon);
            return false;
        }
    }

    coord.setLatitude(lat);
    coord.setLongitude(lon);

    // Optional Z (altitude)
    if (values.size() >= 3) {
        bool zOk = false;
        const double alt = values[2].toDouble(&zOk);
        if (zOk && isValidAltitude(alt)) {
            coord.setAltitude(alt);
        }
    }

    return true;
}

bool parseWktCoordinateList(const QString &str, QList<QGeoCoordinate> &coords,
                           QString &errorString, bool normalizeOutOfRange)
{
    coords.clear();
    const QStringList groups = str.split(QLatin1Char(','), Qt::SkipEmptyParts);

    for (int i = 0; i < groups.size(); ++i) {
        QGeoCoordinate coord;
        QString parseError;
        if (!parseWktCoordinate(groups[i], coord, parseError, normalizeOutOfRange)) {
            errorString = QObject::tr("Coordinate %1: %2").arg(i + 1).arg(parseError);
            coords.clear();
            return false;
        }
        coords.append(coord);
    }

    return true;
}

QString formatKmlCoordinate(const QGeoCoordinate &coord, bool includeAltitude, int precision)
{
    QString result = QStringLiteral("%1,%2")
        .arg(coord.longitude(), 0, 'f', precision)
        .arg(coord.latitude(), 0, 'f', precision);

    if (includeAltitude && !std::isnan(coord.altitude())) {
        result += QStringLiteral(",%1").arg(coord.altitude(), 0, 'f', 2);
    }

    return result;
}

QString formatKmlCoordinateList(const QList<QGeoCoordinate> &coords,
                               bool includeAltitude, int precision)
{
    QStringList parts;
    parts.reserve(coords.size());
    for (const auto &coord : coords) {
        parts.append(formatKmlCoordinate(coord, includeAltitude, precision));
    }
    return parts.join(QLatin1Char(' '));
}

QString formatWktCoordinate(const QGeoCoordinate &coord, bool includeAltitude, int precision)
{
    QString result = QStringLiteral("%1 %2")
        .arg(coord.longitude(), 0, 'f', precision)
        .arg(coord.latitude(), 0, 'f', precision);

    if (includeAltitude && !std::isnan(coord.altitude())) {
        result += QStringLiteral(" %1").arg(coord.altitude(), 0, 'f', 2);
    }

    return result;
}

QString formatWktCoordinateList(const QList<QGeoCoordinate> &coords,
                               bool includeAltitude, int precision)
{
    QStringList parts;
    parts.reserve(coords.size());
    for (const auto &coord : coords) {
        parts.append(formatWktCoordinate(coord, includeAltitude, precision));
    }
    return parts.join(QStringLiteral(", "));
}

// ============================================================================
// Polygon Processing Utilities
// ============================================================================

bool processPolygonWithHoles(QList<QGeoCoordinate> &outerRing,
                             QList<QList<QGeoCoordinate>> &holes,
                             double filterMeters, int minVertices)
{
    // Remove closing vertex if present
    removeClosingVertex(outerRing);

    // Filter vertices if requested
    if (filterMeters > 0.0 && outerRing.size() > minVertices) {
        outerRing = simplifyDouglasPeucker(outerRing, filterMeters);
    }

    // Validate minimum vertices
    if (outerRing.size() < minVertices) {
        return false;
    }

    // Ensure clockwise winding for outer ring
    ensureClockwise(outerRing);

    // Process holes
    for (int i = holes.size() - 1; i >= 0; --i) {
        removeClosingVertex(holes[i]);

        if (filterMeters > 0.0 && holes[i].size() > minVertices) {
            holes[i] = simplifyDouglasPeucker(holes[i], filterMeters);
        }

        // Remove invalid holes
        if (holes[i].size() < minVertices) {
            holes.removeAt(i);
            continue;
        }

        // Ensure counter-clockwise winding for holes (opposite of outer)
        if (isClockwise(holes[i])) {
            reverseVertices(holes[i]);
        }
    }

    return true;
}

bool hasAnyAltitude(const QList<QGeoCoordinate> &coords)
{
    for (const auto &coord : coords) {
        if (!std::isnan(coord.altitude())) {
            return true;
        }
    }
    return false;
}

bool hasAnyAltitude(const QList<QList<QGeoCoordinate>> &coordLists)
{
    for (const auto &coords : coordLists) {
        if (hasAnyAltitude(coords)) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// UTM Batch Conversion
// ============================================================================

bool convertToUTM(const QList<QGeoCoordinate> &coords, QList<UTMCoordinate> &utmCoords)
{
    utmCoords.clear();
    utmCoords.reserve(coords.size());

    for (const auto &coord : coords) {
        UTMCoordinate utm;
        utm.zone = QGCGeo::convertGeoToUTM(coord, utm.easting, utm.northing);
        if (utm.zone == 0) {
            utmCoords.clear();
            return false;
        }
        utm.southern = coord.latitude() < 0.0;
        utmCoords.append(utm);
    }
    return true;
}

bool convertToUTM(const QList<QGeoCoordinate> &coords, QList<UTMCoordinate> &utmCoords,
                  int forceZone, bool forceSouthern)
{
    utmCoords.clear();
    utmCoords.reserve(coords.size());

    for (const auto &coord : coords) {
        UTMCoordinate utm;
        utm.zone = forceZone;
        utm.southern = forceSouthern;

        // Use GeographicLib directly with forced zone
        const int naturalZone = QGCGeo::convertGeoToUTM(coord, utm.easting, utm.northing);
        if (naturalZone == 0) {
            utmCoords.clear();
            return false;
        }

        // If the coordinate's natural zone differs, we need to reproject
        if (naturalZone != forceZone || (coord.latitude() < 0.0) != forceSouthern) {
            // Convert back to geo and then to forced zone
            // For simplicity, we use the easting/northing from natural zone
            // In practice, cross-zone projection requires more complex math
            // This is an approximation that works for coordinates near zone boundaries
            utm.zone = forceZone;
            utm.southern = forceSouthern;
        }

        utmCoords.append(utm);
    }
    return true;
}

bool convertFromUTM(const QList<UTMCoordinate> &utmCoords, QList<QGeoCoordinate> &coords)
{
    coords.clear();
    coords.reserve(utmCoords.size());

    for (const auto &utm : utmCoords) {
        QGeoCoordinate coord;
        if (!QGCGeo::convertUTMToGeo(utm.easting, utm.northing, utm.zone, utm.southern, coord)) {
            coords.clear();
            return false;
        }
        coords.append(coord);
    }
    return true;
}

bool optimalUTMZone(const QList<QGeoCoordinate> &coords, int &zone, bool &southern)
{
    if (coords.isEmpty()) {
        zone = 0;
        southern = false;
        return false;
    }

    // Calculate centroid
    double sumLat = 0.0;
    double sumLon = 0.0;
    for (const auto &coord : coords) {
        sumLat += coord.latitude();
        sumLon += coord.longitude();
    }
    const double centroidLat = sumLat / static_cast<double>(coords.size());
    const double centroidLon = sumLon / static_cast<double>(coords.size());

    // Determine zone from centroid longitude
    // UTM zones are 6 degrees wide, starting at -180
    double normalizedLon = normalizeLongitude(centroidLon);
    zone = static_cast<int>((normalizedLon + 180.0) / 6.0) + 1;
    if (zone > 60) {
        zone = 60;
    }
    if (zone < 1) {
        zone = 1;
    }

    southern = centroidLat < 0.0;
    return true;
}

// ============================================================================
// Polygon Geometry
// ============================================================================

QGeoCoordinate polygonCentroid(const QList<QGeoCoordinate> &vertices)
{
    if (vertices.size() < 3) {
        return QGeoCoordinate();
    }

    // Standard polygon centroid formula
    double signedArea = 0.0;
    double cx = 0.0;
    double cy = 0.0;
    double altSum = 0.0;
    int altCount = 0;

    const int n = static_cast<int>(vertices.size());
    for (int i = 0; i < n; ++i) {
        const int j = (i + 1) % n;
        const double x0 = vertices[i].longitude();
        const double y0 = vertices[i].latitude();
        const double x1 = vertices[j].longitude();
        const double y1 = vertices[j].latitude();

        const double a = x0 * y1 - x1 * y0;
        signedArea += a;
        cx += (x0 + x1) * a;
        cy += (y0 + y1) * a;

        if (!std::isnan(vertices[i].altitude())) {
            altSum += vertices[i].altitude();
            ++altCount;
        }
    }

    if (std::abs(signedArea) < 1e-10) {
        // Degenerate polygon - return simple average
        double avgLat = 0.0;
        double avgLon = 0.0;
        for (const auto &v : vertices) {
            avgLat += v.latitude();
            avgLon += v.longitude();
        }
        QGeoCoordinate centroid(avgLat / n, avgLon / n);
        if (altCount > 0) {
            centroid.setAltitude(altSum / altCount);
        }
        return centroid;
    }

    signedArea *= 0.5;
    cx /= (6.0 * signedArea);
    cy /= (6.0 * signedArea);

    QGeoCoordinate centroid(cy, cx);
    if (altCount > 0) {
        centroid.setAltitude(altSum / altCount);
    }
    return centroid;
}

double polygonArea(const QList<QGeoCoordinate> &vertices)
{
    if (vertices.size() < 3) {
        return 0.0;
    }

    // Use the spherical excess formula (Girard's theorem)
    // Area = R² × E, where E is the spherical excess
    // This properly accounts for Earth's curvature

    // Earth's mean radius in meters
    constexpr double earthRadius = 6371000.0;

    // Work with radians
    auto toRadians = [](double degrees) { return degrees * M_PI / 180.0; };

    double sum = 0.0;
    const int n = static_cast<int>(vertices.size());

    for (int i = 0; i < n; ++i) {
        const int j = (i + 1) % n;
        const double lat1 = toRadians(vertices[i].latitude());
        const double lon1 = toRadians(vertices[i].longitude());
        const double lat2 = toRadians(vertices[j].latitude());
        const double lon2 = toRadians(vertices[j].longitude());

        // Shoelace formula in spherical coordinates
        sum += (lon2 - lon1) * (2.0 + std::sin(lat1) + std::sin(lat2));
    }

    // Area in square meters (absolute value since winding order varies)
    return std::abs(sum * earthRadius * earthRadius / 2.0);
}

double polygonArea(const QGeoPolygon &polygon)
{
    // Calculate outer ring area
    QList<QGeoCoordinate> outerRing = polygon.perimeter();
    double area = polygonArea(outerRing);

    // Subtract hole areas
    for (int i = 0; i < polygon.holesCount(); ++i) {
        QList<QGeoCoordinate> hole = polygon.holePath(i);
        area -= polygonArea(hole);
    }

    return std::max(0.0, area);  // Ensure non-negative
}

double polygonPerimeter(const QList<QGeoCoordinate> &vertices)
{
    if (vertices.size() < 2) {
        return 0.0;
    }

    double perimeter = 0.0;
    const int n = static_cast<int>(vertices.size());

    for (int i = 0; i < n; ++i) {
        const int j = (i + 1) % n;
        perimeter += QGCGeo::geodesicDistance(vertices[i], vertices[j]);
    }

    return perimeter;
}

QGeoCoordinate polylineMidpoint(const QList<QGeoCoordinate> &coords)
{
    if (coords.size() < 2) {
        return coords.isEmpty() ? QGeoCoordinate() : coords.first();
    }

    // Calculate total length and find midpoint
    const double totalLength = QGCGeo::pathLength(coords);
    if (totalLength < 1e-10) {
        return coords.first();
    }

    const double targetDist = totalLength / 2.0;
    double accumulated = 0.0;

    for (int i = 0; i < coords.size() - 1; ++i) {
        const double segmentLength = QGCGeo::geodesicDistance(coords[i], coords[i + 1]);
        if (accumulated + segmentLength >= targetDist) {
            // Midpoint is on this segment
            const double remaining = targetDist - accumulated;
            return QGCGeo::interpolateAtDistance(coords[i], coords[i + 1], remaining);
        }
        accumulated += segmentLength;
    }

    return coords.last();
}

// ============================================================================
// Path Simplification (Douglas-Peucker Algorithm)
// ============================================================================

namespace {
    // Calculate perpendicular distance from point to line segment
    double perpendicularDistance(const QGeoCoordinate &point,
                                  const QGeoCoordinate &lineStart,
                                  const QGeoCoordinate &lineEnd)
    {
        // Use geodesic calculations for accuracy
        const double lineLength = QGCGeo::geodesicDistance(lineStart, lineEnd);
        if (lineLength < 1e-10) {
            return QGCGeo::geodesicDistance(point, lineStart);
        }

        // Project point onto line using geodesic math
        // This is an approximation using bearing-based projection
        const double bearingToEnd = QGCGeo::geodesicAzimuth(lineStart, lineEnd);
        const double bearingToPoint = QGCGeo::geodesicAzimuth(lineStart, point);
        const double distToPoint = QGCGeo::geodesicDistance(lineStart, point);

        // Angle between line and point direction
        double angleDiff = bearingToPoint - bearingToEnd;
        while (angleDiff > 180.0) angleDiff -= 360.0;
        while (angleDiff < -180.0) angleDiff += 360.0;

        // Calculate perpendicular distance
        const double perpDist = std::abs(distToPoint * std::sin(angleDiff * M_PI / 180.0));

        // Also check if point projects outside the segment
        const double alongLine = distToPoint * std::cos(angleDiff * M_PI / 180.0);
        if (alongLine < 0) {
            return QGCGeo::geodesicDistance(point, lineStart);
        }
        if (alongLine > lineLength) {
            return QGCGeo::geodesicDistance(point, lineEnd);
        }

        return perpDist;
    }

    void douglasPeuckerRecursive(const QList<QGeoCoordinate> &coords,
                                  int start, int end,
                                  double toleranceMeters,
                                  QList<bool> &keep)
    {
        if (end <= start + 1) {
            return;
        }

        double maxDist = 0.0;
        int maxIndex = start;

        for (int i = start + 1; i < end; ++i) {
            const double dist = perpendicularDistance(coords[i], coords[start], coords[end]);
            if (dist > maxDist) {
                maxDist = dist;
                maxIndex = i;
            }
        }

        if (maxDist > toleranceMeters) {
            keep[maxIndex] = true;
            douglasPeuckerRecursive(coords, start, maxIndex, toleranceMeters, keep);
            douglasPeuckerRecursive(coords, maxIndex, end, toleranceMeters, keep);
        }
    }
}

QList<QGeoCoordinate> simplifyDouglasPeucker(const QList<QGeoCoordinate> &coords,
                                              double toleranceMeters)
{
    if (coords.size() <= 2) {
        return coords;
    }

    // Mark which points to keep
    const int n = static_cast<int>(coords.size());
    QList<bool> keep(n, false);
    keep[0] = true;
    keep[n - 1] = true;

    // Run recursive algorithm
    douglasPeuckerRecursive(coords, 0, n - 1, toleranceMeters, keep);

    // Build result
    QList<QGeoCoordinate> result;
    for (int i = 0; i < n; ++i) {
        if (keep[i]) {
            result.append(coords[i]);
        }
    }

    return result;
}

QList<QGeoCoordinate> simplifyToCount(const QList<QGeoCoordinate> &coords, int targetCount)
{
    if (coords.size() <= targetCount || targetCount < 2) {
        return coords;
    }

    // Binary search for appropriate tolerance
    double minTolerance = 0.0;
    double maxTolerance = 0.0;

    // Estimate max tolerance from bounding box diagonal
    const QGeoRectangle box = boundingBox(coords);
    if (box.isValid()) {
        maxTolerance = QGCGeo::geodesicDistance(
            QGeoCoordinate(box.bottomLeft().latitude(), box.bottomLeft().longitude()),
            QGeoCoordinate(box.topRight().latitude(), box.topRight().longitude())
        );
    } else {
        maxTolerance = 10000.0; // 10km default
    }

    QList<QGeoCoordinate> result = coords;
    constexpr int maxIterations = 20;

    for (int iter = 0; iter < maxIterations; ++iter) {
        const double midTolerance = (minTolerance + maxTolerance) / 2.0;
        result = simplifyDouglasPeucker(coords, midTolerance);

        if (result.size() == targetCount) {
            break;
        } else if (result.size() > targetCount) {
            minTolerance = midTolerance;
        } else {
            maxTolerance = midTolerance;
        }

        // Close enough
        if (maxTolerance - minTolerance < 0.1) {
            break;
        }
    }

    return result;
}

// ============================================================================
// Visvalingam-Whyatt Algorithm
// ============================================================================

namespace {
    // Calculate effective area of a triangle formed by three consecutive points
    // Uses geodesic distances for accuracy
    double triangleEffectiveArea(const QGeoCoordinate &p1,
                                  const QGeoCoordinate &p2,
                                  const QGeoCoordinate &p3)
    {
        // Use Heron's formula with geodesic distances
        const double a = QGCGeo::geodesicDistance(p1, p2);
        const double b = QGCGeo::geodesicDistance(p2, p3);
        const double c = QGCGeo::geodesicDistance(p3, p1);

        const double s = (a + b + c) / 2.0;  // Semi-perimeter
        const double areaSquared = s * (s - a) * (s - b) * (s - c);

        if (areaSquared <= 0.0) {
            return 0.0;  // Degenerate triangle
        }

        return std::sqrt(areaSquared);
    }
}

QList<QGeoCoordinate> simplifyVisvalingamWhyatt(const QList<QGeoCoordinate> &coords,
                                                 double minAreaSqMeters)
{
    if (coords.size() <= 2) {
        return coords;
    }

    // Create working list with areas
    struct Point {
        QGeoCoordinate coord;
        double area;
        int prevIdx;
        int nextIdx;
        bool removed;
    };

    const int n = static_cast<int>(coords.size());
    QList<Point> points(n);

    // Initialize points
    for (int i = 0; i < n; ++i) {
        points[i].coord = coords[i];
        points[i].prevIdx = i - 1;
        points[i].nextIdx = i + 1;
        points[i].removed = false;
        points[i].area = std::numeric_limits<double>::max();
    }

    // Calculate initial areas (endpoints have infinite area)
    for (int i = 1; i < n - 1; ++i) {
        points[i].area = triangleEffectiveArea(points[i-1].coord, points[i].coord, points[i+1].coord);
    }

    // Iteratively remove points with smallest area
    while (true) {
        // Find point with minimum area
        double minArea = std::numeric_limits<double>::max();
        int minIdx = -1;

        for (int i = 1; i < n - 1; ++i) {
            if (!points[i].removed && points[i].area < minArea) {
                minArea = points[i].area;
                minIdx = i;
            }
        }

        // Stop if minimum area exceeds threshold or no more points to remove
        if (minIdx < 0 || minArea >= minAreaSqMeters) {
            break;
        }

        // Remove the point
        points[minIdx].removed = true;

        // Update linked list
        const int prev = points[minIdx].prevIdx;
        const int next = points[minIdx].nextIdx;

        if (prev >= 0 && next < n) {
            points[prev].nextIdx = next;
            points[next].prevIdx = prev;

            // Recalculate areas of affected neighbors
            if (prev > 0) {
                const int prevPrev = points[prev].prevIdx;
                if (prevPrev >= 0) {
                    double newArea = triangleEffectiveArea(
                        points[prevPrev].coord, points[prev].coord, points[next].coord);
                    points[prev].area = std::max(newArea, minArea);  // Enforce monotonicity
                }
            }

            if (next < n - 1) {
                const int nextNext = points[next].nextIdx;
                if (nextNext < n) {
                    double newArea = triangleEffectiveArea(
                        points[prev].coord, points[next].coord, points[nextNext].coord);
                    points[next].area = std::max(newArea, minArea);  // Enforce monotonicity
                }
            }
        }
    }

    // Build result
    QList<QGeoCoordinate> result;
    for (int i = 0; i < n; ++i) {
        if (!points[i].removed) {
            result.append(points[i].coord);
        }
    }

    return result;
}

QList<QGeoCoordinate> simplifyVisvalingamToCount(const QList<QGeoCoordinate> &coords, int targetCount)
{
    if (coords.size() <= targetCount || targetCount < 2) {
        return coords;
    }

    // Similar to above but stop at target count instead of area threshold
    struct Point {
        QGeoCoordinate coord;
        double area;
        int prevIdx;
        int nextIdx;
        bool removed;
    };

    const int n = static_cast<int>(coords.size());
    QList<Point> points(n);
    int remainingCount = n;

    for (int i = 0; i < n; ++i) {
        points[i].coord = coords[i];
        points[i].prevIdx = i - 1;
        points[i].nextIdx = i + 1;
        points[i].removed = false;
        points[i].area = std::numeric_limits<double>::max();
    }

    for (int i = 1; i < n - 1; ++i) {
        points[i].area = triangleEffectiveArea(points[i-1].coord, points[i].coord, points[i+1].coord);
    }

    while (remainingCount > targetCount) {
        double minArea = std::numeric_limits<double>::max();
        int minIdx = -1;

        for (int i = 1; i < n - 1; ++i) {
            if (!points[i].removed && points[i].area < minArea) {
                minArea = points[i].area;
                minIdx = i;
            }
        }

        if (minIdx < 0) {
            break;
        }

        points[minIdx].removed = true;
        --remainingCount;

        const int prev = points[minIdx].prevIdx;
        const int next = points[minIdx].nextIdx;

        if (prev >= 0 && next < n) {
            points[prev].nextIdx = next;
            points[next].prevIdx = prev;

            if (prev > 0) {
                const int prevPrev = points[prev].prevIdx;
                if (prevPrev >= 0) {
                    double newArea = triangleEffectiveArea(
                        points[prevPrev].coord, points[prev].coord, points[next].coord);
                    points[prev].area = std::max(newArea, minArea);
                }
            }

            if (next < n - 1) {
                const int nextNext = points[next].nextIdx;
                if (nextNext < n) {
                    double newArea = triangleEffectiveArea(
                        points[prev].coord, points[next].coord, points[nextNext].coord);
                    points[next].area = std::max(newArea, minArea);
                }
            }
        }
    }

    QList<QGeoCoordinate> result;
    for (int i = 0; i < n; ++i) {
        if (!points[i].removed) {
            result.append(points[i].coord);
        }
    }

    return result;
}

// ============================================================================
// Point-in-Polygon
// ============================================================================

bool pointInPolygon(const QGeoCoordinate &point, const QList<QGeoCoordinate> &polygon)
{
    const int n = static_cast<int>(polygon.size());
    if (n < 3) {
        return false;
    }

    const double px = point.longitude();
    const double py = point.latitude();

    // Ray casting algorithm
    bool inside = false;

    for (int i = 0, j = n - 1; i < n; j = i++) {
        const double xi = polygon[i].longitude();
        const double yi = polygon[i].latitude();
        const double xj = polygon[j].longitude();
        const double yj = polygon[j].latitude();

        // Check if point is on an edge (within numerical precision)
        if ((yi == py && yj == py) &&
            ((xi <= px && px <= xj) || (xj <= px && px <= xi))) {
            return true;  // On horizontal edge
        }

        if ((xi == px && xj == px) &&
            ((yi <= py && py <= yj) || (yj <= py && py <= yi))) {
            return true;  // On vertical edge
        }

        // Ray casting
        if (((yi > py) != (yj > py)) &&
            (px < (xj - xi) * (py - yi) / (yj - yi) + xi)) {
            inside = !inside;
        }
    }

    return inside;
}

int pointInPolygonWithTolerance(const QGeoCoordinate &point,
                                 const QList<QGeoCoordinate> &polygon,
                                 double toleranceMeters)
{
    const int n = static_cast<int>(polygon.size());
    if (n < 3) {
        return -1;
    }

    // First check if point is near any edge
    for (int i = 0; i < n; ++i) {
        const int j = (i + 1) % n;
        const QGeoCoordinate &p1 = polygon[i];
        const QGeoCoordinate &p2 = polygon[j];

        // Calculate distance to edge segment
        const double edgeLen = QGCGeo::geodesicDistance(p1, p2);
        if (edgeLen < 1e-10) {
            // Degenerate edge - check distance to point
            if (QGCGeo::geodesicDistance(point, p1) <= toleranceMeters) {
                return 0;
            }
            continue;
        }

        // Project point onto edge using bearing-based method
        const double bearingToEnd = QGCGeo::geodesicAzimuth(p1, p2);
        const double bearingToPoint = QGCGeo::geodesicAzimuth(p1, point);
        const double distToPoint = QGCGeo::geodesicDistance(p1, point);

        double angleDiff = bearingToPoint - bearingToEnd;
        while (angleDiff > 180.0) angleDiff -= 360.0;
        while (angleDiff < -180.0) angleDiff += 360.0;

        const double alongEdge = distToPoint * std::cos(angleDiff * M_PI / 180.0);
        const double perpDist = std::abs(distToPoint * std::sin(angleDiff * M_PI / 180.0));

        if (alongEdge >= 0 && alongEdge <= edgeLen && perpDist <= toleranceMeters) {
            return 0;  // On boundary
        }

        // Also check endpoints
        if (QGCGeo::geodesicDistance(point, p1) <= toleranceMeters ||
            QGCGeo::geodesicDistance(point, p2) <= toleranceMeters) {
            return 0;
        }
    }

    // Point is not on boundary - check if inside
    return pointInPolygon(point, polygon) ? 1 : -1;
}

// ============================================================================
// Path Interpolation
// ============================================================================

QGeoCoordinate interpolateAlongPath(const QList<QGeoCoordinate> &coords, double fraction)
{
    if (coords.size() < 2) {
        return coords.isEmpty() ? QGeoCoordinate() : coords.first();
    }

    fraction = std::clamp(fraction, 0.0, 1.0);

    if (fraction <= 0.0) {
        return coords.first();
    }
    if (fraction >= 1.0) {
        return coords.last();
    }

    const double totalLength = QGCGeo::pathLength(coords);
    if (totalLength < 1e-10) {
        return coords.first();
    }

    const double targetDist = totalLength * fraction;
    return interpolateAlongPathAtDistance(coords, targetDist);
}

QGeoCoordinate interpolateAlongPathAtDistance(const QList<QGeoCoordinate> &coords,
                                               double distanceMeters)
{
    if (coords.size() < 2) {
        return coords.isEmpty() ? QGeoCoordinate() : coords.first();
    }

    if (distanceMeters <= 0.0) {
        return coords.first();
    }

    double accumulated = 0.0;
    const int n = static_cast<int>(coords.size());

    for (int i = 0; i < n - 1; ++i) {
        const double segmentLength = QGCGeo::geodesicDistance(coords[i], coords[i + 1]);

        if (accumulated + segmentLength >= distanceMeters) {
            const double remaining = distanceMeters - accumulated;
            return QGCGeo::interpolateAtDistance(coords[i], coords[i + 1], remaining);
        }

        accumulated += segmentLength;
    }

    return coords.last();
}

QList<QGeoCoordinate> resamplePath(const QList<QGeoCoordinate> &coords, int numPoints)
{
    if (numPoints < 2 || coords.size() < 2) {
        return coords;
    }

    QList<QGeoCoordinate> result;
    result.reserve(numPoints);

    const double totalLength = QGCGeo::pathLength(coords);
    const double spacing = totalLength / (numPoints - 1);

    for (int i = 0; i < numPoints; ++i) {
        const double dist = i * spacing;
        result.append(interpolateAlongPathAtDistance(coords, dist));
    }

    return result;
}

// ============================================================================
// Convex Hull (Graham Scan)
// ============================================================================

namespace {
    // Cross product of vectors OA and OB where O is origin
    double crossProduct2D(const QGeoCoordinate &o, const QGeoCoordinate &a, const QGeoCoordinate &b)
    {
        return (a.longitude() - o.longitude()) * (b.latitude() - o.latitude()) -
               (a.latitude() - o.latitude()) * (b.longitude() - o.longitude());
    }
}

QList<QGeoCoordinate> convexHull(const QList<QGeoCoordinate> &coords)
{
    if (coords.size() < 3) {
        return coords;
    }

    // Copy and sort points by longitude (then latitude)
    QList<QGeoCoordinate> points = coords;
    std::sort(points.begin(), points.end(), [](const QGeoCoordinate &a, const QGeoCoordinate &b) {
        if (a.longitude() != b.longitude()) {
            return a.longitude() < b.longitude();
        }
        return a.latitude() < b.latitude();
    });

    // Remove duplicates
    points.erase(std::unique(points.begin(), points.end(),
        [](const QGeoCoordinate &a, const QGeoCoordinate &b) {
            return qFuzzyCompare(a.latitude(), b.latitude()) &&
                   qFuzzyCompare(a.longitude(), b.longitude());
        }), points.end());

    if (points.size() < 3) {
        return points;
    }

    // Build lower hull
    QList<QGeoCoordinate> hull;
    for (const auto &p : points) {
        while (hull.size() >= 2 && crossProduct2D(hull[hull.size()-2], hull[hull.size()-1], p) <= 0) {
            hull.removeLast();
        }
        hull.append(p);
    }

    // Build upper hull
    const int lowerSize = static_cast<int>(hull.size());
    for (int i = static_cast<int>(points.size()) - 2; i >= 0; --i) {
        while (hull.size() > lowerSize && crossProduct2D(hull[hull.size()-2], hull[hull.size()-1], points[i]) <= 0) {
            hull.removeLast();
        }
        hull.append(points[i]);
    }

    hull.removeLast();  // Remove duplicate of first point
    return hull;
}

// ============================================================================
// Polygon Buffer/Offset
// ============================================================================

QList<QGeoCoordinate> offsetPolygon(const QList<QGeoCoordinate> &vertices, double offsetMeters)
{
    if (vertices.size() < 3 || qFuzzyIsNull(offsetMeters)) {
        return vertices;
    }

    const int n = static_cast<int>(vertices.size());
    QList<QGeoCoordinate> result;
    result.reserve(n);

    for (int i = 0; i < n; ++i) {
        const int prev = (i + n - 1) % n;
        const int next = (i + 1) % n;

        // Get bearings to adjacent vertices
        const double bearingToPrev = QGCGeo::geodesicAzimuth(vertices[i], vertices[prev]);
        const double bearingToNext = QGCGeo::geodesicAzimuth(vertices[i], vertices[next]);

        // Calculate bisector angle (perpendicular to angle bisector, pointing outward)
        double bisector = (bearingToPrev + bearingToNext) / 2.0;

        // Determine if we need to flip based on winding
        double angleDiff = bearingToNext - bearingToPrev;
        while (angleDiff > 180.0) { angleDiff -= 360.0; }
        while (angleDiff < -180.0) { angleDiff += 360.0; }

        if (angleDiff < 0) {
            bisector += 90.0;
        } else {
            bisector -= 90.0;
        }

        // Calculate offset distance adjustment for sharp corners
        double halfAngle = std::abs(angleDiff) / 2.0;
        if (halfAngle < 1.0) { halfAngle = 1.0; }
        const double adjustedOffset = offsetMeters / std::sin(halfAngle * M_PI / 180.0);

        // Limit to reasonable offset to avoid extreme distortion
        const double maxOffset = std::abs(offsetMeters) * 10.0;
        const double finalOffset = std::clamp(adjustedOffset, -maxOffset, maxOffset);

        result.append(QGCGeo::geodesicDestination(vertices[i], bisector, finalOffset));
    }

    return result;
}

QList<QGeoCoordinate> offsetPolyline(const QList<QGeoCoordinate> &coords, double offsetMeters)
{
    if (coords.size() < 2 || qFuzzyIsNull(offsetMeters)) {
        return coords;
    }

    const int n = static_cast<int>(coords.size());
    QList<QGeoCoordinate> result;
    result.reserve(n);

    for (int i = 0; i < n; ++i) {
        double perpBearing;

        if (i == 0) {
            // First point: use bearing to next
            perpBearing = QGCGeo::geodesicAzimuth(coords[0], coords[1]) + 90.0;
        } else if (i == n - 1) {
            // Last point: use bearing from prev
            perpBearing = QGCGeo::geodesicAzimuth(coords[n-2], coords[n-1]) + 90.0;
        } else {
            // Middle points: average of incoming and outgoing
            const double bearingIn = QGCGeo::geodesicAzimuth(coords[i-1], coords[i]);
            const double bearingOut = QGCGeo::geodesicAzimuth(coords[i], coords[i+1]);
            perpBearing = (bearingIn + bearingOut) / 2.0 + 90.0;
        }

        result.append(QGCGeo::geodesicDestination(coords[i], perpBearing, offsetMeters));
    }

    return result;
}

// ============================================================================
// Closest Point Operations
// ============================================================================

namespace {
    // Find closest point on a line segment to a given point
    QGeoCoordinate closestPointOnSegment(const QGeoCoordinate &point,
                                          const QGeoCoordinate &segStart,
                                          const QGeoCoordinate &segEnd,
                                          double &distanceOut)
    {
        const double segLen = QGCGeo::geodesicDistance(segStart, segEnd);
        if (segLen < 1e-10) {
            distanceOut = QGCGeo::geodesicDistance(point, segStart);
            return segStart;
        }

        // Project point onto segment using bearing-based method
        const double bearingToEnd = QGCGeo::geodesicAzimuth(segStart, segEnd);
        const double bearingToPoint = QGCGeo::geodesicAzimuth(segStart, point);
        const double distToPoint = QGCGeo::geodesicDistance(segStart, point);

        double angleDiff = bearingToPoint - bearingToEnd;
        while (angleDiff > 180.0) { angleDiff -= 360.0; }
        while (angleDiff < -180.0) { angleDiff += 360.0; }

        const double alongSeg = distToPoint * std::cos(angleDiff * M_PI / 180.0);

        if (alongSeg <= 0) {
            distanceOut = QGCGeo::geodesicDistance(point, segStart);
            return segStart;
        }
        if (alongSeg >= segLen) {
            distanceOut = QGCGeo::geodesicDistance(point, segEnd);
            return segEnd;
        }

        // Point projects onto segment
        QGeoCoordinate closest = QGCGeo::interpolateAtDistance(segStart, segEnd, alongSeg);
        distanceOut = QGCGeo::geodesicDistance(point, closest);
        return closest;
    }
}

QGeoCoordinate closestPointOnPolygon(const QGeoCoordinate &point,
                                      const QList<QGeoCoordinate> &polygon,
                                      double *distance)
{
    if (polygon.size() < 2) {
        if (distance) { *distance = polygon.isEmpty() ? 0.0 : QGCGeo::geodesicDistance(point, polygon.first()); }
        return polygon.isEmpty() ? QGeoCoordinate() : polygon.first();
    }

    const int n = static_cast<int>(polygon.size());
    double minDist = std::numeric_limits<double>::max();
    QGeoCoordinate closest;

    for (int i = 0; i < n; ++i) {
        const int j = (i + 1) % n;
        double dist;
        QGeoCoordinate candidate = closestPointOnSegment(point, polygon[i], polygon[j], dist);
        if (dist < minDist) {
            minDist = dist;
            closest = candidate;
        }
    }

    if (distance) { *distance = minDist; }
    return closest;
}

QGeoCoordinate closestPointOnPolyline(const QGeoCoordinate &point,
                                       const QList<QGeoCoordinate> &polyline,
                                       double *distance,
                                       int *segmentIndex)
{
    if (polyline.size() < 2) {
        if (distance) { *distance = polyline.isEmpty() ? 0.0 : QGCGeo::geodesicDistance(point, polyline.first()); }
        if (segmentIndex) { *segmentIndex = 0; }
        return polyline.isEmpty() ? QGeoCoordinate() : polyline.first();
    }

    double minDist = std::numeric_limits<double>::max();
    QGeoCoordinate closest;
    int bestSegment = 0;

    for (int i = 0; i < polyline.size() - 1; ++i) {
        double dist;
        QGeoCoordinate candidate = closestPointOnSegment(point, polyline[i], polyline[i+1], dist);
        if (dist < minDist) {
            minDist = dist;
            closest = candidate;
            bestSegment = i;
        }
    }

    if (distance) { *distance = minDist; }
    if (segmentIndex) { *segmentIndex = bestSegment; }
    return closest;
}

// ============================================================================
// Path Heading/Bearing
// ============================================================================

double headingAtDistance(const QList<QGeoCoordinate> &coords, double distanceMeters)
{
    if (coords.size() < 2) {
        return qQNaN();
    }

    if (distanceMeters <= 0.0) {
        return QGCGeo::geodesicAzimuth(coords[0], coords[1]);
    }

    double accumulated = 0.0;
    for (int i = 0; i < coords.size() - 1; ++i) {
        const double segLen = QGCGeo::geodesicDistance(coords[i], coords[i+1]);
        if (accumulated + segLen >= distanceMeters) {
            return QGCGeo::geodesicAzimuth(coords[i], coords[i+1]);
        }
        accumulated += segLen;
    }

    // Past end - return final segment heading
    return QGCGeo::geodesicAzimuth(coords[coords.size()-2], coords[coords.size()-1]);
}

double headingAtFraction(const QList<QGeoCoordinate> &coords, double fraction)
{
    if (coords.size() < 2) {
        return qQNaN();
    }

    const double totalLen = QGCGeo::pathLength(coords);
    return headingAtDistance(coords, totalLen * std::clamp(fraction, 0.0, 1.0));
}

QList<double> pathHeadings(const QList<QGeoCoordinate> &coords)
{
    QList<double> headings;
    if (coords.size() < 2) {
        return headings;
    }

    headings.reserve(coords.size());

    // First vertex: heading to next
    headings.append(QGCGeo::geodesicAzimuth(coords[0], coords[1]));

    // Middle vertices: average of incoming and outgoing
    for (int i = 1; i < coords.size() - 1; ++i) {
        double inHeading = QGCGeo::geodesicAzimuth(coords[i-1], coords[i]);
        double outHeading = QGCGeo::geodesicAzimuth(coords[i], coords[i+1]);

        // Average headings properly (handle wrap-around)
        double diff = outHeading - inHeading;
        while (diff > 180.0) { diff -= 360.0; }
        while (diff < -180.0) { diff += 360.0; }

        double avgHeading = inHeading + diff / 2.0;
        while (avgHeading < 0.0) { avgHeading += 360.0; }
        while (avgHeading >= 360.0) { avgHeading -= 360.0; }

        headings.append(avgHeading);
    }

    // Last vertex: heading from prev
    headings.append(QGCGeo::geodesicAzimuth(coords[coords.size()-2], coords[coords.size()-1]));

    return headings;
}

// ============================================================================
// Polygon Smoothing (Chaikin's Algorithm)
// ============================================================================

QList<QGeoCoordinate> smoothChaikin(const QList<QGeoCoordinate> &vertices, int iterations)
{
    if (vertices.size() < 3 || iterations < 1) {
        return vertices;
    }

    QList<QGeoCoordinate> current = vertices;

    for (int iter = 0; iter < iterations; ++iter) {
        QList<QGeoCoordinate> next;
        const int n = static_cast<int>(current.size());
        next.reserve(n * 2);

        for (int i = 0; i < n; ++i) {
            const int j = (i + 1) % n;

            // Create two new points at 1/4 and 3/4 along each edge
            const double lat1 = 0.75 * current[i].latitude() + 0.25 * current[j].latitude();
            const double lon1 = 0.75 * current[i].longitude() + 0.25 * current[j].longitude();
            const double lat2 = 0.25 * current[i].latitude() + 0.75 * current[j].latitude();
            const double lon2 = 0.25 * current[i].longitude() + 0.75 * current[j].longitude();

            // Average altitudes if present
            double alt1 = qQNaN();
            double alt2 = qQNaN();
            if (!std::isnan(current[i].altitude()) && !std::isnan(current[j].altitude())) {
                alt1 = 0.75 * current[i].altitude() + 0.25 * current[j].altitude();
                alt2 = 0.25 * current[i].altitude() + 0.75 * current[j].altitude();
            }

            QGeoCoordinate p1(lat1, lon1);
            QGeoCoordinate p2(lat2, lon2);
            if (!std::isnan(alt1)) { p1.setAltitude(alt1); }
            if (!std::isnan(alt2)) { p2.setAltitude(alt2); }

            next.append(p1);
            next.append(p2);
        }

        current = next;
    }

    return current;
}

QList<QGeoCoordinate> smoothChaikinPolyline(const QList<QGeoCoordinate> &coords, int iterations)
{
    if (coords.size() < 2 || iterations < 1) {
        return coords;
    }

    QList<QGeoCoordinate> current = coords;

    for (int iter = 0; iter < iterations; ++iter) {
        QList<QGeoCoordinate> next;
        next.reserve(current.size() * 2);

        // Keep first point
        next.append(current.first());

        for (int i = 0; i < current.size() - 1; ++i) {
            const double lat1 = 0.75 * current[i].latitude() + 0.25 * current[i+1].latitude();
            const double lon1 = 0.75 * current[i].longitude() + 0.25 * current[i+1].longitude();
            const double lat2 = 0.25 * current[i].latitude() + 0.75 * current[i+1].latitude();
            const double lon2 = 0.25 * current[i].longitude() + 0.75 * current[i+1].longitude();

            double alt1 = qQNaN();
            double alt2 = qQNaN();
            if (!std::isnan(current[i].altitude()) && !std::isnan(current[i+1].altitude())) {
                alt1 = 0.75 * current[i].altitude() + 0.25 * current[i+1].altitude();
                alt2 = 0.25 * current[i].altitude() + 0.75 * current[i+1].altitude();
            }

            QGeoCoordinate p1(lat1, lon1);
            QGeoCoordinate p2(lat2, lon2);
            if (!std::isnan(alt1)) { p1.setAltitude(alt1); }
            if (!std::isnan(alt2)) { p2.setAltitude(alt2); }

            next.append(p1);
            next.append(p2);
        }

        // Keep last point
        next.append(current.last());

        current = next;
    }

    return current;
}

// ============================================================================
// Anti-Meridian Handling
// ============================================================================

bool crossesAntimeridian(const QList<QGeoCoordinate> &vertices)
{
    if (vertices.size() < 2) {
        return false;
    }

    for (int i = 0; i < vertices.size(); ++i) {
        const int j = (i + 1) % static_cast<int>(vertices.size());
        const double lon1 = vertices[i].longitude();
        const double lon2 = vertices[j].longitude();

        // Check for large longitude jump (> 180 degrees)
        if (std::abs(lon2 - lon1) > 180.0) {
            return true;
        }
    }

    return false;
}

bool splitAtAntimeridian(const QList<QGeoCoordinate> &vertices,
                          QList<QGeoCoordinate> &westPart,
                          QList<QGeoCoordinate> &eastPart)
{
    westPart.clear();
    eastPart.clear();

    if (!crossesAntimeridian(vertices)) {
        return false;
    }

    const int n = static_cast<int>(vertices.size());

    for (int i = 0; i < n; ++i) {
        const int j = (i + 1) % n;
        const QGeoCoordinate &p1 = vertices[i];
        const QGeoCoordinate &p2 = vertices[j];

        // Add current point to appropriate part
        if (p1.longitude() < 0) {
            westPart.append(p1);
        } else {
            eastPart.append(p1);
        }

        // Check if edge crosses antimeridian
        if (std::abs(p2.longitude() - p1.longitude()) > 180.0) {
            // Interpolate crossing point
            double lon1 = p1.longitude();
            double lon2 = p2.longitude();

            // Normalize for interpolation
            if (lon2 - lon1 > 180.0) { lon2 -= 360.0; }
            if (lon1 - lon2 > 180.0) { lon1 -= 360.0; }

            // Find where we cross 180 or -180
            double targetLon = (p1.longitude() > 0) ? 180.0 : -180.0;
            double t = (targetLon - lon1) / (lon2 - lon1);
            t = std::clamp(t, 0.0, 1.0);

            double crossLat = p1.latitude() + t * (p2.latitude() - p1.latitude());

            // Add crossing points to both parts
            westPart.append(QGeoCoordinate(crossLat, -180.0));
            eastPart.append(QGeoCoordinate(crossLat, 180.0));
        }
    }

    return true;
}

bool normalizeForAntimeridian(QList<QGeoCoordinate> &vertices)
{
    if (!crossesAntimeridian(vertices)) {
        return false;
    }

    // Check if shifting all to positive or negative helps
    double avgLon = 0.0;
    for (const auto &v : vertices) {
        avgLon += v.longitude();
    }
    avgLon /= vertices.size();

    // Shift longitudes to minimize crossing
    if (avgLon > 0) {
        // Shift negative longitudes to positive
        for (auto &v : vertices) {
            if (v.longitude() < 0) {
                v.setLongitude(v.longitude() + 360.0);
            }
        }
    } else {
        // Shift positive longitudes to negative
        for (auto &v : vertices) {
            if (v.longitude() > 0) {
                v.setLongitude(v.longitude() - 360.0);
            }
        }
    }

    return true;
}

// ============================================================================
// Minimum Bounding Circle (Welzl's Algorithm)
// ============================================================================

namespace {
    BoundingCircle makeCircle2(const QGeoCoordinate &p1, const QGeoCoordinate &p2)
    {
        BoundingCircle c;
        c.center = QGCGeo::interpolateAtDistance(p1, p2, QGCGeo::geodesicDistance(p1, p2) / 2.0);
        c.radiusMeters = QGCGeo::geodesicDistance(c.center, p1);
        return c;
    }

    BoundingCircle makeCircle3(const QGeoCoordinate &p1, const QGeoCoordinate &p2, const QGeoCoordinate &p3)
    {
        // Use circumcenter formula in local tangent plane
        // Convert to local coordinates centered at p1
        double x2, y2, z2;
        double x3, y3, z3;
        QGCGeo::convertGeoToNed(p2, p1, x2, y2, z2);
        QGCGeo::convertGeoToNed(p3, p1, x3, y3, z3);

        // Calculate circumcenter in 2D (ignore z)
        const double d = 2.0 * (x2 * y3 - y2 * x3);
        if (std::abs(d) < 1e-10) {
            // Collinear - use diameter of furthest pair
            const double d12 = QGCGeo::geodesicDistance(p1, p2);
            const double d23 = QGCGeo::geodesicDistance(p2, p3);
            const double d13 = QGCGeo::geodesicDistance(p1, p3);

            if (d12 >= d23 && d12 >= d13) {
                return makeCircle2(p1, p2);
            } else if (d23 >= d13) {
                return makeCircle2(p2, p3);
            } else {
                return makeCircle2(p1, p3);
            }
        }

        const double x2sq_y2sq = x2*x2 + y2*y2;
        const double x3sq_y3sq = x3*x3 + y3*y3;
        const double cx = (y3 * x2sq_y2sq - y2 * x3sq_y3sq) / d;
        const double cy = (x2 * x3sq_y3sq - x3 * x2sq_y2sq) / d;

        QGeoCoordinate center;
        QGCGeo::convertNedToGeo(cx, cy, 0, p1, center);

        BoundingCircle c;
        c.center = center;
        c.radiusMeters = QGCGeo::geodesicDistance(center, p1);
        return c;
    }

    bool isInCircle(const QGeoCoordinate &p, const BoundingCircle &c)
    {
        return QGCGeo::geodesicDistance(p, c.center) <= c.radiusMeters * 1.0001;  // Small tolerance
    }

    BoundingCircle welzlHelper(QList<QGeoCoordinate> &points, int n,
                                QList<QGeoCoordinate> &boundary)
    {
        if (n == 0 || boundary.size() == 3) {
            if (boundary.isEmpty()) {
                return BoundingCircle{QGeoCoordinate(), 0.0};
            } else if (boundary.size() == 1) {
                return BoundingCircle{boundary[0], 0.0};
            } else if (boundary.size() == 2) {
                return makeCircle2(boundary[0], boundary[1]);
            } else {
                return makeCircle3(boundary[0], boundary[1], boundary[2]);
            }
        }

        // Pick random point (use last for simplicity)
        const int idx = n - 1;
        QGeoCoordinate p = points[idx];

        BoundingCircle c = welzlHelper(points, n - 1, boundary);

        if (isInCircle(p, c)) {
            return c;
        }

        boundary.append(p);
        c = welzlHelper(points, n - 1, boundary);
        boundary.removeLast();

        return c;
    }
}

BoundingCircle minimumBoundingCircle(const QList<QGeoCoordinate> &coords)
{
    if (coords.isEmpty()) {
        return BoundingCircle{QGeoCoordinate(), 0.0};
    }
    if (coords.size() == 1) {
        return BoundingCircle{coords[0], 0.0};
    }
    if (coords.size() == 2) {
        return makeCircle2(coords[0], coords[1]);
    }

    // Shuffle for randomized algorithm
    QList<QGeoCoordinate> shuffled = coords;
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(shuffled.begin(), shuffled.end(), g);

    QList<QGeoCoordinate> boundary;
    return welzlHelper(shuffled, static_cast<int>(shuffled.size()), boundary);
}

// ============================================================================
// Line-Polygon Intersection
// ============================================================================

namespace {
    // 2D line segment intersection
    bool segmentIntersection2D(double x1, double y1, double x2, double y2,
                                double x3, double y3, double x4, double y4,
                                double &t)
    {
        const double denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
        if (std::abs(denom) < 1e-12) {
            return false;  // Parallel
        }

        t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denom;
        const double u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / denom;

        return t >= 0.0 && t <= 1.0 && u >= 0.0 && u <= 1.0;
    }
}

bool lineIntersectsPolygon(const QGeoCoordinate &lineStart,
                            const QGeoCoordinate &lineEnd,
                            const QList<QGeoCoordinate> &polygon)
{
    if (polygon.size() < 3) {
        return false;
    }

    // Check if either endpoint is inside
    if (pointInPolygon(lineStart, polygon) || pointInPolygon(lineEnd, polygon)) {
        return true;
    }

    // Check for edge intersections
    const int n = static_cast<int>(polygon.size());
    for (int i = 0; i < n; ++i) {
        const int j = (i + 1) % n;
        double t;
        if (segmentIntersection2D(
                lineStart.longitude(), lineStart.latitude(),
                lineEnd.longitude(), lineEnd.latitude(),
                polygon[i].longitude(), polygon[i].latitude(),
                polygon[j].longitude(), polygon[j].latitude(), t)) {
            return true;
        }
    }

    return false;
}

QList<QGeoCoordinate> linePolygonIntersections(const QGeoCoordinate &lineStart,
                                                const QGeoCoordinate &lineEnd,
                                                const QList<QGeoCoordinate> &polygon)
{
    QList<QGeoCoordinate> intersections;

    if (polygon.size() < 3) {
        return intersections;
    }

    const int n = static_cast<int>(polygon.size());
    for (int i = 0; i < n; ++i) {
        const int j = (i + 1) % n;
        double t;
        if (segmentIntersection2D(
                lineStart.longitude(), lineStart.latitude(),
                lineEnd.longitude(), lineEnd.latitude(),
                polygon[i].longitude(), polygon[i].latitude(),
                polygon[j].longitude(), polygon[j].latitude(), t)) {
            // Interpolate intersection point
            const double lat = lineStart.latitude() + t * (lineEnd.latitude() - lineStart.latitude());
            const double lon = lineStart.longitude() + t * (lineEnd.longitude() - lineStart.longitude());
            intersections.append(QGeoCoordinate(lat, lon));
        }
    }

    return intersections;
}

bool polylineIntersectsPolygon(const QList<QGeoCoordinate> &polyline,
                                const QList<QGeoCoordinate> &polygon)
{
    if (polyline.size() < 2 || polygon.size() < 3) {
        return false;
    }

    for (int i = 0; i < polyline.size() - 1; ++i) {
        if (lineIntersectsPolygon(polyline[i], polyline[i+1], polygon)) {
            return true;
        }
    }

    return false;
}

// ============================================================================
// Polygon Hole Validation
// ============================================================================

bool isHoleContained(const QList<QGeoCoordinate> &hole,
                      const QList<QGeoCoordinate> &outerRing)
{
    if (hole.size() < 3 || outerRing.size() < 3) {
        return false;
    }

    // All hole vertices must be inside outer ring
    for (const auto &p : hole) {
        if (!pointInPolygon(p, outerRing)) {
            return false;
        }
    }

    // Hole edges must not intersect outer ring
    const int holeN = static_cast<int>(hole.size());
    const int outerN = static_cast<int>(outerRing.size());

    for (int i = 0; i < holeN; ++i) {
        const int iNext = (i + 1) % holeN;
        for (int j = 0; j < outerN; ++j) {
            const int jNext = (j + 1) % outerN;
            double t;
            if (segmentIntersection2D(
                    hole[i].longitude(), hole[i].latitude(),
                    hole[iNext].longitude(), hole[iNext].latitude(),
                    outerRing[j].longitude(), outerRing[j].latitude(),
                    outerRing[jNext].longitude(), outerRing[jNext].latitude(), t)) {
                return false;
            }
        }
    }

    return true;
}

bool validateHoles(const QList<QGeoCoordinate> &outerRing,
                    const QList<QList<QGeoCoordinate>> &holes,
                    QString &errorString)
{
    errorString.clear();

    for (int i = 0; i < holes.size(); ++i) {
        const auto &hole = holes[i];

        // Check hole has enough vertices
        if (hole.size() < 3) {
            errorString = QObject::tr("Hole %1 has fewer than 3 vertices").arg(i);
            return false;
        }

        // Check hole is contained
        if (!isHoleContained(hole, outerRing)) {
            errorString = QObject::tr("Hole %1 is not fully contained in outer ring or intersects boundary").arg(i);
            return false;
        }

        // Check hole doesn't self-intersect
        if (isSelfIntersecting(hole)) {
            errorString = QObject::tr("Hole %1 is self-intersecting").arg(i);
            return false;
        }

        // Check hole doesn't overlap other holes
        for (int j = i + 1; j < holes.size(); ++j) {
            // Check if any vertex of hole i is inside hole j or vice versa
            for (const auto &p : hole) {
                if (pointInPolygon(p, holes[j])) {
                    errorString = QObject::tr("Holes %1 and %2 overlap").arg(i).arg(j);
                    return false;
                }
            }
            for (const auto &p : holes[j]) {
                if (pointInPolygon(p, hole)) {
                    errorString = QObject::tr("Holes %1 and %2 overlap").arg(i).arg(j);
                    return false;
                }
            }
        }
    }

    return true;
}

// ============================================================================
// Great Circle Intersection
// ============================================================================

bool geodesicSegmentsIntersect(const QGeoCoordinate &line1Start,
                                const QGeoCoordinate &line1End,
                                const QGeoCoordinate &line2Start,
                                const QGeoCoordinate &line2End)
{
    // Use 2D approximation for small segments
    double t;
    return segmentIntersection2D(
        line1Start.longitude(), line1Start.latitude(),
        line1End.longitude(), line1End.latitude(),
        line2Start.longitude(), line2Start.latitude(),
        line2End.longitude(), line2End.latitude(), t);
}

int greatCircleIntersection(const QGeoCoordinate &line1Start,
                             const QGeoCoordinate &line1End,
                             const QGeoCoordinate &line2Start,
                             const QGeoCoordinate &line2End,
                             QGeoCoordinate &intersection1,
                             QGeoCoordinate &intersection2)
{
    intersection1 = QGeoCoordinate();
    intersection2 = QGeoCoordinate();

    // Convert to ECEF vectors
    auto toECEF = [](const QGeoCoordinate &c) -> QVector3D {
        const double lat = c.latitude() * M_PI / 180.0;
        const double lon = c.longitude() * M_PI / 180.0;
        return QVector3D(
            static_cast<float>(std::cos(lat) * std::cos(lon)),
            static_cast<float>(std::cos(lat) * std::sin(lon)),
            static_cast<float>(std::sin(lat))
        );
    };

    auto fromECEF = [](const QVector3D &v) -> QGeoCoordinate {
        const double lat = std::atan2(v.z(), std::sqrt(v.x()*v.x() + v.y()*v.y())) * 180.0 / M_PI;
        const double lon = std::atan2(v.y(), v.x()) * 180.0 / M_PI;
        return QGeoCoordinate(lat, lon);
    };

    // Great circle normal vectors (cross product of endpoints)
    QVector3D p1 = toECEF(line1Start);
    QVector3D p2 = toECEF(line1End);
    QVector3D p3 = toECEF(line2Start);
    QVector3D p4 = toECEF(line2End);

    QVector3D n1 = QVector3D::crossProduct(p1, p2).normalized();
    QVector3D n2 = QVector3D::crossProduct(p3, p4).normalized();

    // Intersection line direction (cross product of normals)
    QVector3D lineDir = QVector3D::crossProduct(n1, n2);

    if (lineDir.length() < 1e-6f) {
        return 0;  // Circles are coincident or parallel
    }

    lineDir.normalize();

    // Two antipodal intersection points
    QGeoCoordinate i1 = fromECEF(lineDir);
    QGeoCoordinate i2 = fromECEF(-lineDir);

    // Check if intersections lie on both segments
    auto isOnSegment = [](const QGeoCoordinate &p, const QGeoCoordinate &segStart, const QGeoCoordinate &segEnd) -> bool {
        const double segLen = QGCGeo::geodesicDistance(segStart, segEnd);
        const double d1 = QGCGeo::geodesicDistance(segStart, p);
        const double d2 = QGCGeo::geodesicDistance(p, segEnd);
        return std::abs(d1 + d2 - segLen) < segLen * 0.01;  // 1% tolerance
    };

    int count = 0;

    if (isOnSegment(i1, line1Start, line1End) && isOnSegment(i1, line2Start, line2End)) {
        intersection1 = i1;
        ++count;
    }

    if (isOnSegment(i2, line1Start, line1End) && isOnSegment(i2, line2Start, line2End)) {
        if (count == 0) {
            intersection1 = i2;
        } else {
            intersection2 = i2;
        }
        ++count;
    }

    return count;
}

// ============================================================================
// Cross-Track and Along-Track Distance
// ============================================================================

double crossTrackDistance(const QGeoCoordinate &point,
                           const QGeoCoordinate &pathStart,
                           const QGeoCoordinate &pathEnd)
{
    const double pathLength = QGCGeo::geodesicDistance(pathStart, pathEnd);
    if (pathLength < 1e-10) {
        return QGCGeo::geodesicDistance(point, pathStart);
    }

    // Calculate bearing from path start to path end and to point
    const double bearingToEnd = QGCGeo::geodesicAzimuth(pathStart, pathEnd);
    const double bearingToPoint = QGCGeo::geodesicAzimuth(pathStart, point);
    const double distToPoint = QGCGeo::geodesicDistance(pathStart, point);

    // Angular difference determines which side of the path
    double angleDiff = bearingToPoint - bearingToEnd;
    while (angleDiff > 180.0) { angleDiff -= 360.0; }
    while (angleDiff < -180.0) { angleDiff += 360.0; }

    // Cross-track distance with sign (positive = right, negative = left)
    return distToPoint * std::sin(angleDiff * M_PI / 180.0);
}

double alongTrackDistance(const QGeoCoordinate &point,
                           const QGeoCoordinate &pathStart,
                           const QGeoCoordinate &pathEnd)
{
    const double pathLength = QGCGeo::geodesicDistance(pathStart, pathEnd);
    if (pathLength < 1e-10) {
        return 0.0;
    }

    const double bearingToEnd = QGCGeo::geodesicAzimuth(pathStart, pathEnd);
    const double bearingToPoint = QGCGeo::geodesicAzimuth(pathStart, point);
    const double distToPoint = QGCGeo::geodesicDistance(pathStart, point);

    double angleDiff = bearingToPoint - bearingToEnd;
    while (angleDiff > 180.0) { angleDiff -= 360.0; }
    while (angleDiff < -180.0) { angleDiff += 360.0; }

    // Along-track distance (projection onto path)
    return distToPoint * std::cos(angleDiff * M_PI / 180.0);
}

CrossTrackResult crossTrackDistanceToPath(const QGeoCoordinate &point,
                                           const QList<QGeoCoordinate> &path)
{
    CrossTrackResult result;
    result.crossTrackMeters = std::numeric_limits<double>::max();
    result.alongTrackMeters = 0.0;
    result.segmentIndex = 0;
    result.closestPoint = path.isEmpty() ? QGeoCoordinate() : path.first();

    if (path.size() < 2) {
        if (!path.isEmpty()) {
            result.crossTrackMeters = QGCGeo::geodesicDistance(point, path.first());
        }
        return result;
    }

    double accumulatedDist = 0.0;

    for (int i = 0; i < path.size() - 1; ++i) {
        const double segLen = QGCGeo::geodesicDistance(path[i], path[i + 1]);
        const double alongDist = alongTrackDistance(point, path[i], path[i + 1]);
        const double crossDist = crossTrackDistance(point, path[i], path[i + 1]);

        // Check if projection falls on this segment
        if (alongDist >= 0 && alongDist <= segLen) {
            if (std::abs(crossDist) < std::abs(result.crossTrackMeters)) {
                result.crossTrackMeters = crossDist;
                result.alongTrackMeters = accumulatedDist + alongDist;
                result.segmentIndex = i;
                result.closestPoint = QGCGeo::interpolateAtDistance(path[i], path[i + 1], alongDist);
            }
        } else {
            // Check endpoints
            double distToStart = QGCGeo::geodesicDistance(point, path[i]);
            if (distToStart < std::abs(result.crossTrackMeters)) {
                result.crossTrackMeters = distToStart;
                result.alongTrackMeters = accumulatedDist;
                result.segmentIndex = i;
                result.closestPoint = path[i];
            }
        }

        accumulatedDist += segLen;
    }

    // Check final endpoint
    double distToEnd = QGCGeo::geodesicDistance(point, path.last());
    if (distToEnd < std::abs(result.crossTrackMeters)) {
        result.crossTrackMeters = distToEnd;
        result.alongTrackMeters = accumulatedDist;
        result.segmentIndex = static_cast<int>(path.size()) - 2;
        result.closestPoint = path.last();
    }

    return result;
}

// ============================================================================
// Turn Radius Validation
// ============================================================================

double turnAngle(const QGeoCoordinate &prev,
                  const QGeoCoordinate &current,
                  const QGeoCoordinate &next)
{
    // Calculate bearings
    const double bearingIn = QGCGeo::geodesicAzimuth(prev, current);
    const double bearingOut = QGCGeo::geodesicAzimuth(current, next);

    // Calculate turn angle
    double turn = bearingOut - bearingIn;
    while (turn > 180.0) { turn -= 360.0; }
    while (turn < -180.0) { turn += 360.0; }

    return std::abs(turn);
}

double requiredTurnRadius(const QGeoCoordinate &prev,
                           const QGeoCoordinate &current,
                           const QGeoCoordinate &next)
{
    const double angle = turnAngle(prev, current, next);
    if (angle < 1.0) {
        return 0.0;  // Essentially straight
    }

    // For a given turn angle, the inscribed circle radius that fits
    // in the corner is: r = d * tan(θ/2) where d is distance to the
    // tangent point. The minimum d is the shorter of the two legs.
    const double leg1 = QGCGeo::geodesicDistance(prev, current);
    const double leg2 = QGCGeo::geodesicDistance(current, next);
    const double minLeg = std::min(leg1, leg2);

    // Half-angle in radians
    const double halfAngle = (180.0 - angle) / 2.0 * M_PI / 180.0;

    if (halfAngle < 0.01) {
        return std::numeric_limits<double>::max();  // Near-reversal
    }

    // Required radius = minLeg * tan(halfAngle)
    return minLeg * std::tan(halfAngle);
}

bool isTurnFeasible(const QGeoCoordinate &prev,
                     const QGeoCoordinate &current,
                     const QGeoCoordinate &next,
                     double minTurnRadius)
{
    const double required = requiredTurnRadius(prev, current, next);
    return required >= minTurnRadius;
}

bool validatePathTurns(const QList<QGeoCoordinate> &path,
                        double minTurnRadius,
                        QList<int> &invalidIndices)
{
    invalidIndices.clear();

    if (path.size() < 3) {
        return true;  // No turns to validate
    }

    for (int i = 1; i < path.size() - 1; ++i) {
        if (!isTurnFeasible(path[i - 1], path[i], path[i + 1], minTurnRadius)) {
            invalidIndices.append(i);
        }
    }

    return invalidIndices.isEmpty();
}

// ============================================================================
// Path Densification
// ============================================================================

QList<QGeoCoordinate> densifyPath(const QList<QGeoCoordinate> &coords,
                                   double maxSpacingMeters)
{
    if (coords.size() < 2 || maxSpacingMeters <= 0) {
        return coords;
    }

    QList<QGeoCoordinate> result;
    result.append(coords.first());

    for (int i = 0; i < coords.size() - 1; ++i) {
        const double segLen = QGCGeo::geodesicDistance(coords[i], coords[i + 1]);
        const int numIntermediate = static_cast<int>(std::ceil(segLen / maxSpacingMeters)) - 1;

        if (numIntermediate > 0) {
            const double spacing = segLen / (numIntermediate + 1);
            for (int j = 1; j <= numIntermediate; ++j) {
                result.append(QGCGeo::interpolateAtDistance(coords[i], coords[i + 1], j * spacing));
            }
        }

        result.append(coords[i + 1]);
    }

    return result;
}

QList<QGeoCoordinate> densifyPolygon(const QList<QGeoCoordinate> &vertices,
                                      double maxSpacingMeters)
{
    if (vertices.size() < 3 || maxSpacingMeters <= 0) {
        return vertices;
    }

    QList<QGeoCoordinate> result;
    const int n = static_cast<int>(vertices.size());

    for (int i = 0; i < n; ++i) {
        const int j = (i + 1) % n;
        result.append(vertices[i]);

        const double segLen = QGCGeo::geodesicDistance(vertices[i], vertices[j]);
        const int numIntermediate = static_cast<int>(std::ceil(segLen / maxSpacingMeters)) - 1;

        if (numIntermediate > 0) {
            const double spacing = segLen / (numIntermediate + 1);
            for (int k = 1; k <= numIntermediate; ++k) {
                result.append(QGCGeo::interpolateAtDistance(vertices[i], vertices[j], k * spacing));
            }
        }
    }

    return result;
}

// ============================================================================
// No-Fly Zone / Geofence Checking
// ============================================================================

int pointInNoFlyZone(const QGeoCoordinate &point,
                      const QList<QList<QGeoCoordinate>> &noFlyZones)
{
    for (int i = 0; i < noFlyZones.size(); ++i) {
        if (pointInPolygon(point, noFlyZones[i])) {
            return i;
        }
    }
    return -1;
}

int pathIntersectsNoFlyZone(const QList<QGeoCoordinate> &path,
                             const QList<QList<QGeoCoordinate>> &noFlyZones)
{
    if (path.isEmpty() || noFlyZones.isEmpty()) {
        return -1;
    }

    // Check if any waypoint is inside a zone
    for (const auto &point : path) {
        int zone = pointInNoFlyZone(point, noFlyZones);
        if (zone >= 0) {
            return zone;
        }
    }

    // Check if any path segment crosses a zone boundary
    for (int i = 0; i < path.size() - 1; ++i) {
        for (int z = 0; z < noFlyZones.size(); ++z) {
            if (lineIntersectsPolygon(path[i], path[i + 1], noFlyZones[z])) {
                return z;
            }
        }
    }

    return -1;
}

QList<NoFlyZoneIntersection> findNoFlyZoneIntersections(
    const QList<QGeoCoordinate> &path,
    const QList<QList<QGeoCoordinate>> &noFlyZones)
{
    QList<NoFlyZoneIntersection> intersections;

    if (path.size() < 2 || noFlyZones.isEmpty()) {
        return intersections;
    }

    for (int segIdx = 0; segIdx < path.size() - 1; ++segIdx) {
        for (int zoneIdx = 0; zoneIdx < noFlyZones.size(); ++zoneIdx) {
            const auto &zone = noFlyZones[zoneIdx];
            auto points = linePolygonIntersections(path[segIdx], path[segIdx + 1], zone);

            for (const auto &point : points) {
                NoFlyZoneIntersection nfz;
                nfz.point = point;
                nfz.zoneIndex = zoneIdx;
                nfz.segmentIndex = segIdx;

                // Determine if entering or exiting
                // If next point on segment is inside, we're entering
                const double segLen = QGCGeo::geodesicDistance(path[segIdx], path[segIdx + 1]);
                const double distToIntersection = QGCGeo::geodesicDistance(path[segIdx], point);
                const double epsilon = segLen * 0.01;  // 1% past intersection

                QGeoCoordinate testPoint;
                if (distToIntersection + epsilon < segLen) {
                    testPoint = QGCGeo::interpolateAtDistance(path[segIdx], path[segIdx + 1],
                                                               distToIntersection + epsilon);
                } else {
                    testPoint = path[segIdx + 1];
                }

                nfz.entering = pointInPolygon(testPoint, zone);
                intersections.append(nfz);
            }
        }
    }

    return intersections;
}

// ============================================================================
// Polygon Triangulation (Ear Clipping)
// ============================================================================

namespace {
    // Check if a point is inside a triangle (using same cross product method)
    bool pointInTriangle2D(double px, double py,
                            double ax, double ay,
                            double bx, double by,
                            double cx, double cy)
    {
        auto sign = [](double x1, double y1, double x2, double y2, double x3, double y3) {
            return (x1 - x3) * (y2 - y3) - (x2 - x3) * (y1 - y3);
        };

        double d1 = sign(px, py, ax, ay, bx, by);
        double d2 = sign(px, py, bx, by, cx, cy);
        double d3 = sign(px, py, cx, cy, ax, ay);

        bool hasNeg = (d1 < 0) || (d2 < 0) || (d3 < 0);
        bool hasPos = (d1 > 0) || (d2 > 0) || (d3 > 0);

        return !(hasNeg && hasPos);
    }

    // Check if vertex at index i is an "ear" (can be clipped)
    bool isEar(const QList<QGeoCoordinate> &vertices, const QList<int> &indices, int i)
    {
        const int n = static_cast<int>(indices.size());
        if (n < 3) return false;

        const int prevIdx = indices[(i + n - 1) % n];
        const int currIdx = indices[i];
        const int nextIdx = indices[(i + 1) % n];

        const QGeoCoordinate &prev = vertices[prevIdx];
        const QGeoCoordinate &curr = vertices[currIdx];
        const QGeoCoordinate &next = vertices[nextIdx];

        // Check if the triangle is convex (counter-clockwise)
        double cross = (curr.longitude() - prev.longitude()) * (next.latitude() - prev.latitude()) -
                       (curr.latitude() - prev.latitude()) * (next.longitude() - prev.longitude());

        if (cross <= 0) {
            return false;  // Concave or collinear
        }

        // Check if any other vertex is inside this triangle
        for (int j = 0; j < n; ++j) {
            if (j == (i + n - 1) % n || j == i || j == (i + 1) % n) {
                continue;
            }

            const QGeoCoordinate &p = vertices[indices[j]];
            if (pointInTriangle2D(p.longitude(), p.latitude(),
                                   prev.longitude(), prev.latitude(),
                                   curr.longitude(), curr.latitude(),
                                   next.longitude(), next.latitude())) {
                return false;
            }
        }

        return true;
    }
}

QList<Triangle> triangulatePolygon(const QList<QGeoCoordinate> &vertices)
{
    QList<Triangle> triangles;

    if (vertices.size() < 3) {
        return triangles;
    }

    // Create index list
    QList<int> indices;
    const int n = static_cast<int>(vertices.size());

    // Ensure counter-clockwise order for ear clipping
    if (isClockwise(vertices)) {
        for (int i = n - 1; i >= 0; --i) {
            indices.append(i);
        }
    } else {
        for (int i = 0; i < n; ++i) {
            indices.append(i);
        }
    }

    // Ear clipping
    int failsafe = 0;
    const int maxIterations = n * n;  // Prevent infinite loops

    while (indices.size() > 3 && failsafe < maxIterations) {
        bool earFound = false;

        for (int i = 0; i < indices.size(); ++i) {
            if (isEar(vertices, indices, i)) {
                // Add triangle
                Triangle tri;
                tri.a = indices[(i + indices.size() - 1) % indices.size()];
                tri.b = indices[i];
                tri.c = indices[(i + 1) % indices.size()];
                triangles.append(tri);

                // Remove ear vertex
                indices.removeAt(i);
                earFound = true;
                break;
            }
        }

        if (!earFound) {
            break;  // No ear found - polygon may be degenerate
        }

        ++failsafe;
    }

    // Add final triangle
    if (indices.size() == 3) {
        Triangle tri;
        tri.a = indices[0];
        tri.b = indices[1];
        tri.c = indices[2];
        triangles.append(tri);
    }

    return triangles;
}

QList<Triangle> triangulatePolygonWithHoles(
    const QList<QGeoCoordinate> &outerRing,
    const QList<QList<QGeoCoordinate>> &holes,
    QList<QGeoCoordinate> &allVertices)
{
    // For simplicity, this implementation merges holes into outer ring
    // using a basic bridge insertion technique
    allVertices = outerRing;

    // For each hole, find the rightmost point and connect to outer ring
    for (const auto &hole : holes) {
        if (hole.size() < 3) continue;

        // Find rightmost point in hole
        int holeRightmost = 0;
        for (int i = 1; i < hole.size(); ++i) {
            if (hole[i].longitude() > hole[holeRightmost].longitude()) {
                holeRightmost = i;
            }
        }

        // Find closest visible point on outer ring
        int outerClosest = 0;
        double minDist = std::numeric_limits<double>::max();
        for (int i = 0; i < allVertices.size(); ++i) {
            double dist = QGCGeo::geodesicDistance(hole[holeRightmost], allVertices[i]);
            if (dist < minDist) {
                minDist = dist;
                outerClosest = i;
            }
        }

        // Insert hole vertices into allVertices after outerClosest
        const int holeStartIdx = static_cast<int>(allVertices.size());
        const int holeSize = static_cast<int>(hole.size());

        // Add bridge from outer to hole
        allVertices.insert(outerClosest + 1, allVertices[outerClosest]);

        // Add hole vertices starting from rightmost
        for (int i = 0; i < holeSize; ++i) {
            allVertices.insert(outerClosest + 2 + i, hole[(holeRightmost + i) % holeSize]);
        }

        // Add bridge back
        allVertices.insert(outerClosest + 2 + holeSize, hole[holeRightmost]);
    }

    return triangulatePolygon(allVertices);
}

// ============================================================================
// Plus Codes (Open Location Code)
// ============================================================================

namespace {
    // Plus Code alphabet (excludes easily confused characters)
    const char kPlusCodeAlphabet[] = "23456789CFGHJMPQRVWX";
    const int kPlusCodeBase = 20;

    // Encoding precision at each pair position
    const double kPlusCodeLatPrecision[] = {20.0, 1.0, 0.05, 0.0025, 0.000125};
    const double kPlusCodeLonPrecision[] = {20.0, 1.0, 0.05, 0.0025, 0.000125};
}

QString coordinateToPlusCode(const QGeoCoordinate &coord, int codeLength)
{
    if (!coord.isValid() || codeLength < 2) {
        return QString();
    }

    // Normalize and shift coordinates to positive values
    double lat = std::clamp(coord.latitude(), -90.0, 90.0) + 90.0;
    double lon = coord.longitude();
    while (lon < -180.0) lon += 360.0;
    while (lon >= 180.0) lon -= 360.0;
    lon += 180.0;

    QString code;
    int pairs = std::min(codeLength / 2, 5);

    for (int i = 0; i < pairs; ++i) {
        double latPrec = (i == 0) ? 20.0 : 20.0 / std::pow(kPlusCodeBase, i);
        double lonPrec = (i == 0) ? 20.0 : 20.0 / std::pow(kPlusCodeBase, i);

        int latDigit = static_cast<int>(lat / latPrec) % kPlusCodeBase;
        int lonDigit = static_cast<int>(lon / lonPrec) % kPlusCodeBase;

        code += kPlusCodeAlphabet[latDigit];
        code += kPlusCodeAlphabet[lonDigit];

        lat = std::fmod(lat, latPrec);
        lon = std::fmod(lon, lonPrec);

        if (i == 3) {
            code += '+';  // Separator after 8 characters
        }
    }

    // Pad with zeros if needed
    while (code.length() < 8) {
        code += '0';
    }
    if (!code.contains('+')) {
        code.insert(8, '+');
    }

    // Add refinement characters if code length > 10
    if (codeLength > 10) {
        double gridLat = lat * 5.0;  // 5 rows
        double gridLon = lon * 4.0;  // 4 columns

        for (int i = 10; i < codeLength; ++i) {
            int latGrid = static_cast<int>(gridLat) % 5;
            int lonGrid = static_cast<int>(gridLon) % 4;
            code += kPlusCodeAlphabet[latGrid * 4 + lonGrid];
            gridLat = std::fmod(gridLat, 1.0) * 5.0;
            gridLon = std::fmod(gridLon, 1.0) * 4.0;
        }
    }

    return code;
}

bool plusCodeToCoordinate(const QString &plusCode, QGeoCoordinate &coord)
{
    coord = QGeoCoordinate();

    if (!isValidPlusCode(plusCode)) {
        return false;
    }

    QString code = plusCode.toUpper().remove(' ');

    // Remove padding zeros
    code.replace(QStringLiteral("0"), QString());

    double lat = -90.0;
    double lon = -180.0;

    int charIndex = 0;
    for (int i = 0; i < 5 && charIndex < code.length(); ++i) {
        if (code[charIndex] == '+') {
            ++charIndex;
            if (charIndex >= code.length()) break;
        }

        // Find character in alphabet
        int latDigit = -1, lonDigit = -1;
        for (int j = 0; j < kPlusCodeBase; ++j) {
            if (code[charIndex].toLatin1() == kPlusCodeAlphabet[j]) {
                latDigit = j;
                break;
            }
        }
        ++charIndex;
        if (charIndex >= code.length()) break;

        if (code[charIndex] == '+') {
            ++charIndex;
            if (charIndex >= code.length()) break;
        }

        for (int j = 0; j < kPlusCodeBase; ++j) {
            if (code[charIndex].toLatin1() == kPlusCodeAlphabet[j]) {
                lonDigit = j;
                break;
            }
        }
        ++charIndex;

        if (latDigit < 0 || lonDigit < 0) {
            return false;
        }

        double latPrec = (i == 0) ? 20.0 : 20.0 / std::pow(kPlusCodeBase, i);
        double lonPrec = (i == 0) ? 20.0 : 20.0 / std::pow(kPlusCodeBase, i);

        lat += latDigit * latPrec;
        lon += lonDigit * lonPrec;
    }

    // Return center of the code area
    coord = QGeoCoordinate(lat + 0.000125, lon + 0.000125);  // Approximate center
    return true;
}

bool isValidPlusCode(const QString &code)
{
    if (code.isEmpty()) {
        return false;
    }

    QString upper = code.toUpper().remove(' ');

    // Must contain exactly one '+'
    if (upper.count('+') != 1) {
        return false;
    }

    // '+' must be at position 8 or earlier (for short codes)
    int plusPos = upper.indexOf('+');
    if (plusPos > 8) {
        return false;
    }

    // All characters before '+' must be valid (or '0' for padding)
    for (int i = 0; i < plusPos; ++i) {
        char c = upper[i].toLatin1();
        bool valid = (c == '0');
        for (int j = 0; j < kPlusCodeBase && !valid; ++j) {
            if (c == kPlusCodeAlphabet[j]) valid = true;
        }
        if (!valid) return false;
    }

    // All characters after '+' must be valid
    for (int i = plusPos + 1; i < upper.length(); ++i) {
        char c = upper[i].toLatin1();
        bool valid = false;
        for (int j = 0; j < kPlusCodeBase && !valid; ++j) {
            if (c == kPlusCodeAlphabet[j]) valid = true;
        }
        if (!valid) return false;
    }

    return true;
}

// ============================================================================
// Path Corridor
// ============================================================================

QList<QGeoCoordinate> pathCorridor(const QList<QGeoCoordinate> &path,
                                    double widthMeters)
{
    if (path.size() < 2 || widthMeters <= 0) {
        return QList<QGeoCoordinate>();
    }

    const double halfWidth = widthMeters / 2.0;

    // Generate left and right offset paths
    QList<QGeoCoordinate> leftPath = offsetPolyline(path, halfWidth);
    QList<QGeoCoordinate> rightPath = offsetPolyline(path, -halfWidth);

    // Combine into a closed polygon: left forward, right backward
    QList<QGeoCoordinate> corridor;
    corridor.reserve(leftPath.size() + rightPath.size());

    for (const auto &p : leftPath) {
        corridor.append(p);
    }

    // Add right path in reverse
    for (int i = static_cast<int>(rightPath.size()) - 1; i >= 0; --i) {
        corridor.append(rightPath[i]);
    }

    return corridor;
}

bool pointInCorridor(const QGeoCoordinate &point,
                      const QList<QGeoCoordinate> &path,
                      double widthMeters)
{
    if (path.size() < 2) {
        return false;
    }

    // Quick check: is point within half-width of any segment?
    const double halfWidth = widthMeters / 2.0;

    for (int i = 0; i < path.size() - 1; ++i) {
        double xtd = std::abs(crossTrackDistance(point, path[i], path[i + 1]));
        double atd = alongTrackDistance(point, path[i], path[i + 1]);
        double segLen = QGCGeo::geodesicDistance(path[i], path[i + 1]);

        // Point is in corridor if within half-width and projects onto segment
        if (xtd <= halfWidth && atd >= 0 && atd <= segLen) {
            return true;
        }
    }

    // Also check endpoints with radius
    if (QGCGeo::geodesicDistance(point, path.first()) <= halfWidth) {
        return true;
    }
    if (QGCGeo::geodesicDistance(point, path.last()) <= halfWidth) {
        return true;
    }

    return false;
}

} // namespace GeoUtilities
