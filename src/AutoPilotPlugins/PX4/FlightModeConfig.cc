/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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
///     @brief Flight Mode Configuration
///     @author Don Gagne <don@thegagnes.com

#include "FlightModeConfig.h"
#include "UASManager.h"
#include "QGCMessageBox.h"

const char* FlightModeConfig::_modeSwitchParam = "RC_MAP_MODE_SW";
const char* FlightModeConfig::_manualSwitchParam = "RC_MAP_ACRO_SW";
const char* FlightModeConfig::_assistSwitchParam = "RC_MAP_POSCTL_SW";
const char* FlightModeConfig::_returnSwitchParam = "RC_MAP_RETURN_SW";
const char* FlightModeConfig::_loiterSwitchParam = "RC_MAP_LOITER_SW";

FlightModeConfig::FlightModeConfig(QWidget *parent) :
    QWidget(parent),
    _mav(NULL),
    _paramMgr(NULL),
    _ui(new Ui::FlightModeConfig)
{
    Q_CHECK_PTR(_ui);
    
    _ui->setupUi(this);
    
    // Expectation is that we have an active uas, it is connected and the parameters are ready
    _mav = UASManager::instance()->getActiveUAS();
    Q_ASSERT(_mav);
    
    _paramMgr = _mav->getParamManager();
    Q_ASSERT(_paramMgr);
    Q_ASSERT(_paramMgr->parametersReady());
        
    _componentId = _paramMgr->getDefaultComponentId();
    
    connect(_paramMgr, SIGNAL(parameterUpdated(int, QString, QVariant)), this, SLOT(_parameterUpdated(int, QString, QVariant)));
    
    _initUi();
}

/// @brief Intialize the UI elements
void FlightModeConfig::_initUi(void) {
    struct Combo2Param {
        QGCComboBox*    combo;
        QButtonGroup*   buttonGroup;
        const char*     param;
        const char*     description;
    };
    
    struct Combo2Param rgCombo2ParamMap[] = {
        { _ui->modeSwitchChannel,   _ui->modeSwitchGroup,   _modeSwitchParam,   "Mode Switch" },
        { _ui->manualSwitchChannel, _ui->manualSwitchGroup, _manualSwitchParam, "Manual Switch" },
        { _ui->assistSwitchChannel, _ui->assistSwitchGroup, _assistSwitchParam, "Assist Switch" },
        { _ui->returnSwitchChannel, _ui->returnSwitchGroup, _returnSwitchParam, "Return Switch" },
        { _ui->loiterSwitchChannel, _ui->loiterSwitchGroup, _loiterSwitchParam, "Loiter Switch" },
        { NULL,                     NULL,                   "RC_MAP_THROTTLE",  "Throttle" },
        { NULL,                     NULL,                   "RC_MAP_YAW",       "Rudder" },
        { NULL,                     NULL,                   "RC_MAP_ROLL",      "Aileron" },
        { NULL,                     NULL,                   "RC_MAP_PITCH",     "Elevator" },
        { NULL,                     NULL,                   "RC_MAP_AUX1",      "Aux1" },
        { NULL,                     NULL,                   "RC_MAP_AUX2",      "Aux2" },
        { NULL,                     NULL,                   "RC_MAP_AUX3",      "Aux3" }
    };
    
    // Setup up maps
    for (size_t i=0; i<sizeof(rgCombo2ParamMap)/sizeof(rgCombo2ParamMap[0]); i++) {
        struct Combo2Param* map = &rgCombo2ParamMap[i];
        
        if (map->combo) {
            Q_ASSERT(!_mapChannelCombo2Param.contains(map->combo));
            _mapChannelCombo2Param[map->combo] = map->param;

            Q_ASSERT(!_mapChannelCombo2ButtonGroup.contains(map->combo));
            _mapChannelCombo2ButtonGroup[map->combo] = map->buttonGroup;
            
            // We connect to activated because we only want signals from user initiated changes
            connect(map->combo, SIGNAL(activated(int)), this, SLOT(_channelSelected(int)));
        }
        
        Q_ASSERT(!_mapParam2FunctionInfo.contains(map->param));
        _mapParam2FunctionInfo[map->param] = map->description;
    }
    
    _updateAllSwitches();
    
    // Finally if RC Calibration has not been performed disable the entire widget and inform user
    if (_getChannelMapForParam(_modeSwitchParam) == 0) {
        // FIXME: Do something more than disable
        setEnabled(false);
    }
}

void FlightModeConfig::_updateAllSwitches(void)
{
    foreach(QGCComboBox* combo, _mapChannelCombo2Param.keys()) {
        _updateSwitchUiGroup(combo);
    }
}

/// @brief Selects the channel in the combo for the given parameter
void FlightModeConfig::_selectChannelForParam(QGCComboBox* combo, const QString& parameter)
{
    int channel = _getChannelMapForParam(parameter);
    
    int index = combo->findData(channel);
    Q_ASSERT(index != -1);
    
    combo->setCurrentIndex(index);
}

/// @brief Fills a channel selection combo box with channel values
void FlightModeConfig::_updateSwitchUiGroup(QGCComboBox* combo)
{
    QMap<int, QString> mapChannelUsed2Description;
    
    // Make a map of all channels currently mapped
    foreach(QString paramId, _mapParam2FunctionInfo.keys()) {
        int channel = _getChannelMapForParam(paramId);
        if (channel != 0) {
            mapChannelUsed2Description[channel] = _mapParam2FunctionInfo[paramId];
        }
    }
    
    combo->clear();
    combo->addItem("Disabled", 0);
    for (int i=1; i<=_maxChannels; i++) {
        QString comboText = QString("Channel %1").arg(i);
        
        if (mapChannelUsed2Description.contains(i)) {
            comboText += QString(" (%1)").arg(mapChannelUsed2Description[i]);
        }
        
        combo->addItem(comboText, i);
    }
    
    _selectChannelForParam(combo, _mapChannelCombo2Param[combo]);
    
    // Enable/Disable radio buttons appropriately
    
    int channelMap = _getChannelMapForParam(_mapChannelCombo2Param[combo]);
    
    Q_ASSERT(_mapChannelCombo2ButtonGroup.contains(combo));
    Q_ASSERT(_mapChannelCombo2ButtonGroup[combo]->buttons().count() > 0);
    
    foreach(QAbstractButton* button, _mapChannelCombo2ButtonGroup[combo]->buttons()) {
        QRadioButton* radio = qobject_cast<QRadioButton*>(button);
        
        Q_ASSERT(radio);
        
        radio->setEnabled(channelMap != 0);
    }
}

/// @brief Returns channel mapping for the specified parameters
int FlightModeConfig::_getChannelMapForParam(const QString& paramId)
{
    QVariant value;
    bool found = _paramMgr->getParameterValue(_componentId, paramId, value);
    Q_UNUSED(found);
    Q_ASSERT(found);
    
    bool conversionOk;
    int channel = value.toInt(&conversionOk);
    Q_UNUSED(conversionOk);
    Q_ASSERT(conversionOk);
    
    return channel;
}

/// @brief Called when a selection occurs in one of the channel combo boxes
void FlightModeConfig::_channelSelected(int requestedMapping)
{
    QGCComboBox* combo = qobject_cast<QGCComboBox*>(sender());
    Q_ASSERT(combo);
    Q_ASSERT(_mapChannelCombo2Param.contains(combo));
    
    // Check for no-op
    
    int currentMapping = _getChannelMapForParam(_mapChannelCombo2Param[combo]);
    if (currentMapping == requestedMapping) {
        return;
    }
    
    // Make sure that channel is not already mapped
    
    if (requestedMapping == 0) {
        // Channel was disabled
        if (combo == _ui->modeSwitchChannel) {
            // Mode switch is not allowed to be disabled.
            QGCMessageBox::warning(tr("Flight Mode"), "Mode Switch is a required switch and cannot be disabled.");
            _selectChannelForParam(combo, _mapChannelCombo2Param[combo]);
            return;
        }
    } else {
        // Channel was remapped to a non-disabled channel
        foreach(QString paramId, _mapParam2FunctionInfo.keys()) {
            int usedChannelMap = _getChannelMapForParam(paramId);

            if (requestedMapping == usedChannelMap) {
                QGCMessageBox::warning(tr("Flight Mode"), "You cannot use a channel which is already mapped.");
                _selectChannelForParam(combo, _mapChannelCombo2Param[combo]);
                return;
            }
        }
    }
    
    // Save the new setting to the parameter
    _paramMgr->setParameter(_componentId, _mapChannelCombo2Param[combo], requestedMapping);
    _paramMgr->sendPendingParameters(true, false);
}

void FlightModeConfig::_parameterUpdated(int componentId, QString parameter, QVariant value)
{
    Q_UNUSED(componentId);
    Q_UNUSED(value);
    
    // If this is a function mapping parameter update the combos
    if (_mapParam2FunctionInfo.contains(parameter)) {
        _updateAllSwitches();
    }
    
    // Finally if RC Calibration suddenly needs to be re-done disable the entire widget and inform user
    if (parameter == _modeSwitchParam) {
        // FIXME: Do something more than disable
        if (isEnabled() == false && value.toInt() != 0) {
            setEnabled(true);
        } else if (isEnabled() == true && value.toInt() == 0) {
            setEnabled(false);
        }
    }
}