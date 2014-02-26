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

#include "FlightModeConfig.h"

// We used the _rgModeInfo* arrays to populate the combo box choices. The numeric value
// is the flight mode value that corresponds to the label. We store that value in the
// combo box item data. There is a different set or each vehicle type.

const FlightModeConfig::FlightModeInfo_t FlightModeConfig::_rgModeInfoFixedWing[] = {
    { "Manual",     0 },
    { "Circle",     1 },
    { "Stabilize",  2 },
    { "Training",   3 },
    { "FBW A",      5 },
    { "FBW B",      6 },
    { "Auto",       10 },
    { "RTL",        11 },
    { "Loiter",     12 },
    { "Guided",     15 },
};

const FlightModeConfig::FlightModeInfo_t FlightModeConfig::_rgModeInfoRotor[] = {
    { "Stabilize",  0 },
    { "Acro",       1 },
    { "Alt Hold",   2 },
    { "Auto",       3 },
    { "Guided",     4 },
    { "Loiter",     5 },
    { "RTL",        6 },
    { "Circle",     7 },
    { "Position",   8 },
    { "Land",       9 },
    { "OF_Loiter",  10 },
    { "Toy A",      11 },
    { "Toy M",      12 },
};

const FlightModeConfig::FlightModeInfo_t FlightModeConfig::_rgModeInfoRover[] = {
    { "Manual",     0 },
    { "Learning",   2 },
    { "Steering",   3 },
    { "Hold",       4 },
    { "Auto",       10 },
    { "RTL",        11 },
    { "Guided",     15 },
};

// We use the _rgModeParam* arrays to store the parameter names for each selectable
// flight mode. The order of these corresponds to the combox box order as well. Array
// element 0, is the parameter for mode0ComboBox, array element 1 = mode1ComboBox and
// so on. The number of elements in the array also determines how many combo boxes we
// are going to need. Different vehicle types have different numbers of selectable
// flight modes.

const char* FlightModeConfig::_rgModeParamFixedWing[_cModes] = { "FLTMODE1", "FLTMODE2", "FLTMODE3", "FLTMODE4", "FLTMODE5", "FLTMODE6" };

const char* FlightModeConfig::_rgModeParamRotor[_cModes] = { "FLTMODE1", "FLTMODE2", "FLTMODE3", "FLTMODE4", "FLTMODE5", "FLTMODE6" };

const char* FlightModeConfig::_rgModeParamRover[_cModes] = { "MODE1", "MODE2", "MODE3", "MODE4", "MODE5", "MODE6" };

// Parameter which contains simple mode bitmask
const char* FlightModeConfig::_simpleModeBitMaskParam = "SIMPLE";

// Parameter which controls which RC channel mode switching is on.
// ArduCopter is hardcoded so no param
const char* FlightModeConfig::_modeSwitchRCChannelParamFixedWing = "FLTMODE_CH";
const char* FlightModeConfig::_modeSwitchRCChannelParamRover = "MODE_CH";

// PWM values which are the boundaries between each mode selection
const int FlightModeConfig::_rgModePWMBoundary[_cModes] = { 1230, 1360, 1490, 1620, 1749, 20000 };

FlightModeConfig::FlightModeConfig(QWidget *parent) :
    AP2ConfigWidget(parent),
    _rgModeInfo(NULL),
    _cModeInfo(0),
    _rgModeParam(NULL),
    _simpleModeSupported(false),
    _modeSwitchRCChannel(_modeSwitchRCChannelInvalid)
{
    _ui.setupUi(this);
    
    // Find all the widgets we are going to programmatically control
    for (size_t i=0; i<_cModes; i++) {
        _rgLabel[i] = findChild<QLabel*>(QString("mode%1Label").arg(i));
        _rgCombo[i] = findChild<QComboBox*>(QString("mode%1ComboBox").arg(i));
        _rgSimpleModeCheckBox[i] = findChild<QCheckBox*>(QString("mode%1SimpleCheckBox").arg(i));
        _rgPWMLabel[i] = findChild<QLabel*>(QString("mode%1PWMLabel").arg(i));
        Q_ASSERT(_rgLabel[i]);
        Q_ASSERT(_rgCombo[i]);
        Q_ASSERT(_rgSimpleModeCheckBox[i]);
        Q_ASSERT(_rgPWMLabel[i]);
    }
    
    // Start disabled until we get a UAS
    setEnabled(false);

    connect(_ui.savePushButton, SIGNAL(clicked()), this, SLOT(saveButtonClicked()));
    initConnections();
}

void FlightModeConfig::activeUASSet(UASInterface *uas)
{
    // Clear the highlighting on PWM labels
    for (size_t i=0; i<_cModes; i++) {
        _rgPWMLabel[i]->setStyleSheet(QString(""));
    }

    // Disconnect from old UAS
    if (m_uas)
    {
        disconnect(m_uas, SIGNAL(remoteControlChannelRawChanged(int, float)), this, SLOT(remoteControlChannelRawChanged(int, float)));
    }
    
    // Connect to new UAS (if any)
    AP2ConfigWidget::activeUASSet(uas);
    if (!uas) {
        setEnabled(false);
        return;
    }
    connect(m_uas, SIGNAL(remoteControlChannelRawChanged(int, float)), this, SLOT(remoteControlChannelRawChanged(int, float)));
    setEnabled(true);
    
    // Select the correct set of Flight Modes and Flight Mode Parameters. If the rc channel
    // associated with the mode switch is settable through a param, it is invalid until
    // we get that param through.
    switch (m_uas->getSystemType()) {
        case MAV_TYPE_FIXED_WING:
            _rgModeInfo = &_rgModeInfoFixedWing[0];
            _cModeInfo = sizeof(_rgModeInfoFixedWing)/sizeof(_rgModeInfoFixedWing[0]);
            _rgModeParam = _rgModeParamFixedWing;
            _simpleModeSupported = false;
            _modeSwitchRCChannelParam = _modeSwitchRCChannelParamFixedWing;
            _modeSwitchRCChannel = _modeSwitchRCChannelInvalid;
            break;
        case MAV_TYPE_GROUND_ROVER:
            _rgModeInfo = &_rgModeInfoRover[0];
            _cModeInfo = sizeof(_rgModeInfoRover)/sizeof(_rgModeInfoRover[0]);
            _rgModeParam = _rgModeParamRover;
            _simpleModeSupported = false;
            _modeSwitchRCChannelParam = _modeSwitchRCChannelParamRover;
            _modeSwitchRCChannel = _modeSwitchRCChannelInvalid;
            break;
        case MAV_TYPE_QUADROTOR:
        case MAV_TYPE_TRICOPTER:
        case MAV_TYPE_HEXAROTOR:
        case MAV_TYPE_OCTOROTOR:
        case MAV_TYPE_HELICOPTER:
            _rgModeInfo = &_rgModeInfoRotor[0];
            _cModeInfo = sizeof(_rgModeInfoRotor)/sizeof(_rgModeInfoRotor[0]);
            _rgModeParam = _rgModeParamRotor;
            _simpleModeSupported = true;
            _modeSwitchRCChannelParam = NULL;
            _modeSwitchRCChannel = _modeSwitchRCChannelRotor; // Rotor is harcoded
            break;
        default:
            // We've gotten a mav type we can't handle, just disable to whole thing
            qDebug() << QString("Unknown System Type %1").arg(m_uas->getSystemType());
            setEnabled(false);
            return;
    }

    // Set up the combo boxes
    for (size_t i=0; i<_cModes; i++) {
        // Fill each combo box with the available flight modes
        for (size_t j=0; j<_cModeInfo; j++) {
            _rgCombo[i]->addItem(_rgModeInfo[j].label, QVariant(QChar((char)_rgModeInfo[j].value)));
        }
        
        // Not all vehicle types support simple mode, hide/show as appropriate
        _rgSimpleModeCheckBox[i]->setEnabled(_simpleModeSupported);
    }
}

void FlightModeConfig::saveButtonClicked(void)
{
    // Save the current setting for each flight mode slot
    for (size_t i=0; i<_cModes; i++) {
        m_uas->getParamManager()->setParameter(1, _rgModeParam[i], _rgCombo[i]->itemData(_rgCombo[i]->currentIndex()));
        QVariant var =_rgCombo[i]->itemData(_rgCombo[i]->currentIndex());
    }
    
    // Save Simple Mode bit mask if supported
    if (_simpleModeSupported) {
        int bitMask = 0;
        for (size_t i=0; i<_cModes; i++) {
            if (_rgSimpleModeCheckBox[i]->isChecked()) {
                bitMask |= 1 << i;
            }
        }
        m_uas->getParamManager()->setParameter(1, _simpleModeBitMaskParam, QVariant(QChar(bitMask)));
    }
}

// Higlights the PWM label for currently selected mode according the mode switch
// rc channel value.
void FlightModeConfig::remoteControlChannelRawChanged(int chan, float val)
{
    // Until we get the _modeSwitchRCChannel value from a parameter it will be set
    // to -1, which is an invalid channel thus the labels won't update
    if (chan == _modeSwitchRCChannel)
    {
        size_t highlightIndex = _cModes; // initialize to unreachable index
        
        for (size_t i=0; i<_cModes; i++) {
            if (val < _rgModePWMBoundary[i]) {
                highlightIndex = i;
                break;
            }
        }
        
        for (size_t i=0; i<_cModes; i++) {
            QString styleSheet;
            if (i == highlightIndex) {
                styleSheet = "background-color: rgb(0, 255, 0);color: rgb(0, 0, 0);";
            } else {
                styleSheet = "";
            }
            _rgPWMLabel[i]->setStyleSheet(styleSheet);
        }
    }
}

void FlightModeConfig::parameterChanged(int uas, int component, QString parameterName, QVariant value)
{
    Q_UNUSED(uas);
    Q_UNUSED(component);
    
    int iValue = value.toInt();
    
    if (parameterName == _modeSwitchRCChannelParam) {
        _modeSwitchRCChannel = iValue - 1;  // 1-based in params
    } else if (parameterName == _simpleModeBitMaskParam) {
        if (_simpleModeSupported) {
            for (size_t i=0; i<_cModes; i++) {
                _rgSimpleModeCheckBox[i]->setCheckState((iValue & (1 << i)) ? Qt::Checked : Qt::Unchecked);
            }
        } else {
            qDebug() << "Received simple mode parameter on non simple mode vehicle type";
        }
    } else {
        // Loop over the flight mode params looking for a match
        for (size_t i=0; i<_cModes; i++) {
            if (parameterName == _rgModeParam[i]) {
                // We found a match, i is now the index of the combo box which displays that mode slot
                // Loop over the mode info till we find the matching value, this tells us which row in the
                // combo box to select.
                QComboBox* combo = _rgCombo[i];
                for (size_t j=0; j<_cModeInfo; j++) {
                    if (_rgModeInfo[j].value == iValue) {
                        combo->setCurrentIndex(j);
                        return;
                    }
                }
                
                // We should have been able to find the flight mode value. If we didn't, we have been passed a
                // flight mode that we don't understand. Possibly a newer firmware version we are not yet up
                // to date with. In this case, add a new item to the combo box to represent it.
                qDebug() << QString("Unknown flight mode value %1").arg(iValue);
                combo->addItem(QString("%1 - Unknown").arg(iValue), QVariant(iValue));
                combo->setCurrentIndex(combo->count() - 1);
            }
        }
    }
}
