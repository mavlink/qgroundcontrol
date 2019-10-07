/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "StructureScanPlanCreator.h"
#include "PlanMasterController.h"
#include "MissionSettingsItem.h"

StructureScanPlanCreator::StructureScanPlanCreator(PlanMasterController* planMasterController, QObject* parent)
    : PlanCreator(planMasterController, MissionController::patternStructureScanName, QStringLiteral("/qmlimages/PlanCreator/StructureScanPlanCreator.png"), parent)
{

}

void StructureScanPlanCreator::createPlan(const QGeoCoordinate& mapCenterCoord)
{
    _planMasterController->removeAll();
    int seqNum = _missionController->insertComplexMissionItem(
                                    MissionController::patternStructureScanName,
                                    mapCenterCoord,
                                    _missionController->visualItems()->count());
    if (_planMasterController->managerVehicle()->fixedWing()) {
        _missionController->insertComplexMissionItem(
                    MissionController::patternFWLandingName,
                    mapCenterCoord,
                    _missionController->visualItems()->count());
    } else {
        MissionSettingsItem* settingsItem = _missionController->visualItems()->value<MissionSettingsItem*>(0);
        settingsItem->setMissionEndRTL(true);
    }
    _missionController->setCurrentPlanViewIndex(seqNum, false);
}
