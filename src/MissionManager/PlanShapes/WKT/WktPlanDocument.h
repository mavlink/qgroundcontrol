#pragma once

#include "PlanDocument.h"

#include <QtCore/QList>
#include <QtPositioning/QGeoCoordinate>

class MissionItem;
class Vehicle;
class QmlObjectListModel;

/// Used to convert a Plan to a WKT (Well-Known Text) document
/// @see https://en.wikipedia.org/wiki/Well-known_text_representation_of_geometry
class WktPlanDocument : public PlanDocumentBase
{
public:
    WktPlanDocument() = default;

    void addMission(Vehicle* vehicle, QmlObjectListModel* visualItems, QList<MissionItem*> rgMissionItems) override;

    bool saveToFile(const QString& filename, QString& errorString) const;

    /// Get WKT representation of the flight path as LINESTRING
    QString flightPathWkt() const;

    /// Get WKT representation of waypoints as MULTIPOINT
    QString waypointsWkt() const;

    /// Get WKT representation of survey areas as MULTIPOLYGON
    QString surveyAreasWkt() const;

    /// Get combined WKT output (GEOMETRYCOLLECTION)
    QString combinedWkt() const;

private:
    bool _includeAltitude = true;
};
