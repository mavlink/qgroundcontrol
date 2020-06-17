/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "PX4AdvancedFlightModesController.h"
#include "QGCMAVLink.h"

#include <QVariant>
#include <QQmlProperty>

PX4AdvancedFlightModesController::PX4AdvancedFlightModesController(void) :
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
    QStringList usedParams = QStringList({
        "RC_MAP_THROTTLE", "RC_MAP_YAW", "RC_MAP_PITCH", "RC_MAP_ROLL", "RC_MAP_FLAPS", "RC_MAP_AUX1", "RC_MAP_AUX2",
        "RC_MAP_MODE_SW",  "RC_MAP_RETURN_SW", "RC_MAP_LOITER_SW", "RC_MAP_POSCTL_SW", "RC_MAP_OFFB_SW", "RC_MAP_ACRO_SW"});

    if (!_allParametersExists(FactSystem::defaultComponentId, usedParams)) {
        return;
    }

    _init();
    _validateConfiguration();

    connect(_vehicle, &Vehicle::rcChannelsChanged, this, &PX4AdvancedFlightModesController::_rcChannelsChanged);
}

void PX4AdvancedFlightModesController::_init(void)
{
    // FIXME: What about VTOL? That confuses the whole Flight Mode naming scheme
    _fixedWing = _vehicle->fixedWing();
    
    // We need to know min and max for channel in order to calculate percentage range
    for (int channel=0; channel<_chanMax; channel++) {
        QString rcMinParam, rcMaxParam, rcRevParam;
        
        rcMinParam = QString("RC%1_MIN").arg(channel+1);
        rcMaxParam = QString("RC%1_MAX").arg(channel+1);
        rcRevParam = QString("RC%1_REV").arg(channel+1);

        _rgRCMin[channel] = getParameterFact(FactSystem::defaultComponentId, rcMinParam)->rawValue().toInt();
        _rgRCMax[channel] = getParameterFact(FactSystem::defaultComponentId, rcMaxParam)->rawValue().toInt();
        
        float floatReversed = getParameterFact(-1, rcRevParam)->rawValue().toFloat();
        _rgRCReversed[channel] = floatReversed == -1.0f;
        
        _rcValues[channel] = 0.0;
    }

    // RC_CHAN_CNT parameter is set by Radio Cal to specify the number of radio channels.
    if (parameterExists(FactSystem::defaultComponentId, "RC_CHAN_CNT")) {
        _channelCount = getParameterFact(FactSystem::defaultComponentId, "RC_CHAN_CNT")->rawValue().toInt();
    } else {
        _channelCount =_chanMax;
    }
    if (_channelCount <= 0 || _channelCount > _chanMax) {
        // Parameter exists, but has not yet been set or is invalid. Use default
        _channelCount = _chanMax;
    }
    
    int modeChannel = getParameterFact(-1, "RC_MAP_MODE_SW")->rawValue().toInt();
    int posCtlChannel = getParameterFact(-1, "RC_MAP_POSCTL_SW")->rawValue().toInt();
    int loiterChannel = getParameterFact(-1, "RC_MAP_LOITER_SW")->rawValue().toInt();
    
    if (posCtlChannel == 0) {
        // PosCtl disabled so AltCtl must move back to main Mode switch
        _assistModeVisible = false;
    } else {
        // Assist mode is visible if AltCtl/PosCtl are on separate channel from main Mode switch
        _assistModeVisible = posCtlChannel != modeChannel;
    }
    
    if (loiterChannel == 0) {
        // Loiter disabled so Mission must move back to main Mode switch
        _autoModeVisible = false;
    } else {
        // Auto mode is visible if Mission/Loiter are on separate channel from main Mode switch
        _autoModeVisible = loiterChannel != modeChannel;
    }

    // Setup the channel combobox model
    QVector<int> usedChannels;

    for (const QString &attitudeParam : {"RC_MAP_THROTTLE", "RC_MAP_YAW", "RC_MAP_PITCH", "RC_MAP_ROLL", "RC_MAP_FLAPS", "RC_MAP_AUX1", "RC_MAP_AUX2"}) {
        int channel = getParameterFact(-1, attitudeParam)->rawValue().toInt();
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
void PX4AdvancedFlightModesController::_validateConfiguration(void)
{
    _validConfiguration = true;
    
    // Make sure switches are valid and within channel range

    const QStringList switchParams = {"RC_MAP_MODE_SW", "RC_MAP_ACRO_SW",  "RC_MAP_POSCTL_SW", "RC_MAP_LOITER_SW", "RC_MAP_RETURN_SW", "RC_MAP_OFFB_SW"};
    QList<int> switchMappings;

    for(int i=0, end = switchParams.count(); i < end; i++) {
        int map = getParameterFact(FactSystem::defaultComponentId, switchParams[i])->rawValue().toInt();
        switchMappings << map;
        
        if (map < 0 || map > _channelCount) {
            _validConfiguration = false;
            _configurationErrors += tr("%1 is set to %2. Mapping must between 0 and %3 (inclusive).\n").arg(switchParams[i]).arg(map).arg(_channelCount);
        }
    }
    
    // Make sure mode switches are not double-mapped

    const QStringList attitudeParams = {"RC_MAP_THROTTLE", "RC_MAP_YAW", "RC_MAP_PITCH", "RC_MAP_ROLL", "RC_MAP_FLAPS", "RC_MAP_AUX1", "RC_MAP_AUX2"};
    for (int i=0, end = attitudeParams.count(); i < end; i++) {
        int map = getParameterFact(FactSystem::defaultComponentId, attitudeParams[i])->rawValue().toInt();
        if (map == 0) {
            continue;
        }

        for (int j=0; j<switchParams.count(); j++) {
            if (map == switchMappings[j]) {
                _validConfiguration = false;
                _configurationErrors += tr("%1 is set to same channel as %2.\n").arg(switchParams[j], attitudeParams[i]);
            }
        }
    }
    
    // Validate thresholds within range
    for(const QString &thresholdParam : {"RC_ASSIST_TH", "RC_AUTO_TH", "RC_ACRO_TH", "RC_POSCTL_TH", "RC_LOITER_TH", "RC_RETURN_TH", "RC_OFFB_TH"}) {
        float threshold = getParameterFact(-1, thresholdParam)->rawValue().toFloat();
        if (threshold < 0.0f || threshold > 1.0f) {
            _validConfiguration = false;
            _configurationErrors += tr("%1 is set to %2. Threshold must between 0.0 and 1.0 (inclusive).\n").arg(thresholdParam).arg(threshold);
        }
    }
}

/// Connected to Vehicle::rcChannelsChanged signal
void PX4AdvancedFlightModesController::_rcChannelsChanged(int channelCount, int pwmValues[Vehicle::cMaxRcChannels])
{
    for (int channel=0; channel<channelCount; channel++) {
        int channelValue = pwmValues[channel];

        if (channelValue != -1) {
            if (channelValue < _rgRCMin[channel]) {
                channelValue= _rgRCMin[channel];
            }
            if (channelValue > _rgRCMax[channel]) {
                channelValue= _rgRCMax[channel];
            }

            float percentRange = (channelValue - _rgRCMin[channel]) / (float)(_rgRCMax[channel] - _rgRCMin[channel]);
            if (_rgRCReversed[channel]) {
                percentRange = 1.0 - percentRange;
            }

            _rcValues[channel] = percentRange;
        }
    }
    
    _recalcModeSelections();
    
    emit switchLiveRangeChanged();
}

double PX4AdvancedFlightModesController::_switchLiveRange(const QString& param)
{
    int channel = getParameterFact(-1, param)->rawValue().toInt();
    if (channel == 0) {
        return 0.0;
    } else {
        return _rcValues[channel - 1];
    }
}

double PX4AdvancedFlightModesController::manualModeRcValue(void)
{
    return _switchLiveRange("RC_MAP_MODE_SW");
}

double PX4AdvancedFlightModesController::assistModeRcValue(void)
{
    return manualModeRcValue();
}

double PX4AdvancedFlightModesController::autoModeRcValue(void)
{
    return manualModeRcValue();
}

double PX4AdvancedFlightModesController::acroModeRcValue(void)
{
    return _switchLiveRange("RC_MAP_ACRO_SW");
}

double PX4AdvancedFlightModesController::altCtlModeRcValue(void)
{
    int posCtlSwitchChannel = getParameterFact(-1, "RC_MAP_POSCTL_SW")->rawValue().toInt();
    
    if (posCtlSwitchChannel == 0) {
        return _switchLiveRange("RC_MAP_MODE_SW");
    } else {
        return _switchLiveRange("RC_MAP_POSCTL_SW");
    }
}

double PX4AdvancedFlightModesController::posCtlModeRcValue(void)
{
    return _switchLiveRange("RC_MAP_POSCTL_SW");
}

double PX4AdvancedFlightModesController::missionModeRcValue(void)
{
    int returnSwitchChannel = getParameterFact(-1, "RC_MAP_RETURN_SW")->rawValue().toInt();
    int loiterSwitchChannel = getParameterFact(-1, "RC_MAP_LOITER_SW")->rawValue().toInt();

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

double PX4AdvancedFlightModesController::loiterModeRcValue(void)
{
    return _switchLiveRange("RC_MAP_LOITER_SW");
}

double PX4AdvancedFlightModesController::returnModeRcValue(void)
{
    return _switchLiveRange("RC_MAP_RETURN_SW");
}

double PX4AdvancedFlightModesController::offboardModeRcValue(void)
{
    return _switchLiveRange("RC_MAP_OFFB_SW");
}

void PX4AdvancedFlightModesController::_recalcModeSelections(void)
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
    int modeSwitchChannel =     getParameterFact(-1, "RC_MAP_MODE_SW")->rawValue().toInt() - 1;
    int acroSwitchChannel =     getParameterFact(-1, "RC_MAP_ACRO_SW")->rawValue().toInt() - 1;
    int posCtlSwitchChannel =   getParameterFact(-1, "RC_MAP_POSCTL_SW")->rawValue().toInt() - 1;
    int loiterSwitchChannel =   getParameterFact(-1, "RC_MAP_LOITER_SW")->rawValue().toInt() - 1;
    int returnSwitchChannel =   getParameterFact(-1, "RC_MAP_RETURN_SW")->rawValue().toInt() - 1;
    int offboardSwitchChannel = getParameterFact(-1, "RC_MAP_OFFB_SW")->rawValue().toInt() - 1;
    
    double autoThreshold =      getParameterFact(-1, "RC_AUTO_TH")->rawValue().toDouble();
    double assistThreshold =    getParameterFact(-1, "RC_ASSIST_TH")->rawValue().toDouble();
    double acroThreshold =      getParameterFact(-1, "RC_ACRO_TH")->rawValue().toDouble();
    double posCtlThreshold =    getParameterFact(-1, "RC_POSCTL_TH")->rawValue().toDouble();
    double loiterThreshold =    getParameterFact(-1, "RC_LOITER_TH")->rawValue().toDouble();
    double returnThreshold =    getParameterFact(-1, "RC_RETURN_TH")->rawValue().toDouble();
    double offboardThreshold =  getParameterFact(-1, "RC_OFFB_TH")->rawValue().toDouble();
    
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

void PX4AdvancedFlightModesController::_recalcModeRows(void)
{
    int modeSwitchChannel =     getParameterFact(-1, "RC_MAP_MODE_SW")->rawValue().toInt();
    int acroSwitchChannel =     getParameterFact(-1, "RC_MAP_ACRO_SW")->rawValue().toInt();
    int posCtlSwitchChannel =   getParameterFact(-1, "RC_MAP_POSCTL_SW")->rawValue().toInt();
    int loiterSwitchChannel =   getParameterFact(-1, "RC_MAP_LOITER_SW")->rawValue().toInt();
    int returnSwitchChannel =   getParameterFact(-1, "RC_MAP_RETURN_SW")->rawValue().toInt();
    int offboardSwitchChannel = getParameterFact(-1, "RC_MAP_OFFB_SW")->rawValue().toInt();
    
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

double PX4AdvancedFlightModesController::manualModeThreshold(void)
{
    return 0.0;
}

double PX4AdvancedFlightModesController::assistModeThreshold(void)
{
    return getParameterFact(-1, "RC_ASSIST_TH")->rawValue().toDouble();
}

double PX4AdvancedFlightModesController::autoModeThreshold(void)
{
    return getParameterFact(-1, "RC_AUTO_TH")->rawValue().toDouble();
}

double PX4AdvancedFlightModesController::acroModeThreshold(void)
{
    return getParameterFact(-1, "RC_ACRO_TH")->rawValue().toDouble();
}

double PX4AdvancedFlightModesController::altCtlModeThreshold(void)
{
    return _assistModeVisible ? 0.0 : getParameterFact(-1, "RC_ASSIST_TH")->rawValue().toDouble();
}

double PX4AdvancedFlightModesController::posCtlModeThreshold(void)
{
    return getParameterFact(-1, "RC_POSCTL_TH")->rawValue().toDouble();
}

double PX4AdvancedFlightModesController::missionModeThreshold(void)
{
    return _autoModeVisible ? 0.0 : getParameterFact(-1, "RC_AUTO_TH")->rawValue().toDouble();
}


double PX4AdvancedFlightModesController::loiterModeThreshold(void)
{
    return getParameterFact(-1, "RC_LOITER_TH")->rawValue().toDouble();
}

double PX4AdvancedFlightModesController::returnModeThreshold(void)
{
    return getParameterFact(-1, "RC_RETURN_TH")->rawValue().toDouble();
}

double PX4AdvancedFlightModesController::offboardModeThreshold(void)
{
    return getParameterFact(-1, "RC_OFFB_TH")->rawValue().toDouble();
}

void PX4AdvancedFlightModesController::setAssistModeThreshold(double threshold)
{
    getParameterFact(-1, "RC_ASSIST_TH")->setRawValue(threshold);
    _recalcModeSelections();
}

void PX4AdvancedFlightModesController::setAutoModeThreshold(double threshold)
{
    getParameterFact(-1, "RC_AUTO_TH")->setRawValue(threshold);
    _recalcModeSelections();
}

void PX4AdvancedFlightModesController::setAcroModeThreshold(double threshold)
{
    getParameterFact(-1, "RC_ACRO_TH")->setRawValue(threshold);
    _recalcModeSelections();
}

void PX4AdvancedFlightModesController::setAltCtlModeThreshold(double threshold)
{
    setAssistModeThreshold(threshold);
}

void PX4AdvancedFlightModesController::setPosCtlModeThreshold(double threshold)
{
    getParameterFact(-1, "RC_POSCTL_TH")->setRawValue(threshold);
    _recalcModeSelections();
}

void PX4AdvancedFlightModesController::setMissionModeThreshold(double threshold)
{
    setAutoModeThreshold(threshold);
}

void PX4AdvancedFlightModesController::setLoiterModeThreshold(double threshold)
{
    getParameterFact(-1, "RC_LOITER_TH")->setRawValue(threshold);
    _recalcModeSelections();
}

void PX4AdvancedFlightModesController::setReturnModeThreshold(double threshold)
{
    getParameterFact(-1, "RC_RETURN_TH")->setRawValue(threshold);
    _recalcModeSelections();
}

void PX4AdvancedFlightModesController::setOffboardModeThreshold(double threshold)
{
    getParameterFact(-1, "RC_OFFB_TH")->setRawValue(threshold);
    _recalcModeSelections();
}

int PX4AdvancedFlightModesController::_channelToChannelIndex(int channel)
{
    return _channelListModelChannel.lastIndexOf(channel);
}

int PX4AdvancedFlightModesController::_channelToChannelIndex(const QString& channelParam)
{
    return _channelToChannelIndex(getParameterFact(-1, channelParam)->rawValue().toInt());
}

int PX4AdvancedFlightModesController::manualModeChannelIndex(void)
{
    return _channelToChannelIndex("RC_MAP_MODE_SW");
}

int PX4AdvancedFlightModesController::assistModeChannelIndex(void)
{
    return _channelToChannelIndex("RC_MAP_MODE_SW");
}

int PX4AdvancedFlightModesController::autoModeChannelIndex(void)
{
    return _channelToChannelIndex("RC_MAP_MODE_SW");
}

int PX4AdvancedFlightModesController::acroModeChannelIndex(void)
{
    return _channelToChannelIndex("RC_MAP_ACRO_SW");
}

int PX4AdvancedFlightModesController::altCtlModeChannelIndex(void)
{
    int posCtlSwitchChannel = getParameterFact(-1, "RC_MAP_POSCTL_SW")->rawValue().toInt();
    
    if (posCtlSwitchChannel == 0) {
        return _channelToChannelIndex("RC_MAP_MODE_SW");
    } else {
        return _channelToChannelIndex(posCtlSwitchChannel);
    }
}

int PX4AdvancedFlightModesController::posCtlModeChannelIndex(void)
{
    return _channelToChannelIndex("RC_MAP_POSCTL_SW");
}

int PX4AdvancedFlightModesController::loiterModeChannelIndex(void)
{
    return _channelToChannelIndex("RC_MAP_LOITER_SW");
}

int PX4AdvancedFlightModesController::missionModeChannelIndex(void)
{
    int loiterSwitchChannel = getParameterFact(-1, "RC_MAP_LOITER_SW")->rawValue().toInt();
    
    if (loiterSwitchChannel == 0) {
        return _channelToChannelIndex("RC_MAP_MODE_SW");
    } else {
        return _channelToChannelIndex(loiterSwitchChannel);
    }
}

int PX4AdvancedFlightModesController::returnModeChannelIndex(void)
{
    return _channelToChannelIndex("RC_MAP_RETURN_SW");
}

int PX4AdvancedFlightModesController::offboardModeChannelIndex(void)
{
    return _channelToChannelIndex("RC_MAP_OFFB_SW");
}

int PX4AdvancedFlightModesController::_channelIndexToChannel(int index)
{
    return _channelListModelChannel[index];
}

void PX4AdvancedFlightModesController::setManualModeChannelIndex(int index)
{
    getParameterFact(-1, "RC_MAP_MODE_SW")->setRawValue(_channelIndexToChannel(index));
    
    _recalcModeSelections();
    _recalcModeRows();
    emit channelIndicesChanged();

}

void PX4AdvancedFlightModesController::setAcroModeChannelIndex(int index)
{
    getParameterFact(-1, "RC_MAP_ACRO_SW")->setRawValue(_channelIndexToChannel(index));
    
    _recalcModeSelections();
    _recalcModeRows();
}

void PX4AdvancedFlightModesController::setPosCtlModeChannelIndex(int index)
{
    int channel = _channelIndexToChannel(index);
    
    getParameterFact(-1, "RC_MAP_POSCTL_SW")->setRawValue(channel);
    
    if (channel == 0) {
        // PosCtl disabled so AltCtl must move back to main Mode switch
        _assistModeVisible = false;
    } else {
        // Assist mode is visible if AltCtl/PosCtl are on separate channel from main Mode switch
        _assistModeVisible = channel != getParameterFact(-1, "RC_MAP_MODE_SW")->rawValue().toInt();
    }
    
    emit modesVisibleChanged();

    _recalcModeSelections();
    _recalcModeRows();
    emit channelIndicesChanged();

}

void PX4AdvancedFlightModesController::setLoiterModeChannelIndex(int index)
{
    int channel = _channelIndexToChannel(index);
    
    getParameterFact(-1, "RC_MAP_LOITER_SW")->setRawValue(channel);
    
    if (channel == 0) {
        // Loiter disabled so Mission must move back to main Mode switch
        _autoModeVisible = false;
    } else {
        // Auto mode is visible if Mission/Loiter are on separate channel from main Mode switch
        _autoModeVisible = channel != getParameterFact(-1, "RC_MAP_MODE_SW")->rawValue().toInt();
    }
    
    emit modesVisibleChanged();
    
    _recalcModeSelections();
    _recalcModeRows();
    emit channelIndicesChanged();

}

void PX4AdvancedFlightModesController::setReturnModeChannelIndex(int index)
{
    getParameterFact(-1, "RC_MAP_RETURN_SW")->setRawValue(_channelIndexToChannel(index));
    _recalcModeSelections();
    _recalcModeRows();
    emit channelIndicesChanged();

}

void PX4AdvancedFlightModesController::setOffboardModeChannelIndex(int index)
{
    getParameterFact(-1, "RC_MAP_OFFB_SW")->setRawValue(_channelIndexToChannel(index));
    _recalcModeSelections();
    _recalcModeRows();
    emit channelIndicesChanged();

}

void PX4AdvancedFlightModesController::generateThresholds(void)
{
    // Reset all thresholds to 0.0
    
    QStringList thresholdParams;
    
    thresholdParams << "RC_ASSIST_TH" << "RC_AUTO_TH" << "RC_ACRO_TH" << "RC_POSCTL_TH" << "RC_LOITER_TH" << "RC_RETURN_TH" << "RC_OFFB_TH";
    
    foreach(const QString &thresholdParam, thresholdParams) {
        getParameterFact(-1, thresholdParam)->setRawValue(0.0f);
    }
    
    // Redistribute
    
    int modeChannel =       getParameterFact(-1, "RC_MAP_MODE_SW")->rawValue().toInt();
    int acroChannel =       getParameterFact(-1, "RC_MAP_ACRO_SW")->rawValue().toInt();
    int posCtlChannel =     getParameterFact(-1, "RC_MAP_POSCTL_SW")->rawValue().toInt();
    int loiterChannel =     getParameterFact(-1, "RC_MAP_LOITER_SW")->rawValue().toInt();
    int returnChannel =     getParameterFact(-1, "RC_MAP_RETURN_SW")->rawValue().toInt();
    int offboardChannel =   getParameterFact(-1, "RC_MAP_OFFB_SW")->rawValue().toInt();
    
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
            getParameterFact(-1, "RC_ACRO_TH")->setRawValue(currentThreshold);
            acroChannel = 0;
        }
        
        currentThreshold += increment;
        getParameterFact(-1, "RC_ASSIST_TH")->setRawValue(currentThreshold);
        if (posCtlOnModeSwitch) {
            currentThreshold += increment;
            getParameterFact(-1, "RC_POSCTL_TH")->setRawValue(currentThreshold);
            posCtlChannel = 0;
        }
        
        currentThreshold += increment;
        getParameterFact(-1, "RC_AUTO_TH")->setRawValue(currentThreshold);
        if (loiterOnModeSwitch) {
            currentThreshold += increment;
            getParameterFact(-1, "RC_LOITER_TH")->setRawValue(currentThreshold);
            loiterChannel = 0;
        }
        
        if (returnOnModeSwitch) {
            currentThreshold += increment;
            getParameterFact(-1, "RC_RETURN_TH")->setRawValue(currentThreshold);
            returnChannel = 0;
        }
        
        if (offboardOnModeSwitch) {
            currentThreshold += increment;
            getParameterFact(-1, "RC_OFFB_TH")->setRawValue(currentThreshold);
            offboardChannel = 0;
        }
    }
    
    if (acroChannel != 0) {
        // If only two positions don't set threshold at midrange. Setting to 0.25
        // allows for this channel to work with either a two or three position switch
        getParameterFact(-1, "RC_ACRO_TH")->setRawValue(0.25f);
    }
    
    if (posCtlChannel != 0) {
        getParameterFact(-1, "RC_POSCTL_TH")->setRawValue(0.25f);
    }
    
    if (loiterChannel != 0) {
        getParameterFact(-1, "RC_LOITER_TH")->setRawValue(0.25f);
    }
    
    if (returnChannel != 0) {
        getParameterFact(-1, "RC_RETURN_TH")->setRawValue(0.25f);
    }
    
    if (offboardChannel != 0) {
        getParameterFact(-1, "RC_OFFB_TH")->setRawValue(0.25f);
    }
    
    emit thresholdsChanged();
}
