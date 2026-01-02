#include "BlankPlanCreator.h"
#include "PlanMasterController.h"

BlankPlanCreator::BlankPlanCreator(PlanMasterController* planMasterController, QObject* parent)
    : PlanCreator(planMasterController, tr("No Template"), QStringLiteral("/qmlimages/PlanCreator/BlankPlanCreator.png"), parent)
{
    _blankPlan = true;
}

void BlankPlanCreator::createPlan(const QGeoCoordinate& /*mapCenterCoord*/)
{
    _planMasterController->removeAll();
}
