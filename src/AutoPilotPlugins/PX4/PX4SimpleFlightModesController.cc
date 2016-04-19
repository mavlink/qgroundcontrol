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

#include "PX4SimpleFlightModesController.h"
#include "QGCMAVLink.h"
#include "AutoPilotPluginManager.h"

#include <QVariant>
#include <QQmlProperty>

PX4SimpleFlightModesController::PX4SimpleFlightModesController(void)
    : _activeFlightMode(0)
    , _channelCount(Vehicle::cMaxRcChannels)

{
    QStringList usedParams;
    usedParams << QStringLiteral("COM_FLTMODE1") << QStringLiteral("COM_FLTMODE2") << QStringLiteral("COM_FLTMODE3")
               << QStringLiteral("COM_FLTMODE4") << QStringLiteral("COM_FLTMODE5") << QStringLiteral("COM_FLTMODE6")
               << QStringLiteral("RC_MAP_FLTMODE");
    if (!_allParametersExists(FactSystem::defaultComponentId, usedParams)) {
        return;
    }

    connect(_vehicle, &Vehicle::rcChannelsChanged, this, &PX4SimpleFlightModesController::_rcChannelsChanged);
}

/// Connected to Vehicle::rcChannelsChanged signal
void PX4SimpleFlightModesController::_rcChannelsChanged(int channelCount, int pwmValues[Vehicle::cMaxRcChannels])
{
    _rcChannelValues.clear();
    for (int i=0; i<channelCount; i++) {
        _rcChannelValues.append(pwmValues[i]);
    }
    emit rcChannelValuesChanged();

    int flightModeChannel = getParameterFact(-1, "RC_MAP_FLTMODE")->rawValue().toInt() - 1;

    int flightModeReversed = getParameterFact(1, QString("RC%1_REV").arg(flightModeChannel + 1))->rawValue().toInt();

    if (flightModeChannel < 0 || flightModeChannel > channelCount) {
        return;
    }

    _activeFlightMode = 0;
    int channelValue = pwmValues[flightModeChannel];
    if (channelValue != -1) {
        bool found = false;
        int rgThreshold[] = { 1200, 1360, 1490, 1620, 1900 };
        for (int i=0; i<5; i++) {
            if (channelValue <= rgThreshold[i]) {
                _activeFlightMode = i + 1;
                found = true;
                break;
            }
        }

        if (!found) {
            _activeFlightMode = 6;
        }

        if (flightModeReversed == -1) {
            _activeFlightMode = 6 - (_activeFlightMode - 1);
        }
    }

    emit activeFlightModeChanged(_activeFlightMode);
}
