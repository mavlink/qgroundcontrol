/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "WaypointsPlanCreator.h"
#include "PlanMasterController.h"
#include "FixedWingLandingComplexItem.h"

WaypointsPlanCreator::WaypointsPlanCreator(PlanMasterController* planMasterController, QObject* parent)
    : PlanCreator(planMasterController, tr("Waypoints"), QStringLiteral("/qmlimages/PlanCreator/WaypointsPlanCreator.png"), parent)
{

}

void WaypointsPlanCreator::createPlan(QVariantList coordList)
{
    PlanCreator::createPlan(coordList);

    auto coordRect              = _convertCoordList(coordList);
    auto insetCoordRect         = _insetCoordRect(coordRect, 0.05);
    auto takeoffCoord           = insetCoordRect.bottomRightCoord;
    auto wp1Coord               = insetCoordRect.topRightCoord;
    auto wp2Coord               = insetCoordRect.topLeftCoord;

    VisualMissionItem* takeoffItem = _missionController->insertTakeoffItem(takeoffCoord, -1);
    _missionController->insertSimpleMissionItem(wp1Coord, -1);
    _missionController->insertSimpleMissionItem(wp2Coord, -1);
    _missionController->insertLandItem(takeoffCoord, -1);
    _missionController->setCurrentPlanViewSeqNum(takeoffItem->sequenceNumber(), true);
}
