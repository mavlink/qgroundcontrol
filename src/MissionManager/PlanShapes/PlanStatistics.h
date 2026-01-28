#pragma once

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoRectangle>

class MissionItem;
class Vehicle;
class QmlObjectListModel;

/// Statistics and analysis for a flight plan using GeoUtilities
class PlanStatistics
{
public:
    PlanStatistics();

    /// Analyze a mission and compute statistics
    void analyze(Vehicle* vehicle, QmlObjectListModel* visualItems, QList<MissionItem*> rgMissionItems);

    /// Clear all computed statistics
    void clear();

    // ========================================================================
    // Basic Distance/Area Statistics
    // ========================================================================

    /// Total flight path distance in meters
    double totalDistanceMeters() const { return _totalDistance; }

    /// Number of waypoints with coordinates
    int waypointCount() const { return _waypointCount; }

    /// Minimum altitude in the mission (meters above home)
    double minAltitudeMeters() const { return _minAltitude; }

    /// Maximum altitude in the mission (meters above home)
    double maxAltitudeMeters() const { return _maxAltitude; }

    /// Total area covered by all survey polygons (square meters)
    double surveyAreaSqMeters() const { return _surveyArea; }

    // ========================================================================
    // Turn Analysis
    // ========================================================================

    /// Turn angle statistics
    struct TurnInfo {
        int waypointIndex;       ///< Index of waypoint where turn occurs
        double angleDegrees;     ///< Turn angle (0=straight, 180=reversal)
        double requiredRadiusM;  ///< Minimum turn radius needed
        bool feasible;           ///< True if turn is feasible for given minTurnRadius
    };

    /// Get all turn information
    QList<TurnInfo> turnInfo() const { return _turns; }

    /// Set minimum turn radius for feasibility checks (default: 50m)
    void setMinTurnRadius(double radiusMeters) { _minTurnRadius = radiusMeters; }
    double minTurnRadius() const { return _minTurnRadius; }

    /// Number of infeasible turns (for fixed-wing planning)
    int infeasibleTurnCount() const;

    /// Maximum turn angle in the path (degrees)
    double maxTurnAngle() const { return _maxTurnAngle; }

    /// Average turn angle (degrees)
    double avgTurnAngle() const { return _avgTurnAngle; }

    // ========================================================================
    // Bounding Box
    // ========================================================================

    /// Bounding rectangle for all mission coordinates
    QGeoRectangle boundingBox() const { return _boundingBox; }

    /// Center of the mission area
    QGeoCoordinate center() const;

    // ========================================================================
    // Segment Statistics
    // ========================================================================

    /// Segment statistics
    struct SegmentInfo {
        int startIndex;
        int endIndex;
        double distanceMeters;
        double headingDegrees;
    };

    /// Get all segment information
    QList<SegmentInfo> segmentInfo() const { return _segments; }

    /// Shortest segment distance (meters)
    double shortestSegmentMeters() const { return _shortestSegment; }

    /// Longest segment distance (meters)
    double longestSegmentMeters() const { return _longestSegment; }

    /// Average segment distance (meters)
    double avgSegmentMeters() const { return _avgSegment; }

    // ========================================================================
    // Text Summary
    // ========================================================================

    /// Get a human-readable summary of statistics
    QString summary() const;

    /// Get detailed turn analysis as text
    QString turnAnalysisSummary() const;

private:
    void _analyzeFlightPath(Vehicle* vehicle, QList<MissionItem*> rgMissionItems);
    void _analyzeTurns();
    void _analyzeSegments();
    void _analyzeSurveyAreas(QmlObjectListModel* visualItems);

    QList<QGeoCoordinate> _flightPath;
    double _minTurnRadius = 50.0;

    // Computed statistics
    double _totalDistance = 0.0;
    int _waypointCount = 0;
    double _minAltitude = 0.0;
    double _maxAltitude = 0.0;
    double _surveyArea = 0.0;
    QGeoRectangle _boundingBox;

    // Turn analysis
    QList<TurnInfo> _turns;
    double _maxTurnAngle = 0.0;
    double _avgTurnAngle = 0.0;

    // Segment analysis
    QList<SegmentInfo> _segments;
    double _shortestSegment = 0.0;
    double _longestSegment = 0.0;
    double _avgSegment = 0.0;
};
