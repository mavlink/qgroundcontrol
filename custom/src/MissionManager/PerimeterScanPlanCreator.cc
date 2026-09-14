#include "PerimeterScanPlanCreator.h"

#include "MissionController.h"
#include "PerimeterScanComplexItem.h"
#include "PlanMasterController.h"
#include "QGCMAVLink.h"

PerimeterScanPlanCreator::PerimeterScanPlanCreator(PlanMasterController *planMasterController)
    : PlanCreator(planMasterController,
                  PerimeterScanComplexItem::tr(PerimeterScanComplexItem::canonicalName),
                  QStringLiteral("/qmlimages/PlanCreator/SurveyPlanCreator.png"),
                  QGCMAVLink::allVehicleClasses())
{
}

void PerimeterScanPlanCreator::createPlan(const QGeoCoordinate &mapCenterCoord)
{
    _planMasterController->removeAll();
    VisualMissionItem *takeoffItem = _missionController->insertTakeoffItem(mapCenterCoord, -1);
    _missionController->insertComplexMissionItem(
        PerimeterScanComplexItem::canonicalName, mapCenterCoord, -1);
    _missionController->insertLandItem(mapCenterCoord, -1);
    _missionController->setCurrentPlanViewSeqNum(takeoffItem->sequenceNumber(), true);
}
