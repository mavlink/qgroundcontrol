/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

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
