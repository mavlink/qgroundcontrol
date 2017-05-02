/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "ViewWidgetController.h"
#include "MultiVehicleManager.h"
#include "UAS.h"
#include "QGCApplication.h"

ViewWidgetController::ViewWidgetController(void) :
	_autopilot(NULL),
	_uas(NULL)
{
    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::parameterReadyVehicleAvailableChanged, this, &ViewWidgetController::_vehicleAvailable);
}

void ViewWidgetController::_vehicleAvailable(bool available)
{
    if (_uas) {
        _uas = NULL;
        _autopilot = NULL;
        emit pluginDisconnected();
    }
    
    if (available) {
        Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
        
        _uas = vehicle->uas();
        _autopilot = vehicle->autopilotPlugin();
        emit pluginConnected(QVariant::fromValue(_autopilot));
    }
}

void ViewWidgetController::checkForVehicle(void)
{
    _vehicleAvailable(qgcApp()->toolbox()->multiVehicleManager()->activeVehicle());
}
