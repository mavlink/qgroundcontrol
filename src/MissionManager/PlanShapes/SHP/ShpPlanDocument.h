#pragma once

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtPositioning/QGeoCoordinate>

class MissionItem;
class Vehicle;
class QmlObjectListModel;

/// Used to convert a Plan to a set of Shapefile documents
/// Creates multiple files due to SHP format limitation (one geometry type per file):
///   - {basename}_waypoints.shp - Point features with attributes
///   - {basename}_path.shp - LineString for flight path
///   - {basename}_areas.shp - Polygons for survey/structure areas
class ShpPlanDocument
{
public:
    ShpPlanDocument();

    void addMission(Vehicle* vehicle, QmlObjectListModel* visualItems, QList<MissionItem*> rgMissionItems);

    /// Export to shapefile set. The filename should end in .shp
    /// Creates multiple files with suffixes (_waypoints, _path, _areas)
    /// @param baseFilename Base filename (e.g., "mission.shp" creates mission_waypoints.shp, etc.)
    /// @param errorString Output error message on failure
    /// @return true on success
    bool exportToFiles(const QString& baseFilename, QString& errorString) const;

    /// Returns list of created files (populated after exportToFiles)
    QStringList createdFiles() const { return _createdFiles; }

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

    bool _exportWaypoints(const QString& filename, QString& errorString) const;
    bool _exportFlightPath(const QString& filename, QString& errorString) const;
    bool _exportAreas(const QString& filename, QString& errorString) const;

    static QString _makeFilename(const QString& baseFilename, const QString& suffix);

    QList<QGeoCoordinate> _flightPathCoords;
    QList<WaypointInfo> _waypoints;
    QList<QList<QGeoCoordinate>> _areaPolygons;
    QStringList _areaNames;

    mutable QStringList _createdFiles;
};
