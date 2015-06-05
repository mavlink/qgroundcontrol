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

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "FlightModesComponentController.h"
#include "QGCMAVLink.h"
#include "UASManager.h"
#include "AutoPilotPluginManager.h"

#include <QVariant>
#include <QQmlProperty>

FlightModesComponentController::FlightModesComponentController(void) :
    _liveRCValues(false),
    _validConfiguration(false),
    _channelCount(18)
{
    QStringList usedParams;
    usedParams << "RC_MAP_THROTTLE" << "RC_MAP_YAW" << "RC_MAP_PITCH" << "RC_MAP_ROLL" << "RC_MAP_FLAPS" << "RC_MAP_AUX1" << "RC_MAP_AUX2" << "RC_MAP_ACRO_SW";
    if (!_allParametersExists(FactSystem::defaultComponentId, usedParams)) {
        return;
    }
    
    _initRcValues();
    _validateConfiguration();
}

FlightModesComponentController::~FlightModesComponentController()
{
    setSendLiveRCSwitchRanges(false);
}


void FlightModesComponentController::_initRcValues(void)
{
    for (int i=0; i<_chanMax; i++) {
        _rcValues << 1.0;
    }
}

/// This will look for parameter settings which would cause the config to not run correctly.
/// It will set _validConfiguration and _configurationErrors as needed.
void FlightModesComponentController::_validateConfiguration(void)
{
    _validConfiguration = true;
    
    _channelCount = parameterExists(FactSystem::defaultComponentId, "RC_CHAN_CNT") ? getParameterFact(FactSystem::defaultComponentId, "RC_CHAN_CNT")->value().toInt() : _chanMax;
    if (_channelCount <= 0 || _channelCount > _chanMax) {
        // Parameter exists, but has not yet been set or is invalid. Use default
        _channelCount = _chanMax;
    }
    
    // Make sure switches are valid and within channel range
    
    QStringList switchParams, switchNames;
    QList<int> switchMappings;
    
    switchParams << "RC_MAP_MODE_SW" << "RC_MAP_RETURN_SW" << "RC_MAP_LOITER_SW" << "RC_MAP_POSCTL_SW" << "RC_MAP_OFFB_SW" << "RC_MAP_ACRO_SW";
    switchNames << "Mode Switch" << "Return Switch" << "Loiter Switch" << "PosCtl Switch" << "Offboard Switch" << "Acro Switch";
    
    for(int i=0; i<switchParams.count(); i++) {
        int map = getParameterFact(FactSystem::defaultComponentId, switchParams[i])->value().toInt();
        switchMappings << map;
        
        if (map < 0 || map > _channelCount) {
            _validConfiguration = false;
            _configurationErrors += QString("%1 is set to %2. Mapping must between 0 and %3 (inclusive).\n").arg(switchNames[i]).arg(map).arg(_channelCount);
        }
    }
    
    // Make sure switches are not double-mapped
    
    QStringList attitudeParams, attitudeNames;
    
    attitudeParams << "RC_MAP_THROTTLE" << "RC_MAP_YAW" << "RC_MAP_PITCH" << "RC_MAP_ROLL" << "RC_MAP_FLAPS" << "RC_MAP_AUX1" << "RC_MAP_AUX2";
    attitudeNames << "Throttle" << "Yaw" << "Pitch" << "Roll" << "Flaps" << "Aux1" << "Aux2";

    for (int i=0; i<attitudeParams.count(); i++) {
        int map = getParameterFact(FactSystem::defaultComponentId, attitudeParams[i])->value().toInt();

        for (int j=0; j<switchParams.count(); j++) {
            if (map != 0 && map == switchMappings[j]) {
                _validConfiguration = false;
                _configurationErrors += QString("%1 is set to same channel as %2.\n").arg(switchNames[j]).arg(attitudeNames[i]);
            }
        }
    }
    
    // Check for switches that must be on their own channel
    
    QStringList singleSwitchParams, singleSwitchNames;
    
    singleSwitchParams << "RC_MAP_RETURN_SW" << "RC_MAP_OFFB_SW";
    singleSwitchNames << "Return Switch" << "Offboard Switch";
    
    for (int i=0; i<singleSwitchParams.count(); i++) {
        int map = getParameterFact(FactSystem::defaultComponentId, singleSwitchParams[i])->value().toInt();
        
        for (int j=0; j<switchParams.count(); j++) {
            if (map != 0 && singleSwitchParams[i] != switchParams[j] && map == switchMappings[j]) {
                _validConfiguration = false;
                _configurationErrors += QString("%1 must be on seperate channel, but is set to same channel as %2.\n").arg(singleSwitchNames[i]).arg(switchParams[j]);
            }
        }
    }
}

void FlightModesComponentController::setSendLiveRCSwitchRanges(bool start)
{
    if (start) {
        
        // We need to know min and max for channel in order to calculate percentage range
        for (int i=0; i<_chanMax; i++) {
            QString rcMinParam, rcMaxParam, rcRevParam;
            
            rcMinParam = QString("RC%1_MIN").arg(i+1);
            rcMaxParam = QString("RC%1_MAX").arg(i+1);
            rcRevParam = QString("RC%1_REV").arg(i+1);
            
            QVariant value;
            
            _rgRCMin[i] = getParameterFact(FactSystem::defaultComponentId, rcMinParam)->value().toInt();
            _rgRCMax[i] = getParameterFact(FactSystem::defaultComponentId, rcMaxParam)->value().toInt();
            
            float floatReversed = getParameterFact(-1, rcRevParam)->value().toFloat();
            _rgRCReversed[i] = floatReversed == -1.0f;
        }
        
        _uas->startCalibration(UASInterface::StartCalibrationRadio);
        connect(_uas, &UASInterface::remoteControlChannelRawChanged, this, &FlightModesComponentController::_remoteControlChannelRawChanged);
    } else {
        disconnect(_uas, &UASInterface::remoteControlChannelRawChanged, this, &FlightModesComponentController::_remoteControlChannelRawChanged);
        _uas->stopCalibration();
        _initRcValues();
        emit switchLiveRangeChanged();
    }
}

/// @brief This routine is called whenever a raw value for an RC channel changes.
///     @param chan RC channel on which signal is coming from (0-based)
///     @param fval Current value for channel
void FlightModesComponentController::_remoteControlChannelRawChanged(int chan, float fval)
{
    Q_ASSERT(chan >= 0 && chan <= _chanMax);
    
    if (fval < _rgRCMin[chan]) {
        fval= _rgRCMin[chan];
    }
    if (fval > _rgRCMax[chan]) {
        fval= _rgRCMax[chan];
    }
    
    float percentRange = (fval - _rgRCMin[chan]) / (float)(_rgRCMax[chan] - _rgRCMin[chan]);
    if (_rgRCReversed[chan]) {
        percentRange = 1.0 - percentRange;
    }
    
    _rcValues[chan] = percentRange;
    emit switchLiveRangeChanged();
}

double FlightModesComponentController::_switchLiveRange(const QString& param)
{
    QVariant value;
    
    int channel = getParameterFact(-1, param)->value().toInt();
    if (channel == 0) {
        return 1.0;
    } else {
        return _rcValues[channel - 1];
    }
}

double FlightModesComponentController::modeSwitchLiveRange(void)
{
    return _switchLiveRange("RC_MAP_MODE_SW");
}

double FlightModesComponentController::returnSwitchLiveRange(void)
{
    return _switchLiveRange("RC_MAP_RETURN_SW");
}

double FlightModesComponentController::offboardSwitchLiveRange(void)
{
    return _switchLiveRange("RC_MAP_OFFB_SW");
}

double FlightModesComponentController::loiterSwitchLiveRange(void)
{
    return _switchLiveRange("RC_MAP_LOITER_SW");
}

double FlightModesComponentController::posCtlSwitchLiveRange(void)
{
    return _switchLiveRange("RC_MAP_POSCTL_SW");
}

double FlightModesComponentController::acroSwitchLiveRange(void)
{
    return _switchLiveRange("RC_MAP_ACRO_SW");
}
