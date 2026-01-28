#include "GeoJsonPlanDocument.h"
#include "MissionCommandTree.h"
#include "MissionCommandUIInfo.h"
#include "MissionItem.h"
#include "Vehicle.h"
#include "QmlObjectListModel.h"
#include "TransectStyleComplexItem.h"

#include <QtCore/QJsonDocument>

GeoJsonPlanDocument::GeoJsonPlanDocument()
{
}

QJsonArray GeoJsonPlanDocument::_coordToJsonArray(const QGeoCoordinate& coord)
{
    QJsonArray arr;
    arr.append(coord.longitude());
    arr.append(coord.latitude());
    if (!qIsNaN(coord.altitude())) {
        arr.append(coord.altitude());
    }
    return arr;
}

QJsonObject GeoJsonPlanDocument::_createFeature(const QString& geometryType, const QJsonValue& coordinates, const QJsonObject& properties)
{
    QJsonObject geometry;
    geometry[QStringLiteral("type")] = geometryType;
    geometry[QStringLiteral("coordinates")] = coordinates;

    QJsonObject feature;
    feature[QStringLiteral("type")] = QStringLiteral("Feature");
    feature[QStringLiteral("geometry")] = geometry;
    feature[QStringLiteral("properties")] = properties;

    return feature;
}

void GeoJsonPlanDocument::_addFlightPath(Vehicle* vehicle, QList<MissionItem*> rgMissionItems)
{
    if (rgMissionItems.count() == 0) {
        return;
    }

    QJsonArray lineCoords;
    QGeoCoordinate homeCoord = rgMissionItems[0]->coordinate();

    for (const MissionItem* item : rgMissionItems) {
        const MissionCommandUIInfo* uiInfo = MissionCommandTree::instance()->getUIInfo(vehicle, QGCMAVLink::VehicleClassGeneric, item->command());
        if (uiInfo) {
            double altAdjustment = item->frame() == MAV_FRAME_GLOBAL ? 0 : homeCoord.altitude();

            if (uiInfo->isTakeoffCommand() && !vehicle->fixedWing()) {
                QGeoCoordinate coord = homeCoord;
                coord.setAltitude(item->param7() + altAdjustment);
                lineCoords.append(_coordToJsonArray(coord));
            }

            if (uiInfo->specifiesCoordinate() && !uiInfo->isStandaloneCoordinate()) {
                QGeoCoordinate coord = item->coordinate();
                coord.setAltitude(coord.altitude() + altAdjustment);
                lineCoords.append(_coordToJsonArray(coord));
            }
        }
    }

    if (lineCoords.count() >= 2) {
        QJsonObject properties;
        properties[QStringLiteral("name")] = QStringLiteral("Flight Path");
        properties[QStringLiteral("featureType")] = QStringLiteral("flightPath");

        _features.append(_createFeature(QStringLiteral("LineString"), lineCoords, properties));
    }
}

void GeoJsonPlanDocument::_addWaypoints(Vehicle* vehicle, QList<MissionItem*> rgMissionItems)
{
    if (rgMissionItems.count() == 0) {
        return;
    }

    QGeoCoordinate homeCoord = rgMissionItems[0]->coordinate();

    for (const MissionItem* item : rgMissionItems) {
        const MissionCommandUIInfo* uiInfo = MissionCommandTree::instance()->getUIInfo(vehicle, QGCMAVLink::VehicleClassGeneric, item->command());
        if (uiInfo && uiInfo->specifiesCoordinate()) {
            double altAdjustment = item->frame() == MAV_FRAME_GLOBAL ? 0 : homeCoord.altitude();
            QGeoCoordinate coord = item->coordinate();
            coord.setAltitude(coord.altitude() + altAdjustment);

            QJsonObject properties;
            properties[QStringLiteral("featureType")] = QStringLiteral("waypoint");
            properties[QStringLiteral("sequenceNumber")] = item->sequenceNumber();
            properties[QStringLiteral("command")] = static_cast<int>(item->command());
            properties[QStringLiteral("commandName")] = uiInfo->friendlyName();
            properties[QStringLiteral("altitudeAMSL")] = coord.altitude();
            properties[QStringLiteral("altitudeRelative")] = coord.altitude() - homeCoord.altitude();
            properties[QStringLiteral("isStandalone")] = uiInfo->isStandaloneCoordinate();

            QString name = QString::number(item->sequenceNumber());
            if (item->command() != MAV_CMD_NAV_WAYPOINT) {
                name += QStringLiteral(" ") + uiInfo->friendlyName();
            }
            properties[QStringLiteral("name")] = name;

            _features.append(_createFeature(QStringLiteral("Point"), _coordToJsonArray(coord), properties));
        }
    }
}

void GeoJsonPlanDocument::_addComplexItems(QmlObjectListModel* visualItems)
{
    int areaIndex = 0;
    for (int i = 0; i < visualItems->count(); i++) {
        auto* transectItem = visualItems->value<TransectStyleComplexItem*>(i);
        if (transectItem) {
            QGCMapPolygon* polygon = transectItem->surveyAreaPolygon();
            if (polygon && polygon->count() >= 3) {
                // Build coordinate ring (GeoJSON polygons need closed rings)
                QJsonArray ring;
                for (int j = 0; j < polygon->count(); j++) {
                    ring.append(_coordToJsonArray(polygon->vertexCoordinate(j)));
                }
                // Close the ring
                ring.append(_coordToJsonArray(polygon->vertexCoordinate(0)));

                // GeoJSON Polygon coordinates is array of rings (first is exterior)
                QJsonArray coordinates;
                coordinates.append(ring);

                QJsonObject properties;
                properties[QStringLiteral("featureType")] = QStringLiteral("surveyArea");
                properties[QStringLiteral("name")] = transectItem->patternName();
                properties[QStringLiteral("index")] = areaIndex++;

                _features.append(_createFeature(QStringLiteral("Polygon"), coordinates, properties));
            }
        }
    }
}

void GeoJsonPlanDocument::addMission(Vehicle* vehicle, QmlObjectListModel* visualItems, QList<MissionItem*> rgMissionItems)
{
    _addFlightPath(vehicle, rgMissionItems);
    _addWaypoints(vehicle, rgMissionItems);
    _addComplexItems(visualItems);
}

QJsonObject GeoJsonPlanDocument::toJsonObject() const
{
    QJsonObject root;
    root[QStringLiteral("type")] = QStringLiteral("FeatureCollection");
    root[QStringLiteral("features")] = _features;

    // Add metadata
    QJsonObject properties;
    properties[QStringLiteral("generator")] = QCoreApplication::applicationName();
    properties[QStringLiteral("generatorVersion")] = QCoreApplication::applicationVersion();
    root[QStringLiteral("properties")] = properties;

    return root;
}

QByteArray GeoJsonPlanDocument::toJson() const
{
    QJsonDocument doc(toJsonObject());
    return doc.toJson(QJsonDocument::Indented);
}
