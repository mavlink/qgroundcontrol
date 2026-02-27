#include "SurveyPlanCreator.h"
#include "PlanMasterController.h"
#include "SurveyComplexItem.h"

SurveyPlanCreator::SurveyPlanCreator(PlanMasterController* planMasterController, QObject* parent)
    : PlanCreator(planMasterController, SurveyComplexItem::name, QStringLiteral("/qmlimages/PlanCreator/SurveyPlanCreator.png"), parent)
{

}

void SurveyPlanCreator::createPlan(const QGeoCoordinate& mapCenterCoord)
{
    _planMasterController->removeAll();
    VisualMissionItem* takeoffItem = _missionController->insertTakeoffItem(mapCenterCoord, -1);
    _missionController->insertComplexMissionItem(SurveyComplexItem::name, mapCenterCoord, -1);
    _missionController->insertLandItem(mapCenterCoord, -1);
    _missionController->setCurrentPlanViewSeqNum(takeoffItem->sequenceNumber(), true);
}
