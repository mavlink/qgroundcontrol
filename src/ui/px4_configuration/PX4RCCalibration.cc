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

#include <QSettings>

#include "PX4RCCalibration.h"
#include "UASManager.h"
#include "QGCMessageBox.h"
#include "AutoPilotPluginManager.h"

QGC_LOGGING_CATEGORY(PX4RCCalibrationLog, "PX4RCCalibrationLog")

const int PX4RCCalibration::_updateInterval = 150;              ///< Interval for timer which updates radio channel widgets
const int PX4RCCalibration::_rcCalPWMCenterPoint = ((PX4RCCalibration::_rcCalPWMValidMaxValue - PX4RCCalibration::_rcCalPWMValidMinValue) / 2.0f) + PX4RCCalibration::_rcCalPWMValidMinValue;
// FIXME: Double check these mins againt 150% throws
const int PX4RCCalibration::_rcCalPWMValidMinValue = 1300;      ///< Largest valid minimum PWM Min range value
const int PX4RCCalibration::_rcCalPWMValidMaxValue = 1700;      ///< Smallest valid maximum PWM Max range value
const int PX4RCCalibration::_rcCalPWMDefaultMinValue = 1000;    ///< Default value for Min if not set
const int PX4RCCalibration::_rcCalPWMDefaultMaxValue = 2000;    ///< Default value for Max if not set
const int PX4RCCalibration::_rcCalRoughCenterDelta = 50;        ///< Delta around center point which is considered to be roughly centered
const int PX4RCCalibration::_rcCalMoveDelta = 300;            ///< Amount of delta past center which is considered stick movement
const int PX4RCCalibration::_rcCalSettleDelta = 20;           ///< Amount of delta which is considered no stick movement
const int PX4RCCalibration::_rcCalMinDelta = 100;             ///< Amount of delta allowed around min value to consider channel at min

const int PX4RCCalibration::_stickDetectSettleMSecs = 500;

const char*  PX4RCCalibration::_imageFilePrefix = ":/res/calibration/";
const char*  PX4RCCalibration::_imageFileMode1Dir = "mode1/";
const char*  PX4RCCalibration::_imageFileMode2Dir = "mode2/";
const char*  PX4RCCalibration::_imageCenter = "radioCenter.png";
const char*  PX4RCCalibration::_imageHome = "radioHome.png";
const char*  PX4RCCalibration::_imageThrottleUp = "radioThrottleUp.png";
const char*  PX4RCCalibration::_imageThrottleDown = "radioThrottleDown.png";
const char*  PX4RCCalibration::_imageYawLeft = "radioYawLeft.png";
const char*  PX4RCCalibration::_imageYawRight = "radioYawRight";
const char*  PX4RCCalibration::_imageRollLeft = "radioRollLeft.png";
const char*  PX4RCCalibration::_imageRollRight = "radioRollRight.png";
const char*  PX4RCCalibration::_imagePitchUp = "radioPitchUp";
const char*  PX4RCCalibration::_imagePitchDown = "radioPitchDown";
const char*  PX4RCCalibration::_imageSwitchMinMax = "radioSwitchMinMax";

const char* PX4RCCalibration::_settingsGroup = "RadioCalibration";
const char* PX4RCCalibration::_settingsKeyTransmitterMode = "TransmitterMode";

const struct PX4RCCalibration::FunctionInfo PX4RCCalibration::_rgFunctionInfo[PX4RCCalibration::rcCalFunctionMax] = {
    //Parameter          required
    { "RC_MAP_ROLL" },
    { "RC_MAP_PITCH" },
    { "RC_MAP_YAW" },
    { "RC_MAP_THROTTLE" },
    { "RC_MAP_MODE_SW" },
    { "RC_MAP_POSCTL_SW" },
    { "RC_MAP_LOITER_SW" },
    { "RC_MAP_RETURN_SW" },
    { "RC_MAP_ACRO_SW" },
    { "RC_MAP_FLAPS" },
    { "RC_MAP_AUX1" },
    { "RC_MAP_AUX2" },
};

PX4RCCalibration::PX4RCCalibration(QWidget *parent) :
    QWidget(parent),
    _currentStep(-1),
    _transmitterMode(2),
    _chanCount(0),
    _rcCalState(rcCalStateChannelWait),
    _uas(NULL),
    _ui(new Ui::PX4RCCalibration),
    _unitTestMode(false)
{
    _ui->setupUi(this);
    
    // Initialize arrays of monitor control pointers. This allows for more efficient code writing using "for" loops.
    for (int chan=0; chan<_chanMax; chan++) {
        QString radioWidgetName;
        
        radioWidgetName = "channel%1Value";
        RCValueWidget* monitorWidget = findChild<RCValueWidget*>(radioWidgetName.arg(chan+1));
        Q_ASSERT(monitorWidget);
        _rgRCValueMonitorWidget[chan] = monitorWidget;
        monitorWidget->setSmallMode();    // Monitor display uses small display
        
        radioWidgetName = "channel%1Label";
        QLabel* monitorLabel = findChild<QLabel*>(radioWidgetName.arg(chan+1));
        Q_ASSERT(monitorLabel);
        _rgRCValueMonitorLabel[chan] = monitorLabel;
    }

    // Initialize array of attitude controls. Order here doesn't matter.
    _rgAttitudeControl[0].function = rcCalFunctionThrottle;
    _rgAttitudeControl[0].valueWidget = _ui->throttleValue;
    _rgAttitudeControl[1].function = rcCalFunctionYaw;
    _rgAttitudeControl[1].valueWidget = _ui->yawValue;
    _rgAttitudeControl[2].function = rcCalFunctionRoll;
    _rgAttitudeControl[2].valueWidget = _ui->rollValue;
    _rgAttitudeControl[3].function = rcCalFunctionPitch;
    _rgAttitudeControl[3].valueWidget = _ui->pitchValue;
    _rgAttitudeControl[4].function = rcCalFunctionFlaps;
    _rgAttitudeControl[4].valueWidget = _ui->flapsValue;
    
    // This code assume we already have an active UAS with ready parameters
    _uas = UASManager::instance()->getActiveUAS();
    Q_ASSERT(_uas);
    
    // Connect new signals
    
    bool fSucceeded;
    Q_UNUSED(fSucceeded);
    fSucceeded =  connect(_uas, SIGNAL(remoteControlChannelRawChanged(int,float)), this, SLOT(_remoteControlChannelRawChanged(int,float)));
    Q_ASSERT(fSucceeded);
    
    _autopilot = AutoPilotPluginManager::instance()->getInstanceForAutoPilotPlugin(_uas);
    Q_ASSERT(_autopilot);
    Q_ASSERT(_autopilot->pluginReady());
    
    connect(_ui->spektrumBind, &QPushButton::clicked, this, &PX4RCCalibration::_spektrumBind);
    
    _updateTimer.setInterval(150);
    _updateTimer.start();

    connect(_ui->rcCalCancel, &QPushButton::clicked, this, &PX4RCCalibration::_stopCalibration);
    connect(_ui->rcCalSkip, &QPushButton::clicked, this, &PX4RCCalibration::_skipButton);
    connect(_ui->rcCalNext, &QPushButton::clicked, this, &PX4RCCalibration::_nextButton);
    
    connect(_ui->rollTrim, &QPushButton::clicked, this, &PX4RCCalibration::_trimNYI);
    connect(_ui->yawTrim, &QPushButton::clicked, this, &PX4RCCalibration::_trimNYI);
    connect(_ui->pitchTrim, &QPushButton::clicked, this, &PX4RCCalibration::_trimNYI);
    connect(_ui->throttleTrim, &QPushButton::clicked, this, &PX4RCCalibration::_trimNYI);
    
    _loadSettings();

    if (_transmitterMode == 1) {
        _ui->mode1->setChecked(true);
        _mode1Toggled(true);
    } else if (_transmitterMode == 2) {
        _ui->mode2->setChecked(true);
        _mode2Toggled(true);
    } else {
        Q_ASSERT(false);
    }
    
    connect(_ui->mode1, &QAbstractButton::toggled, this, &PX4RCCalibration::_mode1Toggled);
    connect(_ui->mode2, &QAbstractButton::toggled, this, &PX4RCCalibration::_mode2Toggled);
    
    _stopCalibration();
    
    connect(&_updateTimer, &QTimer::timeout, this, &PX4RCCalibration::_updateView);
    _setInternalCalibrationValuesFromParameters();
}

PX4RCCalibration::~PX4RCCalibration()
{
    _storeSettings();
}

/// @brief Returns the state machine entry for the specified state.
const PX4RCCalibration::stateMachineEntry* PX4RCCalibration::_getStateMachineEntry(int step)
{
    static const char* msgBegin = "Lower the Throttle stick all the way down as shown in diagram.\nReset all transmitter trims to center.\n\n"
                                 "It is recommended to disconnect all motors for additional safety, however, the system is designed to not arm during the calibration.\n\n"
                                 "Click Next to continue";
    static const char* msgThrottleUp = "Move the Throttle stick all the way up and hold it there...";
    static const char* msgThrottleDown = "Move the Throttle stick all the way down and leave it there...";
    static const char* msgYawLeft = "Move the Yaw stick all the way to the left and hold it there...";
    static const char* msgYawRight = "Move the Yaw stick all the way to the right and hold it there...";
    static const char* msgRollLeft = "Move the Roll stick all the way to the left and hold it there...";
    static const char* msgRollRight = "Move the Roll stick all the way to the right and hold it there...";
    static const char* msgPitchDown = "Move the Pitch stick all the way down and hold it there...";
    static const char* msgPitchUp = "Move the Pitch stick all the way up and hold it there...";
    static const char* msgPitchCenter = "Allow the Pitch stick to move back to center...";
    static const char* msgAux1Switch = "Move the switch or dial you want to use for Aux1.\n\n"
                                            "You can click Skip if you don't want to assign.";
    static const char* msgAux2Switch = "Move the switch or dial you want to use for Aux2.\n\n"
                                            "You can click Skip if you don't want to assign.";
    static const char* msgSwitchMinMax = "Move all the transmitter switches and/or dials back and forth to their extreme positions.";
    static const char* msgFlapsDetect = "Move the switch or dial you want to use for Flaps back and forth a few times. "
                                            "Then leave the switch/dial at the position you want to use for Flaps fully extended.\n\n"
                                            "Click Next to continue.\n"
                                            "If you won't be using Flaps, click Skip.";
    static const char* msgFlapsUp = "Move the switch or dial you want to use for Flaps to the position you want to use for Flaps fully retracted.";
    static const char* msgComplete = "All settings have been captured. Click Next to write the new parameters to your board.";
    
    static const stateMachineEntry rgStateMachine[] = {
        //Function
        { rcCalFunctionMax,                 msgBegin,           _imageHome,         &PX4RCCalibration::_inputCenterWaitBegin,   &PX4RCCalibration::_saveAllTrims,       NULL },
        { rcCalFunctionThrottle,            msgThrottleUp,      _imageThrottleUp,   &PX4RCCalibration::_inputStickDetect,       NULL,                                   NULL },
        { rcCalFunctionThrottle,            msgThrottleDown,    _imageThrottleDown, &PX4RCCalibration::_inputStickMin,          NULL,                                   NULL },
        { rcCalFunctionYaw,                 msgYawRight,        _imageYawRight,     &PX4RCCalibration::_inputStickDetect,       NULL,                                   NULL },
        { rcCalFunctionYaw,                 msgYawLeft,         _imageYawLeft,      &PX4RCCalibration::_inputStickMin,          NULL,                                   NULL },
        { rcCalFunctionRoll,                msgRollRight,       _imageRollRight,    &PX4RCCalibration::_inputStickDetect,       NULL,                                   NULL },
        { rcCalFunctionRoll,                msgRollLeft,        _imageRollLeft,     &PX4RCCalibration::_inputStickMin,          NULL,                                   NULL },
        { rcCalFunctionPitch,               msgPitchUp,         _imagePitchUp,      &PX4RCCalibration::_inputStickDetect,       NULL,                                   NULL },
        { rcCalFunctionPitch,               msgPitchDown,       _imagePitchDown,    &PX4RCCalibration::_inputStickMin,          NULL,                                   NULL },
        { rcCalFunctionPitch,               msgPitchCenter,     _imageHome,         &PX4RCCalibration::_inputCenterWait,        NULL,                                   NULL },
        { rcCalFunctionMax,                 msgSwitchMinMax,    _imageSwitchMinMax, &PX4RCCalibration::_inputSwitchMinMax,      &PX4RCCalibration::_nextStep,           NULL },
        { rcCalFunctionFlaps,               msgFlapsDetect,     _imageThrottleDown, &PX4RCCalibration::_inputFlapsDetect,       &PX4RCCalibration::_saveFlapsDown,      &PX4RCCalibration::_skipFlaps },
        { rcCalFunctionFlaps,               msgFlapsUp,         _imageThrottleDown, &PX4RCCalibration::_inputFlapsUp,           NULL,                                   NULL },
        { rcCalFunctionAux1,                msgAux1Switch,      _imageThrottleDown, &PX4RCCalibration::_inputSwitchDetect,      NULL,                                   &PX4RCCalibration::_nextStep },
        { rcCalFunctionAux2,                msgAux2Switch,      _imageThrottleDown, &PX4RCCalibration::_inputSwitchDetect,      NULL,                                   &PX4RCCalibration::_nextStep },
        { rcCalFunctionMax,                 msgComplete,        _imageThrottleDown, NULL,                                       &PX4RCCalibration::_writeCalibration,   NULL },
    };
    
    Q_ASSERT(step >=0 && step < (int)(sizeof(rgStateMachine) / sizeof(rgStateMachine[0])));
    
    return &rgStateMachine[step];
}

void PX4RCCalibration::_nextStep(void)
{
    _currentStep++;
    _setupCurrentState();
}


/// @brief Sets up the state machine according to the current step from _currentStep.
void PX4RCCalibration::_setupCurrentState(void)
{
   const stateMachineEntry* state = _getStateMachineEntry(_currentStep);
    
    _ui->rcCalStatus->setText(state->instructions);
    
    _setHelpImage(state->image);
    
    _stickDetectChannel = _chanMax;
    _stickDetectSettleStarted = false;
    
    _rcCalSaveCurrentValues();
    
    _ui->rcCalNext->setEnabled(state->nextFn != NULL);
    _ui->rcCalSkip->setEnabled(state->skipFn != NULL);
}

/// @brief This routine is called whenever a raw value for an RC channel changes. It will call the input
/// function as specified by the state machine.
///     @param chan RC channel on which signal is coming from (0-based)
///     @param fval Current value for channel
void PX4RCCalibration::_remoteControlChannelRawChanged(int chan, float fval)
{
    Q_ASSERT(chan >= 0 && chan <= _chanMax);
    
    // We always update raw values
    _rcRawValue[chan] = fval;
    
    qCDebug(PX4RCCalibrationLog) << "Raw value" << chan << fval;
    
    if (_currentStep == -1) {
        // Track the receiver channel count by keeping track of how many channels we see
        if (chan + 1 > (int)_chanCount) {
            _chanCount = chan + 1;
            _ui->receiverInfo->setText(tr("%1 channel receiver").arg(_chanCount));
            if (_chanCount < _chanMinimum) {
                _ui->rcCalStatus->setText(tr("Detected %1 radio channels. To operate PX4, you need at least %2 channels.").arg(_chanCount).arg(_chanMinimum));
            } else {
                _ui->rcCalStatus->clear();
            }
        }
    }
    
    if (_currentStep != -1) {
        const stateMachineEntry* state = _getStateMachineEntry(_currentStep);
        Q_ASSERT(state);
        if (state->rcInputFn) {
            (this->*state->rcInputFn)(state->function, chan, fval);
        }
    }
}

void PX4RCCalibration::_nextButton(void)
{
    if (_currentStep == -1) {
        // Need to have enough channels
        if (_chanCount < _chanMinimum) {
            if (_unitTestMode) {
                emit nextButtonMessageBoxDisplayed();
            } else {
                QGCMessageBox::warning(tr("Receiver"), tr("Detected %1 radio channels. To operate PX4, you need at least %2 channels.").arg(_chanCount).arg(_chanMinimum));
            }
            return;
        }
        _startCalibration();
    } else {
        const stateMachineEntry* state = _getStateMachineEntry(_currentStep);
        Q_ASSERT(state);
        Q_ASSERT(state->nextFn);
        (this->*state->nextFn)();
    }
}

void PX4RCCalibration::_skipButton(void)
{
    Q_ASSERT(_currentStep != -1);
    
    const stateMachineEntry* state = _getStateMachineEntry(_currentStep);
    Q_ASSERT(state);
    Q_ASSERT(state->skipFn);
    (this->*state->skipFn)();
}

void PX4RCCalibration::_trimNYI(void)
{
    QGCMessageBox::warning(tr("Set Trim"), tr("Setting individual trims is not yet implemented. You will need to go through full calibration to set trims."));
}

void PX4RCCalibration::_saveAllTrims(void)
{
    // We save all trims as the first step. At this point no channels are mapped but it should still
    // allow us to get good trims for the roll/pitch/yaw/throttle even though we don't know which
    // channels they are yet. AS we continue through the process the other channels will get their
    // trims reset to correct values.
    
    for (int i=0; i<_chanCount; i++) {
        qCDebug(PX4RCCalibrationLog) << "_saveAllTrims trim" << _rcRawValue[i];
        _rgChannelInfo[i].rcTrim = _rcRawValue[i];
    }
    _nextStep();
}

/// @brief Waits for the sticks to be centered, enabling Next when done.
void PX4RCCalibration::_inputCenterWaitBegin(enum rcCalFunctions function, int chan, int value)
{
    Q_UNUSED(function);
    Q_UNUSED(chan);
    Q_UNUSED(value);
    
    // FIXME: Doesn't wait for center
    
    _ui->rcCalNext->setEnabled(true);
}

bool PX4RCCalibration::_stickSettleComplete(int value)
{
    // We are waiting for the stick to settle out to a max position
    
    if (abs(_stickDetectValue - value) > _rcCalSettleDelta) {
        // Stick is moving too much to consider stopped
        
        qCDebug(PX4RCCalibrationLog) << "_stickSettleComplete still moving, _stickDetectValue:value" << _stickDetectValue << value;

        _stickDetectValue = value;
        _stickDetectSettleStarted = false;
    } else {
        // Stick is still positioned within the specified small range
        
        if (_stickDetectSettleStarted) {
            // We have already started waiting
            
            if (_stickDetectSettleElapsed.elapsed() > _stickDetectSettleMSecs) {
                // Stick has stayed positioned in one place long enough, detection is complete.
                return true;
            }
        } else {
            // Start waiting for the stick to stay settled for _stickDetectSettleWaitMSecs msecs
            
            qCDebug(PX4RCCalibrationLog) << "_stickSettleComplete starting settle timer, _stickDetectValue:value" << _stickDetectValue << value;
            
            _stickDetectSettleStarted = true;
            _stickDetectSettleElapsed.start();
        }
    }
    
    return false;
}

void PX4RCCalibration::_inputStickDetect(enum rcCalFunctions function, int channel, int value)
{
    qCDebug(PX4RCCalibrationLog) << "_inputStickDetect function:channel:value" << _rgFunctionInfo[function].parameterName << channel << value;
    
    // If this channel is already used in a mapping we can't use it again
    if (_rgChannelInfo[channel].function != rcCalFunctionMax) {
        return;
    }
    
    if (_stickDetectChannel == _chanMax) {
        // We have not detected enough movement on a channel yet
        
        if (abs(_rcValueSave[channel] - value) > _rcCalMoveDelta) {
            // Stick has moved far enough to consider it as being selected for the function
            
            qCDebug(PX4RCCalibrationLog) << "_inputStickDetect starting settle wait, function:channel:value" << function << channel << value;
            
            // Setup up to detect stick being pegged to min or max value
            _stickDetectChannel = channel;
            _stickDetectInitialValue = value;
            _stickDetectValue = value;
        }
    } else if (channel == _stickDetectChannel) {
        if (_stickSettleComplete(value)) {
            ChannelInfo* info = &_rgChannelInfo[channel];
            
            qCDebug(PX4RCCalibrationLog) << "_inputStickDetect settle complete, function:channel:value" << function << channel << value;
            
            // Stick detection is complete. Stick should be at max position.
            // Map the channel to the function
            _rgFunctionChannelMapping[function] = channel;
            info->function = function;
            
            // Channel should be at max value, if it is below initial set point the the channel is reversed.
            info->reversed = value < _rcValueSave[channel];
            
            if (info->reversed) {
                _rgRCValueMonitorWidget[channel]->setMin(value);
                _rgRCValueMonitorWidget[channel]->setMinValid(true);
                _rgChannelInfo[channel].rcMin = value;
            } else {
                _rgRCValueMonitorWidget[channel]->setMax(value);
                _rgRCValueMonitorWidget[channel]->setMaxValid(true);
                _rgChannelInfo[channel].rcMax = value;
            }
            
            _nextStep();
        }
    }
}

void PX4RCCalibration::_inputStickMin(enum rcCalFunctions function, int channel, int value)
{
    // We only care about the channel mapped to the function we are working on
    if (_rgFunctionChannelMapping[function] != channel) {
        return;
    }

    if (_stickDetectChannel == _chanMax) {
        // Setup up to detect stick being pegged to extreme position
        if (_rgChannelInfo[channel].reversed) {
            if (value > _rcCalPWMCenterPoint + _rcCalMoveDelta) {
                _stickDetectChannel = channel;
                _stickDetectInitialValue = value;
                _stickDetectValue = value;
            }
        } else {
            if (value < _rcCalPWMCenterPoint - _rcCalMoveDelta) {
                _stickDetectChannel = channel;
                _stickDetectInitialValue = value;
                _stickDetectValue = value;
            }
        }
    } else {
        // We are waiting for the selected channel to settle out
        
        if (_stickSettleComplete(value)) {
            ChannelInfo* info = &_rgChannelInfo[channel];
            
            // Stick detection is complete. Stick should be at min position.
            if (info->reversed) {
                _rgRCValueMonitorWidget[channel]->setMax(value);
                _rgRCValueMonitorWidget[channel]->setMaxValid(true);
                _rgChannelInfo[channel].rcMax = value;
            } else {
                _rgRCValueMonitorWidget[channel]->setMin(value);
                _rgRCValueMonitorWidget[channel]->setMinValid(true);
                _rgChannelInfo[channel].rcMin = value;
            }
            
            _nextStep();
        }
    }
}

void PX4RCCalibration::_inputCenterWait(enum rcCalFunctions function, int channel, int value)
{
    // We only care about the channel mapped to the function we are working on
    if (_rgFunctionChannelMapping[function] != channel) {
        return;
    }
    
    if (_stickDetectChannel == _chanMax) {
        // Sticks have not yet moved close enough to center
        
        if (abs(_rcCalPWMCenterPoint - value) < _rcCalRoughCenterDelta) {
            // Stick has moved close enough to center that we can start waiting for it to settle
            _stickDetectChannel = channel;
            _stickDetectInitialValue = value;
            _stickDetectValue = value;
        }
    } else {
        if (_stickSettleComplete(value)) {
            _nextStep();
        }
    }
}

/// @brief Saves min/max for non-mapped channels
void PX4RCCalibration::_inputSwitchMinMax(enum rcCalFunctions function, int channel, int value)
{
    Q_UNUSED(function);

    // If the channel is mapped we already have min/max
    if (_rgChannelInfo[channel].function != rcCalFunctionMax) {
        return;
    }
    
    if (abs(_rcCalPWMCenterPoint - value) > _rcCalMoveDelta) {
        // Stick has moved far enough from center to consider for min/max
        if (value < _rcCalPWMCenterPoint) {
            int minValue = qMin(_rgChannelInfo[channel].rcMin, value);
            
            qCDebug(PX4RCCalibrationLog) << "_inputSwitchMinMax setting min channel:min" << channel << minValue;
            
            _rgChannelInfo[channel].rcMin = minValue;
            _rgRCValueMonitorWidget[channel]->setMin(minValue);
            _rgRCValueMonitorWidget[channel]->setMinValid(true);
        } else {
            int maxValue = qMax(_rgChannelInfo[channel].rcMax, value);
            
            qCDebug(PX4RCCalibrationLog) << "_inputSwitchMinMax setting max channel:max" << channel << maxValue;
            
            _rgChannelInfo[channel].rcMax = maxValue;
            _rgRCValueMonitorWidget[channel]->setMax(maxValue);
            _rgRCValueMonitorWidget[channel]->setMaxValid(true);
        }
    }
}

void PX4RCCalibration::_skipFlaps(void)
{
    // Flaps channel may have been identified. Clear it out.
    for (int i=0; i<_chanCount; i++) {
        if (_rgChannelInfo[i].function == PX4RCCalibration::rcCalFunctionFlaps) {
            _rgChannelInfo[i].function = rcCalFunctionMax;
        }
    }
    _rgFunctionChannelMapping[PX4RCCalibration::rcCalFunctionFlaps] = _chanMax;

    // Skip over flap steps
    _currentStep += 2;
    _setupCurrentState();
}

void PX4RCCalibration::_saveFlapsDown(void)
{
    int channel = _rgFunctionChannelMapping[rcCalFunctionFlaps];
    
    if (channel == _chanMax) {
        // Channel not yet mapped, still waiting for switch to move
        if (_unitTestMode) {
            emit nextButtonMessageBoxDisplayed();
        } else {
            QGCMessageBox::warning(tr("Flaps switch"), tr("Flaps switch has not yet been detected."));
        }
        return;
    }
    
    Q_ASSERT(channel != -1);
    ChannelInfo* info = &_rgChannelInfo[channel];
    
    int rcValue = _rcRawValue[channel];
    
    // Switch detection is complete. Switch should be at flaps fully extended position.
    
    // Channel should be at max value, if it is below initial set point the channel is reversed.
    info->reversed = rcValue < _rcValueSave[channel];
    
    if (info->reversed) {
        _rgRCValueMonitorWidget[channel]->setMin(rcValue);
        _rgRCValueMonitorWidget[channel]->setMinValid(true);
        _rgChannelInfo[channel].rcMin = rcValue;
    } else {
        _rgRCValueMonitorWidget[channel]->setMax(rcValue);
        _rgRCValueMonitorWidget[channel]->setMaxValid(true);
        _rgChannelInfo[channel].rcMax = rcValue;
    }
    
    _nextStep();
}

void PX4RCCalibration::_inputFlapsUp(enum rcCalFunctions function, int channel, int value)
{
    Q_UNUSED(function);
    
    // FIXME: Duplication
    
    Q_ASSERT(function == rcCalFunctionFlaps);
    
    // We only care about the channel mapped to flaps
    if (_rgFunctionChannelMapping[rcCalFunctionFlaps] != channel) {
        return;
    }
    
    if (_stickDetectChannel == _chanMax) {
        // Setup up to detect stick being pegged to extreme position
        if (_rgChannelInfo[channel].reversed) {
            if (value > _rcCalPWMCenterPoint + _rcCalMoveDelta) {
                _stickDetectChannel = channel;
                _stickDetectInitialValue = value;
                _stickDetectValue = value;
            }
        } else {
            if (value < _rcCalPWMCenterPoint - _rcCalMoveDelta) {
                _stickDetectChannel = channel;
                _stickDetectInitialValue = value;
                _stickDetectValue = value;
            }
        }
    } else {
        // We are waiting for the selected channel to settle out
        
        if (_stickSettleComplete(value)) {
            ChannelInfo* info = &_rgChannelInfo[channel];
            
            // Stick detection is complete. Stick should be at min position.
            if (info->reversed) {
                _rgRCValueMonitorWidget[channel]->setMax(value);
                _rgRCValueMonitorWidget[channel]->setMaxValid(true);
                _rgChannelInfo[channel].rcMax = value;
            } else {
                _rgRCValueMonitorWidget[channel]->setMin(value);
                _rgRCValueMonitorWidget[channel]->setMinValid(true);
                _rgChannelInfo[channel].rcMin = value;
            }
            
            _nextStep();
        }
    }
}

void PX4RCCalibration::_switchDetect(enum rcCalFunctions function, int channel, int value, bool moveToNextStep)
{
    // If this channel is already used in a mapping we can't use it again
    if (_rgChannelInfo[channel].function != rcCalFunctionMax) {
        return;
    }
    
    if (abs(_rcValueSave[channel] - value) > _rcCalMoveDelta) {
        ChannelInfo* info = &_rgChannelInfo[channel];
        
        // Switch has moved far enough to consider it as being selected for the function
        
        // Map the channel to the function
        _rgChannelInfo[channel].function = function;
        _rgFunctionChannelMapping[function] = channel;
        info->function = function;
        
        qCDebug(PX4RCCalibrationLog) << "Function:" << function << "mapped to:" << channel;
        
        if (moveToNextStep) {
            _nextStep();
        }
    }
}

void PX4RCCalibration::_inputSwitchDetect(enum rcCalFunctions function, int channel, int value)
{
    _switchDetect(function, channel, value, true /* move to next step after detection */);
}

void PX4RCCalibration::_inputFlapsDetect(enum rcCalFunctions function, int channel, int value)
{
    _switchDetect(function, channel, value, false /* do not move to next step after detection */);
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
    
    // Initialize attitude function mapping to function channel not set
    for (size_t i=0; i<rcCalFunctionMax; i++) {
        _rgFunctionChannelMapping[i] = _chanMax;
    }
    
    _showMinMaxOnRadioWidgets(false);
    _showTrimOnRadioWidgets(false);
    
    // Reserve the existing Flight Mode switch settings channels so we don't re-use them
    
    static const rcCalFunctions rgFlightModeFunctions[] = {
        rcCalFunctionModeSwitch,
        rcCalFunctionPosCtlSwitch,
        rcCalFunctionLoiterSwitch,
        rcCalFunctionReturnSwitch };
    static const size_t crgFlightModeFunctions = sizeof(rgFlightModeFunctions) / sizeof(rgFlightModeFunctions[0]);

    for (size_t i=0; i < crgFlightModeFunctions; i++) {
        QVariant value;
        enum rcCalFunctions curFunction = rgFlightModeFunctions[i];
        
        bool ok;
        int switchChannel = _autopilot->getParameterFact(_rgFunctionInfo[curFunction].parameterName)->value().toInt(&ok);
        Q_ASSERT(ok);
        
        // Parameter: 1-based channel, 0=not mapped
        // _rgFunctionChannelMapping: 0-based channel, _chanMax=not mapped
        
        if (switchChannel != 0) {
            qCDebug(PX4RCCalibrationLog) << "Reserving 0-based switch channel" << switchChannel - 1;
            _rgFunctionChannelMapping[curFunction] = switchChannel - 1;
            _rgChannelInfo[switchChannel - 1].function = curFunction;
        }
    }
}

/// @brief Sets internal calibration values from the stored parameters
void PX4RCCalibration::_setInternalCalibrationValuesFromParameters(void)
{
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
    
    bool convertOk;
    
    for (int i = 0; i < _chanMax; ++i) {
        struct ChannelInfo* info = &_rgChannelInfo[i];
        
        info->rcTrim = _autopilot->getParameterFact(trimTpl.arg(i+1))->value().toInt(&convertOk);
        Q_ASSERT(convertOk);
        
        info->rcMin = _autopilot->getParameterFact(minTpl.arg(i+1))->value().toInt(&convertOk);
        Q_ASSERT(convertOk);

        info->rcMax = _autopilot->getParameterFact(maxTpl.arg(i+1))->value().toInt(&convertOk);
        Q_ASSERT(convertOk);

        float floatReversed = _autopilot->getParameterFact(revTpl.arg(i+1))->value().toFloat(&convertOk);
        Q_ASSERT(convertOk);
        Q_ASSERT(floatReversed == 1.0f || floatReversed == -1.0f);
        info->reversed = floatReversed == -1.0f;
    }
    
    for (int i=0; i<rcCalFunctionMax; i++) {
        int32_t paramChannel;
        
        paramChannel = _autopilot->getParameterFact(_rgFunctionInfo[i].parameterName)->value().toInt(&convertOk);
        Q_ASSERT(convertOk);
        
        if (paramChannel != 0) {
            _rgFunctionChannelMapping[i] = paramChannel - 1;
            _rgChannelInfo[paramChannel - 1].function = (enum rcCalFunctions)i;
        }
    }
    
    _showMinMaxOnRadioWidgets(true);
    _showTrimOnRadioWidgets(true);
}

/// @brief Sets a connected Spektrum receiver into bind mode
void PX4RCCalibration::_spektrumBind(void)
{

    Q_ASSERT(_uas);
    
    QMessageBox bindTypeMsg(this);
    
    bindTypeMsg.setWindowModality(Qt::ApplicationModal);
    QPushButton* dsm2Mode = bindTypeMsg.addButton("DSM2", QMessageBox::AcceptRole);
    QPushButton* dsmx7Mode = bindTypeMsg.addButton("DSMX (7 channels or less)", QMessageBox::AcceptRole);
    QPushButton* dsmx8Mode = bindTypeMsg.addButton("DSMX (8 channels or more)", QMessageBox::AcceptRole);
    bindTypeMsg.addButton(QMessageBox::Cancel);
    
    bindTypeMsg.setWindowTitle(tr("Spektrum Bind"));
    bindTypeMsg.setText(tr("Place Spektrum satellite receiver in bind mode. Select which mode below."));
    
    int bindType = 0;
    if (bindTypeMsg.exec() != QMessageBox::Cancel) {
        if (bindTypeMsg.clickedButton() == dsm2Mode) {
            bindType = 0;
        } else if (bindTypeMsg.clickedButton() == dsmx7Mode) {
            bindType = 1;
        } else if (bindTypeMsg.clickedButton() == dsmx8Mode) {
            bindType = 2;
        } else {
            Q_ASSERT(false);
        }
        _uas->pairRX(0, bindType);
    }
}

/// @brief Validates the current settings against the calibration rules resetting values as necessary.
void PX4RCCalibration::_validateCalibration(void)
{
    for (int chan = 0; chan<_chanMax; chan++) {
        struct ChannelInfo* info = &_rgChannelInfo[chan];
        
        if (chan < _chanCount) {
            // Validate Min/Max values. Although the channel appears as available we still may
            // not have good min/max/trim values for it. Set to defaults if needed.
            if (info->rcMin > _rcCalPWMValidMinValue || info->rcMax < _rcCalPWMValidMaxValue) {
                qCDebug(PX4RCCalibrationLog) << "_validateCalibration resetting channel" << chan;
                info->rcMin = _rcCalPWMDefaultMinValue;
                info->rcMax = _rcCalPWMDefaultMaxValue;
                info->rcTrim = info->rcMin + ((info->rcMax - info->rcMin) / 2);
            } else {
                switch (_rgChannelInfo[chan].function) {
                    case rcCalFunctionThrottle:
                    case rcCalFunctionYaw:
                    case rcCalFunctionRoll:
                    case rcCalFunctionPitch:
                        // Make sure trim is within min/max
                        if (info->rcTrim < info->rcMin) {
                            info->rcTrim = info->rcMin;
                        } else if (info->rcTrim > info->rcMax) {
                            info->rcTrim = info->rcMax;
                        }
                        break;
                    default:
                        // Non-attitude control channels have calculated trim
                        info->rcTrim = info->rcMin + ((info->rcMax - info->rcMin) / 2);
                        break;
                }
                
            }
        } else {
            // Unavailable channels are set to defaults
            qCDebug(PX4RCCalibrationLog) << "_validateCalibration resetting unavailable channel" << chan;
            info->rcMin = _rcCalPWMDefaultMinValue;
            info->rcMax = _rcCalPWMDefaultMaxValue;
            info->rcTrim = info->rcMin + ((info->rcMax - info->rcMin) / 2);
            info->reversed = false;
        }
    }
}


/// @brief Saves the rc calibration values to the board parameters.
void PX4RCCalibration::_writeCalibration(void)
{
    if (!_uas) return;
    
    _uas->stopCalibration();
    
    _validateCalibration();
    
    QString minTpl("RC%1_MIN");
    QString maxTpl("RC%1_MAX");
    QString trimTpl("RC%1_TRIM");
    QString revTpl("RC%1_REV");
    
    // Note that the rc parameters are all float, so you must cast to float in order to get the right QVariant
    for (int chan = 0; chan<_chanMax; chan++) {
        struct ChannelInfo* info = &_rgChannelInfo[chan];
        int                 oneBasedChannel = chan + 1;
        
        _autopilot->getParameterFact(trimTpl.arg(oneBasedChannel))->setValue((float)info->rcTrim);
        _autopilot->getParameterFact(minTpl.arg(oneBasedChannel))->setValue((float)info->rcMin);
        _autopilot->getParameterFact(maxTpl.arg(oneBasedChannel))->setValue((float)info->rcMax);
        _autopilot->getParameterFact(revTpl.arg(oneBasedChannel))->setValue(info->reversed ? -1.0f : 1.0f);
    }
    
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
        _autopilot->getParameterFact(_rgFunctionInfo[i].parameterName)->setValue(paramChannel);
    }
    
    // If the RC_CHAN_COUNT parameter is available write the channel count
    if (_autopilot->parameterExists("RC_CHAN_CNT")) {
        _autopilot->getParameterFact("RC_CHAN_CNT")->setValue(_chanCount);
    }
    
    _stopCalibration();
    _setInternalCalibrationValuesFromParameters();
}

void PX4RCCalibration::_updateView()
{
    // Update the available channels
    for (int chan=0; chan<_chanCount; chan++) {
        RCValueWidget* valueWidget = _rgRCValueMonitorWidget[chan];
        
        valueWidget->setVisible(true);
        _rgRCValueMonitorLabel[chan]->setVisible(true);
        //qCDebug(PX4RCCalibrationLog) << "Visible" << valueWidget->objectName() << chan;
        
        struct ChannelInfo* info = &_rgChannelInfo[chan];
        valueWidget->setValueAndMinMax(_rcRawValue[chan], info->rcMin, info->rcMax);
        valueWidget->setTrim(info->rcTrim);
        valueWidget->setReversed(info->reversed);
    }
    
    // Update attitude controls
    for (int i=0; i<_attitudeControls; i++) {
        struct AttitudeInfo* attitudeInfo = &_rgAttitudeControl[i];
        
        if (_rgFunctionChannelMapping[attitudeInfo->function] != _chanMax) {
            int channel = _rgFunctionChannelMapping[attitudeInfo->function];
            struct ChannelInfo* info = &_rgChannelInfo[channel];
            RCValueWidget* valueWidget = attitudeInfo->valueWidget;
            
            attitudeInfo->valueWidget->setValueAndMinMax(_rcRawValue[channel], info->rcMin, info->rcMax);
            valueWidget->setTrim(info->rcTrim);
            valueWidget->setReversed(info->reversed);
        }
    }
    
    // Hide non-available channels
    for (int chan=_chanCount; chan<_chanMax; chan++) {
        _rgRCValueMonitorWidget[chan]->setVisible(false);
        _rgRCValueMonitorLabel[chan]->setVisible(false);
        qCDebug(PX4RCCalibrationLog) << "Hiding channel" << _rgRCValueMonitorWidget[chan]->objectName() << chan;
    }
}

/// @brief Starts the calibration process
void PX4RCCalibration::_startCalibration(void)
{
    Q_ASSERT(_chanCount >= _chanMinimum);
    
    _resetInternalCalibrationValues();
    
    // Let the mav known we are starting calibration. This should turn off motors and so forth.
    _uas->startCalibration(UASInterface::StartCalibrationRadio);
    
    _ui->rcCalNext->setText(tr("Next"));
    _ui->rcCalCancel->setEnabled(true);
    
    _currentStep = 0;
    _setupCurrentState();
}

/// @brief Cancels the calibration process, setting things back to initial state.
void PX4RCCalibration::_stopCalibration(void)
{
    _currentStep = -1;
    
    if (_uas) {
        _uas->stopCalibration();
        _setInternalCalibrationValuesFromParameters();
    } else {
        _resetInternalCalibrationValues();
    }
    
    _ui->rcCalStatus->clear();

    _ui->rcCalNext->setText(tr("Calibrate"));
    _ui->rcCalNext->setEnabled(true);
    _ui->rcCalCancel->setEnabled(false);
    _ui->rcCalSkip->setEnabled(false);
    
    _setHelpImage(_imageCenter);
}

/// @brief Saves the current channel values, so that we can detect when the use moves an input.
void PX4RCCalibration::_rcCalSaveCurrentValues(void)
{
	qCDebug(PX4RCCalibrationLog) << "_rcCalSaveCurrentValues";
    for (unsigned i = 0; i < _chanMax; i++) {
        _rcValueSave[i] = _rcRawValue[i];
    }
}

/// @brief Set up the Save state of calibration.
void PX4RCCalibration::_rcCalSave(void)
{
    _rcCalState = rcCalStateSave;
    
    _ui->rcCalStatus->setText(tr("The current calibration settings are now displayed for each channel on screen.\n\n"
                                "Click the Next button to upload calibration to board. Click Cancel if you don't want to save these values."));

    _ui->rcCalNext->setEnabled(true);
    _ui->rcCalSkip->setEnabled(false);
    _ui->rcCalCancel->setEnabled(true);
    
    // This updates the internal values according to the validation rules. Then _updateView will tick and update ui
    // such that the settings that will be written our are displayed.
    _validateCalibration();
    
    _showMinMaxOnRadioWidgets(true);
}

/// @brief Shows or hides the min/max values of the channel widgets.
///     @param show true: show the min/max values, false: hide the min/max values
void PX4RCCalibration::_showMinMaxOnRadioWidgets(bool show)
{
    // Turn on min/max display for all radio widgets
    for (int i=0; i<_chanMax; i++) {
        RCValueWidget* radioWidget = _rgRCValueMonitorWidget[i];
        Q_ASSERT(radioWidget);
        
        radioWidget->showMinMax(show);
        if (show) {
            if (radioWidget->min() <= _rcCalPWMValidMinValue) {
                radioWidget->setMinValid(true);
            }
            if (radioWidget->max() >= _rcCalPWMValidMaxValue) {
                radioWidget->setMaxValid(true);
            }
        } else {
            radioWidget->setMinValid(false);
            radioWidget->setMaxValid(false);
        }
    }
}

/// @brief Shows or hides the trim values of the channel widgets.
///     @param show true: show the trim values, false: hide the trim values
void PX4RCCalibration::_showTrimOnRadioWidgets(bool show)
{
    // Turn on trim display for all radio widgets
    for (int i=0; i<_chanMax; i++) {
        RCValueWidget* radioWidget = _rgRCValueMonitorWidget[i];
        Q_ASSERT(radioWidget);
        
        radioWidget->showTrim(show);
    }
}


void PX4RCCalibration::_loadSettings(void)
{
    QSettings settings;
    
    settings.beginGroup(_settingsGroup);
    _transmitterMode = settings.value(_settingsKeyTransmitterMode, 2).toInt();
    settings.endGroup();
    
    if (_transmitterMode != 1 || _transmitterMode != 2) {
        _transmitterMode = 2;
    }
}

void PX4RCCalibration::_storeSettings(void)
{
    QSettings settings;
    
    settings.beginGroup(_settingsGroup);
    settings.setValue(_settingsKeyTransmitterMode, _transmitterMode);
    settings.endGroup();
}

void PX4RCCalibration::_mode1Toggled(bool checked)
{
    if (checked) {
        _transmitterMode = 1;
        if (_currentStep != -1) {
            const stateMachineEntry* state = _getStateMachineEntry(_currentStep);
            _setHelpImage(state->image);
        }
    }
}

void PX4RCCalibration::_mode2Toggled(bool checked)
{
    if (checked) {
        _transmitterMode = 2;
        if (_currentStep != -1) {
            const stateMachineEntry* state = _getStateMachineEntry(_currentStep);
            _setHelpImage(state->image);
        }
    }
}

void PX4RCCalibration::_setHelpImage(const char* imageFile)
{
    QString file = _imageFilePrefix;
    
    if (_transmitterMode == 1) {
        file += _imageFileMode1Dir;
    } else if (_transmitterMode == 2) {
        file += _imageFileMode2Dir;
    } else {
        Q_ASSERT(false);
    }
    file += imageFile;
    
    qCDebug(PX4RCCalibrationLog) << "_setHelpImage" << file;
    
    _ui->radioIcon->setPixmap(QPixmap(file));
}
