/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CorridorScanPlanCreator.h"
#include "PlanMasterController.h"
#include "MissionSettingsItem.h"
#include "FixedWingLandingComplexItem.h"

CorridorScanPlanCreator::CorridorScanPlanCreator(PlanMasterController* planMasterController, QObject* parent)
    : PlanCreator(planMasterController, MissionController::patternCorridorScanName, QStringLiteral("/qmlimages/PlanCreator/CorridorScanPlanCreator.png"), parent)
{

}

void CorridorScanPlanCreator::createPlan(const QGeoCoordinate& mapCenterCoord)
{
    _planMasterController->removeAll();
    VisualMissionItem* takeoffItem = _missionController->insertTakeoffItem(mapCenterCoord, -1);
    _missionController->insertComplexMissionItem(MissionController::patternCorridorScanName, mapCenterCoord, -1);
    // We want the operator to coordinate his landing for VTOL and Fixed Wing. An RTL is not really predictable.
    if(!_planMasterController->managerVehicle()->vtol() && !_planMasterController->managerVehicle()->fixedWing()) {
        _missionController->insertLandItem(mapCenterCoord, -1);
    }
    _missionController->setCurrentPlanViewSeqNum(takeoffItem->sequenceNumber(), true);
}
