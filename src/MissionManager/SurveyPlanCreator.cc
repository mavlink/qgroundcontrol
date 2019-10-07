/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SurveyPlanCreator.h"
#include "PlanMasterController.h"
#include "MissionSettingsItem.h"

SurveyPlanCreator::SurveyPlanCreator(PlanMasterController* planMasterController, QObject* parent)
    : PlanCreator(planMasterController, MissionController::patternSurveyName, QStringLiteral("/qmlimages/PlanCreator/SurveyPlanCreator.png"), parent)
{

}

void SurveyPlanCreator::createPlan(const QGeoCoordinate& mapCenterCoord)
{
    _planMasterController->removeAll();
    int seqNum = _missionController->insertComplexMissionItem(
                                    MissionController::patternSurveyName,
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
