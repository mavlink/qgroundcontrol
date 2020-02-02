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

PlanCreator::PlanCreator(PlanMasterController* planMasterController, QString name, QString imageResource, QObject* parent)
    : QObject               (parent)
    , _planMasterController (planMasterController)
    , _missionController    (planMasterController->missionController())
    , _name                 (name)
    , _imageResource        (imageResource)
{

}
