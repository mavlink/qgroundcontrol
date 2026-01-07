/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCGeo.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QString>

#include <cmath>

#include <GeographicLib/Geocentric.hpp>
#include <GeographicLib/Geodesic.hpp>
#include <GeographicLib/GeodesicLine.hpp>
#include <GeographicLib/LocalCartesian.hpp>
#include <GeographicLib/MGRS.hpp>
#include <GeographicLib/PolygonArea.hpp>
#include <GeographicLib/UTMUPS.hpp>

QGC_LOGGING_CATEGORY(QGCGeoLog, "Utilities.QGCGeo")

namespace QGCGeo
{

// ============================================================================
// NED (North-East-Down) Local Tangent Plane
// ============================================================================

void convertGeoToNed(const QGeoCoordinate &coord, const QGeoCoordinate &origin, double &x, double &y, double &z)
{
    if (coord == origin) {
        x = y = z = 0.0;
        return;
    }

    // Handle NaN altitude (QGeoCoordinate without altitude set)
    const double originAlt = std::isnan(origin.altitude()) ? 0.0 : origin.altitude();
    const double coordAlt = std::isnan(coord.altitude()) ? 0.0 : coord.altitude();

    double east, north, up;
    GeographicLib::LocalCartesian ltp(origin.latitude(), origin.longitude(), originAlt,
                                      GeographicLib::Geocentric::WGS84());
    ltp.Forward(coord.latitude(), coord.longitude(), coordAlt, east, north, up);

    // Convert ENU to NED
    x = north;
    y = east;
    z = -up;
}

void convertNedToGeo(double x, double y, double z, const QGeoCoordinate &origin, QGeoCoordinate &coord)
{
    // Convert NED to ENU
    const double east = y;
    const double north = x;
    const double up = -z;

    // Handle NaN altitude
    const double originAlt = std::isnan(origin.altitude()) ? 0.0 : origin.altitude();

    double lat, lon, alt;
    GeographicLib::LocalCartesian ltp(origin.latitude(), origin.longitude(), originAlt,
                                      GeographicLib::Geocentric::WGS84());
    ltp.Reverse(east, north, up, lat, lon, alt);

    coord.setLatitude(lat);
    coord.setLongitude(lon);
    coord.setAltitude(alt);
}

// ============================================================================
// ENU (East-North-Up) Local Tangent Plane
// ============================================================================

QVector3D convertGpsToEnu(const QGeoCoordinate &coord, const QGeoCoordinate &ref)
{
    double x, y, z;
    GeographicLib::LocalCartesian ltp(ref.latitude(), ref.longitude(), ref.altitude(),
                                      GeographicLib::Geocentric::WGS84());
    ltp.Forward(coord.latitude(), coord.longitude(), coord.altitude(), x, y, z);
    return QVector3D(x, y, z);
}

QGeoCoordinate convertEnuToGps(const QVector3D &enu, const QGeoCoordinate &ref)
{
    double lat, lon, alt;
    GeographicLib::LocalCartesian ltp(ref.latitude(), ref.longitude(), ref.altitude(),
                                      GeographicLib::Geocentric::WGS84());
    ltp.Reverse(enu.x(), enu.y(), enu.z(), lat, lon, alt);
    return QGeoCoordinate(lat, lon, alt);
}

// ============================================================================
// ECEF (Earth-Centered Earth-Fixed)
// ============================================================================

QVector3D convertGeodeticToEcef(const QGeoCoordinate &coord)
{
    double x, y, z;
    GeographicLib::Geocentric::WGS84().Forward(coord.latitude(), coord.longitude(), coord.altitude(), x, y, z);
    return QVector3D(x, y, z);
}

QGeoCoordinate convertEcefToGeodetic(const QVector3D &ecef)
{
    double lat, lon, alt;
    GeographicLib::Geocentric::WGS84().Reverse(ecef.x(), ecef.y(), ecef.z(), lat, lon, alt);
    return QGeoCoordinate(lat, lon, alt);
}

QVector3D convertEcefToEnu(const QVector3D &ecef, const QGeoCoordinate &ref)
{
    // ECEF -> Geodetic
    double lat, lon, h;
    GeographicLib::Geocentric::WGS84().Reverse(ecef.x(), ecef.y(), ecef.z(), lat, lon, h);

    // Geodetic -> ENU
    double x, y, z;
    GeographicLib::LocalCartesian ltp(ref.latitude(), ref.longitude(), ref.altitude(),
                                      GeographicLib::Geocentric::WGS84());
    ltp.Forward(lat, lon, h, x, y, z);
    return QVector3D(x, y, z);
}

QVector3D convertEnuToEcef(const QVector3D &enu, const QGeoCoordinate &ref)
{
    // ENU -> Geodetic
    double lat, lon, h;
    GeographicLib::LocalCartesian ltp(ref.latitude(), ref.longitude(), ref.altitude(),
                                      GeographicLib::Geocentric::WGS84());
    ltp.Reverse(enu.x(), enu.y(), enu.z(), lat, lon, h);

    // Geodetic -> ECEF
    double x, y, z;
    GeographicLib::Geocentric::WGS84().Forward(lat, lon, h, x, y, z);
    return QVector3D(x, y, z);
}

// ============================================================================
// UTM (Universal Transverse Mercator)
// ============================================================================

int convertGeoToUTM(const QGeoCoordinate &coord, double &easting, double &northing)
{
    try {
        int zone;
        bool northp;
        GeographicLib::UTMUPS::Forward(coord.latitude(), coord.longitude(), zone, northp, easting, northing);
        return zone;
    } catch (const GeographicLib::GeographicErr &e) {
        qCDebug(QGCGeoLog) << e.what();
        return 0;
    }
}

bool convertUTMToGeo(double easting, double northing, int zone, bool southhemi, QGeoCoordinate &coord)
{
    try {
        double lat, lon;
        GeographicLib::UTMUPS::Reverse(zone, !southhemi, easting, northing, lat, lon);
        coord.setLatitude(lat);
        coord.setLongitude(lon);
        return true;
    } catch (const GeographicLib::GeographicErr &e) {
        qCDebug(QGCGeoLog) << e.what();
        return false;
    }
}

// ============================================================================
// MGRS (Military Grid Reference System)
// ============================================================================

QString convertGeoToMGRS(const QGeoCoordinate &coord)
{
    try {
        int zone;
        bool northp;
        double x, y;
        GeographicLib::UTMUPS::Forward(coord.latitude(), coord.longitude(), zone, northp, x, y);

        std::string mgrs;
        GeographicLib::MGRS::Forward(zone, northp, x, y, coord.latitude(), 5, mgrs);

        // Format with spaces: "32TMT6588647092" -> "32TMT 65886 47092"
        const QString qstr = QString::fromStdString(mgrs);
        for (int i = qstr.length() - 1; i >= 0; i--) {
            if (!qstr.at(i).isDigit()) {
                const int numLen = (qstr.length() - i - 1) / 2;
                return qstr.left(i + 1) + " " + qstr.mid(i + 1, numLen) + " " + qstr.mid(i + 1 + numLen);
            }
        }
        return qstr;
    } catch (const GeographicLib::GeographicErr &e) {
        qCDebug(QGCGeoLog) << e.what();
        return QString();
    }
}

bool convertMGRSToGeo(const QString &mgrs, QGeoCoordinate &coord)
{
    try {
        int zone, prec;
        bool northp;
        double x, y;
        GeographicLib::MGRS::Reverse(mgrs.simplified().replace(" ", "").toStdString(), zone, northp, x, y, prec);

        double lat, lon;
        GeographicLib::UTMUPS::Reverse(zone, northp, x, y, lat, lon);

        coord.setLatitude(lat);
        coord.setLongitude(lon);
        return true;
    } catch (const GeographicLib::GeographicErr &e) {
        qCDebug(QGCGeoLog) << e.what();
        return false;
    }
}

// ============================================================================
// Geodesic Calculations (Great Circle on Ellipsoid)
// ============================================================================

double geodesicDistance(const QGeoCoordinate &from, const QGeoCoordinate &to)
{
    double distance;
    GeographicLib::Geodesic::WGS84().Inverse(from.latitude(), from.longitude(),
                                             to.latitude(), to.longitude(), distance);
    return distance;
}

double geodesicAzimuth(const QGeoCoordinate &from, const QGeoCoordinate &to)
{
    double distance, azimuth1, azimuth2;
    GeographicLib::Geodesic::WGS84().Inverse(from.latitude(), from.longitude(),
                                             to.latitude(), to.longitude(), distance, azimuth1, azimuth2);

    // Normalize to [0, 360)
    if (azimuth1 < 0.0) {
        azimuth1 += 360.0;
    }
    return azimuth1;
}

QGeoCoordinate geodesicDestination(const QGeoCoordinate &from, double azimuth, double distance)
{
    double lat, lon;
    GeographicLib::Geodesic::WGS84().Direct(from.latitude(), from.longitude(), azimuth, distance, lat, lon);
    return QGeoCoordinate(lat, lon, from.altitude());
}

// ============================================================================
// Path and Polygon Calculations
// ============================================================================

double pathLength(const QList<QGeoCoordinate> &path)
{
    if (path.size() < 2) {
        return 0.0;
    }

    double totalLength = 0.0;
    for (int i = 1; i < path.size(); ++i) {
        totalLength += geodesicDistance(path[i - 1], path[i]);
    }
    return totalLength;
}

double polygonArea(const QList<QGeoCoordinate> &polygon)
{
    if (polygon.size() < 3) {
        return 0.0;
    }

    GeographicLib::PolygonArea poly(GeographicLib::Geodesic::WGS84());
    for (const QGeoCoordinate &coord : polygon) {
        poly.AddPoint(coord.latitude(), coord.longitude());
    }

    double perimeter, area;
    poly.Compute(false, true, perimeter, area);
    // Area sign indicates winding order (positive = counter-clockwise, negative = clockwise).
    // Return absolute value since we only care about magnitude.
    return qAbs(area);
}

double polygonPerimeter(const QList<QGeoCoordinate> &polygon)
{
    if (polygon.size() < 2) {
        return 0.0;
    }

    GeographicLib::PolygonArea poly(GeographicLib::Geodesic::WGS84());
    for (const QGeoCoordinate &coord : polygon) {
        poly.AddPoint(coord.latitude(), coord.longitude());
    }

    double perimeter, area;
    poly.Compute(false, true, perimeter, area);
    return perimeter;
}

QList<QGeoCoordinate> interpolatePath(const QGeoCoordinate &from, const QGeoCoordinate &to, int numPoints)
{
    QList<QGeoCoordinate> result;

    // Clamp to reasonable bounds to prevent excessive memory allocation
    constexpr int kMaxPoints = 10000;
    if (numPoints < 2) {
        numPoints = 2;
    } else if (numPoints > kMaxPoints) {
        qCWarning(QGCGeoLog) << "interpolatePath: numPoints" << numPoints << "exceeds maximum, clamping to" << kMaxPoints;
        numPoints = kMaxPoints;
    }

    if (from == to) {
        for (int i = 0; i < numPoints; ++i) {
            result.append(from);
        }
        return result;
    }

    // Create GeodesicLine for efficient multi-point interpolation
    const GeographicLib::GeodesicLine line = GeographicLib::Geodesic::WGS84().InverseLine(
        from.latitude(), from.longitude(), to.latitude(), to.longitude());

    const double totalDistance = line.Distance();
    const double altDiff = to.altitude() - from.altitude();

    for (int i = 0; i < numPoints; ++i) {
        const double fraction = static_cast<double>(i) / (numPoints - 1);
        const double distance = totalDistance * fraction;

        double lat, lon;
        line.Position(distance, lat, lon);

        const double alt = from.altitude() + altDiff * fraction;
        result.append(QGeoCoordinate(lat, lon, alt));
    }

    return result;
}

QGeoCoordinate interpolateAtDistance(const QGeoCoordinate &from, const QGeoCoordinate &to, double distance)
{
    if (from == to || distance <= 0.0) {
        return from;
    }

    const GeographicLib::GeodesicLine line = GeographicLib::Geodesic::WGS84().InverseLine(
        from.latitude(), from.longitude(), to.latitude(), to.longitude());

    const double totalDistance = line.Distance();

    if (distance >= totalDistance) {
        return to;
    }

    double lat, lon;
    line.Position(distance, lat, lon);

    // Linear altitude interpolation
    const double fraction = distance / totalDistance;
    const double alt = from.altitude() + (to.altitude() - from.altitude()) * fraction;

    return QGeoCoordinate(lat, lon, alt);
}

} // namespace QGCGeo
