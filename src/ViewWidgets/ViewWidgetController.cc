/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

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
        Q_ASSERT(_autopilot);
        emit pluginConnected(QVariant::fromValue(_autopilot));
    }
}
Q_INVOKABLE void ViewWidgetController::checkForVehicle(void)
{
    _vehicleAvailable(qgcApp()->toolbox()->multiVehicleManager()->activeVehicle());
}
