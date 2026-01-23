#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QList>
#include <QtPositioning/QGeoCoordinate>

class MissionItem;
class Vehicle;
class QmlObjectListModel;

/// Used to convert a Plan to a GeoJSON FeatureCollection document
class GeoJsonPlanDocument
{
public:
    GeoJsonPlanDocument();

    void addMission(Vehicle* vehicle, QmlObjectListModel* visualItems, QList<MissionItem*> rgMissionItems);

    QJsonObject toJsonObject() const;
    QByteArray toJson() const;

private:
    void _addFlightPath(Vehicle* vehicle, QList<MissionItem*> rgMissionItems);
    void _addWaypoints(Vehicle* vehicle, QList<MissionItem*> rgMissionItems);
    void _addComplexItems(QmlObjectListModel* visualItems);

    static QJsonArray _coordToJsonArray(const QGeoCoordinate& coord);
    static QJsonObject _createFeature(const QString& geometryType, const QJsonValue& coordinates, const QJsonObject& properties);

    QJsonArray _features;
};
