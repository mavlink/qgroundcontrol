/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "BlankPlanCreator.h"
#include "PlanMasterController.h"

BlankPlanCreator::BlankPlanCreator(PlanMasterController* planMasterController, QObject* parent)
    : PlanCreator(planMasterController, tr("Empty Plan"), QStringLiteral("/qmlimages/PlanCreator/BlankPlanCreator.png"), parent)
{

}

void BlankPlanCreator::createPlan(const QGeoCoordinate& /*mapCenterCoord*/)
{
    _planMasterController->removeAll();
}
