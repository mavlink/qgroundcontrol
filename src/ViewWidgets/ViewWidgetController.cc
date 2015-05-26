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
#include "UASManager.h"
#include "AutoPilotPluginManager.h"

ViewWidgetController::ViewWidgetController(void) :
	_autopilot(NULL),
	_uas(NULL)
{
	_uasManager = UASManager::instance();
	Q_ASSERT(_uasManager);
	
	connect(_uasManager, &UASManagerInterface::activeUASSet, this, &ViewWidgetController::_activeUasChanged);
}

void ViewWidgetController::_activeUasChanged(UASInterface* currentUas)
{
	if (currentUas != _uas) {
		if (_uas) {
			disconnect(_autopilot, &AutoPilotPlugin::pluginReadyChanged, this, &ViewWidgetController::_pluginReadyChanged);
			_uas = NULL;
			_autopilot = NULL;
			emit pluginDisconnected();
		}
		
		if (currentUas) {
			_uas = currentUas;
			_autopilot = AutoPilotPluginManager::instance()->getInstanceForAutoPilotPlugin(currentUas).data();
			Q_ASSERT(_autopilot);
			
			connect(_autopilot, &AutoPilotPlugin::pluginReadyChanged, this, &ViewWidgetController::_pluginReadyChanged);
			if (_autopilot->pluginReady()) {
				_pluginReadyChanged(true);
			}
		}
	}
}

void ViewWidgetController::_pluginReadyChanged(bool pluginReady)
{
	Q_ASSERT(_autopilot);
	
	if (pluginReady) {
		emit pluginConnected(QVariant::fromValue(_autopilot));
	} else {
		_activeUasChanged(NULL);
	}
}

Q_INVOKABLE void ViewWidgetController::checkForVehicle(void)
{
	_activeUasChanged(_uasManager->getActiveUAS());
}
