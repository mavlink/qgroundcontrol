#pragma once

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtPositioning/QGeoCoordinate>

class MissionItem;
class Vehicle;
class QmlObjectListModel;

/// Used to convert a Plan to an OGC GeoPackage (.gpkg) document
/// Creates a single SQLite-based file with multiple feature tables:
///   - waypoints - Point features with attributes
///   - flight_path - LineString for flight path
///   - areas - Polygons for survey/structure areas
class GeoPackagePlanDocument
{
public:
    GeoPackagePlanDocument();

    void addMission(Vehicle* vehicle, QmlObjectListModel* visualItems, QList<MissionItem*> rgMissionItems);

    /// Export to GeoPackage file
    /// @param filename Output filename (should end in .gpkg)
    /// @param errorString Output error message on failure
    /// @return true on success
    bool exportToFile(const QString& filename, QString& errorString) const;

    struct WaypointInfo {
        QGeoCoordinate coordinate;
        int sequenceNumber = 0;
        int command = 0;
        QString commandName;
        double altitudeAMSL = 0;
        double altitudeRelative = 0;
        bool isStandalone = false;
    };

private:
    void _addFlightPath(Vehicle* vehicle, QList<MissionItem*> rgMissionItems);
    void _addWaypoints(Vehicle* vehicle, QList<MissionItem*> rgMissionItems);
    void _addComplexItems(QmlObjectListModel* visualItems);

    QList<QGeoCoordinate> _flightPathCoords;
    QList<WaypointInfo> _waypoints;
    QList<QList<QGeoCoordinate>> _areaPolygons;
    QStringList _areaNames;
};
