#include "StructureScanPlanCreator.h"
#include "PlanMasterController.h"
#include "QGCMAVLink.h"
#include "StructureScanComplexItem.h"

StructureScanPlanCreator::StructureScanPlanCreator(PlanMasterController* planMasterController)
    : PlanCreator(planMasterController, StructureScanComplexItem::name, QStringLiteral("/qmlimages/PlanCreator/StructureScanPlanCreator.png"), {QGCMAVLink::VehicleClassMultiRotor, QGCMAVLink::VehicleClassVTOL})
{

}

void StructureScanPlanCreator::createPlan(const QGeoCoordinate& mapCenterCoord)
{
    _planMasterController->removeAll();
    VisualMissionItem* takeoffItem = _missionController->insertTakeoffItem(mapCenterCoord, -1);
    _missionController->insertComplexMissionItem(StructureScanComplexItem::name, mapCenterCoord, -1)->setWizardMode(true);
    _missionController->insertLandItem(mapCenterCoord, -1);
    _missionController->setCurrentPlanViewSeqNum(takeoffItem->sequenceNumber(), true);
}
