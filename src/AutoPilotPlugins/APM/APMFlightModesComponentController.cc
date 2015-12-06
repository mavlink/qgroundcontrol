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
    : _channelCount(Vehicle::cMaxRcChannels)
{
    QStringList usedParams;
    usedParams << "FLTMODE1" << "FLTMODE2" << "FLTMODE3" << "FLTMODE4" << "FLTMODE5" << "FLTMODE6";
    if (!_allParametersExists(FactSystem::defaultComponentId, usedParams)) {
        return;
    }

    _init();
    _validateConfiguration();
    
    connect(_vehicle, &Vehicle::rcChannelsChanged, this, &APMFlightModesComponentController::_rcChannelsChanged);
}

void APMFlightModesComponentController::_init(void)
{
    _fixedWing = _vehicle->vehicleType() == MAV_TYPE_FIXED_WING;
}

/// This will look for parameter settings which would cause the config to not run correctly.
/// It will set _validConfiguration and _configurationErrors as needed.
void APMFlightModesComponentController::_validateConfiguration(void)
{
    _validConfiguration = true;
#if 0
    
    // Make sure switches are valid and within channel range
    
    QStringList switchParams;
    QList<int> switchMappings;
    
    switchParams << "RC_MAP_MODE_SW"  << "RC_MAP_ACRO_SW"  << "RC_MAP_POSCTL_SW" << "RC_MAP_LOITER_SW" << "RC_MAP_RETURN_SW" << "RC_MAP_OFFB_SW";
    
    for(int i=0; i<switchParams.count(); i++) {
        int map = getParameterFact(FactSystem::defaultComponentId, switchParams[i])->rawValue().toInt();
        switchMappings << map;
        
        if (map < 0 || map > _channelCount) {
            _validConfiguration = false;
            _configurationErrors += QString("%1 is set to %2. Mapping must between 0 and %3 (inclusive).\n").arg(switchParams[i]).arg(map).arg(_channelCount);
        }
    }
    
    // Make sure mode switches are not double-mapped
    
    QStringList attitudeParams;
    
    attitudeParams << "RC_MAP_THROTTLE" << "RC_MAP_YAW" << "RC_MAP_PITCH" << "RC_MAP_ROLL" << "RC_MAP_FLAPS" << "RC_MAP_AUX1" << "RC_MAP_AUX2";

    for (int i=0; i<attitudeParams.count(); i++) {
        int map = getParameterFact(FactSystem::defaultComponentId, attitudeParams[i])->rawValue().toInt();

        for (int j=0; j<switchParams.count(); j++) {
            if (map != 0 && map == switchMappings[j]) {
                _validConfiguration = false;
                _configurationErrors += QString("%1 is set to same channel as %2.\n").arg(switchParams[j]).arg(attitudeParams[i]);
            }
        }
    }
    
    // Validate thresholds within range
    
    QStringList thresholdParams;
    
    thresholdParams << "RC_ASSIST_TH" << "RC_AUTO_TH" << "RC_ACRO_TH" << "RC_POSCTL_TH" << "RC_LOITER_TH" << "RC_RETURN_TH" << "RC_OFFB_TH";
    
    foreach(QString thresholdParam, thresholdParams) {
        float threshold = getParameterFact(-1, thresholdParam)->rawValue().toFloat();
        if (threshold < 0.0f || threshold > 1.0f) {
            _validConfiguration = false;
            _configurationErrors += QString("%1 is set to %2. Threshold must between 0.0 and 1.0 (inclusive).\n").arg(thresholdParam).arg(threshold);
        }
    }
#endif
}

/// Connected to Vehicle::rcChannelsChanged signal
void APMFlightModesComponentController::_rcChannelsChanged(int channelCount, int pwmValues[Vehicle::cMaxRcChannels])
{
    for (int channel=0; channel<channelCount; channel++) {
        _rcValues[channel] = pwmValues[channel];
    }
}
