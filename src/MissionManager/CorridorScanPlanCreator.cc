#include "CorridorScanPlanCreator.h"
#include "PlanMasterController.h"
#include "QGCMAVLink.h"
#include "CorridorScanComplexItem.h"

CorridorScanPlanCreator::CorridorScanPlanCreator(PlanMasterController* planMasterController)
    : PlanCreator(planMasterController, CorridorScanComplexItem::name, QStringLiteral("/qmlimages/PlanCreator/CorridorScanPlanCreator.png"), QGCMAVLink::allVehicleClasses())
{

}

void CorridorScanPlanCreator::createPlan(const QGeoCoordinate& mapCenterCoord)
{
    _planMasterController->removeAll();
    VisualMissionItem* takeoffItem = _missionController->insertTakeoffItem(mapCenterCoord, -1);
    _missionController->insertComplexMissionItem(CorridorScanComplexItem::name, mapCenterCoord, -1);
    _missionController->insertLandItem(mapCenterCoord, -1);
    _missionController->setCurrentPlanViewSeqNum(takeoffItem->sequenceNumber(), true);
}
