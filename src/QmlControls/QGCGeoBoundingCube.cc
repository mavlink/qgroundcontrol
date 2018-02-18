/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCGeoBoundingCube.h"
#include <QDebug>

double QGCGeoBoundingCube::MaxAlt    =  1000000.0;
double QGCGeoBoundingCube::MinAlt    = -1000000.0;
double QGCGeoBoundingCube::MaxNorth  =  90.0;
double QGCGeoBoundingCube::MaxSouth  = -90.0;
double QGCGeoBoundingCube::MaxWest   = -180.0;
double QGCGeoBoundingCube::MaxEast   =  180.0;

//-----------------------------------------------------------------------------
bool
QGCGeoBoundingCube::isValid() const
{
    return pointNW.isValid() && pointSE.isValid() && pointNW.latitude() != MaxSouth && pointSE.latitude() != MaxNorth && \
           pointNW.longitude() != MaxEast && pointSE.longitude() != MaxWest && pointNW.altitude() < MaxAlt and pointSE.altitude() > MinAlt;
}

//-----------------------------------------------------------------------------
QGeoCoordinate
QGCGeoBoundingCube::center() const
{
    double lat = (((pointNW.latitude()  + 90.0)  + (pointSE.latitude()  + 90.0))  / 2.0) - 90.0;
    double lon = (((pointNW.longitude() + 180.0) + (pointSE.longitude() + 180.0)) / 2.0) - 180.0;
    double alt = (pointNW.altitude() + pointSE.altitude()) / 2.0;
    //qDebug() << pointNW << pointSE << QGeoCoordinate(lat, lon, alt);
    return QGeoCoordinate(lat, lon, alt);
}

//-----------------------------------------------------------------------------
QList<QGeoCoordinate>
QGCGeoBoundingCube::polygon2D() const
{
    QList<QGeoCoordinate> coords;
    coords.append(QGeoCoordinate(pointNW.latitude(), pointNW.longitude(), pointSE.altitude()));
    coords.append(QGeoCoordinate(pointNW.latitude(), pointSE.longitude(), pointSE.altitude()));
    coords.append(QGeoCoordinate(pointSE.latitude(), pointSE.longitude(), pointSE.altitude()));
    coords.append(QGeoCoordinate(pointSE.latitude(), pointNW.longitude(), pointSE.altitude()));
    coords.append(QGeoCoordinate(pointNW.latitude(), pointNW.longitude(), pointSE.altitude()));
    return coords;
}

//-----------------------------------------------------------------------------
double
QGCGeoBoundingCube::width() const
{
    QGeoCoordinate ne = QGeoCoordinate(pointNW.latitude(), pointSE.longitude());
    return pointNW.distanceTo(ne);
}

//-----------------------------------------------------------------------------
double
QGCGeoBoundingCube::height() const
{
    QGeoCoordinate sw = QGeoCoordinate(pointSE.latitude(), pointNW.longitude());
    return pointNW.distanceTo(sw);
}

//-----------------------------------------------------------------------------
double
QGCGeoBoundingCube::area() const
{
    // Area in km^2
    double a = (height() / 1000.0) * (width() / 1000.0);
    //qDebug() << "Area:" << a;
    return a;
}

//-----------------------------------------------------------------------------
double
QGCGeoBoundingCube::radius() const
{
    return pointNW.distanceTo(pointSE) / 2.0;
}
