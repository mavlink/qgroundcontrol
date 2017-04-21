/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "PlanElementController.h"
#include "QGCApplication.h"
#include "MultiVehicleManager.h"

PlanElementController::PlanElementController(QObject* parent)
    : QObject(parent)
    , _multiVehicleMgr(qgcApp()->toolbox()->multiVehicleManager())
    , _activeVehicle(_multiVehicleMgr->offlineEditingVehicle())
    , _editMode(false)
{

}

PlanElementController::~PlanElementController()
{

}

void PlanElementController::start(bool editMode)
{
    _editMode = editMode;
}

void PlanElementController::startStaticActiveVehicle(Vehicle* vehicle)
{
    Q_UNUSED(vehicle);
    _editMode = false;
}
