#pragma once

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtPositioning/QGeoCoordinate>

class MissionItem;
class Vehicle;
class QmlObjectListModel;

/// Base class for plan document exporters
/// Provides common structure and methods that subclasses can use or override.
/// Subclasses inherit protected data members for flight path, waypoints, and survey areas.
class PlanDocumentBase
{
public:
    PlanDocumentBase() = default;
    virtual ~PlanDocumentBase() = default;

    /// Process a mission and extract geometry/waypoint data
    /// Default implementation calls the protected extraction methods
    virtual void addMission(Vehicle* vehicle, QmlObjectListModel* visualItems, QList<MissionItem*> rgMissionItems);

    /// Clear all extracted data
    virtual void clear();

    /// Check if document has any content
    virtual bool isEmpty() const;

protected:
    // ========================================================================
    // Common extraction methods - subclasses can call these or override addMission
    // ========================================================================

    /// Extract flight path coordinates from mission items
    /// Populates _flightPath with coordinates that form the flight track
    void extractFlightPath(Vehicle* vehicle, const QList<MissionItem*>& missionItems);

    /// Extract waypoint coordinates from mission items
    /// Populates _waypoints with coordinates that specify locations
    void extractWaypoints(Vehicle* vehicle, const QList<MissionItem*>& missionItems);

    /// Extract survey area polygons from complex items
    /// Populates _surveyAreas and _surveyAreaNames from TransectStyleComplexItem polygons
    void extractSurveyAreas(QmlObjectListModel* visualItems);

    // ========================================================================
    // Data members
    // ========================================================================

    /// Flight path as coordinate list
    QList<QGeoCoordinate> _flightPath;

    /// Waypoint coordinates
    QList<QGeoCoordinate> _waypoints;

    /// Survey area polygons (from complex items)
    QList<QList<QGeoCoordinate>> _surveyAreas;

    /// Names for survey areas (parallel to _surveyAreas)
    QStringList _surveyAreaNames;
};
