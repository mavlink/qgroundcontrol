#include "BlankPlanCreator.h"
#include "PlanMasterController.h"
#include "QGCMAVLink.h"

BlankPlanCreator::BlankPlanCreator(PlanMasterController* planMasterController)
    : PlanCreator(planMasterController, tr("No Template"), QStringLiteral("/qmlimages/PlanCreator/BlankPlanCreator.png"), QGCMAVLink::allVehicleClasses(), true)
{
}

void BlankPlanCreator::createPlan(const QGeoCoordinate& /*mapCenterCoord*/)
{
    _planMasterController->removeAll();
}
