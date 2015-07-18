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
    _validConfiguration(false),
    _channelCount(18),
    _manualModeSelected(false),
    _assistModeSelected(false),
    _autoModeSelected(false),
    _acroModeSelected(false),
    _altCtlModeSelected(false),
    _posCtlModeSelected(false),
    _missionModeSelected(false),
    _loiterModeSelected(false),
    _returnModeSelected(false),
    _offboardModeSelected(false)
{
    QStringList usedParams;
    usedParams << "RC_MAP_THROTTLE" << "RC_MAP_YAW" << "RC_MAP_PITCH" << "RC_MAP_ROLL" << "RC_MAP_FLAPS" << "RC_MAP_AUX1" << "RC_MAP_AUX2" <<
        "RC_MAP_MODE_SW" << "RC_MAP_RETURN_SW" << "RC_MAP_LOITER_SW" << "RC_MAP_POSCTL_SW" << "RC_MAP_OFFB_SW" << "RC_MAP_ACRO_SW";
    if (!_allParametersExists(FactSystem::defaultComponentId, usedParams)) {
        return;
    }

    _init();
    _validateConfiguration();
    
    connect(_uas, &UASInterface::remoteControlChannelRawChanged, this, &FlightModesComponentController::_remoteControlChannelRawChanged);
}

FlightModesComponentController::~FlightModesComponentController()
{
    disconnect(_uas, &UASInterface::remoteControlChannelRawChanged, this, &FlightModesComponentController::_remoteControlChannelRawChanged);
}

void FlightModesComponentController::_init(void)
{
    // FIXME: What about VTOL? That confuses the whole Flight Mode naming scheme
    _fixedWing = _uas->getSystemType() == MAV_TYPE_FIXED_WING;
    
    // We need to know min and max for channel in order to calculate percentage range
    for (int channel=0; channel<_chanMax; channel++) {
        QString rcMinParam, rcMaxParam, rcRevParam;
        
        rcMinParam = QString("RC%1_MIN").arg(channel+1);
        rcMaxParam = QString("RC%1_MAX").arg(channel+1);
        rcRevParam = QString("RC%1_REV").arg(channel+1);
        
        QVariant value;
        
        _rgRCMin[channel] = getParameterFact(FactSystem::defaultComponentId, rcMinParam)->value().toInt();
        _rgRCMax[channel] = getParameterFact(FactSystem::defaultComponentId, rcMaxParam)->value().toInt();
        
        float floatReversed = getParameterFact(-1, rcRevParam)->value().toFloat();
        _rgRCReversed[channel] = floatReversed == -1.0f;
        
        _rcValues[channel] = 0.0;
    }

    // RC_CHAN_CNT parameter is set by Radio Cal to specify the number of radio channels.
    if (parameterExists(FactSystem::defaultComponentId, "RC_CHAN_CNT")) {
        _channelCount = getParameterFact(FactSystem::defaultComponentId, "RC_CHAN_CNT")->value().toInt();
    } else {
        _channelCount =_chanMax;
    }
    if (_channelCount <= 0 || _channelCount > _chanMax) {
        // Parameter exists, but has not yet been set or is invalid. Use default
        _channelCount = _chanMax;
    }
    
    int modeChannel = getParameterFact(-1, "RC_MAP_MODE_SW")->value().toInt();
    int posCtlChannel = getParameterFact(-1, "RC_MAP_POSCTL_SW")->value().toInt();
    int loiterChannel = getParameterFact(-1, "RC_MAP_LOITER_SW")->value().toInt();
    
    if (posCtlChannel == 0) {
        // PosCtl disabled so AltCtl must move back to main Mode switch
        _assistModeVisible = false;
    } else {
        // Assist mode is visible if AltCtl/PosCtl are on seperate channel from main Mode switch
        _assistModeVisible = posCtlChannel != modeChannel;
    }
    
    if (loiterChannel == 0) {
        // Loiter disabled so Mission must move back to main Mode switch
        _autoModeVisible = false;
    } else {
        // Auto mode is visible if Mission/Loiter are on seperate channel from main Mode switch
        _autoModeVisible = loiterChannel != modeChannel;
    }
    
    // Setup the channel combobox model
    
    QList<int> usedChannels;
    QStringList attitudeParams;
    
    attitudeParams << "RC_MAP_THROTTLE" << "RC_MAP_YAW" << "RC_MAP_PITCH" << "RC_MAP_ROLL" << "RC_MAP_FLAPS" << "RC_MAP_AUX1" << "RC_MAP_AUX2";
    foreach(QString attitudeParam, attitudeParams) {
        int channel = getParameterFact(-1, attitudeParam)->value().toInt();
        if (channel != 0) {
            usedChannels << channel;
        }
    }
    
    _channelListModel << "Disabled";
    _channelListModelChannel << 0;
    for (int channel=1; channel<=_channelCount; channel++) {
        if (!usedChannels.contains(channel)) {
            _channelListModel << QString("Channel %1").arg(channel);
            _channelListModelChannel << channel;
        }
    }
    
    // Setup reserved channels string for ui
    
    bool first = true;
    foreach (int usedChannel, usedChannels) {
        if (!first) {
            _reservedChannels += ", ";
        }
        _reservedChannels += QString("%1").arg(usedChannel);
        first = false;
    }
    
    _recalcModeRows();
}

/// This will look for parameter settings which would cause the config to not run correctly.
/// It will set _validConfiguration and _configurationErrors as needed.
void FlightModesComponentController::_validateConfiguration(void)
{
    _validConfiguration = true;
    
    // Make sure switches are valid and within channel range
    
    QStringList switchParams;
    QList<int> switchMappings;
    
    switchParams << "RC_MAP_MODE_SW"  << "RC_MAP_ACRO_SW"  << "RC_MAP_POSCTL_SW" << "RC_MAP_LOITER_SW" << "RC_MAP_RETURN_SW" << "RC_MAP_OFFB_SW";
    
    for(int i=0; i<switchParams.count(); i++) {
        int map = getParameterFact(FactSystem::defaultComponentId, switchParams[i])->value().toInt();
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
        int map = getParameterFact(FactSystem::defaultComponentId, attitudeParams[i])->value().toInt();

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
        float threshold = getParameterFact(-1, thresholdParam)->value().toFloat();
        if (threshold < 0.0f || threshold > 1.0f) {
            _validConfiguration = false;
            _configurationErrors += QString("%1 is set to %2. Threshold must between 0.0 and 1.0 (inclusive).\n").arg(thresholdParam).arg(threshold);
        }
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
    
    _recalcModeSelections();
    
    emit switchLiveRangeChanged();
}

double FlightModesComponentController::_switchLiveRange(const QString& param)
{
    QVariant value;
    
    int channel = getParameterFact(-1, param)->value().toInt();
    if (channel == 0) {
        return 0.0;
    } else {
        return _rcValues[channel - 1];
    }
}

double FlightModesComponentController::manualModeRcValue(void)
{
    return _switchLiveRange("RC_MAP_MODE_SW");
}

double FlightModesComponentController::assistModeRcValue(void)
{
    return manualModeRcValue();
}

double FlightModesComponentController::autoModeRcValue(void)
{
    return manualModeRcValue();
}

double FlightModesComponentController::acroModeRcValue(void)
{
    return _switchLiveRange("RC_MAP_ACRO_SW");
}

double FlightModesComponentController::altCtlModeRcValue(void)
{
    int posCtlSwitchChannel = getParameterFact(-1, "RC_MAP_POSCTL_SW")->value().toInt();
    
    if (posCtlSwitchChannel == 0) {
        return _switchLiveRange("RC_MAP_MODE_SW");
    } else {
        return _switchLiveRange("RC_MAP_POSCTL_SW");
    }
}

double FlightModesComponentController::posCtlModeRcValue(void)
{
    return _switchLiveRange("RC_MAP_POSCTL_SW");
}

double FlightModesComponentController::missionModeRcValue(void)
{
    int returnSwitchChannel = getParameterFact(-1, "RC_MAP_RETURN_SW")->value().toInt();
    int loiterSwitchChannel = getParameterFact(-1, "RC_MAP_LOITER_SW")->value().toInt();

    const char* switchChannelParam = "RC_MAP_MODE_SW";
    
    if (returnSwitchChannel == 0) {
        if (loiterSwitchChannel != 0) {
            switchChannelParam = "RC_MAP_LOITER_SW";
        }
    } else {
        if (loiterSwitchChannel == 0) {
            switchChannelParam = "RC_MAP_RETURN_SW";
        } else {
            switchChannelParam = "RC_MAP_LOITER_SW";
        }
    }
    
    return _switchLiveRange(switchChannelParam);
}

double FlightModesComponentController::loiterModeRcValue(void)
{
    return _switchLiveRange("RC_MAP_LOITER_SW");
}

double FlightModesComponentController::returnModeRcValue(void)
{
    return _switchLiveRange("RC_MAP_RETURN_SW");
}

double FlightModesComponentController::offboardModeRcValue(void)
{
    return _switchLiveRange("RC_MAP_OFFB_SW");
}

void FlightModesComponentController::_recalcModeSelections(void)
{
    _manualModeSelected = false;
    _assistModeSelected = false;
    _autoModeSelected = false;
    _acroModeSelected = false;
    _altCtlModeSelected = false;
    _posCtlModeSelected = false;
    _missionModeSelected = false;
    _loiterModeSelected = false;
    _returnModeSelected = false;
    _offboardModeSelected = false;
    
    // Convert channels to 0-based, -1 signals not mapped
    int modeSwitchChannel =     getParameterFact(-1, "RC_MAP_MODE_SW")->value().toInt() - 1;
    int acroSwitchChannel =     getParameterFact(-1, "RC_MAP_ACRO_SW")->value().toInt() - 1;
    int posCtlSwitchChannel =   getParameterFact(-1, "RC_MAP_POSCTL_SW")->value().toInt() - 1;
    int loiterSwitchChannel =   getParameterFact(-1, "RC_MAP_LOITER_SW")->value().toInt() - 1;
    int returnSwitchChannel =   getParameterFact(-1, "RC_MAP_RETURN_SW")->value().toInt() - 1;
    int offboardSwitchChannel = getParameterFact(-1, "RC_MAP_OFFB_SW")->value().toInt() - 1;
    
    double autoThreshold =      getParameterFact(-1, "RC_AUTO_TH")->value().toDouble();
    double assistThreshold =    getParameterFact(-1, "RC_ASSIST_TH")->value().toDouble();
    double acroThreshold =      getParameterFact(-1, "RC_ACRO_TH")->value().toDouble();
    double posCtlThreshold =    getParameterFact(-1, "RC_POSCTL_TH")->value().toDouble();
    double loiterThreshold =    getParameterFact(-1, "RC_LOITER_TH")->value().toDouble();
    double returnThreshold =    getParameterFact(-1, "RC_RETURN_TH")->value().toDouble();
    double offboardThreshold =  getParameterFact(-1, "RC_OFFB_TH")->value().toDouble();
    
    if (modeSwitchChannel >= 0) {
        if (offboardSwitchChannel >= 0 && _rcValues[offboardSwitchChannel] >= offboardThreshold) {
            _offboardModeSelected = true;
        } else if (returnSwitchChannel >= 0 && _rcValues[returnSwitchChannel] >= returnThreshold) {
            _returnModeSelected = true;
        } else {
            if (_rcValues[modeSwitchChannel] >= autoThreshold) {
                _autoModeSelected = true;
                if (loiterSwitchChannel >= 0 && _rcValues[loiterSwitchChannel] >= loiterThreshold) {
                    _loiterModeSelected = true;
                } else {
                    _missionModeSelected = true;
                }
            } else if (_rcValues[modeSwitchChannel] >= assistThreshold) {
                _assistModeSelected = true;
                if (posCtlSwitchChannel >= 0 && _rcValues[posCtlSwitchChannel] >= posCtlThreshold) {
                    _posCtlModeSelected = true;
                } else {
                    _altCtlModeSelected = true;
                }
            } else if (acroSwitchChannel >= 0 && _rcValues[acroSwitchChannel] >= acroThreshold) {
                _acroModeSelected = true;
            } else {
                _manualModeSelected = true;
            }
        }
    }
    
    emit modesSelectedChanged();
}

void FlightModesComponentController::_recalcModeRows(void)
{
    int modeSwitchChannel =     getParameterFact(-1, "RC_MAP_MODE_SW")->value().toInt();
    int acroSwitchChannel =     getParameterFact(-1, "RC_MAP_ACRO_SW")->value().toInt();
    int posCtlSwitchChannel =   getParameterFact(-1, "RC_MAP_POSCTL_SW")->value().toInt();
    int loiterSwitchChannel =   getParameterFact(-1, "RC_MAP_LOITER_SW")->value().toInt();
    int returnSwitchChannel =   getParameterFact(-1, "RC_MAP_RETURN_SW")->value().toInt();
    int offboardSwitchChannel = getParameterFact(-1, "RC_MAP_OFFB_SW")->value().toInt();
    
    if (modeSwitchChannel == 0) {
        _acroModeRow =      0;
        _assistModeRow =    1;
        _autoModeRow =      2;
        _altCtlModeRow =    3;
        _posCtlModeRow =    4;
        _loiterModeRow =    5;
        _missionModeRow =   6;
        _returnModeRow =    7;
        _offboardModeRow =  8;
    } else {
        int row = 0;
        
        // First set is all switches on main mode channel
        
        if (acroSwitchChannel == modeSwitchChannel) {
            _acroModeRow = row++;
        }
        _assistModeRow = row++;
        if (posCtlSwitchChannel == modeSwitchChannel) {
            _altCtlModeRow = row++;
            _posCtlModeRow = row++;
        } else if (posCtlSwitchChannel == 0) {
            _altCtlModeRow = row++;
        }
        _autoModeRow = row++;
        if (loiterSwitchChannel == modeSwitchChannel) {
            _missionModeRow = row++;
            _loiterModeRow = row++;
        } else if (loiterSwitchChannel == 0) {
            _missionModeRow = row++;
        }
        if (returnSwitchChannel == modeSwitchChannel) {
            _returnModeRow = row++;
        }
        if (offboardSwitchChannel == modeSwitchChannel) {
            _offboardModeRow = row++;
        }
        
        // Now individual enabled switches not on main mode channel
        
        if (acroSwitchChannel != 0 && acroSwitchChannel != modeSwitchChannel) {
            _acroModeRow = row++;
        }
        if (posCtlSwitchChannel != 0 && posCtlSwitchChannel != modeSwitchChannel) {
            _altCtlModeRow = row++;
            _posCtlModeRow = row++;
        }
        if (loiterSwitchChannel != 0 && loiterSwitchChannel != modeSwitchChannel) {
            _missionModeRow = row++;
            _loiterModeRow = row++;
        }
        if (returnSwitchChannel != 0 && returnSwitchChannel != modeSwitchChannel) {
            _returnModeRow = row++;
        }
        if (offboardSwitchChannel != 0 && offboardSwitchChannel != modeSwitchChannel) {
            _offboardModeRow = row++;
        }
        
        // Now disabled switches
        
        if (acroSwitchChannel == 0) {
            _acroModeRow = row++;
        }
        if (posCtlSwitchChannel == 0) {
            _posCtlModeRow = row++;
        }
        if (loiterSwitchChannel == 0) {
            _loiterModeRow = row++;
        }
        if (returnSwitchChannel == 0) {
            _returnModeRow = row++;
        }
        if (offboardSwitchChannel == 0) {
            _offboardModeRow = row++;
        }
    }
    
    emit modeRowsChanged();
}

double FlightModesComponentController::manualModeThreshold(void)
{
    return 0.0;
}

double FlightModesComponentController::assistModeThreshold(void)
{
    return getParameterFact(-1, "RC_ASSIST_TH")->value().toDouble();
}

double FlightModesComponentController::autoModeThreshold(void)
{
    return getParameterFact(-1, "RC_AUTO_TH")->value().toDouble();
}

double FlightModesComponentController::acroModeThreshold(void)
{
    return getParameterFact(-1, "RC_ACRO_TH")->value().toDouble();
}

double FlightModesComponentController::altCtlModeThreshold(void)
{
    return _assistModeVisible ? 0.0 : getParameterFact(-1, "RC_ASSIST_TH")->value().toDouble();
}

double FlightModesComponentController::posCtlModeThreshold(void)
{
    return getParameterFact(-1, "RC_POSCTL_TH")->value().toDouble();
}

double FlightModesComponentController::missionModeThreshold(void)
{
    return _autoModeVisible ? 0.0 : getParameterFact(-1, "RC_AUTO_TH")->value().toDouble();
}


double FlightModesComponentController::loiterModeThreshold(void)
{
    return getParameterFact(-1, "RC_LOITER_TH")->value().toDouble();
}

double FlightModesComponentController::returnModeThreshold(void)
{
    return getParameterFact(-1, "RC_RETURN_TH")->value().toDouble();
}

double FlightModesComponentController::offboardModeThreshold(void)
{
    return getParameterFact(-1, "RC_OFFB_TH")->value().toDouble();
}

void FlightModesComponentController::setAssistModeThreshold(double threshold)
{
    getParameterFact(-1, "RC_ASSIST_TH")->setValue(threshold);
    _recalcModeSelections();
}

void FlightModesComponentController::setAutoModeThreshold(double threshold)
{
    getParameterFact(-1, "RC_AUTO_TH")->setValue(threshold);
    _recalcModeSelections();
}

void FlightModesComponentController::setAcroModeThreshold(double threshold)
{
    getParameterFact(-1, "RC_ACRO_TH")->setValue(threshold);
    _recalcModeSelections();
}

void FlightModesComponentController::setAltCtlModeThreshold(double threshold)
{
    setAssistModeThreshold(threshold);
}

void FlightModesComponentController::setPosCtlModeThreshold(double threshold)
{
    getParameterFact(-1, "RC_POSCTL_TH")->setValue(threshold);
    _recalcModeSelections();
}

void FlightModesComponentController::setMissionModeThreshold(double threshold)
{
    setAutoModeThreshold(threshold);
}

void FlightModesComponentController::setLoiterModeThreshold(double threshold)
{
    getParameterFact(-1, "RC_LOITER_TH")->setValue(threshold);
    _recalcModeSelections();
}

void FlightModesComponentController::setReturnModeThreshold(double threshold)
{
    getParameterFact(-1, "RC_RETURN_TH")->setValue(threshold);
    _recalcModeSelections();
}

void FlightModesComponentController::setOffboardModeThreshold(double threshold)
{
    getParameterFact(-1, "RC_OFFB_TH")->setValue(threshold);
    _recalcModeSelections();
}

int FlightModesComponentController::_channelToChannelIndex(int channel)
{
    return _channelListModelChannel.lastIndexOf(channel);
}

int FlightModesComponentController::_channelToChannelIndex(const QString& channelParam)
{
    return _channelToChannelIndex(getParameterFact(-1, channelParam)->value().toInt());
}

int FlightModesComponentController::manualModeChannelIndex(void)
{
    return _channelToChannelIndex("RC_MAP_MODE_SW");
}

int FlightModesComponentController::assistModeChannelIndex(void)
{
    return _channelToChannelIndex("RC_MAP_MODE_SW");
}

int FlightModesComponentController::autoModeChannelIndex(void)
{
    return _channelToChannelIndex("RC_MAP_MODE_SW");
}

int FlightModesComponentController::acroModeChannelIndex(void)
{
    return _channelToChannelIndex("RC_MAP_ACRO_SW");
}

int FlightModesComponentController::altCtlModeChannelIndex(void)
{
    int posCtlSwitchChannel = getParameterFact(-1, "RC_MAP_POSCTL_SW")->value().toInt();
    
    if (posCtlSwitchChannel == 0) {
        return _channelToChannelIndex("RC_MAP_MODE_SW");
    } else {
        return _channelToChannelIndex(posCtlSwitchChannel);
    }
}

int FlightModesComponentController::posCtlModeChannelIndex(void)
{
    return _channelToChannelIndex("RC_MAP_POSCTL_SW");
}

int FlightModesComponentController::loiterModeChannelIndex(void)
{
    return _channelToChannelIndex("RC_MAP_LOITER_SW");
}

int FlightModesComponentController::missionModeChannelIndex(void)
{
    int loiterSwitchChannel = getParameterFact(-1, "RC_MAP_LOITER_SW")->value().toInt();
    
    if (loiterSwitchChannel == 0) {
        return _channelToChannelIndex("RC_MAP_MODE_SW");
    } else {
        return _channelToChannelIndex(loiterSwitchChannel);
    }
}

int FlightModesComponentController::returnModeChannelIndex(void)
{
    return _channelToChannelIndex("RC_MAP_RETURN_SW");
}

int FlightModesComponentController::offboardModeChannelIndex(void)
{
    return _channelToChannelIndex("RC_MAP_OFFB_SW");
}

int FlightModesComponentController::_channelIndexToChannel(int index)
{
    return _channelListModelChannel[index];
}

void FlightModesComponentController::setManualModeChannelIndex(int index)
{
    getParameterFact(-1, "RC_MAP_MODE_SW")->setValue(_channelIndexToChannel(index));
    
    _recalcModeSelections();
    _recalcModeRows();
}

void FlightModesComponentController::setAcroModeChannelIndex(int index)
{
    getParameterFact(-1, "RC_MAP_ACRO_SW")->setValue(_channelIndexToChannel(index));
    
    _recalcModeSelections();
    _recalcModeRows();
}

void FlightModesComponentController::setPosCtlModeChannelIndex(int index)
{
    int channel = _channelIndexToChannel(index);
    
    getParameterFact(-1, "RC_MAP_POSCTL_SW")->setValue(channel);
    
    if (channel == 0) {
        // PosCtl disabled so AltCtl must move back to main Mode switch
        _assistModeVisible = false;
    } else {
        // Assist mode is visible if AltCtl/PosCtl are on seperate channel from main Mode switch
        _assistModeVisible = channel != getParameterFact(-1, "RC_MAP_MODE_SW")->value().toInt();
    }
    
    emit altCtlModeChannelIndexChanged(index);
    emit modesVisibleChanged();

    _recalcModeSelections();
    _recalcModeRows();
}

void FlightModesComponentController::setLoiterModeChannelIndex(int index)
{
    int channel = _channelIndexToChannel(index);
    
    getParameterFact(-1, "RC_MAP_LOITER_SW")->setValue(channel);
    
    if (channel == 0) {
        // Loiter disabled so Mission must move back to main Mode switch
        _autoModeVisible = false;
    } else {
        // Auto mode is visible if Mission/Loiter are on seperate channel from main Mode switch
        _autoModeVisible = channel != getParameterFact(-1, "RC_MAP_MODE_SW")->value().toInt();
    }
    
    emit missionModeChannelIndexChanged(index);
    emit modesVisibleChanged();
    
    _recalcModeSelections();
    _recalcModeRows();
}

void FlightModesComponentController::setReturnModeChannelIndex(int index)
{
    getParameterFact(-1, "RC_MAP_RETURN_SW")->setValue(_channelIndexToChannel(index));
    _recalcModeSelections();
    _recalcModeRows();
}

void FlightModesComponentController::setOffboardModeChannelIndex(int index)
{
    getParameterFact(-1, "RC_MAP_OFFB_SW")->setValue(_channelIndexToChannel(index));
    _recalcModeSelections();
    _recalcModeRows();
}

void FlightModesComponentController::generateThresholds(void)
{
    // Reset all thresholds to 0.0
    
    QStringList thresholdParams;
    
    thresholdParams << "RC_ASSIST_TH" << "RC_AUTO_TH" << "RC_ACRO_TH" << "RC_POSCTL_TH" << "RC_LOITER_TH" << "RC_RETURN_TH" << "RC_OFFB_TH";
    
    foreach(QString thresholdParam, thresholdParams) {
        getParameterFact(-1, thresholdParam)->setValue(0.0f);
    }
    
    // Redistribute
    
    int modeChannel =       getParameterFact(-1, "RC_MAP_MODE_SW")->value().toInt();
    int acroChannel =       getParameterFact(-1, "RC_MAP_ACRO_SW")->value().toInt();
    int posCtlChannel =     getParameterFact(-1, "RC_MAP_POSCTL_SW")->value().toInt();
    int loiterChannel =     getParameterFact(-1, "RC_MAP_LOITER_SW")->value().toInt();
    int returnChannel =     getParameterFact(-1, "RC_MAP_RETURN_SW")->value().toInt();
    int offboardChannel =   getParameterFact(-1, "RC_MAP_OFFB_SW")->value().toInt();
    
    if (modeChannel != 0) {
        int positions = 3;  // Manual/Assist/Auto always exist
        
        bool acroOnModeSwitch = modeChannel == acroChannel;
        bool posCtlOnModeSwitch = modeChannel == posCtlChannel;
        bool loiterOnModeSwitch = modeChannel == loiterChannel;
        bool returnOnModeSwitch = modeChannel == returnChannel;
        bool offboardOnModeSwitch = modeChannel == offboardChannel;
        
        positions += acroOnModeSwitch ? 1 : 0;
        positions += posCtlOnModeSwitch ? 1 : 0;
        positions += loiterOnModeSwitch ? 1 : 0;
        positions += returnOnModeSwitch ? 1 : 0;
        positions += offboardOnModeSwitch ? 1 : 0;
        
        float increment = 1.0f / positions;
        float currentThreshold = 0.0f;
        
        if (acroOnModeSwitch) {
            currentThreshold += increment;
            getParameterFact(-1, "RC_ACRO_TH")->setValue(currentThreshold);
            acroChannel = 0;
        }
        
        currentThreshold += increment;
        getParameterFact(-1, "RC_ASSIST_TH")->setValue(currentThreshold);
        if (posCtlOnModeSwitch) {
            currentThreshold += increment;
            getParameterFact(-1, "RC_POSCTL_TH")->setValue(currentThreshold);
            posCtlChannel = 0;
        }
        
        currentThreshold += increment;
        getParameterFact(-1, "RC_AUTO_TH")->setValue(currentThreshold);
        if (loiterOnModeSwitch) {
            currentThreshold += increment;
            getParameterFact(-1, "RC_LOITER_TH")->setValue(currentThreshold);
            loiterChannel = 0;
        }
        
        if (returnOnModeSwitch) {
            currentThreshold += increment;
            getParameterFact(-1, "RC_RETURN_TH")->setValue(currentThreshold);
            returnChannel = 0;
        }
        
        if (offboardOnModeSwitch) {
            currentThreshold += increment;
            getParameterFact(-1, "RC_OFFB_TH")->setValue(currentThreshold);
            offboardChannel = 0;
        }
    }
    
    if (acroChannel != 0) {
        // If only two positions don't set threshold at midrange. Setting to 0.25
        // allows for this channel to work with either a two or three position switch
        getParameterFact(-1, "RC_ACRO_TH")->setValue(0.25f);
    }
    
    if (posCtlChannel != 0) {
        getParameterFact(-1, "RC_POSCTL_TH")->setValue(0.25f);
    }
    
    if (loiterChannel != 0) {
        getParameterFact(-1, "RC_LOITER_TH")->setValue(0.25f);
    }
    
    if (returnChannel != 0) {
        getParameterFact(-1, "RC_RETURN_TH")->setValue(0.25f);
    }
    
    if (offboardChannel != 0) {
        getParameterFact(-1, "RC_OFFB_TH")->setValue(0.25f);
    }
    
    emit thresholdsChanged();
}
