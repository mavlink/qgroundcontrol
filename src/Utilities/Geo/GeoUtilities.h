#pragma once

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPolygon>
#include <QtPositioning/QGeoRectangle>

#include <functional>

/// @file GeoUtilities.h
/// @brief Geographic utility functions for validation, normalization, and geometric operations
///
/// GeoUtilities provides foundational operations for working with geographic coordinates:
///
/// @section bounding Bounding Box
/// - boundingBox() - Compute enclosing rectangle for coordinate lists
/// - expandBoundingBox() - Incrementally grow a bounding box
///
/// @section normalization Coordinate Normalization
/// - normalizeLongitude() - Wrap longitude to [-180, 180]
/// - normalizeLatitude() - Clamp latitude to [-90, 90]
/// - normalizeCoordinate() - Normalize both lat/lon
/// - validateAndNormalizeCoordinates() - Batch normalize with optional warnings
///
/// @section polygon Polygon Operations
/// - removeClosingVertex() - Remove duplicate closing vertex from closed polygons
/// - ensureClockwise() - Ensure consistent winding order
/// - isSelfIntersecting() - Check for edge crossings
/// - validatePolygon() - Comprehensive polygon validation
///
/// @section validation Coordinate Validation
/// - isValidCoordinate() - Check lat/lon bounds
/// - validateCoordinates() - Batch validation with error reporting
/// - isValidAltitude() - Check altitude bounds
/// - validateAltitudes() - Batch altitude validation
///
/// @see GeoFormatRegistry for file I/O operations using these utilities
namespace GeoUtilities
{
    // ========================================================================
    // Constants
    // ========================================================================

    /// Minimum vertices required for a valid polygon
    static constexpr int kMinPolygonVertices = 3;

    /// Minimum vertices required for a valid polyline
    static constexpr int kMinPolylineVertices = 2;

    /// Default threshold for detecting polygon closure (first == last vertex)
    static constexpr double kPolygonClosureThresholdMeters = 1.0;

    // ========================================================================
    // Bounding Box Utilities (using QGeoRectangle)
    // ========================================================================

    /// Compute bounding box for a list of coordinates
    QGeoRectangle boundingBox(const QList<QGeoCoordinate> &coords);

    /// Compute bounding box for multiple coordinate lists (e.g., multiple polygons)
    QGeoRectangle boundingBox(const QList<QList<QGeoCoordinate>> &coordLists);

    /// Expand a bounding box to include a coordinate
    void expandBoundingBox(QGeoRectangle &box, const QGeoCoordinate &coord);

    /// Expand a bounding box to include another bounding box
    void expandBoundingBox(QGeoRectangle &box, const QGeoRectangle &other);

    // ========================================================================
    // Coordinate Normalization
    // ========================================================================

    /// Normalize longitude to [-180, 180] range
    /// @param longitude Input longitude (may be outside normal range)
    /// @return Normalized longitude in [-180, 180]
    double normalizeLongitude(double longitude);

    /// Normalize latitude to [-90, 90] range (clamped, not wrapped)
    /// @param latitude Input latitude (may be outside normal range)
    /// @return Clamped latitude in [-90, 90]
    double normalizeLatitude(double latitude);

    /// Normalize a coordinate (longitude wrapped, latitude clamped)
    /// @param coord Input coordinate
    /// @return New coordinate with normalized lat/lon, altitude preserved
    QGeoCoordinate normalizeCoordinate(const QGeoCoordinate &coord);

    /// Normalize all coordinates in a list
    void normalizeCoordinates(QList<QGeoCoordinate> &coords);

    /// Callback for logging validation warnings during coordinate normalization
    /// @param index Coordinate index
    /// @param type "latitude" or "longitude" or "altitude"
    /// @param value The out-of-range value
    using ValidationWarningCallback = std::function<void(int index, const QString &type, double value)>;

    /// Validate and normalize coordinates, with optional warning callback for out-of-range values
    /// This combines validation, normalization, and optional logging in one pass.
    /// @param coords Coordinates to validate and normalize (modified in place)
    /// @param warningCallback Optional callback for logging out-of-range values
    void validateAndNormalizeCoordinates(QList<QGeoCoordinate> &coords,
                                         const ValidationWarningCallback &warningCallback = nullptr);

    /// Remove duplicate closing vertex if polygon is closed (first == last)
    /// @param coords Polygon vertices (modified in place)
    /// @return true if a closing vertex was removed
    bool removeClosingVertex(QList<QGeoCoordinate> &coords);

    /// Ensure polygon has a closing vertex (first == last)
    /// @param coords Polygon vertices (modified in place)
    /// @param thresholdMeters Distance threshold for considering vertices equal (default: 1m)
    /// @return true if a closing vertex was added
    bool ensureClosingVertex(QList<QGeoCoordinate> &coords,
                             double thresholdMeters = kPolygonClosureThresholdMeters);

    /// Ensure polygon vertices are in clockwise order (reverses if counter-clockwise)
    /// @param coords Polygon vertices (modified in place)
    /// @return true if vertices were reversed
    bool ensureClockwise(QList<QGeoCoordinate> &coords);

    // ========================================================================
    // Altitude Validation
    // ========================================================================

    /// Default altitude limits for validation (permissive to avoid rejecting valid data)
    static constexpr double kDefaultMinAltitude = -500.0;   // Below sea level (Dead Sea, mines)
    static constexpr double kDefaultMaxAltitude = 100000.0; // ~330,000 ft (below Kármán line)

    /// Typical altitude limits for drone/UAV operations (for warnings, not rejection)
    static constexpr double kTypicalDroneMinAltitude = 0.0;     // Ground level
    static constexpr double kTypicalDroneMaxAltitude = 10000.0; // ~33,000 ft (commercial airspace)

    /// Validate that an altitude is within acceptable range
    /// @param altitude Altitude to validate (meters AMSL)
    /// @param minAltitude Minimum allowed altitude (default: -500m)
    /// @param maxAltitude Maximum allowed altitude (default: 10000m)
    /// @return true if altitude is valid or NaN (no altitude)
    bool isValidAltitude(double altitude,
                         double minAltitude = kDefaultMinAltitude,
                         double maxAltitude = kDefaultMaxAltitude);

    /// Validate altitude of a coordinate
    /// @return true if coordinate has no altitude or altitude is in range
    bool isValidAltitude(const QGeoCoordinate &coord,
                         double minAltitude = kDefaultMinAltitude,
                         double maxAltitude = kDefaultMaxAltitude);

    /// Validate altitudes of all coordinates in a list
    /// @param coords Coordinates to validate
    /// @param errorString Output: description of first invalid altitude found
    /// @param minAltitude Minimum allowed altitude
    /// @param maxAltitude Maximum allowed altitude
    /// @return true if all altitudes are valid
    bool validateAltitudes(const QList<QGeoCoordinate> &coords, QString &errorString,
                           double minAltitude = kDefaultMinAltitude,
                           double maxAltitude = kDefaultMaxAltitude);

    // ========================================================================
    // Polygon Validation
    // ========================================================================

    /// Check if a polygon is self-intersecting (2D check, ignores altitude)
    /// Uses the Shamos-Hoey algorithm concept with O(n²) simple implementation
    /// @param vertices Polygon vertices (open or closed)
    /// @return true if any non-adjacent edges intersect
    bool isSelfIntersecting(const QList<QGeoCoordinate> &vertices);

    /// Check if polygon vertices are in clockwise order (2D)
    /// @param vertices Polygon vertices
    /// @return true if clockwise, false if counter-clockwise
    bool isClockwise(const QList<QGeoCoordinate> &vertices);

    /// Reverse vertex order (to change winding direction)
    void reverseVertices(QList<QGeoCoordinate> &vertices);

    /// Validate a polygon for common issues
    /// @param vertices Polygon vertices
    /// @param errorString Output: description of validation errors
    /// @return true if polygon is valid
    bool validatePolygon(const QList<QGeoCoordinate> &vertices, QString &errorString);

    // ========================================================================
    // Shape Conversion
    // ========================================================================

    /// Default number of vertices for circle polygon approximation
    static constexpr int kDefaultCircleVertices = 72;

    /// Convert a circle to a polygon approximation using geodesic calculations
    /// @param center Circle center coordinate
    /// @param radiusMeters Circle radius in meters
    /// @param numVertices Number of vertices (default: 72 for 5° increments)
    /// @return List of polygon vertices approximating the circle
    QList<QGeoCoordinate> circleToPolygon(const QGeoCoordinate &center, double radiusMeters,
                                          int numVertices = kDefaultCircleVertices);

    // ========================================================================
    // Coordinate Validation
    // ========================================================================

    /// Check if coordinate has valid lat/lon (ignores altitude)
    bool isValidCoordinate(const QGeoCoordinate &coord);

    /// Validate all coordinates in a list
    /// @param coords Coordinates to validate
    /// @param errorString Output: description of first invalid coordinate
    /// @return true if all coordinates are valid
    bool validateCoordinates(const QList<QGeoCoordinate> &coords, QString &errorString);

    /// Validate coordinates for a list of simple polygons (no holes)
    /// @param polygons List of polygon coordinate lists
    /// @param errorString Output: description of first invalid coordinate
    /// @return true if all coordinates are valid
    bool validatePolygonListCoordinates(const QList<QList<QGeoCoordinate>> &polygons,
                                        QString &errorString);

    /// Validate coordinates for a list of polylines
    /// @param polylines List of polyline coordinate lists
    /// @param errorString Output: description of first invalid coordinate
    /// @return true if all coordinates are valid
    bool validatePolylineListCoordinates(const QList<QList<QGeoCoordinate>> &polylines,
                                         QString &errorString);

    /// Validate coordinates for a QGeoPolygon (perimeter + holes)
    /// @param polygon Polygon with potential holes
    /// @param errorString Output: description of first invalid coordinate
    /// @param polygonIndex Optional index for error messages (default -1 = omit)
    /// @return true if all coordinates are valid
    bool validateGeoPolygonCoordinates(const QGeoPolygon &polygon, QString &errorString,
                                       int polygonIndex = -1);

    /// Validate coordinates for a list of QGeoPolygons
    /// @param polygons List of polygons with potential holes
    /// @param errorString Output: description of first invalid coordinate
    /// @return true if all coordinates are valid
    bool validateGeoPolygonListCoordinates(const QList<QGeoPolygon> &polygons,
                                           QString &errorString);

    // ========================================================================
    // Coordinate Parsing (consolidated from format-specific helpers)
    // ========================================================================

    /// Parse a KML-style coordinate string: "lon,lat[,alt]"
    /// @param str Single coordinate string (e.g., "-122.5,37.8,100")
    /// @param[out] coord Parsed coordinate
    /// @param[out] errorString Error message if parsing fails
    /// @param normalizeOutOfRange If true, normalize out-of-range values instead of failing
    /// @return true if parsing succeeded
    bool parseKmlCoordinate(const QString &str, QGeoCoordinate &coord,
                           QString &errorString, bool normalizeOutOfRange = true);

    /// Parse a list of KML-style coordinates: "lon,lat[,alt] lon,lat[,alt] ..."
    /// @param str Space-separated coordinate string
    /// @param[out] coords Parsed coordinates
    /// @param[out] errorString Error message if parsing fails
    /// @param normalizeOutOfRange If true, normalize out-of-range values
    /// @return true if all coordinates parsed successfully
    bool parseKmlCoordinateList(const QString &str, QList<QGeoCoordinate> &coords,
                                QString &errorString, bool normalizeOutOfRange = true);

    /// Parse a WKT-style coordinate string: "x y [z]" (x=lon, y=lat, z=alt)
    /// @param str Single coordinate string (e.g., "-122.5 37.8 100")
    /// @param[out] coord Parsed coordinate
    /// @param[out] errorString Error message if parsing fails
    /// @param normalizeOutOfRange If true, normalize out-of-range values
    /// @return true if parsing succeeded
    bool parseWktCoordinate(const QString &str, QGeoCoordinate &coord,
                           QString &errorString, bool normalizeOutOfRange = true);

    /// Parse a list of WKT-style coordinates: "x y [z], x y [z], ..."
    /// @param str Comma-separated coordinate string
    /// @param[out] coords Parsed coordinates
    /// @param[out] errorString Error message if parsing fails
    /// @param normalizeOutOfRange If true, normalize out-of-range values
    /// @return true if all coordinates parsed successfully
    bool parseWktCoordinateList(const QString &str, QList<QGeoCoordinate> &coords,
                               QString &errorString, bool normalizeOutOfRange = true);

    /// Format a coordinate as KML-style string: "lon,lat[,alt]"
    /// @param coord Coordinate to format
    /// @param includeAltitude Include altitude in output (if coordinate has valid altitude)
    /// @param precision Decimal places for lat/lon (default 8, ~1mm precision)
    /// @return Formatted string
    QString formatKmlCoordinate(const QGeoCoordinate &coord, bool includeAltitude = true,
                               int precision = 8);

    /// Format coordinates as KML-style string: "lon,lat[,alt] lon,lat[,alt] ..."
    QString formatKmlCoordinateList(const QList<QGeoCoordinate> &coords,
                                   bool includeAltitude = true, int precision = 8);

    /// Format a coordinate as WKT-style string: "x y [z]"
    /// @param coord Coordinate to format
    /// @param includeAltitude Include altitude in output
    /// @param precision Decimal places (default 8)
    /// @return Formatted string
    QString formatWktCoordinate(const QGeoCoordinate &coord, bool includeAltitude = true,
                               int precision = 8);

    /// Format coordinates as WKT-style string: "x y [z], x y [z], ..."
    QString formatWktCoordinateList(const QList<QGeoCoordinate> &coords,
                                   bool includeAltitude = true, int precision = 8);

    // ========================================================================
    // Polygon Processing Utilities
    // ========================================================================

    /// Process a polygon with holes: filter vertices, validate, ensure proper winding
    /// @param outerRing Outer ring vertices (modified in place)
    /// @param holes Hole vertex lists (modified in place)
    /// @param filterMeters Vertex filter distance (0 to disable)
    /// @param minVertices Minimum vertices required after filtering
    /// @return true if polygon is valid after processing
    bool processPolygonWithHoles(QList<QGeoCoordinate> &outerRing,
                                 QList<QList<QGeoCoordinate>> &holes,
                                 double filterMeters = 0.0,
                                 int minVertices = kMinPolygonVertices);

    /// Check if any coordinate in a list has a valid (non-NaN) altitude
    /// @param coords Coordinates to check
    /// @return true if at least one coordinate has valid altitude
    bool hasAnyAltitude(const QList<QGeoCoordinate> &coords);

    /// Check if any coordinate in multiple lists has a valid altitude
    /// @param coordLists List of coordinate lists to check
    /// @return true if at least one coordinate has valid altitude
    bool hasAnyAltitude(const QList<QList<QGeoCoordinate>> &coordLists);

    // ========================================================================
    // UTM Batch Conversion
    // ========================================================================

    /// UTM coordinate with zone information
    struct UTMCoordinate {
        double easting = 0.0;   ///< UTM easting in meters
        double northing = 0.0;  ///< UTM northing in meters
        int zone = 0;           ///< UTM zone (1-60), 0 = invalid
        bool southern = false;  ///< True if southern hemisphere
    };

    /// Convert a list of geodetic coordinates to UTM
    /// @param coords Geodetic coordinates to convert
    /// @param utmCoords Output: UTM coordinates (same size as input)
    /// @return true if all conversions succeeded
    /// @note Each coordinate may end up in a different UTM zone
    bool convertToUTM(const QList<QGeoCoordinate> &coords, QList<UTMCoordinate> &utmCoords);

    /// Convert a list of geodetic coordinates to UTM with forced zone
    /// @param coords Geodetic coordinates to convert
    /// @param utmCoords Output: UTM coordinates (same size as input)
    /// @param forceZone UTM zone to use for all coordinates (1-60)
    /// @param forceSouthern True to force southern hemisphere
    /// @return true if all conversions succeeded
    /// @note Forcing zone is useful when all coords should be in same projection
    bool convertToUTM(const QList<QGeoCoordinate> &coords, QList<UTMCoordinate> &utmCoords,
                      int forceZone, bool forceSouthern);

    /// Convert a list of UTM coordinates to geodetic
    /// @param utmCoords UTM coordinates to convert
    /// @param coords Output: Geodetic coordinates (same size as input)
    /// @return true if all conversions succeeded
    bool convertFromUTM(const QList<UTMCoordinate> &utmCoords, QList<QGeoCoordinate> &coords);

    /// Determine optimal UTM zone for a set of coordinates (based on centroid)
    /// @param coords Geodetic coordinates
    /// @param[out] zone Optimal UTM zone (1-60)
    /// @param[out] southern True if centroid is in southern hemisphere
    /// @return true if zone could be determined, false if coords is empty
    bool optimalUTMZone(const QList<QGeoCoordinate> &coords, int &zone, bool &southern);

    // ========================================================================
    // Polygon Geometry
    // ========================================================================

    /// Calculate the centroid (center of mass) of a polygon
    /// @param vertices Polygon vertices (open or closed, at least 3 required)
    /// @return Centroid coordinate, or invalid coordinate if insufficient vertices
    /// @note Uses the standard polygon centroid formula. For geodesic accuracy
    ///       on large polygons, consider projecting to UTM first.
    QGeoCoordinate polygonCentroid(const QList<QGeoCoordinate> &vertices);

    /// Calculate the area of a polygon on the Earth's surface
    /// @param vertices Polygon vertices (open or closed, at least 3 required)
    /// @return Area in square meters, or 0.0 if insufficient vertices
    /// @note Uses the spherical excess formula (Girard's theorem) which accounts for
    ///       Earth's curvature. Results are accurate for polygons of any size.
    ///       The polygon should be simple (non-self-intersecting).
    double polygonArea(const QList<QGeoCoordinate> &vertices);

    /// Calculate the area of a polygon with holes on the Earth's surface
    /// @param polygon QGeoPolygon with outer ring and optional holes
    /// @return Area in square meters (outer area minus hole areas)
    double polygonArea(const QGeoPolygon &polygon);

    /// Calculate the perimeter length of a polygon on the Earth's surface
    /// @param vertices Polygon vertices (open or closed, at least 2 required)
    /// @return Perimeter in meters, or 0.0 if insufficient vertices
    double polygonPerimeter(const QList<QGeoCoordinate> &vertices);

    /// Calculate the centroid of a polyline (midpoint of path)
    /// @param coords Polyline coordinates (at least 2 required)
    /// @return Midpoint coordinate, or invalid coordinate if insufficient points
    QGeoCoordinate polylineMidpoint(const QList<QGeoCoordinate> &coords);

    // ========================================================================
    // Path Simplification
    // ========================================================================

    /// Simplify a path using the Douglas-Peucker algorithm
    /// @param coords Input coordinates (polyline or polygon boundary)
    /// @param toleranceMeters Maximum perpendicular distance from simplified line
    /// @return Simplified coordinate list
    /// @note Preserves start and end points. For polygons, pass without closing vertex.
    QList<QGeoCoordinate> simplifyDouglasPeucker(const QList<QGeoCoordinate> &coords,
                                                  double toleranceMeters);

    /// Simplify a path to approximately N points using Douglas-Peucker
    /// @param coords Input coordinates
    /// @param targetCount Approximate target number of points (minimum 2)
    /// @return Simplified coordinate list with approximately targetCount points
    /// @note Uses binary search to find appropriate tolerance
    QList<QGeoCoordinate> simplifyToCount(const QList<QGeoCoordinate> &coords, int targetCount);

    /// Simplify a path using Visvalingam-Whyatt algorithm (area-based)
    /// @param coords Input coordinates (polyline or polygon boundary)
    /// @param minAreaSqMeters Minimum triangle area to preserve
    /// @return Simplified coordinate list
    /// @note Better preserves shape character than Douglas-Peucker for some use cases
    QList<QGeoCoordinate> simplifyVisvalingamWhyatt(const QList<QGeoCoordinate> &coords,
                                                     double minAreaSqMeters);

    /// Simplify a path to exactly N points using Visvalingam-Whyatt
    /// @param coords Input coordinates
    /// @param targetCount Target number of points (minimum 2)
    /// @return Simplified coordinate list with exactly targetCount points
    QList<QGeoCoordinate> simplifyVisvalingamToCount(const QList<QGeoCoordinate> &coords, int targetCount);

    // ========================================================================
    // Point-in-Polygon
    // ========================================================================

    /// Check if a point is inside a polygon (ray casting algorithm)
    /// @param point Point to test
    /// @param polygon Polygon vertices (open or closed)
    /// @return true if point is inside or on boundary
    /// @note Uses 2D test (ignores altitude)
    bool pointInPolygon(const QGeoCoordinate &point, const QList<QGeoCoordinate> &polygon);

    /// Check if a point is inside a polygon with edge tolerance
    /// @param point Point to test
    /// @param polygon Polygon vertices
    /// @param toleranceMeters Distance tolerance for boundary
    /// @return -1 if outside, 0 if on boundary (within tolerance), 1 if inside
    int pointInPolygonWithTolerance(const QGeoCoordinate &point,
                                     const QList<QGeoCoordinate> &polygon,
                                     double toleranceMeters);

    // ========================================================================
    // Path Interpolation
    // ========================================================================

    /// Interpolate a point along a path at a given fraction
    /// @param coords Path coordinates (at least 2 required)
    /// @param fraction Position along path [0.0 = start, 1.0 = end]
    /// @return Interpolated coordinate, or invalid if coords has fewer than 2 points
    /// @note Uses geodesic interpolation for accuracy
    QGeoCoordinate interpolateAlongPath(const QList<QGeoCoordinate> &coords, double fraction);

    /// Interpolate a point along a path at a given distance
    /// @param coords Path coordinates (at least 2 required)
    /// @param distanceMeters Distance from start in meters
    /// @return Interpolated coordinate, or last point if distance exceeds path length
    QGeoCoordinate interpolateAlongPathAtDistance(const QList<QGeoCoordinate> &coords,
                                                   double distanceMeters);

    /// Resample a path to have evenly-spaced points
    /// @param coords Input path coordinates
    /// @param numPoints Number of output points (minimum 2)
    /// @return Resampled path with evenly-spaced points
    QList<QGeoCoordinate> resamplePath(const QList<QGeoCoordinate> &coords, int numPoints);

    // ========================================================================
    // Convex Hull
    // ========================================================================

    /// Calculate the convex hull of a set of coordinates using Graham scan
    /// @param coords Input coordinates (at least 3 required for non-degenerate hull)
    /// @return Convex hull vertices in counter-clockwise order
    /// @note Returns empty list if fewer than 3 unique points
    QList<QGeoCoordinate> convexHull(const QList<QGeoCoordinate> &coords);

    // ========================================================================
    // Polygon Buffer/Offset
    // ========================================================================

    /// Offset a polygon by a distance (positive = outward, negative = inward)
    /// @param vertices Polygon vertices
    /// @param offsetMeters Offset distance in meters (positive = expand, negative = shrink)
    /// @return Offset polygon vertices
    /// @note Uses geodesic calculations. May produce self-intersecting results for
    ///       large negative offsets on concave polygons.
    QList<QGeoCoordinate> offsetPolygon(const QList<QGeoCoordinate> &vertices, double offsetMeters);

    /// Offset a polyline by a distance (perpendicular offset)
    /// @param coords Polyline coordinates
    /// @param offsetMeters Offset distance in meters (positive = left, negative = right)
    /// @return Offset polyline coordinates
    QList<QGeoCoordinate> offsetPolyline(const QList<QGeoCoordinate> &coords, double offsetMeters);

    // ========================================================================
    // Closest Point Operations
    // ========================================================================

    /// Find the closest point on a polygon boundary to a given point
    /// @param point Reference point
    /// @param polygon Polygon vertices
    /// @param[out] distance Distance to closest point in meters (optional)
    /// @return Closest point on polygon boundary
    QGeoCoordinate closestPointOnPolygon(const QGeoCoordinate &point,
                                          const QList<QGeoCoordinate> &polygon,
                                          double *distance = nullptr);

    /// Find the closest point on a polyline to a given point
    /// @param point Reference point
    /// @param polyline Polyline coordinates
    /// @param[out] distance Distance to closest point in meters (optional)
    /// @param[out] segmentIndex Index of segment containing closest point (optional)
    /// @return Closest point on polyline
    QGeoCoordinate closestPointOnPolyline(const QGeoCoordinate &point,
                                           const QList<QGeoCoordinate> &polyline,
                                           double *distance = nullptr,
                                           int *segmentIndex = nullptr);

    // ========================================================================
    // Path Heading/Bearing
    // ========================================================================

    /// Get the heading (bearing) at a specific distance along a path
    /// @param coords Path coordinates (at least 2 required)
    /// @param distanceMeters Distance from start in meters
    /// @return Heading in degrees [0, 360), or NaN if path is degenerate
    double headingAtDistance(const QList<QGeoCoordinate> &coords, double distanceMeters);

    /// Get the heading at a specific fraction along a path
    /// @param coords Path coordinates (at least 2 required)
    /// @param fraction Position along path [0.0 = start, 1.0 = end]
    /// @return Heading in degrees [0, 360), or NaN if path is degenerate
    double headingAtFraction(const QList<QGeoCoordinate> &coords, double fraction);

    /// Get headings at all vertices of a path (average of incoming/outgoing)
    /// @param coords Path coordinates
    /// @return List of headings at each vertex
    QList<double> pathHeadings(const QList<QGeoCoordinate> &coords);

    // ========================================================================
    // Polygon Smoothing
    // ========================================================================

    /// Smooth a polygon using Chaikin's corner-cutting algorithm
    /// @param vertices Polygon vertices
    /// @param iterations Number of smoothing iterations (1-5 recommended)
    /// @return Smoothed polygon vertices
    /// @note Each iteration approximately doubles the vertex count
    QList<QGeoCoordinate> smoothChaikin(const QList<QGeoCoordinate> &vertices, int iterations = 2);

    /// Smooth a polyline using Chaikin's algorithm (preserves endpoints)
    /// @param coords Polyline coordinates
    /// @param iterations Number of smoothing iterations
    /// @return Smoothed polyline coordinates
    QList<QGeoCoordinate> smoothChaikinPolyline(const QList<QGeoCoordinate> &coords, int iterations = 2);

    // ========================================================================
    // Anti-Meridian Handling
    // ========================================================================

    /// Check if a polygon crosses the anti-meridian (±180° longitude)
    /// @param vertices Polygon vertices
    /// @return true if polygon crosses the anti-meridian
    bool crossesAntimeridian(const QList<QGeoCoordinate> &vertices);

    /// Split a polygon that crosses the anti-meridian into two polygons
    /// @param vertices Polygon vertices (must cross anti-meridian)
    /// @param[out] westPart Polygon part on the western side (lon < 0)
    /// @param[out] eastPart Polygon part on the eastern side (lon > 0)
    /// @return true if split was performed, false if polygon doesn't cross
    bool splitAtAntimeridian(const QList<QGeoCoordinate> &vertices,
                              QList<QGeoCoordinate> &westPart,
                              QList<QGeoCoordinate> &eastPart);

    /// Normalize a polygon for anti-meridian handling
    /// @param vertices Polygon vertices (modified in place)
    /// @return true if polygon was modified (shifted longitudes)
    /// @note Shifts longitudes to avoid crossing ±180° when possible
    bool normalizeForAntimeridian(QList<QGeoCoordinate> &vertices);

    // ========================================================================
    // Minimum Bounding Circle
    // ========================================================================

    /// Result of minimum bounding circle calculation
    struct BoundingCircle {
        QGeoCoordinate center;  ///< Circle center
        double radiusMeters;    ///< Circle radius in meters
    };

    /// Calculate the minimum bounding circle (smallest enclosing circle)
    /// @param coords Input coordinates
    /// @return Bounding circle, or invalid center with radius 0 if empty
    /// @note Uses Welzl's algorithm with geodesic distance
    BoundingCircle minimumBoundingCircle(const QList<QGeoCoordinate> &coords);

    // ========================================================================
    // Line-Polygon Intersection
    // ========================================================================

    /// Check if a line segment intersects a polygon
    /// @param lineStart Start of line segment
    /// @param lineEnd End of line segment
    /// @param polygon Polygon vertices
    /// @return true if line intersects polygon boundary or passes through interior
    bool lineIntersectsPolygon(const QGeoCoordinate &lineStart,
                                const QGeoCoordinate &lineEnd,
                                const QList<QGeoCoordinate> &polygon);

    /// Find all intersection points between a line and a polygon
    /// @param lineStart Start of line segment
    /// @param lineEnd End of line segment
    /// @param polygon Polygon vertices
    /// @return List of intersection points (may be empty)
    QList<QGeoCoordinate> linePolygonIntersections(const QGeoCoordinate &lineStart,
                                                    const QGeoCoordinate &lineEnd,
                                                    const QList<QGeoCoordinate> &polygon);

    /// Check if a polyline intersects a polygon
    /// @param polyline Polyline coordinates
    /// @param polygon Polygon vertices
    /// @return true if any segment of polyline intersects polygon
    bool polylineIntersectsPolygon(const QList<QGeoCoordinate> &polyline,
                                    const QList<QGeoCoordinate> &polygon);

    // ========================================================================
    // Polygon Hole Validation
    // ========================================================================

    /// Check if a hole is completely contained within an outer ring
    /// @param hole Hole vertices
    /// @param outerRing Outer ring vertices
    /// @return true if hole is fully inside outer ring and doesn't intersect boundary
    bool isHoleContained(const QList<QGeoCoordinate> &hole,
                          const QList<QGeoCoordinate> &outerRing);

    /// Validate all holes in a polygon
    /// @param outerRing Outer ring vertices
    /// @param holes List of hole vertex lists
    /// @param[out] errorString Description of first error found
    /// @return true if all holes are valid (contained, non-overlapping)
    bool validateHoles(const QList<QGeoCoordinate> &outerRing,
                        const QList<QList<QGeoCoordinate>> &holes,
                        QString &errorString);

    // ========================================================================
    // Great Circle Intersection
    // ========================================================================

    /// Find intersection points of two great circles defined by line segments
    /// @param line1Start Start of first line segment
    /// @param line1End End of first line segment
    /// @param line2Start Start of second line segment
    /// @param line2End End of second line segment
    /// @param[out] intersection1 First intersection point (if any)
    /// @param[out] intersection2 Second intersection point (antipodal, if any)
    /// @return Number of intersections on the segments (0, 1, or 2)
    /// @note Great circles intersect at two antipodal points; this returns
    ///       only intersections that lie on both line segments
    int greatCircleIntersection(const QGeoCoordinate &line1Start,
                                 const QGeoCoordinate &line1End,
                                 const QGeoCoordinate &line2Start,
                                 const QGeoCoordinate &line2End,
                                 QGeoCoordinate &intersection1,
                                 QGeoCoordinate &intersection2);

    /// Check if two geodesic line segments intersect
    /// @param line1Start Start of first segment
    /// @param line1End End of first segment
    /// @param line2Start Start of second segment
    /// @param line2End End of second segment
    /// @return true if segments intersect
    bool geodesicSegmentsIntersect(const QGeoCoordinate &line1Start,
                                    const QGeoCoordinate &line1End,
                                    const QGeoCoordinate &line2Start,
                                    const QGeoCoordinate &line2End);

    // ========================================================================
    // Cross-Track and Along-Track Distance
    // ========================================================================

    /// Calculate cross-track distance (perpendicular distance from point to great circle path)
    /// @param point The point to measure from
    /// @param pathStart Start of the path segment
    /// @param pathEnd End of the path segment
    /// @return Cross-track distance in meters (positive = right of path, negative = left)
    /// @note Uses geodesic calculation for accuracy
    double crossTrackDistance(const QGeoCoordinate &point,
                               const QGeoCoordinate &pathStart,
                               const QGeoCoordinate &pathEnd);

    /// Calculate along-track distance (distance along path to closest point)
    /// @param point The point to project onto the path
    /// @param pathStart Start of the path segment
    /// @param pathEnd End of the path segment
    /// @return Along-track distance in meters from pathStart
    double alongTrackDistance(const QGeoCoordinate &point,
                               const QGeoCoordinate &pathStart,
                               const QGeoCoordinate &pathEnd);

    /// Result of cross-track analysis for a complete path
    struct CrossTrackResult {
        double crossTrackMeters;    ///< Perpendicular distance (+ right, - left)
        double alongTrackMeters;    ///< Distance along path to closest point
        int segmentIndex;           ///< Index of closest segment
        QGeoCoordinate closestPoint; ///< Closest point on path
    };

    /// Calculate cross-track distance from a point to a multi-segment path
    /// @param point The point to measure from
    /// @param path The path (at least 2 points)
    /// @return Cross-track result with distance and segment info
    CrossTrackResult crossTrackDistanceToPath(const QGeoCoordinate &point,
                                               const QList<QGeoCoordinate> &path);

    // ========================================================================
    // Turn Radius Validation (Fixed-Wing Path Planning)
    // ========================================================================

    /// Calculate the turn angle at a waypoint
    /// @param prev Previous waypoint
    /// @param current Current waypoint (turn location)
    /// @param next Next waypoint
    /// @return Turn angle in degrees [0, 180] (0 = straight, 180 = reversal)
    double turnAngle(const QGeoCoordinate &prev,
                      const QGeoCoordinate &current,
                      const QGeoCoordinate &next);

    /// Check if a turn can be completed within a given radius constraint
    /// @param prev Previous waypoint
    /// @param current Current waypoint
    /// @param next Next waypoint
    /// @param minTurnRadius Minimum turn radius in meters
    /// @return true if turn is feasible (leg lengths sufficient for turn radius)
    bool isTurnFeasible(const QGeoCoordinate &prev,
                         const QGeoCoordinate &current,
                         const QGeoCoordinate &next,
                         double minTurnRadius);

    /// Validate all turns in a path for a given minimum turn radius
    /// @param path Path waypoints (at least 3 for any turns)
    /// @param minTurnRadius Minimum turn radius in meters
    /// @param[out] invalidIndices Indices of waypoints with infeasible turns
    /// @return true if all turns are feasible
    bool validatePathTurns(const QList<QGeoCoordinate> &path,
                            double minTurnRadius,
                            QList<int> &invalidIndices);

    /// Calculate required turn radius for a given turn
    /// @param prev Previous waypoint
    /// @param current Current waypoint
    /// @param next Next waypoint
    /// @return Required turn radius in meters (based on leg geometry)
    double requiredTurnRadius(const QGeoCoordinate &prev,
                               const QGeoCoordinate &current,
                               const QGeoCoordinate &next);

    // ========================================================================
    // Path Densification
    // ========================================================================

    /// Densify a path by adding intermediate points at maximum spacing
    /// @param coords Input path coordinates
    /// @param maxSpacingMeters Maximum distance between consecutive points
    /// @return Densified path with all segments <= maxSpacingMeters
    QList<QGeoCoordinate> densifyPath(const QList<QGeoCoordinate> &coords,
                                       double maxSpacingMeters);

    /// Densify a polygon by adding intermediate points at maximum spacing
    /// @param vertices Input polygon vertices
    /// @param maxSpacingMeters Maximum distance between consecutive vertices
    /// @return Densified polygon with all edges <= maxSpacingMeters
    QList<QGeoCoordinate> densifyPolygon(const QList<QGeoCoordinate> &vertices,
                                          double maxSpacingMeters);

    // ========================================================================
    // No-Fly Zone / Geofence Checking
    // ========================================================================

    /// Check if a path intersects any no-fly zone
    /// @param path Path to check
    /// @param noFlyZones List of no-fly zone polygons
    /// @return Index of first intersecting zone, or -1 if none
    int pathIntersectsNoFlyZone(const QList<QGeoCoordinate> &path,
                                 const QList<QList<QGeoCoordinate>> &noFlyZones);

    /// Check if a point is inside any no-fly zone
    /// @param point Point to check
    /// @param noFlyZones List of no-fly zone polygons
    /// @return Index of containing zone, or -1 if none
    int pointInNoFlyZone(const QGeoCoordinate &point,
                          const QList<QList<QGeoCoordinate>> &noFlyZones);

    /// Find all intersections between a path and no-fly zones
    /// @param path Path to check
    /// @param noFlyZones List of no-fly zone polygons
    /// @return List of intersection points with zone indices
    struct NoFlyZoneIntersection {
        QGeoCoordinate point;  ///< Intersection point
        int zoneIndex;         ///< Index of no-fly zone
        int segmentIndex;      ///< Index of path segment
        bool entering;         ///< true if entering zone, false if exiting
    };
    QList<NoFlyZoneIntersection> findNoFlyZoneIntersections(
        const QList<QGeoCoordinate> &path,
        const QList<QList<QGeoCoordinate>> &noFlyZones);

    // ========================================================================
    // Polygon Triangulation
    // ========================================================================

    /// Triangle represented by three coordinate indices
    struct Triangle {
        int a, b, c;  ///< Indices into the original vertex list
    };

    /// Triangulate a simple polygon using ear clipping algorithm
    /// @param vertices Polygon vertices (simple polygon, no holes)
    /// @return List of triangles (indices into vertices)
    /// @note Polygon must be simple (non-self-intersecting)
    QList<Triangle> triangulatePolygon(const QList<QGeoCoordinate> &vertices);

    /// Triangulate a polygon with holes
    /// @param outerRing Outer boundary vertices
    /// @param holes List of hole vertex lists
    /// @param[out] allVertices Combined vertex list (outer + holes)
    /// @return List of triangles (indices into allVertices)
    QList<Triangle> triangulatePolygonWithHoles(
        const QList<QGeoCoordinate> &outerRing,
        const QList<QList<QGeoCoordinate>> &holes,
        QList<QGeoCoordinate> &allVertices);

    // ========================================================================
    // Plus Codes (Open Location Code)
    // ========================================================================

    /// Convert a coordinate to a Plus Code (Open Location Code)
    /// @param coord Coordinate to encode
    /// @param codeLength Length of code (10, 11, 12, etc. - default 10 for ~14m precision)
    /// @return Plus Code string (e.g., "8FVC9G8F+6W")
    QString coordinateToPlusCode(const QGeoCoordinate &coord, int codeLength = 10);

    /// Convert a Plus Code to a coordinate
    /// @param plusCode Plus Code string
    /// @param[out] coord Decoded coordinate (center of code area)
    /// @return true if conversion succeeded
    bool plusCodeToCoordinate(const QString &plusCode, QGeoCoordinate &coord);

    /// Check if a string is a valid Plus Code
    /// @param code String to validate
    /// @return true if valid Plus Code format
    bool isValidPlusCode(const QString &code);

    // ========================================================================
    // Path Corridor (Buffer Around Polyline)
    // ========================================================================

    /// Generate a corridor polygon around a path
    /// @param path Center line of corridor
    /// @param widthMeters Total corridor width (half on each side)
    /// @return Polygon vertices defining the corridor
    /// @note Useful for visualizing flight corridors or creating geofences
    QList<QGeoCoordinate> pathCorridor(const QList<QGeoCoordinate> &path,
                                        double widthMeters);

    /// Check if a point is within a path corridor
    /// @param point Point to check
    /// @param path Center line of corridor
    /// @param widthMeters Total corridor width
    /// @return true if point is within corridor
    bool pointInCorridor(const QGeoCoordinate &point,
                          const QList<QGeoCoordinate> &path,
                          double widthMeters);
}
