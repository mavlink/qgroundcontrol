/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QDebug>
#include <QString>

#include <cmath>
#include <limits>

#include "QGCGeo.h"
#include "UTMUPS.hpp"
#include "MGRS.hpp"

// These defines are private
#define M_DEG_TO_RAD (M_PI / 180.0)
#define M_RAD_TO_DEG (180.0 / M_PI)
#define CONSTANTS_RADIUS_OF_EARTH 6371000 // meters (m)

static const double epsilon = std::numeric_limits<double>::epsilon();

void convertGeoToNed(QGeoCoordinate coord, QGeoCoordinate origin, double* x, double* y, double* z)
{
    if (coord == origin) {
        // Short circuit to prevent NaNs in calculation
        *x = *y = *z = 0;
        return;
    }

    double lat_rad = coord.latitude() * M_DEG_TO_RAD;
    double lon_rad = coord.longitude() * M_DEG_TO_RAD;

    double ref_lon_rad = origin.longitude() * M_DEG_TO_RAD;
    double ref_lat_rad = origin.latitude() * M_DEG_TO_RAD;

    double sin_lat = sin(lat_rad);
    double cos_lat = cos(lat_rad);
    double cos_d_lon = cos(lon_rad - ref_lon_rad);

    double ref_sin_lat = sin(ref_lat_rad);
    double ref_cos_lat = cos(ref_lat_rad);

    double c = acos(ref_sin_lat * sin_lat + ref_cos_lat * cos_lat * cos_d_lon);
    double k = (fabs(c) < epsilon) ? 1.0 : (c / sin(c));

    *x = k * (ref_cos_lat * sin_lat - ref_sin_lat * cos_lat * cos_d_lon) * CONSTANTS_RADIUS_OF_EARTH;
    *y = k * cos_lat * sin(lon_rad - ref_lon_rad) * CONSTANTS_RADIUS_OF_EARTH;

    *z = -(coord.altitude() - origin.altitude());
}

void convertNedToGeo(double x, double y, double z, QGeoCoordinate origin, QGeoCoordinate *coord) {
    double x_rad = x / CONSTANTS_RADIUS_OF_EARTH;
    double y_rad = y / CONSTANTS_RADIUS_OF_EARTH;
    double c = sqrt(x_rad * x_rad + y_rad * y_rad);
    double sin_c = sin(c);
    double cos_c = cos(c);

    double ref_lon_rad = origin.longitude() * M_DEG_TO_RAD;
    double ref_lat_rad = origin.latitude() * M_DEG_TO_RAD;

    double ref_sin_lat = sin(ref_lat_rad);
    double ref_cos_lat = cos(ref_lat_rad);

    double lat_rad;
    double lon_rad;

    if (fabs(c) > epsilon) {
        lat_rad = asin(cos_c * ref_sin_lat + (x_rad * sin_c * ref_cos_lat) / c);
        lon_rad = (ref_lon_rad + atan2(y_rad * sin_c, c * ref_cos_lat * cos_c - x_rad * ref_sin_lat * sin_c));

    } else {
        lat_rad = ref_lat_rad;
        lon_rad = ref_lon_rad;
    }

    coord->setLatitude(lat_rad * M_RAD_TO_DEG);
    coord->setLongitude(lon_rad * M_RAD_TO_DEG);

    coord->setAltitude(-z + origin.altitude());
}

int convertGeoToUTM(const QGeoCoordinate& coord, double& easting, double& northing)
{
    try {
        int zone;
        bool northp;
        GeographicLib::UTMUPS::Forward(coord.latitude(), coord.longitude(), zone, northp, easting, northing);
        return zone;
    } catch(...) {
        return 0;
    }
}

bool convertUTMToGeo(double easting, double northing, int zone, bool southhemi, QGeoCoordinate& coord)
{
    double lat, lon;
    try {
        GeographicLib::UTMUPS::Reverse(zone, !southhemi, easting, northing, lat, lon);
    } catch(...) {
        return false;
    }
    coord.setLatitude(lat);
    coord.setLongitude(lon);

    return true;
}

QString convertGeoToMGRS(const QGeoCoordinate& coord)
{
    int zone;
    bool northp;
    double x, y;
    std::string mgrs;

    try {
        GeographicLib::UTMUPS::Forward(coord.latitude(), coord.longitude(), zone, northp, x, y);
        GeographicLib::MGRS::Forward(zone, northp, x, y, coord.latitude(), 5, mgrs);
    } catch(...) {
        mgrs = "";
    }

    QString qstr = QString::fromStdString(mgrs);
    for (int i = qstr.length() - 1; i >= 0; i--) {
        if (!qstr.at(i).isDigit()) {
            int l = (qstr.length() - i) / 2;
            return qstr.left(i + 1) + " " + qstr.mid(i + 1, l) + " " + qstr.mid(i + 1 + l);
        }
    }

    return qstr;
}

bool convertMGRSToGeo(QString mgrs, QGeoCoordinate& coord)
{
    int zone, prec;
    bool northp;
    double x, y;
    double lat, lon;

    try {
        GeographicLib::MGRS::Reverse(mgrs.simplified().replace(" ", "").toStdString(), zone, northp, x, y, prec);
        GeographicLib::UTMUPS::Reverse(zone, northp, x, y, lat, lon);
    } catch(...) {
        return false;
    }
    coord.setLatitude(lat);
    coord.setLongitude(lon);

    return true;
}
