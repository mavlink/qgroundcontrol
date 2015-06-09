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
///     @brief Radio Config Qml Controller
///     @author Don Gagne <don@thegagnes.com

#include "RadioComponentController.h"
#include "UASManager.h"
#include "QGCMessageBox.h"
#include "AutoPilotPluginManager.h"

#include <QSettings>

QGC_LOGGING_CATEGORY(RadioComponentControllerLog, "RadioComponentControllerLog")

#ifdef UNITTEST_BUILD
// Nasty hack to expose controller to unit test code
RadioComponentController* RadioComponentController::_unitTestController = NULL;
#endif

const int RadioComponentController::_updateInterval = 150;              ///< Interval for timer which updates radio channel widgets
const int RadioComponentController::_rcCalPWMCenterPoint = ((RadioComponentController::_rcCalPWMValidMaxValue - RadioComponentController::_rcCalPWMValidMinValue) / 2.0f) + RadioComponentController::_rcCalPWMValidMinValue;
// FIXME: Double check these mins againt 150% throws
const int RadioComponentController::_rcCalPWMValidMinValue = 1300;      ///< Largest valid minimum PWM Min range value
const int RadioComponentController::_rcCalPWMValidMaxValue = 1700;      ///< Smallest valid maximum PWM Max range value
const int RadioComponentController::_rcCalPWMDefaultMinValue = 1000;    ///< Default value for Min if not set
const int RadioComponentController::_rcCalPWMDefaultMaxValue = 2000;    ///< Default value for Max if not set
const int RadioComponentController::_rcCalRoughCenterDelta = 50;        ///< Delta around center point which is considered to be roughly centered
const int RadioComponentController::_rcCalMoveDelta = 300;            ///< Amount of delta past center which is considered stick movement
const int RadioComponentController::_rcCalSettleDelta = 20;           ///< Amount of delta which is considered no stick movement
const int RadioComponentController::_rcCalMinDelta = 100;             ///< Amount of delta allowed around min value to consider channel at min

const int RadioComponentController::_stickDetectSettleMSecs = 500;

const char*  RadioComponentController::_imageFilePrefix = "calibration/";
const char*  RadioComponentController::_imageFileMode1Dir = "mode1/";
const char*  RadioComponentController::_imageFileMode2Dir = "mode2/";
const char*  RadioComponentController::_imageCenter = "radioCenter.png";
const char*  RadioComponentController::_imageHome = "radioHome.png";
const char*  RadioComponentController::_imageThrottleUp = "radioThrottleUp.png";
const char*  RadioComponentController::_imageThrottleDown = "radioThrottleDown.png";
const char*  RadioComponentController::_imageYawLeft = "radioYawLeft.png";
const char*  RadioComponentController::_imageYawRight = "radioYawRight.png";
const char*  RadioComponentController::_imageRollLeft = "radioRollLeft.png";
const char*  RadioComponentController::_imageRollRight = "radioRollRight.png";
const char*  RadioComponentController::_imagePitchUp = "radioPitchUp.png";
const char*  RadioComponentController::_imagePitchDown = "radioPitchDown.png";
const char*  RadioComponentController::_imageSwitchMinMax = "radioSwitchMinMax.png";

const char* RadioComponentController::_settingsGroup = "RadioCalibration";
const char* RadioComponentController::_settingsKeyTransmitterMode = "TransmitterMode";

const struct RadioComponentController::FunctionInfo RadioComponentController::_rgFunctionInfo[RadioComponentController::rcCalFunctionMax] = {
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

RadioComponentController::RadioComponentController(void) :
    _currentStep(-1),
    _transmitterMode(2),
    _chanCount(0),
    _rcCalState(rcCalStateChannelWait),
    _unitTestMode(false),
    _statusText(NULL),
    _cancelButton(NULL),
    _nextButton(NULL),
    _skipButton(NULL)
{
#ifdef UNITTEST_BUILD
    // Nasty hack to expose controller to unit test code
    _unitTestController = this;
#endif

    connect(_uas, &UASInterface::remoteControlChannelRawChanged, this, &RadioComponentController::_remoteControlChannelRawChanged);
    _loadSettings();
    
    _resetInternalCalibrationValues();
}

void RadioComponentController::start(void)
{
    _stopCalibration();
    _setInternalCalibrationValuesFromParameters();
}

RadioComponentController::~RadioComponentController()
{
    _storeSettings();
}

/// @brief Returns the state machine entry for the specified state.
const RadioComponentController::stateMachineEntry* RadioComponentController::_getStateMachineEntry(int step)
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
        { rcCalFunctionMax,                 msgBegin,           _imageHome,         &RadioComponentController::_inputCenterWaitBegin,   &RadioComponentController::_saveAllTrims,       NULL },
        { rcCalFunctionThrottle,            msgThrottleUp,      _imageThrottleUp,   &RadioComponentController::_inputStickDetect,       NULL,                                           NULL },
        { rcCalFunctionThrottle,            msgThrottleDown,    _imageThrottleDown, &RadioComponentController::_inputStickMin,          NULL,                                           NULL },
        { rcCalFunctionYaw,                 msgYawRight,        _imageYawRight,     &RadioComponentController::_inputStickDetect,       NULL,                                           NULL },
        { rcCalFunctionYaw,                 msgYawLeft,         _imageYawLeft,      &RadioComponentController::_inputStickMin,          NULL,                                           NULL },
        { rcCalFunctionRoll,                msgRollRight,       _imageRollRight,    &RadioComponentController::_inputStickDetect,       NULL,                                           NULL },
        { rcCalFunctionRoll,                msgRollLeft,        _imageRollLeft,     &RadioComponentController::_inputStickMin,          NULL,                                           NULL },
        { rcCalFunctionPitch,               msgPitchUp,         _imagePitchUp,      &RadioComponentController::_inputStickDetect,       NULL,                                           NULL },
        { rcCalFunctionPitch,               msgPitchDown,       _imagePitchDown,    &RadioComponentController::_inputStickMin,          NULL,                                           NULL },
        { rcCalFunctionPitch,               msgPitchCenter,     _imageHome,         &RadioComponentController::_inputCenterWait,        NULL,                                           NULL },
        { rcCalFunctionMax,                 msgSwitchMinMax,    _imageSwitchMinMax, &RadioComponentController::_inputSwitchMinMax,      &RadioComponentController::_advanceState,       NULL },
        { rcCalFunctionFlaps,               msgFlapsDetect,     _imageThrottleDown, &RadioComponentController::_inputFlapsDetect,       &RadioComponentController::_saveFlapsDown,      &RadioComponentController::_skipFlaps },
        { rcCalFunctionFlaps,               msgFlapsUp,         _imageThrottleDown, &RadioComponentController::_inputFlapsUp,           NULL,                                           NULL },
        { rcCalFunctionAux1,                msgAux1Switch,      _imageThrottleDown, &RadioComponentController::_inputSwitchDetect,      NULL,                                           &RadioComponentController::_advanceState },
        { rcCalFunctionAux2,                msgAux2Switch,      _imageThrottleDown, &RadioComponentController::_inputSwitchDetect,      NULL,                                           &RadioComponentController::_advanceState },
        { rcCalFunctionMax,                 msgComplete,        _imageThrottleDown, NULL,                                               &RadioComponentController::_writeCalibration,   NULL },
    };
    
    Q_ASSERT(step >=0 && step < (int)(sizeof(rgStateMachine) / sizeof(rgStateMachine[0])));
    
    return &rgStateMachine[step];
}

void RadioComponentController::_advanceState(void)
{
    _currentStep++;
    _setupCurrentState();
}


/// @brief Sets up the state machine according to the current step from _currentStep.
void RadioComponentController::_setupCurrentState(void)
{
   const stateMachineEntry* state = _getStateMachineEntry(_currentStep);
    
    _statusText->setProperty("text", state->instructions);
    
    _setHelpImage(state->image);
    
    _stickDetectChannel = _chanMax;
    _stickDetectSettleStarted = false;
    
    _rcCalSaveCurrentValues();
    
    _nextButton->setEnabled(state->nextFn != NULL);
    _skipButton->setEnabled(state->skipFn != NULL);
}

/// @brief This routine is called whenever a raw value for an RC channel changes. It will call the input
/// function as specified by the state machine.
///     @param chan RC channel on which signal is coming from (0-based)
///     @param fval Current value for channel
void RadioComponentController::_remoteControlChannelRawChanged(int chan, float fval)
{
    if (chan >= 0 && chan <= _chanMax) {
        // We always update raw values
        _rcRawValue[chan] = fval;
        emit channelRCValueChanged(chan, _rcRawValue[chan]);
        
        // Signal attitude rc values to Qml if mapped
        if (_rgChannelInfo[chan].function != rcCalFunctionMax) {
            switch (_rgChannelInfo[chan].function) {
                case rcCalFunctionRoll:
                    emit rollChannelRCValueChanged(_rcRawValue[chan]);
                    break;
                case rcCalFunctionPitch:
                    emit pitchChannelRCValueChanged(_rcRawValue[chan]);
                    break;
                case rcCalFunctionYaw:
                    emit yawChannelRCValueChanged(_rcRawValue[chan]);
                    break;
                case rcCalFunctionThrottle:
                    emit throttleChannelRCValueChanged(_rcRawValue[chan]);
                    break;
                default:
                    break;

            }
        }
            
        qCDebug(RadioComponentControllerLog) << "Raw value" << chan << fval;
        
        if (_currentStep == -1) {
            // Track the receiver channel count by keeping track of how many channels we see
            if (chan + 1 > (int)_chanCount) {
                _chanCount = chan + 1;
                emit channelCountChanged(_chanCount);
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
}

void RadioComponentController::nextButtonClicked(void)
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

void RadioComponentController::skipButtonClicked(void)
{
    Q_ASSERT(_currentStep != -1);
    
    const stateMachineEntry* state = _getStateMachineEntry(_currentStep);
    Q_ASSERT(state);
    Q_ASSERT(state->skipFn);
    (this->*state->skipFn)();
}

void RadioComponentController::cancelButtonClicked(void)
{
    _stopCalibration();
}

void RadioComponentController::_saveAllTrims(void)
{
    // We save all trims as the first step. At this point no channels are mapped but it should still
    // allow us to get good trims for the roll/pitch/yaw/throttle even though we don't know which
    // channels they are yet. AS we continue through the process the other channels will get their
    // trims reset to correct values.
    
    for (int i=0; i<_chanCount; i++) {
        qCDebug(RadioComponentControllerLog) << "_saveAllTrims trim" << _rcRawValue[i];
        _rgChannelInfo[i].rcTrim = _rcRawValue[i];
    }
    _advanceState();
}

/// @brief Waits for the sticks to be centered, enabling Next when done.
void RadioComponentController::_inputCenterWaitBegin(enum rcCalFunctions function, int chan, int value)
{
    Q_UNUSED(function);
    Q_UNUSED(chan);
    Q_UNUSED(value);
    
    // FIXME: Doesn't wait for center
    
    _nextButton->setEnabled(true);
}

bool RadioComponentController::_stickSettleComplete(int value)
{
    // We are waiting for the stick to settle out to a max position
    
    if (abs(_stickDetectValue - value) > _rcCalSettleDelta) {
        // Stick is moving too much to consider stopped
        
        qCDebug(RadioComponentControllerLog) << "_stickSettleComplete still moving, _stickDetectValue:value" << _stickDetectValue << value;

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
            
            qCDebug(RadioComponentControllerLog) << "_stickSettleComplete starting settle timer, _stickDetectValue:value" << _stickDetectValue << value;
            
            _stickDetectSettleStarted = true;
            _stickDetectSettleElapsed.start();
        }
    }
    
    return false;
}

void RadioComponentController::_inputStickDetect(enum rcCalFunctions function, int channel, int value)
{
    qCDebug(RadioComponentControllerLog) << "_inputStickDetect function:channel:value" << _rgFunctionInfo[function].parameterName << channel << value;
    
    // If this channel is already used in a mapping we can't use it again
    if (_rgChannelInfo[channel].function != rcCalFunctionMax) {
        return;
    }
    
    if (_stickDetectChannel == _chanMax) {
        // We have not detected enough movement on a channel yet
        
        if (abs(_rcValueSave[channel] - value) > _rcCalMoveDelta) {
            // Stick has moved far enough to consider it as being selected for the function
            
            qCDebug(RadioComponentControllerLog) << "_inputStickDetect starting settle wait, function:channel:value" << function << channel << value;
            
            // Setup up to detect stick being pegged to min or max value
            _stickDetectChannel = channel;
            _stickDetectInitialValue = value;
            _stickDetectValue = value;
        }
    } else if (channel == _stickDetectChannel) {
        if (_stickSettleComplete(value)) {
            ChannelInfo* info = &_rgChannelInfo[channel];
            
            qCDebug(RadioComponentControllerLog) << "_inputStickDetect settle complete, function:channel:value" << function << channel << value;
            
            // Stick detection is complete. Stick should be at max position.
            // Map the channel to the function
            _rgFunctionChannelMapping[function] = channel;
            info->function = function;
            
            // Channel should be at max value, if it is below initial set point the the channel is reversed.
            info->reversed = value < _rcValueSave[channel];
            
            if (info->reversed) {
                _rgChannelInfo[channel].rcMin = value;
            } else {
                _rgChannelInfo[channel].rcMax = value;
            }
            
            _signalAllAttiudeValueChanges();
            
            _advanceState();
        }
    }
}

void RadioComponentController::_inputStickMin(enum rcCalFunctions function, int channel, int value)
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
                _rgChannelInfo[channel].rcMax = value;
            } else {
                _rgChannelInfo[channel].rcMin = value;
            }

            // Check if this is throttle and set trim accordingly
            if (function == rcCalFunctionThrottle) {
                _rgChannelInfo[channel].rcTrim = value;
            }
            // XXX to support configs which can reverse they need to check a reverse
            // flag here and not do this.
            
            _advanceState();
        }
    }
}

void RadioComponentController::_inputCenterWait(enum rcCalFunctions function, int channel, int value)
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
            _advanceState();
        }
    }
}

/// @brief Saves min/max for non-mapped channels
void RadioComponentController::_inputSwitchMinMax(enum rcCalFunctions function, int channel, int value)
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
            
            qCDebug(RadioComponentControllerLog) << "_inputSwitchMinMax setting min channel:min" << channel << minValue;
            
            _rgChannelInfo[channel].rcMin = minValue;
        } else {
            int maxValue = qMax(_rgChannelInfo[channel].rcMax, value);
            
            qCDebug(RadioComponentControllerLog) << "_inputSwitchMinMax setting max channel:max" << channel << maxValue;
            
            _rgChannelInfo[channel].rcMax = maxValue;
        }
    }
}

void RadioComponentController::_skipFlaps(void)
{
    // Flaps channel may have been identified. Clear it out.
    for (int i=0; i<_chanCount; i++) {
        if (_rgChannelInfo[i].function == RadioComponentController::rcCalFunctionFlaps) {
            _rgChannelInfo[i].function = rcCalFunctionMax;
        }
    }
    _rgFunctionChannelMapping[RadioComponentController::rcCalFunctionFlaps] = _chanMax;

    // Skip over flap steps
    _currentStep += 2;
    _setupCurrentState();
}

void RadioComponentController::_saveFlapsDown(void)
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
        _rgChannelInfo[channel].rcMin = rcValue;
    } else {
        _rgChannelInfo[channel].rcMax = rcValue;
    }
    
    _advanceState();
}

void RadioComponentController::_inputFlapsUp(enum rcCalFunctions function, int channel, int value)
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
                _rgChannelInfo[channel].rcMax = value;
            } else {
                _rgChannelInfo[channel].rcMin = value;
            }
            
            _advanceState();
        }
    }
}

void RadioComponentController::_switchDetect(enum rcCalFunctions function, int channel, int value, bool moveToNextStep)
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
        
        qCDebug(RadioComponentControllerLog) << "Function:" << function << "mapped to:" << channel;
        
        if (moveToNextStep) {
            _advanceState();
        }
    }
}

void RadioComponentController::_inputSwitchDetect(enum rcCalFunctions function, int channel, int value)
{
    _switchDetect(function, channel, value, true /* move to next step after detection */);
}

void RadioComponentController::_inputFlapsDetect(enum rcCalFunctions function, int channel, int value)
{
    _switchDetect(function, channel, value, false /* do not move to next step after detection */);
}

/// @brief Resets internal calibration values to their initial state in preparation for a new calibration sequence.
void RadioComponentController::_resetInternalCalibrationValues(void)
{
    // Set all raw channels to not reversed and center point values
    for (size_t i=0; i<_chanMax; i++) {
        struct ChannelInfo* info = &_rgChannelInfo[i];
        info->function = rcCalFunctionMax;
        info->reversed = false;
        info->rcMin = RadioComponentController::_rcCalPWMCenterPoint;
        info->rcMax = RadioComponentController::_rcCalPWMCenterPoint;
        info->rcTrim = RadioComponentController::_rcCalPWMCenterPoint;
    }
    
    // Initialize attitude function mapping to function channel not set
    for (size_t i=0; i<rcCalFunctionMax; i++) {
        _rgFunctionChannelMapping[i] = _chanMax;
    }
    
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
        int switchChannel = getParameterFact(FactSystem::defaultComponentId, _rgFunctionInfo[curFunction].parameterName)->value().toInt(&ok);
        Q_ASSERT(ok);
        
        // Parameter: 1-based channel, 0=not mapped
        // _rgFunctionChannelMapping: 0-based channel, _chanMax=not mapped
        
        if (switchChannel != 0) {
            qCDebug(RadioComponentControllerLog) << "Reserving 0-based switch channel" << switchChannel - 1;
            _rgFunctionChannelMapping[curFunction] = switchChannel - 1;
            _rgChannelInfo[switchChannel - 1].function = curFunction;
        }
    }
    
    _signalAllAttiudeValueChanges();
}

/// @brief Sets internal calibration values from the stored parameters
void RadioComponentController::_setInternalCalibrationValuesFromParameters(void)
{
    // Initialize all function mappings to not set
    
    for (size_t i=0; i<_chanMax; i++) {
        struct ChannelInfo* info = &_rgChannelInfo[i];
        info->function = rcCalFunctionMax;
    }
    
    for (size_t i=0; i<rcCalFunctionMax; i++) {
        _rgFunctionChannelMapping[i] = _chanMax;
    }
    
    // Pull parameters and update
    
    QString minTpl("RC%1_MIN");
    QString maxTpl("RC%1_MAX");
    QString trimTpl("RC%1_TRIM");
    QString revTpl("RC%1_REV");
    
    bool convertOk;
    
    for (int i = 0; i < _chanMax; ++i) {
        struct ChannelInfo* info = &_rgChannelInfo[i];
        
        info->rcTrim = getParameterFact(FactSystem::defaultComponentId, trimTpl.arg(i+1))->value().toInt(&convertOk);
        Q_ASSERT(convertOk);
        
        info->rcMin = getParameterFact(FactSystem::defaultComponentId, minTpl.arg(i+1))->value().toInt(&convertOk);
        Q_ASSERT(convertOk);

        info->rcMax = getParameterFact(FactSystem::defaultComponentId, maxTpl.arg(i+1))->value().toInt(&convertOk);
        Q_ASSERT(convertOk);

        float floatReversed = getParameterFact(FactSystem::defaultComponentId, revTpl.arg(i+1))->value().toFloat(&convertOk);
        Q_ASSERT(convertOk);
        Q_ASSERT(floatReversed == 1.0f || floatReversed == -1.0f);
        info->reversed = floatReversed == -1.0f;
    }
    
    for (int i=0; i<rcCalFunctionMax; i++) {
        int32_t paramChannel;
        
        paramChannel = getParameterFact(FactSystem::defaultComponentId, _rgFunctionInfo[i].parameterName)->value().toInt(&convertOk);
        Q_ASSERT(convertOk);
        
        if (paramChannel != 0) {
            _rgFunctionChannelMapping[i] = paramChannel - 1;
            _rgChannelInfo[paramChannel - 1].function = (enum rcCalFunctions)i;
        }
    }
    
    _signalAllAttiudeValueChanges();
}

void RadioComponentController::spektrumBindMode(int mode)
{
    _uas->pairRX(0, mode);
}

/// @brief Validates the current settings against the calibration rules resetting values as necessary.
void RadioComponentController::_validateCalibration(void)
{
    for (int chan = 0; chan<_chanMax; chan++) {
        struct ChannelInfo* info = &_rgChannelInfo[chan];
        
        if (chan < _chanCount) {
            // Validate Min/Max values. Although the channel appears as available we still may
            // not have good min/max/trim values for it. Set to defaults if needed.
            if (info->rcMin > _rcCalPWMValidMinValue || info->rcMax < _rcCalPWMValidMaxValue) {
                qCDebug(RadioComponentControllerLog) << "_validateCalibration resetting channel" << chan;
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
            qCDebug(RadioComponentControllerLog) << "_validateCalibration resetting unavailable channel" << chan;
            info->rcMin = _rcCalPWMDefaultMinValue;
            info->rcMax = _rcCalPWMDefaultMaxValue;
            info->rcTrim = info->rcMin + ((info->rcMax - info->rcMin) / 2);
            info->reversed = false;
        }
    }
}


/// @brief Saves the rc calibration values to the board parameters.
void RadioComponentController::_writeCalibration(void)
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
        
        getParameterFact(FactSystem::defaultComponentId, trimTpl.arg(oneBasedChannel))->setValue((float)info->rcTrim);
        getParameterFact(FactSystem::defaultComponentId, minTpl.arg(oneBasedChannel))->setValue((float)info->rcMin);
        getParameterFact(FactSystem::defaultComponentId, maxTpl.arg(oneBasedChannel))->setValue((float)info->rcMax);
        getParameterFact(FactSystem::defaultComponentId, revTpl.arg(oneBasedChannel))->setValue(info->reversed ? -1.0f : 1.0f);
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
        getParameterFact(FactSystem::defaultComponentId, _rgFunctionInfo[i].parameterName)->setValue(paramChannel);
    }
    
    // If the RC_CHAN_COUNT parameter is available write the channel count
    if (parameterExists(FactSystem::defaultComponentId, "RC_CHAN_CNT")) {
        getParameterFact(FactSystem::defaultComponentId, "RC_CHAN_CNT")->setValue(_chanCount);
    }
    
    _stopCalibration();
    _setInternalCalibrationValuesFromParameters();
}

/// @brief Starts the calibration process
void RadioComponentController::_startCalibration(void)
{
    Q_ASSERT(_chanCount >= _chanMinimum);
    
    _resetInternalCalibrationValues();
    
    // Let the mav known we are starting calibration. This should turn off motors and so forth.
    _uas->startCalibration(UASInterface::StartCalibrationRadio);
    
    _nextButton->setProperty("text", "Next");
    _cancelButton->setEnabled(true);
    
    _currentStep = 0;
    _setupCurrentState();
}

/// @brief Cancels the calibration process, setting things back to initial state.
void RadioComponentController::_stopCalibration(void)
{
    _currentStep = -1;
    
    if (_uas) {
        _uas->stopCalibration();
        _setInternalCalibrationValuesFromParameters();
    } else {
        _resetInternalCalibrationValues();
    }
    
    _statusText->setProperty("text", "");

    _nextButton->setProperty("text", "Calibrate");
    _nextButton->setEnabled(true);
    _cancelButton->setEnabled(false);
    _skipButton->setEnabled(false);
    
    _setHelpImage(_imageCenter);
}

/// @brief Saves the current channel values, so that we can detect when the use moves an input.
void RadioComponentController::_rcCalSaveCurrentValues(void)
{
	qCDebug(RadioComponentControllerLog) << "_rcCalSaveCurrentValues";
    for (unsigned i = 0; i < _chanMax; i++) {
        _rcValueSave[i] = _rcRawValue[i];
    }
}

/// @brief Set up the Save state of calibration.
void RadioComponentController::_rcCalSave(void)
{
    _rcCalState = rcCalStateSave;
    
    _statusText->setProperty("text",
                             "The current calibration settings are now displayed for each channel on screen.\n\n"
                                "Click the Next button to upload calibration to board. Click Cancel if you don't want to save these values.");

    _nextButton->setEnabled(true);
    _skipButton->setEnabled(false);
    _cancelButton->setEnabled(true);
    
    // This updates the internal values according to the validation rules. Then _updateView will tick and update ui
    // such that the settings that will be written our are displayed.
    _validateCalibration();
}

void RadioComponentController::_loadSettings(void)
{
    QSettings settings;
    
    settings.beginGroup(_settingsGroup);
    _transmitterMode = settings.value(_settingsKeyTransmitterMode, 2).toInt();
    settings.endGroup();
    
    if (_transmitterMode != 1 || _transmitterMode != 2) {
        _transmitterMode = 2;
    }
}

void RadioComponentController::_storeSettings(void)
{
    QSettings settings;
    
    settings.beginGroup(_settingsGroup);
    settings.setValue(_settingsKeyTransmitterMode, _transmitterMode);
    settings.endGroup();
}

void RadioComponentController::_setHelpImage(const char* imageFile)
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
    
    qCDebug(RadioComponentControllerLog) << "_setHelpImage" << file;
    
    _imageHelp = file;
    emit imageHelpChanged(file);
}

int RadioComponentController::channelCount(void)
{
    return _chanCount;
}

int RadioComponentController::rollChannelRCValue(void)
{    
    if (_rgFunctionChannelMapping[rcCalFunctionRoll] != _chanMax) {
        return _rcRawValue[rcCalFunctionRoll];
    } else {
        return 1500;
    }
}

int RadioComponentController::pitchChannelRCValue(void)
{
    if (_rgFunctionChannelMapping[rcCalFunctionPitch] != _chanMax) {
        return _rcRawValue[rcCalFunctionPitch];
    } else {
        return 1500;
    }
}

int RadioComponentController::yawChannelRCValue(void)
{
    if (_rgFunctionChannelMapping[rcCalFunctionYaw] != _chanMax) {
        return _rcRawValue[rcCalFunctionYaw];
    } else {
        return 1500;
    }
}

int RadioComponentController::throttleChannelRCValue(void)
{
    if (_rgFunctionChannelMapping[rcCalFunctionThrottle] != _chanMax) {
        return _rcRawValue[rcCalFunctionThrottle];
    } else {
        return 1500;
    }
}

bool RadioComponentController::rollChannelMapped(void)
{
    return _rgFunctionChannelMapping[rcCalFunctionRoll] != _chanMax;
}

bool RadioComponentController::pitchChannelMapped(void)
{
    return _rgFunctionChannelMapping[rcCalFunctionPitch] != _chanMax;
}

bool RadioComponentController::yawChannelMapped(void)
{
    return _rgFunctionChannelMapping[rcCalFunctionYaw] != _chanMax;
}

bool RadioComponentController::throttleChannelMapped(void)
{
    return _rgFunctionChannelMapping[rcCalFunctionThrottle] != _chanMax;
}

bool RadioComponentController::rollChannelReversed(void)
{
    if (_rgFunctionChannelMapping[rcCalFunctionRoll] != _chanMax) {
        return _rgChannelInfo[_rgFunctionChannelMapping[rcCalFunctionRoll]].reversed;
    } else {
        return false;
    }
}

bool RadioComponentController::pitchChannelReversed(void)
{
    if (_rgFunctionChannelMapping[rcCalFunctionPitch] != _chanMax) {
        return _rgChannelInfo[_rgFunctionChannelMapping[rcCalFunctionPitch]].reversed;
    } else {
        return false;
    }
}

bool RadioComponentController::yawChannelReversed(void)
{
    if (_rgFunctionChannelMapping[rcCalFunctionYaw] != _chanMax) {
        return _rgChannelInfo[_rgFunctionChannelMapping[rcCalFunctionYaw]].reversed;
    } else {
        return false;
    }
}

bool RadioComponentController::throttleChannelReversed(void)
{
    if (_rgFunctionChannelMapping[rcCalFunctionThrottle] != _chanMax) {
        return _rgChannelInfo[_rgFunctionChannelMapping[rcCalFunctionThrottle]].reversed;
    } else {
        return false;
    }
}

void RadioComponentController::setTransmitterMode(int mode)
{
    if (mode == 1 || mode == 2) {
        _transmitterMode = mode;
        if (_currentStep != -1) {
            const stateMachineEntry* state = _getStateMachineEntry(_currentStep);
            _setHelpImage(state->image);
        }
    }
}

void RadioComponentController::_signalAllAttiudeValueChanges(void)
{
    emit rollChannelMappedChanged(rollChannelMapped());
    emit pitchChannelMappedChanged(pitchChannelMapped());
    emit yawChannelMappedChanged(yawChannelMapped());
    emit throttleChannelMappedChanged(throttleChannelMapped());
    
    emit rollChannelReversedChanged(rollChannelReversed());
    emit pitchChannelReversedChanged(pitchChannelReversed());
    emit yawChannelReversedChanged(yawChannelReversed());
    emit throttleChannelReversedChanged(throttleChannelReversed());
}

void RadioComponentController::copyTrims(void)
{
    _uas->startCalibration(UASInterface::StartCalibrationCopyTrims);
}
