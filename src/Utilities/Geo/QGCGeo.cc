/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

// TODO: Use GeoCoords from GeographicLib?

#include "QGCGeo.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QString>
#include <QtCore/QtMath>

#include <GeographicLib/Constants.hpp>
#include <GeographicLib/MGRS.hpp>
#include <GeographicLib/UTMUPS.hpp>

#include <limits>

QGC_LOGGING_CATEGORY(QGCGeoLog, "qgc.geo.qgcgeo")

static constexpr double epsilon = std::numeric_limits<double>::epsilon();

namespace QGCGeo {

void convertGeoToNed(const QGeoCoordinate &coord, const QGeoCoordinate &origin, double &x, double &y, double &z)
{
    if (coord == origin) {
        // Prevent NaNs in calculation
        x = y = z = 0;
        return;
    }

    const double lat_rad = qDegreesToRadians(coord.latitude());
    const double lon_rad = qDegreesToRadians(coord.longitude());

    const double ref_lon_rad = qDegreesToRadians(origin.longitude());
    const double ref_lat_rad = qDegreesToRadians(origin.latitude());

    const double sin_lat = sin(lat_rad);
    const double cos_lat = cos(lat_rad);
    const double cos_d_lon = cos(lon_rad - ref_lon_rad);

    const double ref_sin_lat = sin(ref_lat_rad);
    const double ref_cos_lat = cos(ref_lat_rad);

    const double c = acos(ref_sin_lat * sin_lat + ref_cos_lat * cos_lat * cos_d_lon);
    const double k = (fabs(c) < epsilon) ? 1.0 : (c / sin(c));

    x = k * (ref_cos_lat * sin_lat - ref_sin_lat * cos_lat * cos_d_lon) * GeographicLib::Constants::WGS84_a();
    y = k * cos_lat * sin(lon_rad - ref_lon_rad) * GeographicLib::Constants::WGS84_a();
    z = -(coord.altitude() - origin.altitude());
}

void convertNedToGeo(double x, double y, double z, const QGeoCoordinate &origin, QGeoCoordinate &coord)
{
    const double x_rad = x / GeographicLib::Constants::WGS84_a();
    const double y_rad = y / GeographicLib::Constants::WGS84_a();
    const double c = sqrt(x_rad * x_rad + y_rad * y_rad);
    const double sin_c = sin(c);
    const double cos_c = cos(c);

    const double ref_lon_rad = qDegreesToRadians(origin.longitude());
    const double ref_lat_rad = qDegreesToRadians(origin.latitude());

    const double ref_sin_lat = sin(ref_lat_rad);
    const double ref_cos_lat = cos(ref_lat_rad);

    double lat_rad;
    double lon_rad;

    if (fabs(c) > epsilon) {
        lat_rad = asin(cos_c * ref_sin_lat + (x_rad * sin_c * ref_cos_lat) / c);
        lon_rad = (ref_lon_rad + atan2(y_rad * sin_c, c * ref_cos_lat * cos_c - x_rad * ref_sin_lat * sin_c));
    } else {
        lat_rad = ref_lat_rad;
        lon_rad = ref_lon_rad;
    }

    coord.setLatitude(qRadiansToDegrees(lat_rad));
    coord.setLongitude(qRadiansToDegrees(lon_rad));
    coord.setAltitude(-z + origin.altitude());
}

int convertGeoToUTM(const QGeoCoordinate& coord, double &easting, double &northing)
{
    try {
        int zone;
        bool northp;
        GeographicLib::UTMUPS::Forward(coord.latitude(), coord.longitude(), zone, northp, easting, northing);
        return zone;
    } catch(const GeographicLib::GeographicErr& e) {
        qCDebug(QGCGeoLog) << Q_FUNC_INFO << e.what();
        return 0;
    }
}

bool convertUTMToGeo(double easting, double northing, int zone, bool southhemi, QGeoCoordinate &coord)
{
    double lat, lon;

    try {
        GeographicLib::UTMUPS::Reverse(zone, !southhemi, easting, northing, lat, lon);
    } catch(const GeographicLib::GeographicErr& e) {
        qCDebug(QGCGeoLog) << Q_FUNC_INFO << e.what();
        return false;
    }

    coord.setLatitude(lat);
    coord.setLongitude(lon);

    return true;
}

QString convertGeoToMGRS(const QGeoCoordinate &coord)
{
    std::string mgrs;

    try {
        int zone;
        bool northp;
        double x, y;
        GeographicLib::UTMUPS::Forward(coord.latitude(), coord.longitude(), zone, northp, x, y);
        GeographicLib::MGRS::Forward(zone, northp, x, y, coord.latitude(), 5, mgrs);
    } catch(const GeographicLib::GeographicErr& e) {
        qCDebug(QGCGeoLog) << Q_FUNC_INFO << e.what();
        mgrs = "";
    }

    const QString qstr = QString::fromStdString(mgrs);
    for (int i = qstr.length() - 1; i >= 0; i--) {
        if (!qstr.at(i).isDigit()) {
            const int l = (qstr.length() - i) / 2;
            return qstr.left(i + 1) + " " + qstr.mid(i + 1, l) + " " + qstr.mid(i + 1 + l);
        }
    }

    return qstr;
}

bool convertMGRSToGeo(const QString &mgrs, QGeoCoordinate &coord)
{
    double lat, lon;

    try {
        int zone, prec;
        bool northp;
        double x, y;
        GeographicLib::MGRS::Reverse(mgrs.simplified().replace(" ", "").toStdString(), zone, northp, x, y, prec);
        GeographicLib::UTMUPS::Reverse(zone, northp, x, y, lat, lon);
    } catch(const GeographicLib::GeographicErr& e) {
        qCDebug(QGCGeoLog) << Q_FUNC_INFO << e.what();
        return false;
    }

    coord.setLatitude(lat);
    coord.setLongitude(lon);

    return true;
}

} // namespace QGCGeo
