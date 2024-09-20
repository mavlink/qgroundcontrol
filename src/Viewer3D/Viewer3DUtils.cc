/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Viewer3DUtils.h"

// WGS-84 geodetic constants
#define ins_a 					6378137.0         // WGS-84 Earth semimajor axis (m)
#define ins_b 					6356752.314245     // Derived Earth semiminor axis (m)
#define ins_f 					(ins_a - ins_b) / ins_a           // Ellipsoid Flatness
#define ins_f_inv				1.0 / ins_f       // Inverse flattening
#define ins_a_sq 				ins_a * ins_a
#define ins_b_sq 				ins_b * ins_b
#define ins_e_sq 				ins_f * (2 - ins_f)    // Square of Eccentricity

QVector3D mapGeodeticToEcef(QGeoCoordinate gps_point_)
{
    double lat_rad = gps_point_.latitude() * DEG_TO_RAD;
    double lon_rad = gps_point_.longitude() * DEG_TO_RAD;
    double cos_lat = cos(lat_rad);
    double sin_lat = sin(lat_rad);
    double N = ins_a / sqrt(1 - ins_e_sq * sin_lat * sin_lat);

    QVector3D out_point;
    out_point.setX((N + gps_point_.altitude()) * (cos_lat * cos(lon_rad)));
    out_point.setY((N + gps_point_.altitude()) * (cos_lat * sin(lon_rad)));
    out_point.setZ((gps_point_.altitude() + (1 - ins_e_sq) * N) * sin_lat);

    return out_point;
}

QVector3D mapEcefToEnu(QVector3D ecef_point, QGeoCoordinate ref_gps)
{
    // Convert to radians in notation consistent with the paper:
    double lambda = ref_gps.latitude() * DEG_TO_RAD;
    double phi = ref_gps.longitude() * DEG_TO_RAD;

    double sin_lambda = sin(lambda);
    double cos_lambda = cos(lambda);
    double cos_phi = cos(phi);
    double sin_phi = sin(phi);

    double N = ins_a / sqrt(1 - ins_e_sq * sin_lambda * sin_lambda);

    double x0 = (N + ref_gps.altitude() ) * cos_lambda * cos_phi;
    double y0 = (N + ref_gps.altitude() ) * cos_lambda * sin_phi;
    double z0 = (ref_gps.altitude() + (1 - ins_e_sq) * N) * sin_lambda;

    double xd, yd, zd;
    xd = ecef_point.x() - x0;
    yd = ecef_point.y() - y0;
    zd = ecef_point.z() - z0;

    // This is the matrix multiplication
    double xEast = -sin_phi * xd + cos_phi * yd;
    double yNorth = -cos_phi * sin_lambda * xd - sin_lambda * sin_phi * yd + cos_lambda * zd;
    double zUp = cos_lambda * cos_phi * xd + cos_lambda * sin_phi * yd + sin_lambda * zd;

    return QVector3D(xEast, yNorth, zUp);
}

QVector3D mapGpsToLocalPoint(QGeoCoordinate gps_point_, QGeoCoordinate ref_gps)
{
    QVector3D ecef = mapGeodeticToEcef(gps_point_);
    return mapEcefToEnu(ecef, ref_gps);
}

QVector3D mapEnuToEcef(const QVector3D &enu_point, QGeoCoordinate &ref_gps)
{
    // Convert to radians in notation consistent with the paper:
    double lambda = ref_gps.latitude() * DEG_TO_RAD;
    double phi = ref_gps.longitude() * DEG_TO_RAD;

    double sin_lambda = sin(lambda);
    double cos_lambda = cos(lambda);
    double cos_phi = cos(phi);
    double sin_phi = sin(phi);

    double N = ins_a / sqrt(1 - ins_e_sq * sin_lambda * sin_lambda);

    double x0 = (ref_gps.altitude() + N) * cos_lambda * cos_phi;
    double y0 = (ref_gps.altitude() + N) * cos_lambda * sin_phi;
    double z0 = (ref_gps.altitude() + (1 - ins_e_sq) * N) * sin_lambda;

    double xd = -sin_phi * enu_point.x() - cos_phi * sin_lambda * enu_point.y() + cos_lambda * cos_phi * enu_point.z();
    double yd = cos_phi * enu_point.x() - sin_lambda * sin_phi * enu_point.y() + cos_lambda * sin_phi * enu_point.z();
    double zd = cos_lambda * enu_point.y() + sin_lambda * enu_point.z();

    return QVector3D(xd + x0, yd + y0, zd + z0);
}

QGeoCoordinate mapEcefToGeodetic(const QVector3D &enu_point)
{
    double eps = ins_e_sq / (1.0 - ins_e_sq);
    double p = sqrt(enu_point.x() * enu_point.x() + enu_point.y() * enu_point.y());
    double q = atan2((enu_point.z() * ins_a), (p * ins_b));
    double sin_q = sin(q);
    double cos_q = cos(q);
    double sin_q_3 = sin_q * sin_q * sin_q;
    double cos_q_3 = cos_q * cos_q * cos_q;
    double phi = atan2((enu_point.z() + eps * ins_b * sin_q_3), (p - ins_e_sq * ins_a * cos_q_3));
    double lambda = atan2(enu_point.y(), enu_point.x());
    double v = ins_a / sqrt(1.0 - ins_e_sq * sin(phi) * sin(phi));

    QGeoCoordinate output;
    output.setAltitude((p / cos(phi)) - v);
    output.setLatitude(phi * RAD_TO_DEG);
    output.setLongitude(lambda * RAD_TO_DEG);

    return output;
}

QGeoCoordinate mapLocalToGpsPoint(QVector3D local_point, QGeoCoordinate ref_gps)
{
    QVector3D ecef = mapEnuToEcef(local_point, ref_gps);
    QGeoCoordinate out_point = mapEcefToGeodetic(ecef);

    return out_point;
}
