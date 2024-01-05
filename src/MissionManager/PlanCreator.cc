/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "PlanCreator.h"
#include "PlanMasterController.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "AppSettings.h"

PlanCreator::PlanCreator(PlanMasterController* planMasterController, QString name, QString imageResource, QObject* parent)
    : QObject               (parent)
    , _planMasterController (planMasterController)
    , _missionController    (planMasterController->missionController())
    , _name                 (name)
    , _imageResource        (imageResource)
{

}

void PlanCreator::createPlan(QVariantList /* coordList */)
{
    _planMasterController->renamePlan(_planMasterController->generateNewPlanName(_name));
    _planMasterController->setPlanType(_name);
}

PlanCreator::CoordRect_t PlanCreator::_convertCoordList(const QVariantList& coordList)
{
    CoordRect_t coordRect;

    coordRect.topLeftCoord       = coordList[0].value<QGeoCoordinate>();
    coordRect.bottomRightCoord   = coordList[1].value<QGeoCoordinate>();
    coordRect.topRightCoord      = QGeoCoordinate(coordRect.topLeftCoord.latitude(), coordRect.bottomRightCoord.longitude());
    coordRect.bottomLeftCoord    = QGeoCoordinate(coordRect.bottomRightCoord.latitude(), coordRect.topLeftCoord.longitude());

    return coordRect;
}

QGeoCoordinate PlanCreator::_coordRectCenter(const CoordRect_t& coordRect)
{
    auto diagonalDistance = coordRect.topLeftCoord.distanceTo(coordRect.bottomRightCoord);
    return coordRect.topLeftCoord.atDistanceAndAzimuth(diagonalDistance / 2.0, 45);
}

PlanCreator::CoordRect_t PlanCreator::_insetCoordRect(const CoordRect_t& coordRect, double insetPercent)
{
    CoordRect_t insetCoordRect;

    auto insetDistance  = coordRect.topLeftCoord.distanceTo(coordRect.bottomRightCoord) * insetPercent;

    insetCoordRect.topLeftCoord     = coordRect.topLeftCoord.atDistanceAndAzimuth(insetDistance, 90 + 45);
    insetCoordRect.topRightCoord    = coordRect.topRightCoord.atDistanceAndAzimuth(insetDistance, 180 + 45);
    insetCoordRect.bottomLeftCoord  = coordRect.bottomLeftCoord.atDistanceAndAzimuth(insetDistance, 0 + 45);
    insetCoordRect.bottomRightCoord = coordRect.bottomRightCoord.atDistanceAndAzimuth(insetDistance, 270 + 45);

    return insetCoordRect;
}
