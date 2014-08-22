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
///     @brief PX4 RC Calibration Widget
///     @author Don Gagne <don@thegagnes.com

#include <QMessageBox>

#include "PX4RCCalibration.h"
#include "UASManager.h"

const int PX4RCCalibration::_updateInterval = 150;          ///< Interval for timer which updates radio channel widgets
const int PX4RCCalibration::_rcCalPWMValidMinValue = 1000;
const int PX4RCCalibration::_rcCalPWMValidMaxValue = 2000;
const int PX4RCCalibration::_rcCalPWMCenterPoint = ((PX4RCCalibration::_rcCalPWMValidMaxValue - PX4RCCalibration::_rcCalPWMValidMinValue) / 2.0f) + PX4RCCalibration::_rcCalPWMValidMinValue;
const int PX4RCCalibration::_rcCalRoughCenterDelta = 200;   ///< Delta around center point which is considered to be roughly centered
const float PX4RCCalibration::_rcCalMoveDelta = 300.0f;     ///< Amount of delta which is considered stick movement
const float PX4RCCalibration::_rcCalMinDelta = 100.0f;      ///< Amount of delta allowed around min value to consider channel at min

const struct PX4RCCalibration::FunctionInfo PX4RCCalibration::_rgFunctionInfo[PX4RCCalibration::rcCalFunctionMax] = {
    // Name                  Inversion Message   Parameter          required
    { "Roll / Aileron",     "Move stick left.", "RC_MAP_ROLL",      true },
    { "Pitch / Elevator",   "Move stick down.", "RC_MAP_PITCH",     true },
    { "Yaw / Rudder",       "Move stick left",  "RC_MAP_YAW",       true },
    { "Throttle",           "Move stick down",  "RC_MAP_THROTTLE",  true },
    { "Main Mode Switch",   NULL,               "RC_MAP_MODE_SW",   true },
    { "Posctl switch",      NULL,               "RC_MAP_POSCTL_SW", false },
    { "Loiter Switch",      NULL,               "RC_MAP_LOITER_SW", false },
    { "Return Switch",      NULL,               "RC_MAP_RETURN_SW", false },
    { "Flaps",              NULL,               "RC_MAP_FLAPS",     false },
    { "Aux1",               NULL,               "RC_MAP_AUX1",      false },
    { "Aux2",               NULL,               "RC_MAP_AUX2",      false },
};

PX4RCCalibration::PX4RCCalibration(QWidget *parent) :
    QWidget(parent),
    _chanCount(0),
    _rcCalState(rcCalStateChannelWait),
    _mav(NULL),
    _paramMgr(NULL),
    _parameterListUpToDateSignalled(false),
    _ui(new Ui::PX4RCCalibration)
{
    _ui->setupUi(this);
    
    // Setup up attitude control radio widgets
    _ui->rollWidget->setName(_rgFunctionInfo[rcCalFunctionRoll].functionName);
    _ui->pitchWidget->setName(_rgFunctionInfo[rcCalFunctionPitch].functionName);
    _ui->yawWidget->setName(_rgFunctionInfo[rcCalFunctionYaw].functionName);
    _ui->throttleWidget->setName(_rgFunctionInfo[rcCalFunctionThrottle].functionName);
    _ui->rollWidget->setOrientation(Qt::Horizontal);
    _ui->yawWidget->setOrientation(Qt::Horizontal);

    // Initialize arrays of widget control pointers. This allows for more efficient code writing using "for" loops.
    
    // Need to make sure standard channel indices are less then 4. Otherwise our _rgRadioWidget array won't work correctly.
    Q_ASSERT(rcCalFunctionRoll >= 0 && rcCalFunctionRoll < 4);
    Q_ASSERT(rcCalFunctionPitch >= 0 && rcCalFunctionPitch < 4);
    Q_ASSERT(rcCalFunctionYaw >= 0 && rcCalFunctionYaw < 4);
    Q_ASSERT(rcCalFunctionThrottle >= 0 && rcCalFunctionThrottle < 4);
    
    _rgAttitudeRadioWidget[rcCalFunctionRoll] = _ui->rollWidget;
    _rgAttitudeRadioWidget[rcCalFunctionPitch] = _ui->pitchWidget;
    _rgAttitudeRadioWidget[rcCalFunctionYaw] = _ui->yawWidget;
    _rgAttitudeRadioWidget[rcCalFunctionThrottle] = _ui->throttleWidget;
    
    // Testing no attitude control widgets
    _rgAttitudeRadioWidget[rcCalFunctionRoll]->setVisible(false);
    _rgAttitudeRadioWidget[rcCalFunctionPitch]->setVisible(false);
    _rgAttitudeRadioWidget[rcCalFunctionYaw]->setVisible(false);
    _rgAttitudeRadioWidget[rcCalFunctionThrottle]->setVisible(false);

    for (int chan=0; chan<_chanMax; chan++) {
        QString radioWidgetName("radio%1Widget");
        QString radioWidgetUserName("Radio %1");
        
        QGCRadioChannelDisplay* radioWidget = findChild<QGCRadioChannelDisplay*>(radioWidgetName.arg(chan+1));
        Q_ASSERT(radioWidget);
        
        radioWidget->setOrientation(Qt::Horizontal);
        radioWidget->setName(radioWidgetUserName.arg(chan+1));

        _rgRadioWidget[chan] = radioWidget;
    }
    
    _setActiveUAS(UASManager::instance()->getActiveUAS());
    
    // Connect signals
    bool fSucceeded;
    Q_UNUSED(fSucceeded);
    
    fSucceeded = connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(_setActiveUAS(UASInterface*)));
    Q_ASSERT(fSucceeded);

    fSucceeded = connect(_ui->spektrumPairButton, SIGNAL(clicked(bool)), this, SLOT(_toggleSpektrumPairing(bool)));
    Q_ASSERT(fSucceeded);
    
    _updateTimer.setInterval(150);
    _updateTimer.start();
    fSucceeded = connect(&_updateTimer, SIGNAL(timeout()), this, SLOT(_updateView()));
    Q_ASSERT(fSucceeded);

    fSucceeded= connect(_ui->rcCalCancel, SIGNAL(clicked(void)), this, SLOT(_rcCalCancel(void)));
    Q_ASSERT(fSucceeded);

    fSucceeded= connect(_ui->rcCalSkip, SIGNAL(clicked(void)), this, SLOT(_rcCalSkip(void)));
    Q_ASSERT(fSucceeded);

    fSucceeded= connect(_ui->rcCalTryAgain, SIGNAL(clicked(void)), this, SLOT(_rcCalTryAgain(void)));
    Q_ASSERT(fSucceeded);

    fSucceeded= connect(_ui->rcCalNext, SIGNAL(clicked(void)), this, SLOT(_rcCalNext(void)));
    Q_ASSERT(fSucceeded);
    
    _rcCalChannelWait(true);
}

/// @brief Resets internal calibration values to their initial state in preparation for a new calibration sequence.
void PX4RCCalibration::_resetInternalCalibrationValues(void)
{
    // Set all raw channels to not reversed and center point values
    for (size_t i=0; i<_chanMax; i++) {
        struct ChannelInfo* info = &_rgChannelInfo[i];
        info->function = rcCalFunctionMax;
        info->reversed = false;
        info->rcMin = PX4RCCalibration::_rcCalPWMCenterPoint;
        info->rcMax = PX4RCCalibration::_rcCalPWMCenterPoint;
        info->rcTrim = PX4RCCalibration::_rcCalPWMCenterPoint;
    }
    
    // Initialize function mapping to function channel not set
    for (size_t i=0; i<rcCalFunctionMax; i++) {
        _rgFunctionChannelMapping[i] = _chanMax;
    }
    
    _showMinMaxOnRadioWidgets(false);
    _showTrimOnRadioWidgets(false);
}

/// @brief Resets internal calibration values to their initial state in preparation for a new calibration sequence.
void PX4RCCalibration::_setInternalCalibrationValuesFromParameters(void)
{
    Q_ASSERT(_paramMgr);
    
    if (_parameterListUpToDateSignalled) {
        // Initialize all function mappings to not set
        
        for (size_t i=0; i<_chanMax; i++) {
            struct ChannelInfo* info = &_rgChannelInfo[i];
            info->function = rcCalFunctionMax;
        }
        
        for (size_t i=0; i<rcCalFunctionMax; i++) {
            _rgFunctionChannelMapping[i] = _chanMax;
        }
        
        // FIXME: Hardwired component id
        
        // Pull parameters and update
        
        QString minTpl("RC%1_MIN");
        QString maxTpl("RC%1_MAX");
        QString trimTpl("RC%1_TRIM");
        QString revTpl("RC%1_REV");
        QVariant value;
        bool paramFound;
        bool convertOk;
        
        for (int i = 0; i < _chanMax; ++i) {
            struct ChannelInfo* info = &_rgChannelInfo[i];
            
            paramFound = _paramMgr->getParameterValue(50, trimTpl.arg(i+1), value);
            Q_ASSERT(paramFound);
            if (paramFound) {
                info->rcTrim = value.toInt(&convertOk);
                Q_ASSERT(convertOk);
            }
            
            paramFound = _paramMgr->getParameterValue(50, minTpl.arg(i+1), value);
            Q_ASSERT(paramFound);
            if (paramFound) {
                info->rcMin = value.toInt(&convertOk);
                Q_ASSERT(convertOk);
            }

            paramFound = _paramMgr->getParameterValue(50, maxTpl.arg(i+1), value);
            Q_ASSERT(paramFound);
            if (paramFound) {
                info->rcMax = value.toInt(&convertOk);
                Q_ASSERT(convertOk);
            }

            paramFound = _paramMgr->getParameterValue(50, revTpl.arg(i+1), value);
            Q_ASSERT(paramFound);
            if (paramFound) {
                float floatReversed = value.toFloat(&convertOk);
                Q_ASSERT(convertOk);
                Q_ASSERT(floatReversed == 1.0f || floatReversed == -1.0f);
                info->reversed = floatReversed == -1.0f;
            }
        }
        
        for (int i=0; i<rcCalFunctionMax; i++) {
            int32_t paramChannel;
            
            paramFound = _paramMgr->getParameterValue(50, _rgFunctionInfo[i].parameterName, value);
            Q_ASSERT(paramFound);
            if (paramFound) {
                paramChannel = value.toInt(&convertOk);
                Q_ASSERT(convertOk);
                
                if (paramChannel != 0) {
                    _rgFunctionChannelMapping[i] = paramChannel - 1;
                    _rgChannelInfo[paramChannel - 1].function = (enum rcCalFunctions)i;
                }
            }
        }
        
        _showMinMaxOnRadioWidgets(true);
        _showTrimOnRadioWidgets(true);
    }
}

/// @brief Sets a connected Spektrum receiver into bind mode
void PX4RCCalibration::_toggleSpektrumPairing(bool enabled)
{
    Q_UNUSED(enabled);
    
    if (!_ui->dsm2RadioButton->isChecked() && !_ui->dsmxRadioButton->isChecked()
        && !_ui->dsmx8RadioButton->isChecked()) {
        // Reject
        QMessageBox warnMsgBox;
        warnMsgBox.setText(tr("Please select a Spektrum Protocol Version"));
        warnMsgBox.setInformativeText(tr("Please select either DSM2 or DSM-X\ndirectly below the pair button,\nbased on the receiver type."));
        warnMsgBox.setStandardButtons(QMessageBox::Ok);
        warnMsgBox.setDefaultButton(QMessageBox::Ok);
        (void)warnMsgBox.exec();
        return;
    }
    
    UASInterface* mav = UASManager::instance()->getActiveUAS();
    if (mav) {
        int rxSubType;
        if (_ui->dsm2RadioButton->isChecked())
            rxSubType = 0;
        else if (_ui->dsmxRadioButton->isChecked())
            rxSubType = 1;
        else // if (_ui->dsmx8RadioButton->isChecked())
            rxSubType = 2;
        mav->pairRX(0, rxSubType);
    }
}

void PX4RCCalibration::_setActiveUAS(UASInterface* active)
{
    // Disconnect old signals
    if (_mav) {
        disconnect(_mav, SIGNAL(remoteControlChannelRawChanged(int,float)), this, SLOT(_remoteControlChannelRawChanged(int,float)));
        disconnect(_paramMgr, SIGNAL(parameterListUpToDate()), this, SLOT(_parameterListUpToDate()));
        _paramMgr = NULL;
    }
    
    _mav = active;
    
    if (_mav) {
        // Connect new signals
        bool fSucceeded;
        Q_UNUSED(fSucceeded);
        fSucceeded =  connect(_mav, SIGNAL(remoteControlChannelRawChanged(int,float)), this, SLOT(_remoteControlChannelRawChanged(int,float)));
        Q_ASSERT(fSucceeded);
        
        _paramMgr = _mav->getParamManager();
        Q_ASSERT(_paramMgr);

        fSucceeded = connect(_paramMgr, SIGNAL(parameterListUpToDate()), this, SLOT(_parameterListUpToDate()));
        Q_ASSERT(fSucceeded);
    }

    setEnabled(_mav ? true : false);
}

/// @brief Saves the rc calibration values to the board parameters.
///     @param trimsOnly true: write only trim values, false: write all calibration values
void PX4RCCalibration::_writeCalibration(bool trimsOnly)
{
    if (!_mav) return;
    
    _mav->endRadioControlCalibration();
    
    QGCUASParamManagerInterface* paramMgr = _mav->getParamManager();
    Q_ASSERT(paramMgr);
    
    QString minTpl("RC%1_MIN");
    QString maxTpl("RC%1_MAX");
    QString trimTpl("RC%1_TRIM");
    QString revTpl("RC%1_REV");
    
    // Do not write the RC type, as these values depend on this
    // active onboard parameter
    
    for (int i = 0; i < _chanCount; ++i) {
        struct ChannelInfo* info = &_rgChannelInfo[i];

        paramMgr->setPendingParam(0, trimTpl.arg(i+1), info->rcTrim);
        if (!trimsOnly) {
            paramMgr->setPendingParam(0, minTpl.arg(i+1), info->rcMin);
            paramMgr->setPendingParam(0, maxTpl.arg(i+1), info->rcMax);
            paramMgr->setPendingParam(0, revTpl.arg(i+1), info->reversed ? -1.0f : 1.0f);
        }
    }
    
    if (!trimsOnly) {
        // Write function mapping parameters
        for (size_t i=0; i<rcCalFunctionMax; i++) {
            int32_t paramChannel;
            if (_rgFunctionChannelMapping[i] == _chanMax) {
                // 0 signals no mapping
                paramChannel = 0;
            } else {
                // Note that the channel value is 1-based
                paramChannel = _rgFunctionChannelMapping[i] + 1;
            }
            paramMgr->setPendingParam(0, _rgFunctionInfo[i].parameterName, paramChannel);
        }
    }
    
    //let the param mgr manage sending all the pending RC_foo updates and persisting after
    paramMgr->sendPendingParameters(true, true);
}

/// @brief This routine is called whenever a raw value for an RC channel changes. Depending on the current
/// calibration state, it will update internal values and ui accordingly.
///     @param chan RC channel on which signal is coming from (0-based)
///     @param fval Current value for channel
void PX4RCCalibration::_remoteControlChannelRawChanged(int chan, float fval)
{
    Q_ASSERT(chan >=0 && chan <= _chanMax);

    // We always update raw values
    _rcRawValue[chan] = fval;
    
    // Update state machine
    switch (_rcCalState) {
        case rcCalStateChannelWait:
            // While we are waiting detect the minimum number of RC channels
            if (chan + 1 > (int)_chanCount) {
                _chanCount = chan + 1;
                if (_chanCount >= _chanMinimum) {
                    _ui->rcCalNext->setEnabled(true);
                    _ui->rcCalStatus->setText(tr("Detected %1 radio channels.").arg(_chanCount));
                } else if (_chanCount < _chanMinimum) {
                    _ui->rcCalStatus->setText(tr("Detected %1 radio channels. To operate PX4, you need at least %2 channels.").arg(_chanCount).arg(_chanMinimum));
                }
            }
            
            // Capture raw values so channel widgets are live
            _rcRawValue[chan] = fval;
            break;
            
        case rcCalStateIdentify:
            if (!_rcCalStateChannelComplete) {
                // If this channel is already used in a mapping we can't used it again
                bool channelAlreadyMapped = false;
                for (int chanFunction=0; chanFunction<rcCalFunctionMax; chanFunction++) {
                    if (_rgFunctionChannelMapping[chanFunction] == chan) {
                        channelAlreadyMapped = true;
                        break;
                    }
                }
                
                // If the channel moved considerably, pick it
                if (!channelAlreadyMapped && fabsf(_rcValueSave[chan] - fval) > _rcCalMoveDelta) {
                    Q_ASSERT(_rcCalStateCurrentChannel >= 0 && _rcCalStateCurrentChannel < rcCalFunctionMax);
                    _rgFunctionChannelMapping[_rcCalStateCurrentChannel] = chan;
                    _rgChannelInfo[chan].function = (enum rcCalFunctions)_rcCalStateCurrentChannel;
                    _updateView();
                    
                    // Confirm found channel
                    QString msg = tr("Found %1 [Channel %2]").arg(_rgFunctionInfo[_rcCalStateCurrentChannel].functionName).arg(chan);
                    _ui->rcCalFound->setText(msg);
                    //qDebug() << msg;
                    _ui->rcCalTryAgain->setEnabled(true);
                    _ui->rcCalNext->setEnabled(true);
                    _ui->rcCalSkip->setEnabled(false);
                    _rcCalStateChannelComplete =true;
                }
            }
            break;
            
        case rcCalStateMinMax:
            if (fval < _rgChannelInfo[chan].rcMin) {
                _rgChannelInfo[chan].rcMin = fval;
            }
            if (fval > _rgChannelInfo[chan].rcMax) {
                _rgChannelInfo[chan].rcMax = fval;
            }
            break;
            
        case rcCalStateCenterThrottle:
            // If the throttle is roughly centered, enable the Next button
            Q_ASSERT(_rgFunctionChannelMapping[rcCalFunctionThrottle] != _chanMax);
            if (chan == _rgFunctionChannelMapping[rcCalFunctionThrottle] &&
                 fabsf(fval - _rcCalPWMCenterPoint) < _rcCalRoughCenterDelta) {
                _ui->rcCalNext->setEnabled(true);
            }
            break;
        
        case rcCalStateDetectInversion:
            if (!_rcCalStateChannelComplete) {
                // We only care about the channel we are looking for
                Q_ASSERT(_rcCalStateCurrentChannel >= 0 && _rcCalStateCurrentChannel < rcCalFunctionMax);
                if (chan == _rgFunctionChannelMapping[_rcCalStateCurrentChannel]) {
                    // If the channel moved considerably use it to determine inversion
                    if (fabsf(_rcValueSave[chan] - fval) > _rcCalMoveDelta) {
                        // Request was made to move channel to a lower value. If value goes up the channel is reversed.
                        bool reversed = fval > _rcValueSave[chan];
                        
                        _rgChannelInfo[_rcCalStateCurrentChannel].reversed = reversed;
                        _updateView();
                        
                        // Confirm inversion detection
                        QString msg = tr("Channel for %1 ").arg(_rgFunctionInfo[_rcCalStateCurrentChannel].functionName);
                        if (reversed) {
                            msg += tr("is reversed.");
                        } else {
                            msg += tr("is not reversed.");
                        }
                        _ui->rcCalFound->setText(msg);
                        //qDebug() << msg;
                        _ui->rcCalTryAgain->setEnabled(true);
                        _ui->rcCalNext->setEnabled(true);
                        _ui->rcCalSkip->setEnabled(false);
                        _rcCalStateChannelComplete =true;
                    }
                }
            }
            break;
            
        case rcCalStateTrims:
            // Update the trim values for attitude functions only
            if (_rgChannelInfo[chan].function >= rcCalFunctionFirstAttitudeFunction && _rgChannelInfo[chan].function <= rcCalFunctionLastAttitudeFunction) {
                int mappedChannel = _rgFunctionChannelMapping[_rgChannelInfo[chan].function];
                
                // All Attitude Functions should be mapped
                Q_ASSERT(mappedChannel != rcCalFunctionMax);
                
                _rgChannelInfo[mappedChannel].rcTrim = _rcRawValue[mappedChannel];
            }
            
            // Once the throttle is lowered we enable the next button
            Q_ASSERT(_rgFunctionChannelMapping[rcCalFunctionThrottle] != rcCalFunctionMax);
            if (chan == _rgFunctionChannelMapping[rcCalFunctionThrottle]) {
                bool enableNext = false;
                
                // If the value is close enough to min consider the throttle to be lowered (taking into account reversing)
                if ((_rgChannelInfo[chan].reversed && fabsf(_rgChannelInfo[chan].rcMax - fval) < _rcCalMinDelta) ||
                    fabsf(_rgChannelInfo[chan].rcMin - fval) < _rcCalMinDelta) {
                    enableNext = true;
                }
                _ui->rcCalNext->setEnabled(enableNext);
            }
            break;
    
        default:
            // Nothing special required for state
            break;
    }
}

void PX4RCCalibration::_updateView()
{
    // Update Attitude Function channel widgets, disbaled if unmapped
    for (int i=rcCalFunctionFirstAttitudeFunction; i<=rcCalFunctionLastAttitudeFunction; i++) {
        int mappedChannel = _rgFunctionChannelMapping[i];
        if (mappedChannel != _chanMax) {
            struct ChannelInfo* info = &_rgChannelInfo[mappedChannel];
            _rgAttitudeRadioWidget[i]->setEnabled(true);
            _rgAttitudeRadioWidget[i]->setValueAndRange(_rcRawValue[mappedChannel], info->rcMin, info->rcMax);
            _rgAttitudeRadioWidget[i]->setTrim(info->rcTrim);
        } else {
            _rgAttitudeRadioWidget[i]->setEnabled(false);
        }
    }
    
    // Update the available channels
    for (int chan=0; chan<_chanCount; chan++) {
        _rgRadioWidget[chan]->setEnabled(true);
        
        struct ChannelInfo* info = &_rgChannelInfo[chan];
        _rgRadioWidget[chan]->setValueAndRange(_rcRawValue[chan], info->rcMin, info->rcMax);
        _rgRadioWidget[chan]->setTrim(info->rcTrim);
    }
    
    // Disable non-available channels
    for (int chan=_chanCount; chan<_chanMax; chan++) {
        _rgRadioWidget[chan]->setEnabled(false);
    }
    
    // FIXME: Could save some CPU by not doing this on every update
    // Update the channel names for all channels
    for (int chan=0; chan<_chanMax; chan++) {
        struct ChannelInfo* info = &_rgChannelInfo[chan];
        
        QString name;
        int oneBasedChannel = chan+1;
        if (info->function == rcCalFunctionMax) {
            name = tr("Channel %1").arg(oneBasedChannel);
        } else {
            name = tr("%1 [Channel %2]").arg(_rgFunctionInfo[info->function].functionName).arg(oneBasedChannel);
        }
        _rgRadioWidget[chan]->setName(name);
    }
}

/// @brief Cancels the current calibration process and returns to the Channel Wait state.
void PX4RCCalibration::_rcCalCancel(void)
{
    _mav->endRadioControlCalibration();
    _rcCalChannelWait(true);
    _setInternalCalibrationValuesFromParameters();
}

void PX4RCCalibration::_rcCalSkip(void)
{
    // Skip is only allowed for optional function mappings
    Q_ASSERT(_rcCalState ==rcCalStateIdentify);
    
    // This will move us to the next function mapping
    _rcCalNextIdentifyChannelMapping();
}

/// @brief Resets the state machine such that you can retry an identify or inversion detection on a specific
// function.
void PX4RCCalibration::_rcCalTryAgain(void)
{
    // FIXME: NYI for all states
    QMessageBox::information(this, "Not Yet Implemented", "Try Again has not yet been implemented.");
}

/// @brief Called when the Next button is called from the RC Calibration tab. This will either start the calibration process
/// or move it on to the next step.
void PX4RCCalibration::_rcCalNext(void)
{
    switch (_rcCalState) {
        case rcCalStateChannelWait:
            _rcCalBegin();
            break;
            
        case rcCalStateBegin:
            _rcCalStateCurrentChannel = -1;    // _rcCalNextIdentifyChannelMapping will bump up to 0 to start sequence
            _rcCalNextIdentifyChannelMapping();
            break;
            
        case rcCalStateIdentify:
            _rcCalNextIdentifyChannelMapping();
            break;
            
        case rcCalStateMinMax:
            _updateView();
            _rcCalCenterThrottle();
            break;
            
        case rcCalStateCenterThrottle:
            // Setup for inversion detection channel
            _rcCalStateCurrentChannel = rcCalFunctionFirstAttitudeFunction - 1; // _rcCalNextDetectChannelInversion will ++ to start sequence
            _rcCalNextDetectChannelInversion();
            break;
            
        case rcCalStateDetectInversion:
            _rcCalNextDetectChannelInversion();
            break;
            
        case rcCalStateTrims:
            _rcCalSave();
            break;
            
        case rcCalStateSave:
            _writeCalibration(false /* !trimsOnly */);
            _rcCalChannelWait(false);
            break;
            
        default:
            Q_ASSERT(false);
            break;
    }
}

/// @brief Setup for the Channel Wait state of calibration.
///     @param firstTime true: this is the first time a calibration is being performed since this widget was created
void PX4RCCalibration::_rcCalChannelWait(bool firstTime)
{
    _rcCalState = rcCalStateChannelWait;
    
    _resetInternalCalibrationValues();

    if (_chanCount == 0) {
        _ui->rcCalFound->setText(tr("Please turn on Radio"));
        _ui->rcCalNext->setEnabled(false);
    } else {
        if (_chanCount >= _chanMinimum) {
            _ui->rcCalNext->setEnabled(true);
            _ui->rcCalStatus->setText(tr("Detected %1 radio channels.").arg(_chanCount));
        } else if (_chanCount < _chanMinimum) {
            _ui->rcCalNext->setEnabled(false);
            _ui->rcCalStatus->setText(tr("Detected %1 radio channels. To operate PX4, you need at least %2 channels.").arg(_chanCount).arg(_chanMinimum));
        }
    }
    
    if (firstTime) {
        _ui->rcCalFound->clear();
    } else {
        _ui->rcCalFound->setText(tr("Calibration complete"));
    }
    
    _ui->rcCalNext->setText(tr("Start"));
    _ui->rcCalCancel->setEnabled(false);
    _ui->rcCalSkip->setEnabled(false);
    _ui->rcCalTryAgain->setEnabled(false);
}

/// @brief Set up for the Begin state of calibration.
void PX4RCCalibration::_rcCalBegin(void)
{
    Q_ASSERT(_chanCount >= _chanMinimum);
    
    _rcCalState = rcCalStateBegin;
    
    _resetInternalCalibrationValues();
    
    // Let the mav known we are starting calibration. This should turn off motors and so forth.
    // FIXME: XXX magic number: Set to 1 for radio input disable
    _mav->startRadioControlCalibration(1);
    
    _ui->rcCalNext->setText(tr("Next"));
    _ui->rcCalCancel->setEnabled(true);
    _ui->rcCalStatus->setText(tr("Starting RC calibration.\n\n"
                                "Ensure RC transmitter and receiver are powered and connected. It is recommended to disconnect all motors for additional safety, however, the system is designed to not arm during the calibration.\n\n"
                                "Reset all transmitter trims to center, then click Next to continue"));
    _ui->rcCalFound->clear();
}

/// @brief Saves the current channel values, so that we can detect when the use moves an input.
void PX4RCCalibration::_rcCalSaveCurrentValues(void)
{
    for (unsigned i = 0; i < _chanMax; i++) {
        _rcValueSave[i] = _rcRawValue[i];
    }
}

/// @brief Set up for the Identify state of calibration which assigns channels to control functions.
void PX4RCCalibration::_rcCalNextIdentifyChannelMapping(void)
{
    // Move to next channel
    _rcCalStateCurrentChannel++;
    Q_ASSERT(_rcCalStateCurrentChannel >= 0 && _rcCalStateCurrentChannel <= rcCalFunctionMax);
    
    // Be careful not to switch the state until we have a valid value in _rcCalStateCurrentChannel. Otherwise an rc signal could come through
    // and cause _remoteControlChannelRawChanged to get confused.
    _rcCalState = rcCalStateIdentify;
    _rcCalStateChannelComplete = false;
    
    if (_rcCalStateCurrentChannel == rcCalFunctionMax) {
        // If we have processed all channels move to next calibration step
        _rcCalReadChannelsMinMax();
        return;
    }
    
    // Save the current mapping, so we can reset if user decides to skip
    _rcCalStateIdentifyOldMapping = _rgFunctionChannelMapping[_rcCalStateCurrentChannel];
    
    // Save the current channel values so we can detect movement
    _rcCalSaveCurrentValues();
    
    _ui->rcCalStatus->setText(tr("Detecting %1 ...").arg(_rgFunctionInfo[_rcCalStateCurrentChannel].functionName));
    _ui->rcCalFound->setText(tr("Please move stick, switch or potentiometer for this channel all the way up/down or left/right."));
    
    _ui->rcCalNext->setEnabled(false);
    _ui->rcCalTryAgain->setEnabled(false);
    _ui->rcCalSkip->setEnabled(!_rgFunctionInfo[_rcCalStateCurrentChannel].required);
    _ui->rcCalCancel->setEnabled(true);
}

/// @brief Sets up for the Min/Max state of calibration.
void PX4RCCalibration::_rcCalReadChannelsMinMax(void)
{
    _rcCalState = rcCalStateMinMax;
    
    _ui->rcCalStatus->setText(tr("Please move the sticks to their extreme positions, including all switches. Click Next when complete."));
    _ui->rcCalFound->clear();
    
    _ui->rcCalNext->setEnabled(true);
    _ui->rcCalTryAgain->setEnabled(false);
    _ui->rcCalSkip->setEnabled(false);
    _ui->rcCalCancel->setEnabled(true);
    
    _showMinMaxOnRadioWidgets(true);
}

/// @brief Sets up for the Center Throttle state of Calibration which is required prior to detecting channel inversions.
void PX4RCCalibration::_rcCalCenterThrottle(void)
{
    _rcCalState = rcCalStateCenterThrottle;
    
    _ui->rcCalStatus->setText(tr("Next we will be determining which channels need to be reversed.\n\n"
                                "Please center the throttle stick prior to that. The stick should be roughly centered - the exact position is not relevant.\n"
                                "Once centered, leave it there until asked to move it.\n\n"
                                "Click the Next button when done. Next button will only enable when throttle is centered."));
    _ui->rcCalFound->clear();

    _ui->rcCalNext->setEnabled(false);
    _ui->rcCalTryAgain->setEnabled(false);
    _ui->rcCalSkip->setEnabled(false);
    _ui->rcCalCancel->setEnabled(true);
}

/// @brief Set up the Detect Channel Inversion state of calibration.
void PX4RCCalibration::_rcCalNextDetectChannelInversion(void)
{    
    // Move to next channel. We only detect inversion on Attitude control functions.
    _rcCalStateCurrentChannel++;
    Q_ASSERT(_rcCalStateCurrentChannel >= rcCalFunctionFirstAttitudeFunction && _rcCalStateCurrentChannel <= rcCalFunctionLastAttitudeFunction + 1);
    if (_rcCalStateCurrentChannel > rcCalFunctionLastAttitudeFunction) {
        // If we have processed all functions move to next calibration step
        _rcCalTrims();
        return;
    }
    
    // Be careful not to switch the state until we have a valid value in _rcCalStateCurrentChannel. Otherwise an rc signal could come through
    // and cause _remoteControlChannelRawChanged to get confused.
    _rcCalState = rcCalStateDetectInversion;
    _rcCalStateChannelComplete = false;

    
    // Save the current channel values so we can detect movement
    _rcCalSaveCurrentValues();
    
    const struct FunctionInfo* info = &_rgFunctionInfo[_rcCalStateCurrentChannel];
    
    _ui->rcCalStatus->setText(tr("Detecting reversed channels: %1 ...").arg(info->functionName));
    _ui->rcCalFound->setText(info->inversionMsg);

    _ui->rcCalNext->setEnabled(false);
    _ui->rcCalTryAgain->setEnabled(false);
    _ui->rcCalSkip->setEnabled(false);
    _ui->rcCalCancel->setEnabled(true);
}

/// @brief Set up the Trims state of calibration.
void PX4RCCalibration::_rcCalTrims(void)
{
    _rcCalState = rcCalStateTrims;
    
    _ui->rcCalStatus->setText(tr("Next we will be determining Trim values for the two attitude control sticks:\n"
                                "Please set the Throttle stick to the lowest throttle position and leave it there.\n"
                                "Click the Next button to save Trims. Next button will only enable when throttle is lowered."));
    _ui->rcCalFound->clear();

    _ui->rcCalNext->setEnabled(false);
    _ui->rcCalTryAgain->setEnabled(false);
    _ui->rcCalSkip->setEnabled(false);
    _ui->rcCalCancel->setEnabled(true);
    
    _initializeTrims();
    _showTrimOnRadioWidgets(true);
}

/// @brief Initializes the trim values based on current min/max.
void PX4RCCalibration::_initializeTrims(void)
{
    for (int chan=0; chan<=_chanMax; chan++) {
        _rgChannelInfo[chan].rcTrim = ((_rgChannelInfo[chan].rcMax - _rgChannelInfo[chan].rcMin) / 2.0f) + _rgChannelInfo[chan].rcMin;
    }
}

/// @brief Set up the Save state of calibration.
void PX4RCCalibration::_rcCalSave(void)
{
    _rcCalState = rcCalStateSave;
    
    _ui->rcCalStatus->setText(tr("The current calibration settings are now displayed for each channel on screen.\n\n"
                                "Click the Next button to upload calibration to board. Click Cancel if you don't want to save these values."));
    _ui->rcCalFound->clear();

    _ui->rcCalNext->setEnabled(true);
    _ui->rcCalTryAgain->setEnabled(false);
    _ui->rcCalSkip->setEnabled(false);
    _ui->rcCalCancel->setEnabled(true);
}

/// @brief This is used by unit test code to force the calibration state machine to the specified state.
/// With this you can write individual unit tests for each calibration state. Should only be called by
/// unit test code.
void PX4RCCalibration::_unitTestForceCalState(enum rcCalStates state) {

    switch (state) {
        case rcCalStateBegin:
            _rcCalBegin();
            break;
            
        case rcCalStateIdentify:
            _rcCalStateCurrentChannel = -1;    // _rcCalNextIdentifyChannelMapping will bump up to 0 to start sequence
            _rcCalNextIdentifyChannelMapping();
            break;
            
        case rcCalStateMinMax:
            _rcCalReadChannelsMinMax();
            break;
            
        case rcCalStateCenterThrottle:
            _rcCalCenterThrottle();
            break;
            
        case rcCalStateDetectInversion:
            _rcCalStateCurrentChannel = -1;    // _rcCalNextDetectChannelInversion will bump up to 0 to start sequence
            _rcCalNextDetectChannelInversion();
            break;
            
        case rcCalStateTrims:
            _rcCalTrims();
            break;
            
        default:
            // Unsupported force state
            Q_ASSERT(false);
            break;
    }
}

/// @brief Shows or hides the min/max values of the channel widgets.
///     @param show true: show the min/max values, false: hide the min/max values
void PX4RCCalibration::_showMinMaxOnRadioWidgets(bool show)
{
    // Turn on min/max display for all radio widgets
    Q_ASSERT(rcCalFunctionFirstAttitudeFunction == 0);
    for (int i=0; i<=rcCalFunctionLastAttitudeFunction; i++) {
        QGCRadioChannelDisplay* radioWidget = _rgAttitudeRadioWidget[i];
        Q_ASSERT(radioWidget);
        
        radioWidget->showMinMax(show);
    }
    
    for (int i=0; i<_chanMax; i++) {
        QGCRadioChannelDisplay* radioWidget = _rgRadioWidget[i];
        Q_ASSERT(radioWidget);
        
        radioWidget->showMinMax(show);
    }
}

/// @brief Shows or hides the trim values of the channel widgets.
///     @param show true: show the trim values, false: hide the trim values
void PX4RCCalibration::_showTrimOnRadioWidgets(bool show)
{
    // Turn on trim display for all radio widgets
    Q_ASSERT(rcCalFunctionFirstAttitudeFunction == 0);
    for (int i=0; i<=rcCalFunctionLastAttitudeFunction; i++) {
        QGCRadioChannelDisplay* radioWidget = _rgAttitudeRadioWidget[i];
        Q_ASSERT(radioWidget);
        
        radioWidget->showTrim(show);
    }
    
    for (int i=0; i<_chanMax; i++) {
        QGCRadioChannelDisplay* radioWidget = _rgRadioWidget[i];
        Q_ASSERT(radioWidget);
        
        radioWidget->showTrim(show);
    }
}

void PX4RCCalibration::_parameterListUpToDate(void)
{
    _parameterListUpToDateSignalled = true;
    
    if (_rcCalState == rcCalStateChannelWait) {
        _setInternalCalibrationValuesFromParameters();
    }
}
