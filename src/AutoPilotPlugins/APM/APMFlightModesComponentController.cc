/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#include "APMFlightModesComponentController.h"
#include "QGCMAVLink.h"
#include "AutoPilotPluginManager.h"

#include <QVariant>
#include <QQmlProperty>

APMFlightModesComponentController::APMFlightModesComponentController(void)
    : _activeFlightMode(0)
    , _channelCount(Vehicle::cMaxRcChannels)
    , _fixedWing(_vehicle->vehicleType() == MAV_TYPE_FIXED_WING)
{
    QStringList usedParams;
    usedParams << "FLTMODE1" << "FLTMODE2" << "FLTMODE3" << "FLTMODE4" << "FLTMODE5" << "FLTMODE6";
    if (!_allParametersExists(FactSystem::defaultComponentId, usedParams)) {
        return;
    }

    _rgChannelOptionEnabled << QVariant(false) << QVariant(false) << QVariant(false) << QVariant(false) << QVariant(false) << QVariant(false);
    
    connect(_vehicle, &Vehicle::rcChannelsChanged, this, &APMFlightModesComponentController::_rcChannelsChanged);
}

/// Connected to Vehicle::rcChannelsChanged signal
void APMFlightModesComponentController::_rcChannelsChanged(int channelCount, int pwmValues[Vehicle::cMaxRcChannels])
{
    Q_UNUSED(channelCount);

    _activeFlightMode = 0;

    int channelValue = pwmValues[4];
    if (channelValue != -1) {
        int rgThreshold[] = { 1230, 1360, 1490, 1620, 1749 };
        for (int i=0; i<5; i++) {
            if (channelValue <= rgThreshold[i]) {
                _activeFlightMode = i + 1;
                break;
            }
        }
        _activeFlightMode = 5;
    }
    emit activeFlightModeChanged(_activeFlightMode);

    for (int i=0; i<6; i++) {
        _rgChannelOptionEnabled[i] = QVariant(false);
        channelValue = pwmValues[i+7];
        if (channelValue != -1 && channelValue > 1800) {
            _rgChannelOptionEnabled[i] = QVariant(true);
        }
    }
    emit channelOptionEnabledChanged();
}
