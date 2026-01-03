#include "CorridorScanPlanCreator.h"
#include "PlanMasterController.h"
#include "CorridorScanComplexItem.h"

CorridorScanPlanCreator::CorridorScanPlanCreator(PlanMasterController* planMasterController, QObject* parent)
    : PlanCreator(planMasterController, CorridorScanComplexItem::name, QStringLiteral("/qmlimages/PlanCreator/CorridorScanPlanCreator.png"), parent)
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
