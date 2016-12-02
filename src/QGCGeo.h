/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief Coordinate transformation math functions.
///     @link https://github.com/PX4/Firmware/blob/master/src/lib/geo/geo.c
///     @link http://psas.pdx.edu/CoordinateSystem/Latitude_to_LocalTangent.pdf
///     @link http://dspace.dsto.defence.gov.au/dspace/bitstream/1947/3538/1/DSTO-TN-0432.pdf
///     @author David Goodman <dagoodma@gmail.com>

#ifndef QGCGEO_H
#define QGCGEO_H

#include <QGeoCoordinate>

/* Safeguard for systems lacking sincos (e.g. Mac OS X Leopard) */
#ifndef sincos
#define sincos(th,x,y) { (*(x))=sin(th); (*(y))=cos(th); }
#endif

/**
 * @brief Project a geodetic coordinate on to local tangential plane (LTP) as coordinate with East,
 * North, and Down components in meters.
 * @param[in] coord Geodetic coordinate to project onto LTP.
 * @param[in] origin Geoedetic origin for LTP projection.
 * @param[out] x North component of coordinate in local plane.
 * @param[out] y East component of coordinate in local plane.
 * @param[out] z Down component of coordinate in local plane.
 */
void convertGeoToNed(QGeoCoordinate coord, QGeoCoordinate origin, double* x, double* y, double* z);

/**
 * @brief Transform a local (East, North, and Down) coordinate into a geodetic coordinate.
 * @param[in] x North component of local coordinate in meters.
 * @param[in] x East component of local coordinate in meters.
 * @param[in] x Down component of local coordinate in meters.
 * @param[in] origin Geoedetic origin for LTP.
 * @param[out] coord Geodetic coordinate to hold result.
 */
void convertNedToGeo(double x, double y, double z, QGeoCoordinate origin, QGeoCoordinate *coord);

#endif // QGCGEO_H
