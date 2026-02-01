/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

/// @file
/// @brief Geographic coordinate conversion utilities using GeographicLib.
///
/// Provides coordinate transformations between various reference frames:
/// - Geodetic (lat/lon/alt) - WGS84 ellipsoid
/// - NED (North/East/Down) - Local tangent plane
/// - ENU (East/North/Up) - Local tangent plane
/// - ECEF (Earth-Centered Earth-Fixed) - Cartesian
/// - UTM (Universal Transverse Mercator)
/// - MGRS (Military Grid Reference System)
///
/// All conversions use the WGS84 ellipsoid model for accuracy.

#include <QtGui/QVector3D>
#include <QtPositioning/QGeoCoordinate>


namespace QGCGeo
{

// ============================================================================
// NED (North-East-Down) Local Tangent Plane
// ============================================================================

/// Convert geodetic coordinate to NED (North-East-Down) relative to origin.
/// @param coord Geodetic coordinate to convert.
/// @param origin Reference point for local tangent plane.
/// @param[out] x North component in meters.
/// @param[out] y East component in meters.
/// @param[out] z Down component in meters (positive = below origin).
/// @note NaN altitudes are treated as 0.0 (sea level).
void convertGeoToNed(const QGeoCoordinate &coord, const QGeoCoordinate &origin, double &x, double &y, double &z);

/// Convert NED (North-East-Down) coordinate to geodetic.
/// @param x North component in meters.
/// @param y East component in meters.
/// @param z Down component in meters (positive = below origin).
/// @param origin Reference point for local tangent plane.
/// @param[out] coord Resulting geodetic coordinate.
void convertNedToGeo(double x, double y, double z, const QGeoCoordinate &origin, QGeoCoordinate &coord);

// ============================================================================
// ENU (East-North-Up) Local Tangent Plane
// ============================================================================

/// Convert geodetic coordinate to ENU (East-North-Up) relative to reference.
/// @param coord Geodetic coordinate to convert.
/// @param ref Reference point for local tangent plane.
/// @return ENU vector in meters (x=East, y=North, z=Up).
/// @note Uses float precision (QVector3D). For high precision, use NED functions.
QVector3D convertGpsToEnu(const QGeoCoordinate &coord, const QGeoCoordinate &ref);

/// Convert ENU (East-North-Up) coordinate to geodetic.
/// @param enu ENU vector in meters (x=East, y=North, z=Up).
/// @param ref Reference point for local tangent plane.
/// @return Geodetic coordinate.
QGeoCoordinate convertEnuToGps(const QVector3D &enu, const QGeoCoordinate &ref);

// ============================================================================
// ECEF (Earth-Centered Earth-Fixed)
// ============================================================================

/// Convert geodetic coordinate to ECEF (Earth-Centered Earth-Fixed).
/// @param coord Geodetic coordinate (lat/lon/alt).
/// @return ECEF vector in meters.
/// @note Uses float precision (QVector3D). Large coordinates may lose precision.
QVector3D convertGeodeticToEcef(const QGeoCoordinate &coord);

/// Convert ECEF (Earth-Centered Earth-Fixed) to geodetic coordinate.
/// @param ecef ECEF vector in meters.
/// @return Geodetic coordinate (lat/lon/alt).
QGeoCoordinate convertEcefToGeodetic(const QVector3D &ecef);

/// Convert ECEF to ENU relative to a reference point.
/// @param ecef ECEF vector in meters.
/// @param ref Reference point for local tangent plane.
/// @return ENU vector in meters.
QVector3D convertEcefToEnu(const QVector3D &ecef, const QGeoCoordinate &ref);

/// Convert ENU to ECEF relative to a reference point.
/// @param enu ENU vector in meters.
/// @param ref Reference point for local tangent plane.
/// @return ECEF vector in meters.
QVector3D convertEnuToEcef(const QVector3D &enu, const QGeoCoordinate &ref);

// ============================================================================
// UTM (Universal Transverse Mercator)
// ============================================================================

/// Convert geodetic coordinate to UTM.
/// @param coord Geodetic coordinate to convert.
/// @param[out] easting UTM easting in meters.
/// @param[out] northing UTM northing in meters.
/// @return UTM zone (1-60), or 0 on failure.
int convertGeoToUTM(const QGeoCoordinate &coord, double &easting, double &northing);

/// Convert UTM to geodetic coordinate.
/// @param easting UTM easting in meters.
/// @param northing UTM northing in meters.
/// @param zone UTM zone (1-60).
/// @param southhemi True if southern hemisphere.
/// @param[out] coord Resulting geodetic coordinate (altitude = 0).
/// @return True on success, false on failure.
bool convertUTMToGeo(double easting, double northing, int zone, bool southhemi, QGeoCoordinate &coord);

// ============================================================================
// MGRS (Military Grid Reference System)
// ============================================================================

/// Convert geodetic coordinate to MGRS string.
/// @param coord Geodetic coordinate to convert.
/// @return MGRS string (e.g., "32TMT 65886 47092"), or empty string on failure.
QString convertGeoToMGRS(const QGeoCoordinate &coord);

/// Convert MGRS string to geodetic coordinate.
/// @param mgrs MGRS string (spaces optional, e.g., "32TMT6588647092" or "32T MT 65886 47092").
/// @param[out] coord Resulting geodetic coordinate (altitude = 0).
/// @return True on success, false on failure.
bool convertMGRSToGeo(const QString &mgrs, QGeoCoordinate &coord);

// ============================================================================
// Geodesic Calculations (Great Circle on Ellipsoid)
// ============================================================================

/// Calculate geodesic distance between two coordinates using WGS84 ellipsoid.
/// @param from Starting coordinate.
/// @param to Ending coordinate.
/// @return Distance in meters.
/// @note More accurate than QGeoCoordinate::distanceTo() which uses spherical approximation.
double geodesicDistance(const QGeoCoordinate &from, const QGeoCoordinate &to);

/// Calculate geodesic azimuth (bearing) from one coordinate to another using WGS84 ellipsoid.
/// @param from Starting coordinate.
/// @param to Ending coordinate.
/// @return Forward azimuth in degrees [0, 360), clockwise from north.
/// @note More accurate than QGeoCoordinate::azimuthTo() which uses spherical approximation.
double geodesicAzimuth(const QGeoCoordinate &from, const QGeoCoordinate &to);

/// Calculate destination coordinate given start point, azimuth, and distance using WGS84 ellipsoid.
/// @param from Starting coordinate.
/// @param azimuth Forward azimuth in degrees, clockwise from north.
/// @param distance Distance in meters.
/// @return Destination coordinate (altitude copied from start).
QGeoCoordinate geodesicDestination(const QGeoCoordinate &from, double azimuth, double distance);

// ============================================================================
// Path and Polygon Calculations
// ============================================================================

/// Calculate total geodesic length of a path using WGS84 ellipsoid.
/// @param path List of coordinates defining the path.
/// @return Total path length in meters, or 0 if fewer than 2 points.
double pathLength(const QList<QGeoCoordinate> &path);

/// Calculate geodesic area of a polygon using WGS84 ellipsoid.
/// @param polygon List of coordinates defining the polygon vertices (automatically closed).
/// @return Absolute area in square meters, or 0 if fewer than 3 points.
/// @note More accurate than planar projection methods, especially for large polygons.
double polygonArea(const QList<QGeoCoordinate> &polygon);

/// Calculate geodesic perimeter of a polygon using WGS84 ellipsoid.
/// @param polygon List of coordinates defining the polygon vertices (automatically closed).
/// @return Perimeter in meters, or 0 if fewer than 2 points.
double polygonPerimeter(const QList<QGeoCoordinate> &polygon);

/// Interpolate evenly-spaced points along a geodesic path using WGS84 ellipsoid.
/// @param from Starting coordinate.
/// @param to Ending coordinate.
/// @param numPoints Number of points to generate (must be >= 2).
/// @return List of coordinates including start and end points, evenly spaced along the geodesic.
/// @note Uses GeodesicLine internally for efficiency. Altitude is linearly interpolated.
QList<QGeoCoordinate> interpolatePath(const QGeoCoordinate &from, const QGeoCoordinate &to, int numPoints);

/// Get the coordinate at a specific distance along a geodesic path using WGS84 ellipsoid.
/// @param from Starting coordinate.
/// @param to Ending coordinate.
/// @param distance Distance from start in meters (clamped to path length).
/// @return Coordinate at the specified distance along the geodesic. Altitude is linearly interpolated.
/// @note Useful for midpoint: interpolateAtDistance(from, to, geodesicDistance(from, to) / 2)
QGeoCoordinate interpolateAtDistance(const QGeoCoordinate &from, const QGeoCoordinate &to, double distance);

} // namespace QGCGeo
