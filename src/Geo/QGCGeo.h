/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

// LatLonToUTMXY
// Converts a latitude/longitude pair to x and y coordinates in the
// Universal Transverse Mercator projection.
//
// Inputs:
//   lat - Latitude of the point, in radians.
//   lon - Longitude of the point, in radians.
//   zone - UTM zone to be used for calculating values for x and y.
//          If zone is less than 1 or greater than 60, the routine
//          will determine the appropriate zone from the value of lon.
//
// Outputs:
//   x - The x coordinate (easting) of the computed point. (in meters)
//   y - The y coordinate (northing) of the computed point. (in meters)
//
// Returns:
//   The UTM zone used for calculating the values of x and y.
//   If conversion failed the function returns 0
int convertGeoToUTM(const QGeoCoordinate& coord, double& easting, double& northing);

// UTMXYToLatLon
//
// Converts x and y coordinates in the Universal Transverse Mercator//   The UTM zone parameter should be in the range [1,60].

// projection to a latitude/longitude pair.
//
// Inputs:
// x - The easting of the point, in meters.
// y - The northing of the point, in meters.
// zone - The UTM zone in which the point lies.
// southhemi - True if the point is in the southern hemisphere;
//               false otherwise.
//
// Outputs:
// lat - The latitude of the point, in radians.
// lon - The longitude of the point, in radians.
//
// Returns:
// The function returns true if conversion succeeded.
bool convertUTMToGeo(double easting, double northing, int zone, bool southhemi, QGeoCoordinate& coord);

// Converts a latitude/longitude pair to MGRS string
//
// Inputs:
//   coord - Latitude, Longiture coordinate
//
// Returns:
//   The MGRS coordinate string
//   If conversion fails the function returns empty string
QString convertGeoToMGRS(const QGeoCoordinate& coord);

// Converts MGRS string to a latitude/longitude pair.
//
// Inputs:
// mgrs - MGRS coordinate string
//
// Outputs:
// lat - The latitude of the point, in radians.
// lon - The longitude of the point, in radians.
//
// Returns:
// The function returns true if conversion succeeded.
bool convertMGRSToGeo(QString mgrs, QGeoCoordinate& coord);

#endif // QGCGEO_H
