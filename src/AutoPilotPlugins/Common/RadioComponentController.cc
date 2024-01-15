/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief Radio Config Qml Controller
///     @author Don Gagne <don@thegagnes.com

#include "RadioComponentController.h"
#include "QGCApplication.h"

#include <QElapsedTimer>
#include <QSettings>

QGC_LOGGING_CATEGORY(RadioComponentControllerLog, "RadioComponentControllerLog")
QGC_LOGGING_CATEGORY(RadioComponentControllerVerboseLog, "RadioComponentControllerVerboseLog")

#ifdef UNITTEST_BUILD
// Nasty hack to expose controller to unit test code
RadioComponentController* RadioComponentController::_unitTestController = nullptr;
#endif

const int RadioComponentController::_updateInterval = 150;              ///< Interval for timer which updates radio channel widgets
const int RadioComponentController::_rcCalPWMCenterPoint = ((RadioComponentController::_rcCalPWMValidMaxValue - RadioComponentController::_rcCalPWMValidMinValue) / 2.0f) + RadioComponentController::_rcCalPWMValidMinValue;
const int RadioComponentController::_rcCalPWMValidMinValue =    1300;   ///< Largest valid minimum PWM Min range value
const int RadioComponentController::_rcCalPWMValidMaxValue =    1700;   ///< Smallest valid maximum PWM Max range value
const int RadioComponentController::_rcCalPWMDefaultMinValue =  1000;   ///< Default value for Min if not set
const int RadioComponentController::_rcCalPWMDefaultMaxValue =  2000;   ///< Default value for Max if not set
const int RadioComponentController::_rcCalRoughCenterDelta =    50;     ///< Delta around center point which is considered to be roughly centered
const int RadioComponentController::_rcCalMoveDelta =           300;    ///< Amount of delta past center which is considered stick movement
const int RadioComponentController::_rcCalSettleDelta =         20;     ///< Amount of delta which is considered no stick movement
const int RadioComponentController::_rcCalMinDelta =            100;    ///< Amount of delta allowed around min value to consider channel at min

const int RadioComponentController::_stickDetectSettleMSecs = 500;

const char*  RadioComponentController::_imageFilePrefix =   "calibration/";
const char*  RadioComponentController::_imageFileMode1Dir = "mode1/";
const char*  RadioComponentController::_imageFileMode2Dir = "mode2/";
const char*  RadioComponentController::_imageCenter =       "radioCenter.png";
const char*  RadioComponentController::_imageHome =         "radioHome.png";
const char*  RadioComponentController::_imageThrottleUp =   "radioThrottleUp.png";
const char*  RadioComponentController::_imageThrottleDown = "radioThrottleDown.png";
const char*  RadioComponentController::_imageYawLeft =      "radioYawLeft.png";
const char*  RadioComponentController::_imageYawRight =     "radioYawRight.png";
const char*  RadioComponentController::_imageRollLeft =     "radioRollLeft.png";
const char*  RadioComponentController::_imageRollRight =    "radioRollRight.png";
const char*  RadioComponentController::_imagePitchUp =      "radioPitchUp.png";
const char*  RadioComponentController::_imagePitchDown =    "radioPitchDown.png";
const char*  RadioComponentController::_imageSwitchMinMax = "radioSwitchMinMax.png";

const char* RadioComponentController::_settingsGroup =              "RadioCalibration";
const char* RadioComponentController::_settingsKeyTransmitterMode = "TransmitterMode";

const char* RadioComponentController::_px4RevParamFormat =      "RC%1_REV";
const char* RadioComponentController::_apmNewRevParamFormat =   "RC%1_REVERSED";

const struct RadioComponentController::FunctionInfo RadioComponentController::_rgFunctionInfoPX4[RadioComponentController::rcCalFunctionMax] = {
{ "RC_MAP_ROLL" },
{ "RC_MAP_PITCH" },
{ "RC_MAP_YAW" },
{ "RC_MAP_THROTTLE" }
};

const struct RadioComponentController::FunctionInfo RadioComponentController::_rgFunctionInfoAPM[RadioComponentController::rcCalFunctionMax] = {
{ "RCMAP_ROLL" },
{ "RCMAP_PITCH" },
{ "RCMAP_YAW" },
{ "RCMAP_THROTTLE" }
};

RadioComponentController::RadioComponentController(void)
    : _currentStep(-1)
    , _transmitterMode(2)
    , _chanCount(0)
    , _rcCalState(rcCalStateChannelWait)
{
#ifdef UNITTEST_BUILD
    // Nasty hack to expose controller to unit test code
    _unitTestController = this;
#endif

    if (parameterExists(FactSystem::defaultComponentId, QStringLiteral("RC1_REVERSED"))) {
        // Newer ardupilot firmwares have a different reverse param naming scheme and value scheme
        _revParamFormat = _apmNewRevParamFormat;
        _revParamIsBool = true; // param value is boolean 0/1 for reversed or not
    } else {
        // Older ardupilot firmwares share the same naming convention as PX4
        _revParamFormat = _px4RevParamFormat;
        _revParamIsBool = false;    // paeram value if -1 indicates reversed
    }

    connect(_vehicle, &Vehicle::rcChannelsChanged, this, &RadioComponentController::_rcChannelsChanged);
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
const RadioComponentController::stateMachineEntry* RadioComponentController::_getStateMachineEntry(int step) const
{
    static const char* msgBeginPX4 =        QT_TR_NOOP("Lower the Throttle stick all the way down as shown in diagram.\n\n"
                                            "It is recommended to disconnect all motors for additional safety, however, the system is designed to not arm during the calibration.\n\n"
                                            "Click Next to continue");
    static const char* msgBeginAPM =        QT_TR_NOOP("Lower the Throttle stick all the way down as shown in diagram.\nReset all transmitter trims to center.\n\n"
                                            "Please ensure all motor power is disconnected AND all props are removed from the vehicle.\n\n"
                                            "Click Next to continue");
    static const char* msgThrottleUp =      QT_TR_NOOP("Move the Throttle stick all the way up and hold it there...");
    static const char* msgThrottleDown =    QT_TR_NOOP("Move the Throttle stick all the way down and leave it there...");
    static const char* msgYawLeft =         QT_TR_NOOP("Move the Yaw stick all the way to the left and hold it there...");
    static const char* msgYawRight =        QT_TR_NOOP("Move the Yaw stick all the way to the right and hold it there...");
    static const char* msgRollLeft =        QT_TR_NOOP("Move the Roll stick all the way to the left and hold it there...");
    static const char* msgRollRight =       QT_TR_NOOP("Move the Roll stick all the way to the right and hold it there...");
    static const char* msgPitchDown =       QT_TR_NOOP("Move the Pitch stick all the way down and hold it there...");
    static const char* msgPitchUp =         QT_TR_NOOP("Move the Pitch stick all the way up and hold it there...");
    static const char* msgPitchCenter =     QT_TR_NOOP("Allow the Pitch stick to move back to center...");
    static const char* msgSwitchMinMax =    QT_TR_NOOP("Move all the transmitter switches and/or dials back and forth to their extreme positions.");
    static const char* msgComplete =        QT_TR_NOOP("All settings have been captured. Click Next to write the new parameters to your board.");

    static const stateMachineEntry rgStateMachinePX4[] = {
        //Function
        { rcCalFunctionMax,                 msgBeginPX4,        _imageHome,         &RadioComponentController::_inputCenterWaitBegin,   &RadioComponentController::_saveAllTrims,       nullptr },
        { rcCalFunctionThrottle,            msgThrottleUp,      _imageThrottleUp,   &RadioComponentController::_inputStickDetect,       nullptr,                                        nullptr },
        { rcCalFunctionThrottle,            msgThrottleDown,    _imageThrottleDown, &RadioComponentController::_inputStickMin,          nullptr,                                        nullptr },
        { rcCalFunctionYaw,                 msgYawRight,        _imageYawRight,     &RadioComponentController::_inputStickDetect,       nullptr,                                        nullptr },
        { rcCalFunctionYaw,                 msgYawLeft,         _imageYawLeft,      &RadioComponentController::_inputStickMin,          nullptr,                                        nullptr },
        { rcCalFunctionRoll,                msgRollRight,       _imageRollRight,    &RadioComponentController::_inputStickDetect,       nullptr,                                        nullptr },
        { rcCalFunctionRoll,                msgRollLeft,        _imageRollLeft,     &RadioComponentController::_inputStickMin,          nullptr,                                        nullptr },
        { rcCalFunctionPitch,               msgPitchUp,         _imagePitchUp,      &RadioComponentController::_inputStickDetect,       nullptr,                                        nullptr },
        { rcCalFunctionPitch,               msgPitchDown,       _imagePitchDown,    &RadioComponentController::_inputStickMin,          nullptr,                                        nullptr },
        { rcCalFunctionPitch,               msgPitchCenter,     _imageHome,         &RadioComponentController::_inputCenterWait,        nullptr,                                        nullptr },
        { rcCalFunctionMax,                 msgSwitchMinMax,    _imageSwitchMinMax, &RadioComponentController::_inputSwitchMinMax,      &RadioComponentController::_advanceState,       nullptr },
        { rcCalFunctionMax,                 msgComplete,        _imageThrottleDown, nullptr,                                            &RadioComponentController::_writeCalibration,   nullptr },
    };

    static const stateMachineEntry rgStateMachineAPM[] = {
        //Function
        { rcCalFunctionMax,                 msgBeginAPM,        _imageHome,         &RadioComponentController::_inputCenterWaitBegin,   &RadioComponentController::_saveAllTrims,       nullptr },
        { rcCalFunctionThrottle,            msgThrottleUp,      _imageThrottleUp,   &RadioComponentController::_inputStickDetect,       nullptr,                                        nullptr },
        { rcCalFunctionThrottle,            msgThrottleDown,    _imageThrottleDown, &RadioComponentController::_inputStickMin,          nullptr,                                        nullptr },
        { rcCalFunctionYaw,                 msgYawRight,        _imageYawRight,     &RadioComponentController::_inputStickDetect,       nullptr,                                        nullptr },
        { rcCalFunctionYaw,                 msgYawLeft,         _imageYawLeft,      &RadioComponentController::_inputStickMin,          nullptr,                                        nullptr },
        { rcCalFunctionRoll,                msgRollRight,       _imageRollRight,    &RadioComponentController::_inputStickDetect,       nullptr,                                        nullptr },
        { rcCalFunctionRoll,                msgRollLeft,        _imageRollLeft,     &RadioComponentController::_inputStickMin,          nullptr,                                        nullptr },
        { rcCalFunctionPitch,               msgPitchUp,         _imagePitchUp,      &RadioComponentController::_inputStickDetect,       nullptr,                                        nullptr },
        { rcCalFunctionPitch,               msgPitchDown,       _imagePitchDown,    &RadioComponentController::_inputStickMin,          nullptr,                                        nullptr },
        { rcCalFunctionPitch,               msgPitchCenter,     _imageHome,         &RadioComponentController::_inputCenterWait,        nullptr,                                        nullptr },
        { rcCalFunctionMax,                 msgSwitchMinMax,    _imageSwitchMinMax, &RadioComponentController::_inputSwitchMinMax,      &RadioComponentController::_advanceState,       nullptr },
        { rcCalFunctionMax,                 msgComplete,        _imageThrottleDown, nullptr,                                            &RadioComponentController::_writeCalibration,   nullptr },
    };

    bool badStep = false;
    if (step < 0) {
        badStep = true;
    }
    if (_px4Vehicle()) {
        if (step >= static_cast<int>(sizeof(rgStateMachinePX4) / sizeof(rgStateMachinePX4[0]))) {
            badStep = true;
        }
    } else {
        if (step >= static_cast<int>(sizeof(rgStateMachineAPM) / sizeof(rgStateMachineAPM[0]))) {
            badStep = true;
        }
    }
    if (badStep) {
        qWarning() << "Bad step value" << step;
        step = 0;
    }

    const stateMachineEntry* stateMachine = _px4Vehicle() ? rgStateMachinePX4 : rgStateMachineAPM;

    return &stateMachine[step];
}

void RadioComponentController::_advanceState(void)
{
    _currentStep++;
    _setupCurrentState();
}


/// @brief Sets up the state machine according to the current step from _currentStep.
void RadioComponentController::_setupCurrentState(void)
{
    static const char* msgBeginAPMRover = QT_TR_NOOP("Center the Throttle stick as shown in diagram.\nReset all transmitter trims to center.\n\n"
                                          "Please ensure all motor power is disconnected from the vehicle.\n\n"
                                          "Click Next to continue");
    const stateMachineEntry* state = _getStateMachineEntry(_currentStep);

    const char* instructions = state->instructions;
    const char* helpImage = state->image;
    if (_vehicle->rover() && _currentStep == 0) {
        // Hack in center throttle start for Rover. This is to set the correct centered trim for throttle.
        instructions = msgBeginAPMRover;
        helpImage = _imageCenter;
    }
    _statusText->setProperty("text", instructions);
    _setHelpImage(helpImage);

    _stickDetectChannel = _chanMax;
    _stickDetectSettleStarted = false;

    _rcCalSaveCurrentValues();

    _nextButton->setEnabled(state->nextFn != nullptr);
    _skipButton->setEnabled(state->skipFn != nullptr);
}

/// Connected to Vehicle::rcChannelsChanged signal
void RadioComponentController::_rcChannelsChanged(int channelCount, int pwmValues[Vehicle::cMaxRcChannels])
{
    for (int channel=0; channel<channelCount; channel++) {
        int channelValue = pwmValues[channel];

        if (channelValue != -1) {
            qCDebug(RadioComponentControllerVerboseLog) << "Raw value" << channel << channelValue;

            _rcRawValue[channel] = channelValue;
            emit channelRCValueChanged(channel, channelValue);

            // Signal attitude rc values to Qml if mapped
            if (_rgChannelInfo[channel].function != rcCalFunctionMax) {
                switch (_rgChannelInfo[channel].function) {
                case rcCalFunctionRoll:
                    emit rollChannelRCValueChanged(channelValue);
                    break;
                case rcCalFunctionPitch:
                    emit pitchChannelRCValueChanged(channelValue);
                    break;
                case rcCalFunctionYaw:
                    emit yawChannelRCValueChanged(channelValue);
                    break;
                case rcCalFunctionThrottle:
                    emit throttleChannelRCValueChanged(channelValue);
                    break;
                default:
                    break;

                }
            }

            if (_currentStep == -1) {
                if (_chanCount != channelCount) {
                    _chanCount = channelCount;
                    emit channelCountChanged(_chanCount);
                }
            } else {
                const stateMachineEntry* state = _getStateMachineEntry(_currentStep);
                if (state) {
                    if (state->rcInputFn) {
                        (this->*state->rcInputFn)(state->function, channel, channelValue);
                    }
                } else {
                    qWarning() << "Internal error: nullptr _getStateMachineEntry return";
                }
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
                qgcApp()->showAppMessage(QString("Detected %1 radio channels. To operate PX4, you need at least %2 channels.").arg(_chanCount).arg(_chanMinimum));
            }
            return;
        }
        _startCalibration();
    } else {
        const stateMachineEntry* state = _getStateMachineEntry(_currentStep);
        if (state && state->nextFn) {
            (this->*state->nextFn)();
        } else {
            qWarning() << "Internal error: nullptr _getStateMachineEntry return";
        }
    }
}

void RadioComponentController::skipButtonClicked(void)
{
    if (_currentStep == -1) {
        qWarning() << "Internal error: _currentStep == -1";
        return;
    }

    const stateMachineEntry* state = _getStateMachineEntry(_currentStep);
    if (state && state->skipFn) {
        (this->*state->skipFn)();
    } else {
        qWarning() << "Internal error: nullptr _getStateMachineEntry return";
    }
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
        qCDebug(RadioComponentControllerLog) << "_saveAllTrims channel trim" << i<< _rcRawValue[i];
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

        qCDebug(RadioComponentControllerLog) << "_stickSettleComplete Still moving, _stickDetectValue:value" << _stickDetectValue << value;

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

            qCDebug(RadioComponentControllerLog) << "_stickSettleComplete Starting settle timer, _stickDetectValue:value" << _stickDetectValue << value;

            _stickDetectSettleStarted = true;
            _stickDetectSettleElapsed.start();
        }
    }

    return false;
}

void RadioComponentController::_inputStickDetect(enum rcCalFunctions function, int channel, int value)
{
    // If this channel is already used in a mapping we can't use it again
    if (_rgChannelInfo[channel].function != rcCalFunctionMax) {
        return;
    }

    QString functionParamName = _functionInfo()[function].parameterName;
    qCDebug(RadioComponentControllerLog) << "_inputStickDetect function:channel:value" << functionParamName << channel << value;

    if (_stickDetectChannel == _chanMax) {
        // We have not detected enough movement on a channel yet

        if (abs(_rcValueSave[channel] - value) > _rcCalMoveDelta) {
            // Stick has moved far enough to consider it as being selected for the function

            qCDebug(RadioComponentControllerLog) << "_inputStickDetect Starting settle wait";

            // Setup up to detect stick being pegged to min or max value
            _stickDetectChannel = channel;
        }
    } else if (channel == _stickDetectChannel) {
        if (_stickSettleComplete(value)) {
            ChannelInfo* info = &_rgChannelInfo[channel];

            // Map the channel to the function
            _rgFunctionChannelMapping[function] = channel;
            info->function = function;

            // A non-reversed channel should show a higher PWM value than center.
            info->reversed = value < _rcValueSave[channel];
            if (info->reversed) {
                _rgChannelInfo[channel].rcMin = value;
            } else {
                _rgChannelInfo[channel].rcMax = value;
            }

            qCDebug(RadioComponentControllerLog) << "_inputStickDetect Settle complete, reversed:" << info->reversed;

            _signalAllAttitudeValueChanges();

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

    QString functionParamName = _functionInfo()[function].parameterName;
    qCDebug(RadioComponentControllerLog) << "_inputStickMin function:channel:value" << functionParamName << channel << value;

    if (_stickDetectChannel == _chanMax) {
        // Setup up to detect stick being pegged to extreme position
        if (_rgChannelInfo[channel].reversed) {
            if (value > _rcCalPWMCenterPoint + _rcCalMoveDelta) {
                qCDebug(RadioComponentControllerLog) << "_inputStickMin Movement detected, starting settle wait";
                _stickDetectChannel = channel;
                _stickDetectValue = value;
            }
        } else {
            if (value < _rcCalPWMCenterPoint - _rcCalMoveDelta) {
                qCDebug(RadioComponentControllerLog) << "_inputStickMin Movement detected, starting settle wait";
                _stickDetectChannel = channel;
                _stickDetectValue = value;
            }
        }
    } else {
        // We are waiting for the selected channel to settle out
        if (_stickSettleComplete(value)) {
            ChannelInfo* info = &_rgChannelInfo[channel];

            // Stick detection is complete. Stick should be at extreme position.
            if (info->reversed) {
                _rgChannelInfo[channel].rcMax = value;
            } else {
                _rgChannelInfo[channel].rcMin = value;
            }

            qCDebug(RadioComponentControllerLog) << "_inputStickMin Settle complete";

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

    QString functionParamName = _functionInfo()[function].parameterName;
    qCDebug(RadioComponentControllerLog) << "_inputCenterWait function:channel:value" << functionParamName << channel << value;

    if (_stickDetectChannel == _chanMax) {
        // Sticks have not yet moved close enough to center

        if (abs(_rcCalPWMCenterPoint - value) < _rcCalRoughCenterDelta) {
            // Stick has moved close enough to center that we can start waiting for it to settle
            qCDebug(RadioComponentControllerLog) << "_inputCenterWait Center detected. Waiting for settle.";
            _stickDetectChannel = channel;
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

/// @brief Resets internal calibration values to their initial state in preparation for a new calibration sequence.
void RadioComponentController::_resetInternalCalibrationValues(void)
{
    // Set all raw channels to not reversed and center point values
    for (int i=0; i<_chanMax; i++) {
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

    _signalAllAttitudeValueChanges();
}

/// @brief Sets internal calibration values from the stored parameters
void RadioComponentController::_setInternalCalibrationValuesFromParameters(void)
{
    // Initialize all function mappings to not set

    for (int i=0; i<_chanMax; i++) {
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

    for (int i = 0; i < _chanMax; ++i) {
        struct ChannelInfo* info = &_rgChannelInfo[i];

        if (!parameterExists(FactSystem::defaultComponentId, minTpl.arg(i+1))) {
            info->rcTrim = 1500;
            info->rcMin = 1100;
            info->rcMax = 1900;
            info->reversed = false;
            continue;
        }

        Fact* paramFact = getParameterFact(FactSystem::defaultComponentId, trimTpl.arg(i+1));
        if (paramFact) {
            info->rcTrim = paramFact->rawValue().toInt();
        }

        paramFact = getParameterFact(FactSystem::defaultComponentId, minTpl.arg(i+1));
        if (paramFact) {
            info->rcMin = paramFact->rawValue().toInt();
        }

        paramFact = getParameterFact(FactSystem::defaultComponentId, maxTpl.arg(i+1));
        if (paramFact) {
            info->rcMax = getParameterFact(FactSystem::defaultComponentId, maxTpl.arg(i+1))->rawValue().toInt();
        }

        info->reversed = _channelReversedParamValue(i);
    }

    for (int i=0; i<rcCalFunctionMax; i++) {
        int32_t paramChannel;

        const char* paramName = _functionInfo()[i].parameterName;
        if (paramName) {
            Fact* paramFact = getParameterFact(FactSystem::defaultComponentId, paramName);
            if (paramFact) {
                paramChannel = paramFact->rawValue().toInt();

                if (paramChannel > 0 && paramChannel <= _chanMax) {
                    _rgFunctionChannelMapping[i] = paramChannel - 1;
                    _rgChannelInfo[paramChannel - 1].function = static_cast<rcCalFunctions>(i);
                }
            }
        }
    }

    _signalAllAttitudeValueChanges();
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

    if (!_px4Vehicle() && (_vehicle->vehicleType() == MAV_TYPE_HELICOPTER || _vehicle->multiRotor()) &&  _rgChannelInfo[_rgFunctionChannelMapping[rcCalFunctionThrottle]].reversed) {
        // A reversed throttle could lead to dangerous power up issues if the firmware doesn't handle it absolutely correctly in all places.
        // So in this case fail the calibration for anything other than PX4 which is known to be able to handle this correctly.
        emit throttleReversedCalFailure();
    } else {
        _validateCalibration();

        QString minTpl("RC%1_MIN");
        QString maxTpl("RC%1_MAX");
        QString trimTpl("RC%1_TRIM");

        // Note that the rc parameters are all float, so you must cast to float in order to get the right QVariant
        for (int chan = 0; chan<_chanMax; chan++) {
            struct ChannelInfo* info = &_rgChannelInfo[chan];
            int                 oneBasedChannel = chan + 1;

            if (!parameterExists(FactSystem::defaultComponentId, minTpl.arg(chan+1))) {
                continue;
            }

            Fact* paramFact = getParameterFact(FactSystem::defaultComponentId, trimTpl.arg(oneBasedChannel));
            if (paramFact) {
                paramFact->setRawValue(static_cast<float>(info->rcTrim));
            }
            paramFact = getParameterFact(FactSystem::defaultComponentId, minTpl.arg(oneBasedChannel));
            if (paramFact) {
                paramFact->setRawValue(static_cast<float>(info->rcMin));
            }
            paramFact = getParameterFact(FactSystem::defaultComponentId, maxTpl.arg(oneBasedChannel));
            if (paramFact) {
                paramFact->setRawValue(static_cast<float>(info->rcMax));
            }

            // For multi-rotor we can determine reverse setting during radio cal. For anything other than multi-rotor, servo installation
            // may affect channel reversing so we can't automatically determine it. This is ok for PX4 given how it uses mixers, but not for ArduPilot.
            if (_vehicle->px4Firmware() || _vehicle->multiRotor()) {
                // APM multi-rotor has a backwards interpretation of "reversed" on the Pitch control. So be careful.
                bool reversed;
                if (_px4Vehicle() || info->function != rcCalFunctionPitch) {
                    reversed = info->reversed;
                } else {
                    reversed = !info->reversed;
                }
                _setChannelReversedParamValue(chan, reversed);
            }
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
            const char* paramName = _functionInfo()[i].parameterName;
            if (paramName) {
                Fact* paramFact = getParameterFact(FactSystem::defaultComponentId, _functionInfo()[i].parameterName);

                if (paramFact && paramFact->rawValue().toInt() != paramChannel) {
                    paramFact = getParameterFact(FactSystem::defaultComponentId, _functionInfo()[i].parameterName);
                    if (paramFact) {
                        paramFact->setRawValue(paramChannel);
                    }
                }
            }
        }
    }

    if (_px4Vehicle()) {
        // If the RC_CHAN_COUNT parameter is available write the channel count
        if (parameterExists(FactSystem::defaultComponentId, QStringLiteral("RC_CHAN_CNT"))) {
            getParameterFact(FactSystem::defaultComponentId, QStringLiteral("RC_CHAN_CNT"))->setRawValue(_chanCount);
        }
    }

    _stopCalibration();
    _setInternalCalibrationValuesFromParameters();
}

/// @brief Starts the calibration process
void RadioComponentController::_startCalibration(void)
{
    if (_chanCount < _chanMinimum) {
        qWarning() << "Call to RadioComponentController::_startCalibration with _chanCount < _chanMinimum";
        return;
    }

    _resetInternalCalibrationValues();

    // Let the mav known we are starting calibration. This should turn off motors and so forth.
    _vehicle->startCalibration(Vehicle::CalibrationRadio);

    _nextButton->setProperty("text", tr("Next"));
    _cancelButton->setEnabled(true);

    _currentStep = 0;
    _setupCurrentState();
}

/// @brief Cancels the calibration process, setting things back to initial state.
void RadioComponentController::_stopCalibration(void)
{
    _currentStep = -1;

    if (_uas) {
        // Only PX4 is known to support this command in all versions. For other firmware which may or may not
        // support this we don't show errors on failure.
        _vehicle->stopCalibration(_px4Vehicle() ? true : false /* showError */);
        _setInternalCalibrationValuesFromParameters();
    } else {
        _resetInternalCalibrationValues();
    }

    if(_statusText)   _statusText->setProperty("text", "");
    if(_nextButton)   _nextButton->setProperty("text", tr("Calibrate"));
    if(_nextButton)   _nextButton->setEnabled(true);
    if(_cancelButton) _cancelButton->setEnabled(false);
    if(_skipButton)   _skipButton->setEnabled(false);

    _setHelpImage(_imageCenter);
}

/// @brief Saves the current channel values, so that we can detect when the use moves an input.
void RadioComponentController::_rcCalSaveCurrentValues(void)
{
    for (int i = 0; i < _chanMax; i++) {
        _rcValueSave[i] = _rcRawValue[i];
        qCDebug(RadioComponentControllerLog) << "_rcCalSaveCurrentValues channel:value" << i << _rcValueSave[i];
    }
}

/// @brief Set up the Save state of calibration.
void RadioComponentController::_rcCalSave(void)
{
    _rcCalState = rcCalStateSave;

    if(_statusText) _statusText->setProperty(
        "text",
        tr("The current calibration settings are now displayed for each channel on screen.\n\n"
        "Click the Next button to upload calibration to board. Click Cancel if you don't want to save these values."));

    if(_nextButton)     _nextButton->setEnabled(true);
    if(_skipButton)     _skipButton->setEnabled(false);
    if(_cancelButton)   _cancelButton->setEnabled(true);

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

    if (!(_transmitterMode == 1 || _transmitterMode == 2)) {
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
        qWarning() << "Internal error: Bad _transmitterMode value";
        return;
    }
    file += imageFile;

    qCDebug(RadioComponentControllerLog) << "_setHelpImage" << file;

    _imageHelp = file;
    emit imageHelpChanged(file);
}

int RadioComponentController::channelCount(void) const
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

void RadioComponentController::_signalAllAttitudeValueChanges(void)
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
    _vehicle->startCalibration(Vehicle::CalibrationCopyTrims);
}

bool RadioComponentController::_px4Vehicle(void) const
{
    return _vehicle->firmwareType() == MAV_AUTOPILOT_PX4;
}

const struct RadioComponentController::FunctionInfo* RadioComponentController::_functionInfo(void) const
{
    return _px4Vehicle() ? _rgFunctionInfoPX4 : _rgFunctionInfoAPM;
}

bool RadioComponentController::_channelReversedParamValue(int channel)
{
    Fact* paramFact = getParameterFact(FactSystem::defaultComponentId, _revParamFormat.arg(channel+1));
    if (paramFact) {
        if (_revParamIsBool) {
            return paramFact->rawValue().toBool();
        } else {
            bool convertOk;
            float floatReversed = paramFact->rawValue().toFloat(&convertOk);
            if (!convertOk) {
                floatReversed = 1.0f;
            }
            return floatReversed == -1.0f;
        }
    }

    return false;
}

void RadioComponentController::_setChannelReversedParamValue(int channel, bool reversed)
{
    Fact* paramFact = getParameterFact(FactSystem::defaultComponentId, _revParamFormat.arg(channel+1));
    if (paramFact) {
        if (_revParamIsBool) {
            paramFact->setRawValue(reversed);
        } else {
            paramFact->setRawValue(reversed ? -1.0f : 1.0f);
        }
    }
}
