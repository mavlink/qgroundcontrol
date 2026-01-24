#pragma once

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtPositioning/QGeoCoordinate>

#include "MAVLinkLib.h"

/// @file
/// @brief Test data factories using builder patterns
///
/// These factories make it easy to create complex test objects with sensible
/// defaults while allowing customization of specific properties.
///
/// Usage:
/// @code
/// // Create a simple mission with defaults
/// auto coords = MissionBuilder().withWaypoints(5).build();
///
/// // Create a customized mission
/// auto coords = MissionBuilder()
///     .atLocation(47.3977, 8.5456)
///     .withTakeoff(50.0)
///     .withWaypoints(3)
///     .withSpacing(100.0)
///     .withLanding()
///     .build();
///
/// // Create a survey polygon
/// auto polygon = PolygonBuilder()
///     .centeredAt(47.3977, 8.5456)
///     .withRadius(500.0)
///     .withVertices(6)
///     .build();
/// @endcode

namespace TestFactories {

// ============================================================================
// Coordinate Builder
// ============================================================================

/// Builder for creating test coordinates
class CoordinateBuilder
{
public:
    CoordinateBuilder() = default;

    /// Set latitude
    CoordinateBuilder& latitude(double lat) { _latitude = lat; return *this; }

    /// Set longitude
    CoordinateBuilder& longitude(double lon) { _longitude = lon; return *this; }

    /// Set altitude
    CoordinateBuilder& altitude(double alt) { _altitude = alt; return *this; }

    /// Set all at once
    CoordinateBuilder& at(double lat, double lon, double alt = qQNaN())
    {
        _latitude = lat;
        _longitude = lon;
        _altitude = alt;
        return *this;
    }

    /// Offset from current position
    CoordinateBuilder& offsetMeters(double north, double east)
    {
        _offsetNorth = north;
        _offsetEast = east;
        return *this;
    }

    /// Build the coordinate
    QGeoCoordinate build() const
    {
        QGeoCoordinate coord(_latitude, _longitude);
        if (!qIsNaN(_altitude)) {
            coord.setAltitude(_altitude);
        }

        // Apply offset if specified
        if (_offsetNorth != 0.0 || _offsetEast != 0.0) {
            // Approximate: 1 degree latitude = 111km
            const double latOffset = _offsetNorth / 111000.0;
            const double lonOffset = _offsetEast / (111000.0 * std::cos(_latitude * M_PI / 180.0));
            coord.setLatitude(coord.latitude() + latOffset);
            coord.setLongitude(coord.longitude() + lonOffset);
        }

        return coord;
    }

private:
    double _latitude = 47.3977;   // Default: Zurich area
    double _longitude = 8.5456;
    double _altitude = qQNaN();
    double _offsetNorth = 0.0;
    double _offsetEast = 0.0;
};

// ============================================================================
// Mission Builder
// ============================================================================

/// Builder for creating mission coordinate sequences
class MissionBuilder
{
public:
    MissionBuilder() = default;

    /// Set the starting location
    MissionBuilder& atLocation(double lat, double lon)
    {
        _startLat = lat;
        _startLon = lon;
        return *this;
    }

    /// Set the starting coordinate
    MissionBuilder& atLocation(const QGeoCoordinate& coord)
    {
        _startLat = coord.latitude();
        _startLon = coord.longitude();
        if (!qIsNaN(coord.altitude())) {
            _defaultAltitude = coord.altitude();
        }
        return *this;
    }

    /// Add a takeoff point at specified altitude
    MissionBuilder& withTakeoff(double altitude = 50.0)
    {
        _includeTakeoff = true;
        _takeoffAltitude = altitude;
        return *this;
    }

    /// Add N waypoints
    MissionBuilder& withWaypoints(int count)
    {
        _waypointCount = count;
        return *this;
    }

    /// Set spacing between waypoints in meters
    MissionBuilder& withSpacing(double meters)
    {
        _spacingMeters = meters;
        return *this;
    }

    /// Set default altitude for waypoints
    MissionBuilder& withAltitude(double altitude)
    {
        _defaultAltitude = altitude;
        return *this;
    }

    /// Add a landing point
    MissionBuilder& withLanding()
    {
        _includeLanding = true;
        return *this;
    }

    /// Set the pattern for waypoint placement
    enum class Pattern { Line, Square, Circle, Zigzag };
    MissionBuilder& withPattern(Pattern pattern)
    {
        _pattern = pattern;
        return *this;
    }

    /// Build the mission as a list of coordinates
    QList<QGeoCoordinate> build() const
    {
        QList<QGeoCoordinate> coords;
        QGeoCoordinate current(_startLat, _startLon, _defaultAltitude);

        // Add takeoff
        if (_includeTakeoff) {
            QGeoCoordinate takeoff = current;
            takeoff.setAltitude(_takeoffAltitude);
            coords.append(takeoff);
        }

        // Add waypoints based on pattern
        switch (_pattern) {
        case Pattern::Line:
            buildLinePattern(coords, current);
            break;
        case Pattern::Square:
            buildSquarePattern(coords, current);
            break;
        case Pattern::Circle:
            buildCirclePattern(coords, current);
            break;
        case Pattern::Zigzag:
            buildZigzagPattern(coords, current);
            break;
        }

        // Add landing
        if (_includeLanding) {
            QGeoCoordinate landing(_startLat, _startLon, 0);
            coords.append(landing);
        }

        return coords;
    }

private:
    void buildLinePattern(QList<QGeoCoordinate>& coords, QGeoCoordinate start) const
    {
        for (int i = 0; i < _waypointCount; ++i) {
            QGeoCoordinate wp = start.atDistanceAndAzimuth(_spacingMeters * (i + 1), 0); // North
            wp.setAltitude(_defaultAltitude);
            coords.append(wp);
        }
    }

    void buildSquarePattern(QList<QGeoCoordinate>& coords, QGeoCoordinate start) const
    {
        const double bearings[] = {0, 90, 180, 270};
        QGeoCoordinate current = start;

        for (int i = 0; i < _waypointCount; ++i) {
            double bearing = bearings[i % 4];
            current = current.atDistanceAndAzimuth(_spacingMeters, bearing);
            current.setAltitude(_defaultAltitude);
            coords.append(current);
        }
    }

    void buildCirclePattern(QList<QGeoCoordinate>& coords, QGeoCoordinate center) const
    {
        const double angleStep = 360.0 / _waypointCount;
        for (int i = 0; i < _waypointCount; ++i) {
            QGeoCoordinate wp = center.atDistanceAndAzimuth(_spacingMeters, angleStep * i);
            wp.setAltitude(_defaultAltitude);
            coords.append(wp);
        }
    }

    void buildZigzagPattern(QList<QGeoCoordinate>& coords, QGeoCoordinate start) const
    {
        QGeoCoordinate current = start;
        for (int i = 0; i < _waypointCount; ++i) {
            double bearing = (i % 2 == 0) ? 45 : 135;  // Alternate NE and SE
            current = current.atDistanceAndAzimuth(_spacingMeters, bearing);
            current.setAltitude(_defaultAltitude);
            coords.append(current);
        }
    }

    double _startLat = 47.3977;
    double _startLon = 8.5456;
    double _defaultAltitude = 100.0;
    double _takeoffAltitude = 50.0;
    double _spacingMeters = 50.0;
    int _waypointCount = 0;
    bool _includeTakeoff = false;
    bool _includeLanding = false;
    Pattern _pattern = Pattern::Line;
};

// ============================================================================
// Polygon Builder
// ============================================================================

/// Builder for creating test polygons (for surveys, geofences, etc.)
class PolygonBuilder
{
public:
    PolygonBuilder() = default;

    /// Set center location
    PolygonBuilder& centeredAt(double lat, double lon)
    {
        _centerLat = lat;
        _centerLon = lon;
        return *this;
    }

    /// Set center from coordinate
    PolygonBuilder& centeredAt(const QGeoCoordinate& coord)
    {
        _centerLat = coord.latitude();
        _centerLon = coord.longitude();
        return *this;
    }

    /// Set radius in meters
    PolygonBuilder& withRadius(double meters)
    {
        _radiusMeters = meters;
        return *this;
    }

    /// Set number of vertices
    PolygonBuilder& withVertices(int count)
    {
        _vertexCount = qMax(3, count);
        return *this;
    }

    /// Add random variation to vertices (0.0 to 1.0)
    PolygonBuilder& withVariation(double factor)
    {
        _variationFactor = qBound(0.0, factor, 1.0);
        return *this;
    }

    /// Set rotation angle in degrees
    PolygonBuilder& rotatedBy(double degrees)
    {
        _rotationDegrees = degrees;
        return *this;
    }

    /// Build as rectangle instead of regular polygon
    PolygonBuilder& asRectangle(double width, double height)
    {
        _isRectangle = true;
        _rectWidth = width;
        _rectHeight = height;
        return *this;
    }

    /// Build the polygon
    QList<QGeoCoordinate> build() const
    {
        if (_isRectangle) {
            return buildRectangle();
        }
        return buildRegularPolygon();
    }

private:
    QList<QGeoCoordinate> buildRegularPolygon() const
    {
        QList<QGeoCoordinate> coords;
        QGeoCoordinate center(_centerLat, _centerLon);

        const double angleStep = 360.0 / _vertexCount;

        for (int i = 0; i < _vertexCount; ++i) {
            double angle = _rotationDegrees + (angleStep * i);
            double radius = _radiusMeters;

            // Add variation if requested
            if (_variationFactor > 0) {
                // Simple pseudo-random based on index
                double variation = (std::sin(i * 1.618) + 1.0) / 2.0;  // 0 to 1
                radius *= (1.0 - _variationFactor/2) + (variation * _variationFactor);
            }

            QGeoCoordinate vertex = center.atDistanceAndAzimuth(radius, angle);
            coords.append(vertex);
        }

        return coords;
    }

    QList<QGeoCoordinate> buildRectangle() const
    {
        QList<QGeoCoordinate> coords;
        QGeoCoordinate center(_centerLat, _centerLon);

        // Calculate corners relative to center
        const double halfWidth = _rectWidth / 2.0;
        const double halfHeight = _rectHeight / 2.0;

        // Corner offsets (before rotation)
        const double corners[4][2] = {
            {-halfWidth, halfHeight},   // NW
            {halfWidth, halfHeight},    // NE
            {halfWidth, -halfHeight},   // SE
            {-halfWidth, -halfHeight}   // SW
        };

        const double rotRad = _rotationDegrees * M_PI / 180.0;
        const double cosR = std::cos(rotRad);
        const double sinR = std::sin(rotRad);

        for (const auto& corner : corners) {
            // Rotate
            double x = corner[0] * cosR - corner[1] * sinR;
            double y = corner[0] * sinR + corner[1] * cosR;

            // Convert to lat/lon offset
            double distance = std::sqrt(x*x + y*y);
            double bearing = std::atan2(x, y) * 180.0 / M_PI;

            QGeoCoordinate vertex = center.atDistanceAndAzimuth(distance, bearing);
            coords.append(vertex);
        }

        return coords;
    }

    double _centerLat = 47.3977;
    double _centerLon = 8.5456;
    double _radiusMeters = 100.0;
    int _vertexCount = 4;
    double _variationFactor = 0.0;
    double _rotationDegrees = 0.0;
    bool _isRectangle = false;
    double _rectWidth = 100.0;
    double _rectHeight = 100.0;
};

// ============================================================================
// Polyline Builder
// ============================================================================

/// Builder for creating test polylines (corridors, paths, etc.)
class PolylineBuilder
{
public:
    PolylineBuilder() = default;

    /// Set starting location
    PolylineBuilder& startingAt(double lat, double lon)
    {
        _startLat = lat;
        _startLon = lon;
        return *this;
    }

    /// Set starting coordinate
    PolylineBuilder& startingAt(const QGeoCoordinate& coord)
    {
        _startLat = coord.latitude();
        _startLon = coord.longitude();
        return *this;
    }

    /// Set number of points
    PolylineBuilder& withPoints(int count)
    {
        _pointCount = qMax(2, count);
        return *this;
    }

    /// Set segment length in meters
    PolylineBuilder& withSegmentLength(double meters)
    {
        _segmentLength = meters;
        return *this;
    }

    /// Set initial bearing in degrees
    PolylineBuilder& withBearing(double degrees)
    {
        _bearing = degrees;
        return *this;
    }

    /// Add turns every N segments
    PolylineBuilder& withTurnsEvery(int segments, double turnAngle = 90.0)
    {
        _turnEvery = segments;
        _turnAngle = turnAngle;
        return *this;
    }

    /// Build the polyline
    QList<QGeoCoordinate> build() const
    {
        QList<QGeoCoordinate> coords;
        QGeoCoordinate current(_startLat, _startLon);
        coords.append(current);

        double currentBearing = _bearing;

        for (int i = 1; i < _pointCount; ++i) {
            // Apply turn if needed
            if (_turnEvery > 0 && i % _turnEvery == 0) {
                currentBearing += _turnAngle;
            }

            current = current.atDistanceAndAzimuth(_segmentLength, currentBearing);
            coords.append(current);
        }

        return coords;
    }

private:
    double _startLat = 47.3977;
    double _startLon = 8.5456;
    int _pointCount = 2;
    double _segmentLength = 100.0;
    double _bearing = 0.0;
    int _turnEvery = 0;
    double _turnAngle = 90.0;
};

// ============================================================================
// Vehicle Configuration Builder
// ============================================================================

/// Builder for creating vehicle configuration data
class VehicleConfigBuilder
{
public:
    VehicleConfigBuilder() = default;

    /// Set autopilot type
    VehicleConfigBuilder& withAutopilot(MAV_AUTOPILOT autopilot)
    {
        _autopilot = autopilot;
        return *this;
    }

    /// Set vehicle type
    VehicleConfigBuilder& withVehicleType(MAV_TYPE type)
    {
        _vehicleType = type;
        return *this;
    }

    /// Set as PX4 quadcopter
    VehicleConfigBuilder& asPX4Quad()
    {
        _autopilot = MAV_AUTOPILOT_PX4;
        _vehicleType = MAV_TYPE_QUADROTOR;
        return *this;
    }

    /// Set as ArduCopter
    VehicleConfigBuilder& asArduCopter()
    {
        _autopilot = MAV_AUTOPILOT_ARDUPILOTMEGA;
        _vehicleType = MAV_TYPE_QUADROTOR;
        return *this;
    }

    /// Set as ArduPlane
    VehicleConfigBuilder& asArduPlane()
    {
        _autopilot = MAV_AUTOPILOT_ARDUPILOTMEGA;
        _vehicleType = MAV_TYPE_FIXED_WING;
        return *this;
    }

    /// Set as ArduRover
    VehicleConfigBuilder& asArduRover()
    {
        _autopilot = MAV_AUTOPILOT_ARDUPILOTMEGA;
        _vehicleType = MAV_TYPE_GROUND_ROVER;
        return *this;
    }

    /// Set as ArduSub
    VehicleConfigBuilder& asArduSub()
    {
        _autopilot = MAV_AUTOPILOT_ARDUPILOTMEGA;
        _vehicleType = MAV_TYPE_SUBMARINE;
        return *this;
    }

    /// Enable high latency mode
    VehicleConfigBuilder& withHighLatency(bool enabled = true)
    {
        _highLatency = enabled;
        return *this;
    }

    /// Set system ID
    VehicleConfigBuilder& withSystemId(int id)
    {
        _systemId = id;
        return *this;
    }

    // Getters
    MAV_AUTOPILOT autopilot() const { return _autopilot; }
    MAV_TYPE vehicleType() const { return _vehicleType; }
    bool highLatency() const { return _highLatency; }
    int systemId() const { return _systemId; }

private:
    MAV_AUTOPILOT _autopilot = MAV_AUTOPILOT_PX4;
    MAV_TYPE _vehicleType = MAV_TYPE_QUADROTOR;
    bool _highLatency = false;
    int _systemId = 1;
};

// ============================================================================
// Parameter Set Builder
// ============================================================================

/// Builder for creating test parameter sets
class ParameterSetBuilder
{
public:
    ParameterSetBuilder() = default;

    /// Add a parameter
    ParameterSetBuilder& withParam(const QString& name, const QVariant& value)
    {
        _params[name] = value;
        return *this;
    }

    /// Add multiple parameters
    ParameterSetBuilder& withParams(const QMap<QString, QVariant>& params)
    {
        for (auto it = params.begin(); it != params.end(); ++it) {
            _params[it.key()] = it.value();
        }
        return *this;
    }

    /// Add standard PX4 parameters
    ParameterSetBuilder& withPX4Defaults()
    {
        _params["SYS_AUTOSTART"] = 4001;
        _params["COM_ARM_WO_GPS"] = 1;
        _params["COM_RC_IN_MODE"] = 1;
        _params["NAV_ACC_RAD"] = 10.0f;
        _params["MPC_XY_VEL_MAX"] = 12.0f;
        _params["MPC_Z_VEL_MAX_UP"] = 3.0f;
        _params["MPC_Z_VEL_MAX_DN"] = 1.5f;
        return *this;
    }

    /// Add standard ArduCopter parameters
    ParameterSetBuilder& withArduCopterDefaults()
    {
        _params["FRAME_CLASS"] = 1;
        _params["ARMING_CHECK"] = 1;
        _params["FS_THR_ENABLE"] = 1;
        _params["WPNAV_SPEED"] = 500;
        _params["WPNAV_SPEED_UP"] = 250;
        _params["WPNAV_SPEED_DN"] = 150;
        return *this;
    }

    /// Build the parameter map
    QMap<QString, QVariant> build() const { return _params; }

private:
    QMap<QString, QVariant> _params;
};

// ============================================================================
// Convenience Functions
// ============================================================================

/// Create a simple line of waypoints
inline QList<QGeoCoordinate> createWaypointLine(int count, double spacingMeters = 50.0)
{
    return MissionBuilder()
        .withWaypoints(count)
        .withSpacing(spacingMeters)
        .build();
}

/// Create a simple polygon centered at default location
inline QList<QGeoCoordinate> createPolygon(int vertices, double radiusMeters = 100.0)
{
    return PolygonBuilder()
        .withVertices(vertices)
        .withRadius(radiusMeters)
        .build();
}

/// Create a survey rectangle
inline QList<QGeoCoordinate> createSurveyRectangle(double width, double height)
{
    return PolygonBuilder()
        .asRectangle(width, height)
        .build();
}

/// Create a coordinate at the default test location
inline QGeoCoordinate defaultCoordinate()
{
    return CoordinateBuilder().build();
}

/// Create a coordinate with altitude
inline QGeoCoordinate coordinateWithAltitude(double altitude)
{
    return CoordinateBuilder().altitude(altitude).build();
}

} // namespace TestFactories
