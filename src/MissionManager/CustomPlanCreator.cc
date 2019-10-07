/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CustomPlanCreator.h"
#include "PlanMasterController.h"
#include "MissionSettingsItem.h"

CustomPlanCreator::CustomPlanCreator(PlanMasterController* planMasterController, QObject* parent)
    : PlanCreator(planMasterController, tr("Custom"), QStringLiteral("/qmlimages/PlanCreator/CustomPlanCreator.png"), parent)
{

}

void CustomPlanCreator::createPlan(const QGeoCoordinate& mapCenterCoord)
{
    _planMasterController->removeAll();
    int seqNum = _missionController->insertSimpleMissionItem(mapCenterCoord, _missionController->visualItems()->count());
    _missionController->insertSimpleMissionItem(mapCenterCoord.atDistanceAndAzimuth(50, 135), _missionController->visualItems()->count());
    _missionController->insertSimpleMissionItem(mapCenterCoord.atDistanceAndAzimuth(50, -135), _missionController->visualItems()->count());
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
