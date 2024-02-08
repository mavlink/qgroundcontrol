#ifndef VIEWER3DUTILS_H
#define VIEWER3DUTILS_H
#include "math.h"

#include <QGeoCoordinate>
#include <QVector3D>

#ifndef DEG_TO_RAD
#define DEG_TO_RAD 0.017453292519f
#endif

#ifndef RAD_TO_DEG
#define RAD_TO_DEG 57.29577951308f
#endif

///     @author Omid Esrafilian <esrafilian.omid@gmail.com>


QVector3D mapGeodeticToEcef(QGeoCoordinate gps_point_);
QVector3D mapEcefToEnu(QVector3D ecef_point, QGeoCoordinate ref_gps);
QVector3D mapGpsToLocalPoint(QGeoCoordinate gps_point_, QGeoCoordinate ref_gps);

QVector3D mapEnuToEcef(const QVector3D &enu_point, QGeoCoordinate& ref_gps);
QGeoCoordinate mapEcefToGeodetic(const QVector3D &enu_point);
QGeoCoordinate mapLocalToGpsPoint(QVector3D local_point, QGeoCoordinate ref_gps);


#endif // VIEWER3DUTILS_H
