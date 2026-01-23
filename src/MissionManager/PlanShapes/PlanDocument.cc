#include "PlanDocument.h"
#include "MissionCommandTree.h"
#include "MissionCommandUIInfo.h"
#include "MissionItem.h"
#include "Vehicle.h"
#include "QmlObjectListModel.h"
#include "TransectStyleComplexItem.h"

void PlanDocumentBase::addMission(Vehicle* vehicle, QmlObjectListModel* visualItems, QList<MissionItem*> rgMissionItems)
{
    clear();
    extractFlightPath(vehicle, rgMissionItems);
    extractWaypoints(vehicle, rgMissionItems);
    extractSurveyAreas(visualItems);
}

void PlanDocumentBase::clear()
{
    _flightPath.clear();
    _waypoints.clear();
    _surveyAreas.clear();
    _surveyAreaNames.clear();
}

bool PlanDocumentBase::isEmpty() const
{
    return _flightPath.isEmpty() && _waypoints.isEmpty() && _surveyAreas.isEmpty();
}

void PlanDocumentBase::extractFlightPath(Vehicle* vehicle, const QList<MissionItem*>& missionItems)
{
    if (missionItems.count() == 0) {
        return;
    }

    QGeoCoordinate homeCoord = missionItems[0]->coordinate();

    for (const MissionItem* item : missionItems) {
        const MissionCommandUIInfo* uiInfo = MissionCommandTree::instance()->getUIInfo(
            vehicle, QGCMAVLink::VehicleClassGeneric, item->command());

        if (uiInfo) {
            double altAdjustment = item->frame() == MAV_FRAME_GLOBAL ? 0 : homeCoord.altitude();

            if (uiInfo->isTakeoffCommand() && !vehicle->fixedWing()) {
                QGeoCoordinate coord = homeCoord;
                coord.setAltitude(item->param7() + altAdjustment);
                _flightPath.append(coord);
            }

            if (uiInfo->specifiesCoordinate() && !uiInfo->isStandaloneCoordinate()) {
                QGeoCoordinate coord = item->coordinate();
                coord.setAltitude(coord.altitude() + altAdjustment);
                _flightPath.append(coord);
            }
        }
    }
}

void PlanDocumentBase::extractWaypoints(Vehicle* vehicle, const QList<MissionItem*>& missionItems)
{
    if (missionItems.count() == 0) {
        return;
    }

    QGeoCoordinate homeCoord = missionItems[0]->coordinate();

    for (const MissionItem* item : missionItems) {
        const MissionCommandUIInfo* uiInfo = MissionCommandTree::instance()->getUIInfo(
            vehicle, QGCMAVLink::VehicleClassGeneric, item->command());

        if (uiInfo && uiInfo->specifiesCoordinate()) {
            double altAdjustment = item->frame() == MAV_FRAME_GLOBAL ? 0 : homeCoord.altitude();
            QGeoCoordinate coord = item->coordinate();
            coord.setAltitude(coord.altitude() + altAdjustment);
            _waypoints.append(coord);
        }
    }
}

void PlanDocumentBase::extractSurveyAreas(QmlObjectListModel* visualItems)
{
    if (!visualItems) {
        return;
    }

    for (int i = 0; i < visualItems->count(); i++) {
        auto* transectItem = visualItems->value<TransectStyleComplexItem*>(i);
        if (transectItem) {
            QGCMapPolygon* polygon = transectItem->surveyAreaPolygon();
            if (polygon && polygon->count() >= 3) {
                QList<QGeoCoordinate> vertices;
                for (int j = 0; j < polygon->count(); j++) {
                    vertices.append(polygon->vertexCoordinate(j));
                }
                _surveyAreas.append(vertices);
                _surveyAreaNames.append(transectItem->patternName());
            }
        }
    }
}
