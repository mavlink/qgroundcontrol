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

PlanElementController::PlanElementController(QObject* parent)
    : QObject(parent)
    , _activeVehicle(NULL)
    , _multiVehicleMgr(qgcApp()->toolbox()->multiVehicleManager())
    , _editMode(false)
{

}

PlanElementController::~PlanElementController()
{

}

void PlanElementController::start(bool editMode)
{
    _editMode = editMode;
    connect(_multiVehicleMgr, &MultiVehicleManager::activeVehicleChanged, this, &PlanElementController::_activeVehicleChanged);
    _activeVehicleChanged(_multiVehicleMgr->activeVehicle());
}

void PlanElementController::_activeVehicleChanged(Vehicle* activeVehicle)
{
    if (_activeVehicle) {
        Vehicle* vehicleSave = _activeVehicle;
        _activeVehicle = NULL;
        _activeVehicleBeingRemoved(vehicleSave);
    }

    _activeVehicle = activeVehicle;

    if (_activeVehicle) {
        _activeVehicleSet();
    }

    // Whenever vehicle changes we need to update syncInProgress
    emit syncInProgressChanged(syncInProgress());
}
