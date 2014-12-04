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

const char* FlightModeConfig::_mainSwitchParam = "RC_MAP_MODE_SW";
const char* FlightModeConfig::_manualSwitchParam = "RC_MAP_ACRO_SW";
const char* FlightModeConfig::_assistSwitchParam = "RC_MAP_POSCTL_SW";
const char* FlightModeConfig::_returnSwitchParam = "RC_MAP_RETURN_SW";
const char* FlightModeConfig::_loiterSwitchParam = "RC_MAP_LOITER_SW";

FlightModeConfig::FlightModeConfig(QWidget *parent) :
    QWidget(parent),
    _mav(NULL),
    _paramMgr(NULL),
    _parameterListUpToDateSignalled(false),
    _ui(new Ui::FlightModeConfig)
{
    Q_CHECK_PTR(_ui);
    
    _ui->setupUi(this);
    
    _mav = UASManager::instance()->getActiveUAS();
    
    // This implementation does not handle uas coming an going. It expects to be created when there ia an active UAS.
    // It expects to be detroyed when the UAS goes away.
    Q_ASSERT(_mav);
    
    _paramMgr = _mav->getParamManager();
    Q_ASSERT(_paramMgr);
    _componentId = _paramMgr->getDefaultComponentId();
    
    connect(_paramMgr, SIGNAL(parameterListUpToDate()), this, SLOT(_parameterListUpToDate()));
    connect(_paramMgr, SIGNAL(parameterUpdated(int, QString, QVariant)), this, SLOT(_parameterUpdated(int, QString, QVariant)));
    
    if (_paramMgr->parametersReady()) {
        _parameterListUpToDate();
    }
}

FlightModeConfig::~FlightModeConfig()
{
    
}

/// @brief Intialize the UI elements
void FlightModeConfig::_initUi(void) {
    struct Combo2Param {
        QComboBox*  combo;
        const char* param;
        const char* description;
    };
    
    struct Combo2Param rgCombo2ParamMap[] = {
        { _ui->mainSwitchChannel,   _mainSwitchParam,   "Mode Switch" },
        { _ui->manualSwitchChannel, _manualSwitchParam, "Manual Switch" },
        { _ui->assistSwitchChannel, _assistSwitchParam, "Assist Switch" },
        { _ui->returnSwitchChannel, _returnSwitchParam, "Return Switch" },
        { _ui->loiterSwitchChannel, _loiterSwitchParam, "Loiter Switch" },
        { NULL,                     "RC_MAP_THROTTLE",  "Throttle" },
        { NULL,                     "RC_MAP_YAW",       "Rudder" },
        { NULL,                     "RC_MAP_ROLL",      "Aileron" },
        { NULL,                     "RC_MAP_PITCH",     "Elevator" },
        { NULL,                     "RC_MAP_AUX1",      "Aux1" },
        { NULL,                     "RC_MAP_AUX2",      "Aux2" },
        { NULL,                     "RC_MAP_AUX3",      "Aux3" }
    };
    
    // Setup up maps
    for (size_t i=0; i<sizeof(rgCombo2ParamMap)/sizeof(rgCombo2ParamMap[0]); i++) {
        struct Combo2Param* map = &rgCombo2ParamMap[i];
        
        if (map->combo) {
            Q_ASSERT(!_mapChannelCombo2Param.contains(map->combo));
            _mapChannelCombo2Param[map->combo] = map->param;
        }
        
        Q_ASSERT(!_mapParam2FunctionInfo.contains(map->param));
        _mapParam2FunctionInfo[map->param] = map->description;
    }
    
    _updateAllChannelCombos();
}

void FlightModeConfig::_updateAllChannelCombos(void)
{
    foreach(QComboBox* combo, _mapChannelCombo2Param.keys()) {
        _updateChannelCombo(combo);
            
        // We connect to activate because we only want signals from user initiated changes
        connect(combo, SIGNAL(activated(int)), this, SLOT(_channelSelected(int)));
    }
}

void FlightModeConfig::_selectChannel(QComboBox* combo, QString parameter)
{
    QVariant value;
    
    bool found = _paramMgr->getParameterValue(_componentId, parameter, value);
    if (found) {
        int channel = value.toInt();
        Q_ASSERT(value >= 0 && value < _maxChannels );
        
        int index = combo->findData(channel);
        Q_ASSERT(index != -1);
        combo->setCurrentIndex(index);
    } else {
        // Parameter should not be missing
        Q_ASSERT(false);
    }
}

void FlightModeConfig::_parameterListUpToDate(void)
{
    if (!_parameterListUpToDateSignalled) {
        _parameterListUpToDateSignalled = true;
        _initUi();
    }
}

/// @brief Fills a channel selection combo box with channel values
void FlightModeConfig::_updateChannelCombo(QComboBox* combo)
{
    QMap<int, QString> mapChannelUsed2Description;
    
    // Make a map of all channels currently mapped
    foreach(QString param, _mapParam2FunctionInfo.keys()) {
        QVariant value;
        
        bool found = _paramMgr->getParameterValue(_componentId, param, value);
        Q_ASSERT(found);
        
        int channel = value.toInt();
        if (channel != 0) {
            mapChannelUsed2Description[channel] = _mapParam2FunctionInfo[param];
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
    
    _selectChannel(combo, _mapChannelCombo2Param[combo]);
}

/// @brief Called when a selection occurs in one of the channel combo boxes
void FlightModeConfig::_channelSelected(int requestedMapping)
{
    QComboBox* combo = qobject_cast<QComboBox*>(sender());
    Q_ASSERT(combo);
    Q_ASSERT(_mapChannelCombo2Param.contains(combo));
    
    // Check for no-op
    
    QVariant value;
    bool found = _paramMgr->getParameterValue(_componentId, _mapChannelCombo2Param[combo], value);
    Q_ASSERT(found);
    
    int currentMapping = value.toInt();
    if (currentMapping == requestedMapping) {
        return;
    }
    
    // Make sure that channel is not already mapped
    
    if (requestedMapping != 0) {
        foreach(QString param, _mapParam2FunctionInfo.keys()) {
            bool found = _paramMgr->getParameterValue(_componentId, param, value);
            Q_ASSERT(found);

            if (requestedMapping == value.toInt()) {
                QGCMessageBox::warning(tr("Flight Mode"), "You cannot use a channel which is already mapped.");
                _selectChannel(combo, _mapChannelCombo2Param[combo]);
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
        _updateAllChannelCombos();
    }
}