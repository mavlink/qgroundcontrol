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
    connect(_multiVehicleMgr, &MultiVehicleManager::activeVehicleChanged, this, &PlanElementController::_activeVehicleChanged);
    _activeVehicleChanged(_multiVehicleMgr->activeVehicle());
}

void PlanElementController::startStaticActiveVehicle(Vehicle* vehicle)
{
    _editMode = false;
    _activeVehicleChanged(vehicle);
}

void PlanElementController::_activeVehicleChanged(Vehicle* activeVehicle)
{
    if (_activeVehicle) {
        _activeVehicleBeingRemoved();
        _activeVehicle = NULL;
    }

    if (activeVehicle) {
        _activeVehicle = activeVehicle;
    } else {
        _activeVehicle = _multiVehicleMgr->offlineEditingVehicle();
    }
    _activeVehicleSet();

    // Whenever vehicle changes we need to update syncInProgress
    emit syncInProgressChanged(syncInProgress());

    emit vehicleChanged(_activeVehicle);
}
