#include "RemoteControlCalibrationController.h"
#include "Fact.h"
#include "ParameterManager.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

#include <QtCore/QSettings>

QGC_LOGGING_CATEGORY(RemoteControlCalibrationControllerLog, "RemoteControl.RemoteControlCalibrationController")
QGC_LOGGING_CATEGORY(RemoteControlCalibrationControllerVerboseLog, "RemoteControl.RemoteControlCalibrationController:verbose")

static constexpr const char *msgBeginThrottleDown = QT_TR_NOOP(
        "* Lower the Throttle stick all the way down as shown in diagram\n"
        "* Please ensure all motor power is disconnected AND all props are removed from the vehicle.\n"
        "* Click Next to continue"
);
static constexpr const char *msgBeginThrottleCenter = QT_TR_NOOP(
    "* Center the Throttle stick as shown in diagram.\n"
    "* Please ensure all motor power is disconnected from the vehicle.\n"
    "* Click Next to continue"
);
static constexpr const char *msgThrottleUp =    QT_TR_NOOP("Move the Throttle stick all the way up and hold it there...");
static constexpr const char *msgThrottleDown =  QT_TR_NOOP("Move the Throttle stick all the way down and leave it there...");
static constexpr const char *msgYawLeft =       QT_TR_NOOP("Move the Yaw stick all the way to the left and hold it there...");
static constexpr const char *msgYawRight =      QT_TR_NOOP("Move the Yaw stick all the way to the right and hold it there...");
static constexpr const char *msgRollLeft =      QT_TR_NOOP("Move the Roll stick all the way to the left and hold it there...");
static constexpr const char *msgRollRight =     QT_TR_NOOP("Move the Roll stick all the way to the right and hold it there...");
static constexpr const char *msgPitchDown =     QT_TR_NOOP("Move the Pitch stick all the way down and hold it there...");
static constexpr const char *msgPitchUp =       QT_TR_NOOP("Move the Pitch stick all the way up and hold it there...");
static constexpr const char *msgPitchCenter =   QT_TR_NOOP("Allow the Pitch stick to move back to center...");
static constexpr const char *msgSwitchMinMax =  QT_TR_NOOP("Move all the transmitter switches and/or dials back and forth to their extreme positions.");
static constexpr const char *msgComplete =      QT_TR_NOOP("All settings have been captured. Click Next to write the new parameters to your board.");

RemoteControlCalibrationController::RemoteControlCalibrationController(QObject *parent)
    : FactPanelController(parent)
    , _stateMachine{
        { stickFunctionMax,         StateMachineStepStickNeutral,   &RemoteControlCalibrationController::_inputCenterWaitBegin,   &RemoteControlCalibrationController::_saveAllTrims, nullptr },
        { stickFunctionThrottle,    StateMachineStepThrottleUp,     &RemoteControlCalibrationController::_inputStickDetect,       nullptr,                                            nullptr },
        { stickFunctionThrottle,    StateMachineStepThrottleDown,   &RemoteControlCalibrationController::_inputStickMin,          nullptr,                                            nullptr },
        { stickFunctionYaw,         StateMachineStepYawRight,       &RemoteControlCalibrationController::_inputStickDetect,       nullptr,                                            nullptr },
        { stickFunctionYaw,         StateMachineStepYawLeft,        &RemoteControlCalibrationController::_inputStickMin,          nullptr,                                            nullptr },
        { stickFunctionRoll,        StateMachineStepRollRight,      &RemoteControlCalibrationController::_inputStickDetect,       nullptr,                                            nullptr },
        { stickFunctionRoll,        StateMachineStepRollLeft,       &RemoteControlCalibrationController::_inputStickMin,          nullptr,                                            nullptr },
        { stickFunctionPitch,       StateMachineStepPitchUp,        &RemoteControlCalibrationController::_inputStickDetect,       nullptr,                                            nullptr },
        { stickFunctionPitch,       StateMachineStepPitchDown,      &RemoteControlCalibrationController::_inputStickMin,          nullptr,                                            nullptr },
        { stickFunctionPitch,       StateMachineStepPitchCenter,    &RemoteControlCalibrationController::_inputCenterWait,        nullptr,                                            nullptr },
        { stickFunctionMax,         StateMachineStepSwitchMinMax,   &RemoteControlCalibrationController::_inputSwitchMinMax,      &RemoteControlCalibrationController::_advanceState, nullptr },
        { stickFunctionMax,         StateMachineStepComplete,       nullptr,                                                      &RemoteControlCalibrationController::_saveStoredCalibrationValues, nullptr },
    }
{
    // qCDebug(RemoteControlCalibrationControllerLog) << Q_FUNC_INFO << this;

    _loadCalibrationUISettings();

    _resetInternalCalibrationValues();

    _stickDisplayPositions = { _stickDisplayPositionCentered.horizontal, _stickDisplayPositionCentered.vertical,
                               _stickDisplayPositionCentered.horizontal, _stickDisplayPositionCentered.vertical };

    if (_vehicle->rover()) {
        _centeredThrottle = true;
    }

    _stepFunctionToMsgStringMap = {
        { StateMachineStepStickNeutral,    msgBeginThrottleCenter },    // First entry must be adjusted based on throttle centered or not
        { StateMachineStepThrottleUp,      msgThrottleUp },
        { StateMachineStepThrottleDown,    msgThrottleDown },
        { StateMachineStepYawRight,        msgYawRight },
        { StateMachineStepYawLeft,         msgYawLeft },
        { StateMachineStepRollRight,       msgRollRight },
        { StateMachineStepRollLeft,        msgRollLeft },
        { StateMachineStepPitchUp,         msgPitchUp },
        { StateMachineStepPitchDown,       msgPitchDown },
        { StateMachineStepPitchCenter,     msgPitchCenter },
        { StateMachineStepSwitchMinMax,    msgSwitchMinMax },
        { StateMachineStepComplete,        msgComplete },
    };

    // Map for throttle centered neutral position
    _bothStickDisplayPositionThrottleCenteredMap = {
        { StateMachineStepStickNeutral, {
            { 1, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
        }},
        { StateMachineStepThrottleUp, {
            { 1, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYUp } },
            { 2, { _stickDisplayPositionXCenteredYUp,   _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYUp } },
            { 4, { _stickDisplayPositionXCenteredYUp,   _stickDisplayPositionCentered } },
        }},
        { StateMachineStepThrottleDown, {
            { 1, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYDown } },
            { 2, { _stickDisplayPositionXCenteredYDown, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYDown } },
            { 4, { _stickDisplayPositionXCenteredYDown, _stickDisplayPositionCentered } },
        }},
        { StateMachineStepYawRight, {
            { 1, { _stickDisplayPositionXRightYCentered,    _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionXRightYCentered,    _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered,           _stickDisplayPositionXRightYCentered } },
            { 4, { _stickDisplayPositionCentered,           _stickDisplayPositionXRightYCentered } },
        }},
        { StateMachineStepYawLeft, {
            { 1, { _stickDisplayPositionXLeftYCentered, _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionXLeftYCentered, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered,       _stickDisplayPositionXLeftYCentered } },
            { 4, { _stickDisplayPositionCentered,       _stickDisplayPositionXLeftYCentered } },
        }},
        { StateMachineStepRollRight, {
            { 1, { _stickDisplayPositionCentered,           _stickDisplayPositionXRightYCentered } },
            { 2, { _stickDisplayPositionCentered,           _stickDisplayPositionXRightYCentered } },
            { 3, { _stickDisplayPositionXRightYCentered,    _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionXRightYCentered,    _stickDisplayPositionCentered } },
        }},
        { StateMachineStepRollLeft, {
            { 1, { _stickDisplayPositionCentered,       _stickDisplayPositionXLeftYCentered } },
            { 2, { _stickDisplayPositionCentered,       _stickDisplayPositionXLeftYCentered } },
            { 3, { _stickDisplayPositionXLeftYCentered, _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionXLeftYCentered, _stickDisplayPositionCentered } },
        }},
        { StateMachineStepPitchUp, {
            { 1, { _stickDisplayPositionXCenteredYUp,   _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYUp } },
            { 3, { _stickDisplayPositionXCenteredYUp,   _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYUp } },
        }},
        { StateMachineStepPitchDown, {
            { 1, { _stickDisplayPositionXCenteredYDown, _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYDown } },
            { 3, { _stickDisplayPositionXCenteredYDown, _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYDown } },
        }},
        { StateMachineStepPitchCenter, {
            { 1, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
        }},
        { StateMachineStepSwitchMinMax, {
            { 1, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
        }},
        { StateMachineStepComplete, {
            { 1, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
        }},
    };

    // Map for throttle down neutral position
    _bothStickDisplayPositionThrottleDownMap = {
        { StateMachineStepStickNeutral, {
            { 1, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYDown } },
            { 2, { _stickDisplayPositionXCenteredYDown, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYDown } },
            { 4, { _stickDisplayPositionXCenteredYDown, _stickDisplayPositionCentered } },
        }},
        { StateMachineStepThrottleUp, {
            { 1, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYUp } },
            { 2, { _stickDisplayPositionXCenteredYUp,   _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYUp } },
            { 4, { _stickDisplayPositionXCenteredYUp,   _stickDisplayPositionCentered } },
        }},
        { StateMachineStepThrottleDown, {
            { 1, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYDown } },
            { 2, { _stickDisplayPositionXCenteredYDown, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYDown } },
            { 4, { _stickDisplayPositionXCenteredYDown, _stickDisplayPositionCentered } },
        }},
        { StateMachineStepYawRight, {
            { 1, { _stickDisplayPositionXRightYCentered,    _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionXRightYCentered,    _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered,           _stickDisplayPositionXRightYCentered } },
            { 4, { _stickDisplayPositionCentered,           _stickDisplayPositionXRightYCentered } },
        }},
        { StateMachineStepYawLeft, {
            { 1, { _stickDisplayPositionXLeftYCentered, _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionXLeftYCentered, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered,       _stickDisplayPositionXLeftYCentered } },
            { 4, { _stickDisplayPositionCentered,       _stickDisplayPositionXLeftYCentered } },
        }},
        { StateMachineStepRollRight, {
            { 1, { _stickDisplayPositionCentered,           _stickDisplayPositionXRightYCentered } },
            { 2, { _stickDisplayPositionCentered,           _stickDisplayPositionXRightYCentered } },
            { 3, { _stickDisplayPositionXRightYCentered,    _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionXRightYCentered,    _stickDisplayPositionCentered } },
        }},
        { StateMachineStepRollLeft, {
            { 1, { _stickDisplayPositionCentered,       _stickDisplayPositionXLeftYCentered } },
            { 2, { _stickDisplayPositionCentered,       _stickDisplayPositionXLeftYCentered } },
            { 3, { _stickDisplayPositionXLeftYCentered, _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionXLeftYCentered, _stickDisplayPositionCentered } },
        }},
        { StateMachineStepPitchUp, {
            { 1, { _stickDisplayPositionXCenteredYUp,   _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYUp } },
            { 3, { _stickDisplayPositionXCenteredYUp,   _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYUp } },
        }},
        { StateMachineStepPitchDown, {
            { 1, { _stickDisplayPositionXCenteredYDown, _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYDown } },
            { 3, { _stickDisplayPositionXCenteredYDown, _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYDown } },
        }},
        { StateMachineStepPitchCenter, {
            { 1, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
        }},
        { StateMachineStepSwitchMinMax, {
            { 1, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
        }},
        { StateMachineStepComplete, {
            { 1, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
        }},
    };
}

RemoteControlCalibrationController::~RemoteControlCalibrationController()
{
    _saveCalibrationUISettings();

    // qCDebug(RemoteControlCalibrationControllerLog) << Q_FUNC_INFO << this;
}

void RemoteControlCalibrationController::start()
{
    _stopCalibration();
    _readStoredCalibrationValues();
}

const RemoteControlCalibrationController::StateMachineEntry &RemoteControlCalibrationController::_getStateMachineEntry(int step) const
{
    if (step < 0 || step >= _stateMachine.size()) {
        qCWarning(RemoteControlCalibrationControllerLog) << "Bad step value" << step;
        step = 0;
    }

    return _stateMachine[step];
}

void RemoteControlCalibrationController::_advanceState()
{
    _currentStep++;
    _setupCurrentState();
}

void RemoteControlCalibrationController::_setupCurrentState()
{


    auto state = _getStateMachineEntry(_currentStep);

    _stepFunctionToMsgStringMap[StateMachineStepStickNeutral] = _centeredThrottle ? msgBeginThrottleCenter : msgBeginThrottleDown;

    BothSticksDisplayPositions defaultPositions = { _stickDisplayPositionCentered, _stickDisplayPositionCentered };
    BothSticksDisplayPositions bothStickPositions = _centeredThrottle
        ? _bothStickDisplayPositionThrottleCenteredMap.value(state.stepFunction).value(_transmitterMode, defaultPositions)
        : _bothStickDisplayPositionThrottleDownMap.value(state.stepFunction).value(_transmitterMode, defaultPositions);

    _statusText->setProperty("text", _stepFunctionToMsgStringMap.value(state.stepFunction, QString()));
    _stickDisplayPositions = { bothStickPositions.leftStick.horizontal, bothStickPositions.leftStick.vertical,
                               bothStickPositions.rightStick.horizontal, bothStickPositions.rightStick.vertical };
    emit stickDisplayPositionsChanged();

    _stickDetectChannel = _chanMax;
    _stickDetectSettleStarted = false;

    _saveCurrentRawValues();

    _nextButton->setEnabled(state.nextButtonFn != nullptr);
    _skipButton->setEnabled(state.skipButtonFn != nullptr);
}

void RemoteControlCalibrationController::channelValuesChanged(int channelCount, int pwmValues[QGCMAVLink::maxRcChannels])
{
    // Below is a hack that's needed by ELRS
    // ELRS is not sending a full RC_CHANNELS packet, only channel update
    // packets via RC_CHANNELS_RAW, to update the position of the values.
    // Therefore, the number of channels is not set.
    if (channelCount == 0) {
        for (int channel=0; channel<16; channel++) {
            if (pwmValues[channel] != INT16_MAX) channelCount++;
        }
    }

    for (int channel=0; channel<channelCount; channel++) {
        const int channelValue = pwmValues[channel];

        if (channelValue != -1) {
            qCDebug(RemoteControlCalibrationControllerVerboseLog) << "Raw value" << channel << channelValue;

            _channelRawValue[channel] = channelValue;
            emit channelValueChanged(channel, channelValue);

            // Signal attitude rc values to Qml if mapped
            if (_rgChannelInfo[channel].function != stickFunctionMax) {
                switch (_rgChannelInfo[channel].function) {
                case stickFunctionRoll:
                    emit rollChannelValueChanged(channelValue);
                    break;
                case stickFunctionPitch:
                    emit pitchChannelValueChanged(channelValue);
                    break;
                case stickFunctionYaw:
                    emit yawChannelValueChanged(channelValue);
                    break;
                case stickFunctionThrottle:
                    emit throttleChannelValueChanged(channelValue);
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
                auto state = _getStateMachineEntry(_currentStep);
                if (state.channelInputFn) {
                    (this->*state.channelInputFn)(state.function, channel, channelValue);
                }
            }
        }
    }
}

void RemoteControlCalibrationController::nextButtonClicked()
{
    if (_currentStep == -1) {
        // Need to have enough channels
        if (_chanCount < _chanMinimum) {
            qgcApp()->showAppMessage(QStringLiteral("Detected %1 channels. To operate vehicle, you need at least %2 channels.").arg(_chanCount).arg(_chanMinimum));
            return;
        }
        _startCalibration();
    } else {
        auto state = _getStateMachineEntry(_currentStep);
        if (state.nextButtonFn) {
            (this->*state.nextButtonFn)();
        }
    }
}

void RemoteControlCalibrationController::skipButtonClicked()
{
    if (_currentStep == -1) {
        qWarning() << "Internal error: _currentStep == -1";
        return;
    }

    auto state = _getStateMachineEntry(_currentStep);
    if (state.skipButtonFn) {
        (this->*state.skipButtonFn)();
    }
}

void RemoteControlCalibrationController::cancelButtonClicked()
{
    _stopCalibration();
}

void RemoteControlCalibrationController::_saveAllTrims()
{
    // We save all trims as the first step. At this point no channels are mapped but it should still
    // allow us to get good trims for the roll/pitch/yaw/throttle even though we don't know which
    // channels they are yet. AS we continue through the process the other channels will get their
    // trims reset to correct values.

    for (int i=0; i<_chanCount; i++) {
        qCDebug(RemoteControlCalibrationControllerLog) << "_saveAllTrims channel trim" << i<< _channelRawValue[i];
        _rgChannelInfo[i].rcTrim = _channelRawValue[i];
    }
    _advanceState();
}

/// @brief Waits for the sticks to be centered, enabling Next when done.
void RemoteControlCalibrationController::_inputCenterWaitBegin(StickFunction function, int chan, int value)
{
    Q_UNUSED(function);
    Q_UNUSED(chan);
    Q_UNUSED(value);

    // FIXME: Doesn't wait for center
    _nextButton->setEnabled(true);
}

bool RemoteControlCalibrationController::_stickSettleComplete(int value)
{
    // We are waiting for the stick to settle out to a max position

    if (abs(_stickDetectValue - value) > _calSettleDelta) {
        // Stick is moving too much to consider stopped

        qCDebug(RemoteControlCalibrationControllerLog) << "_stickSettleComplete Still moving, _stickDetectValue:value" << _stickDetectValue << value;

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

            qCDebug(RemoteControlCalibrationControllerLog) << "_stickSettleComplete Starting settle timer, _stickDetectValue:value" << _stickDetectValue << value;

            _stickDetectSettleStarted = true;
            _stickDetectSettleElapsed.start();
        }
    }

    return false;
}

void RemoteControlCalibrationController::_inputStickDetect(StickFunction function, int channel, int value)
{
    // If this channel is already used in a mapping we can't use it again
    if (_rgChannelInfo[channel].function != stickFunctionMax) {
        return;
    }

    qCDebug(RemoteControlCalibrationControllerLog) << "_inputStickDetect function:channel:value" << _stickFunctionToString(function) << channel << value;

    if (_stickDetectChannel == _chanMax) {
        // We have not detected enough movement on a channel yet

        if (abs(_channelValueSave[channel] - value) > _calMoveDelta) {
            // Stick has moved far enough to consider it as being selected for the function

            qCDebug(RemoteControlCalibrationControllerLog) << "_inputStickDetect Starting settle wait";

            // Setup up to detect stick being pegged to min or max value
            _stickDetectChannel = channel;
        }
    } else if (channel == _stickDetectChannel) {
        if (_stickSettleComplete(value)) {
            ChannelInfo *const info = &_rgChannelInfo[channel];

            // Map the channel to the function
            _rgFunctionChannelMapping[function] = channel;
            info->function = function;

            // A non-reversed channel should show a higher PWM value than center.
            info->reversed = value < _channelValueSave[channel];
            if (info->reversed) {
                _rgChannelInfo[channel].rcMin = value;
            } else {
                _rgChannelInfo[channel].rcMax = value;
            }

            qCDebug(RemoteControlCalibrationControllerLog) << "_inputStickDetect Settle complete, reversed:" << info->reversed;

            _signalAllAttitudeValueChanges();

            _advanceState();
        }
    }
}

void RemoteControlCalibrationController::_inputStickMin(StickFunction function, int channel, int value)
{
    // We only care about the channel mapped to the function we are working on
    if (_rgFunctionChannelMapping[function] != channel) {
        return;
    }

    qCDebug(RemoteControlCalibrationControllerLog) << "_inputStickMin function:channel:value" << _stickFunctionToString(function) << channel << value;

    if (_stickDetectChannel == _chanMax) {
        // Setup up to detect stick being pegged to extreme position
        if (_rgChannelInfo[channel].reversed) {
            if (value > _calCenterPoint + _calMoveDelta) {
                qCDebug(RemoteControlCalibrationControllerLog) << "_inputStickMin Movement detected, starting settle wait";
                _stickDetectChannel = channel;
                _stickDetectValue = value;
            }
        } else {
            if (value < _calCenterPoint - _calMoveDelta) {
                qCDebug(RemoteControlCalibrationControllerLog) << "_inputStickMin Movement detected, starting settle wait";
                _stickDetectChannel = channel;
                _stickDetectValue = value;
            }
        }
    } else {
        // We are waiting for the selected channel to settle out
        if (_stickSettleComplete(value)) {
            const ChannelInfo *const info = &_rgChannelInfo[channel];

            // Stick detection is complete. Stick should be at extreme position.
            if (info->reversed) {
                _rgChannelInfo[channel].rcMax = value;
            } else {
                _rgChannelInfo[channel].rcMin = value;
            }

            qCDebug(RemoteControlCalibrationControllerLog) << "_inputStickMin Settle complete";

            // Check if this is throttle and set trim accordingly
            if (function == stickFunctionThrottle) {
                _rgChannelInfo[channel].rcTrim = value;
            }
            // XXX to support configs which can reverse they need to check a reverse
            // flag here and not do this.

            _advanceState();
        }
    }
}

void RemoteControlCalibrationController::_inputCenterWait(StickFunction function, int channel, int value)
{
    // We only care about the channel mapped to the function we are working on
    if (_rgFunctionChannelMapping[function] != channel) {
        return;
    }

    qCDebug(RemoteControlCalibrationControllerLog) << "_inputCenterWait function:channel:value" << _stickFunctionToString(function) << channel << value;

    if (_stickDetectChannel == _chanMax) {
        // Sticks have not yet moved close enough to center

        if (abs(_calCenterPoint - value) < _calRoughCenterDelta) {
            // Stick has moved close enough to center that we can start waiting for it to settle
            qCDebug(RemoteControlCalibrationControllerLog) << "_inputCenterWait Center detected. Waiting for settle.";
            _stickDetectChannel = channel;
            _stickDetectValue = value;
        }
    } else {
        if (_stickSettleComplete(value)) {
            _advanceState();
        }
    }
}

void RemoteControlCalibrationController::_inputSwitchMinMax(StickFunction function, int channel, int value)
{
    Q_UNUSED(function);

    // If the channel is mapped we already have min/max
    if (_rgChannelInfo[channel].function != stickFunctionMax) {
        return;
    }

    if (abs(_calCenterPoint - value) > _calMoveDelta) {
        // Stick has moved far enough from center to consider for min/max
        if (value < _calCenterPoint) {
            const int minValue = qMin(_rgChannelInfo[channel].rcMin, value);

            qCDebug(RemoteControlCalibrationControllerLog) << "setting min channel:min" << channel << minValue;

            _rgChannelInfo[channel].rcMin = minValue;
        } else {
            int maxValue = qMax(_rgChannelInfo[channel].rcMax, value);

            qCDebug(RemoteControlCalibrationControllerLog) << "setting max channel:max" << channel << maxValue;

            _rgChannelInfo[channel].rcMax = maxValue;
        }
    }
}

void RemoteControlCalibrationController::_switchDetect(StickFunction function, int channel, int value, bool moveToNextStep)
{
    // If this channel is already used in a mapping we can't use it again
    if (_rgChannelInfo[channel].function != stickFunctionMax) {
        return;
    }

    if (abs(_channelValueSave[channel] - value) > _calMoveDelta) {
        ChannelInfo *const info = &_rgChannelInfo[channel];

        // Switch has moved far enough to consider it as being selected for the function

        // Map the channel to the function
        _rgChannelInfo[channel].function = function;
        _rgFunctionChannelMapping[function] = channel;
        info->function = function;

        qCDebug(RemoteControlCalibrationControllerLog) << "Function:" << function << "mapped to:" << channel;

        if (moveToNextStep) {
            _advanceState();
        }
    }
}

void RemoteControlCalibrationController::_inputSwitchDetect(StickFunction function, int channel, int value)
{
    _switchDetect(function, channel, value, true /* move to next step after detection */);
}

void RemoteControlCalibrationController::_resetInternalCalibrationValues()
{
    // Set all raw channels to not reversed and center point values
    for (int i = 0; i < _chanMax; i++) {
        ChannelInfo *const info = &_rgChannelInfo[i];
        info->function = stickFunctionMax;
        info->reversed = false;
        info->rcMin = RemoteControlCalibrationController::_calCenterPoint;
        info->rcMax = RemoteControlCalibrationController::_calCenterPoint;
        info->rcTrim = RemoteControlCalibrationController::_calCenterPoint;
    }

    // Initialize attitude function mapping to function channel not set
    for (size_t i = 0; i < stickFunctionMax; i++) {
        _rgFunctionChannelMapping[i] = _chanMax;
    }

    _signalAllAttitudeValueChanges();
}

void RemoteControlCalibrationController::_validateAndAdjustCalibrationValues()
{
    for (int chan = 0; chan<_chanMax; chan++) {
        ChannelInfo *info = &_rgChannelInfo[chan];

        if (chan < _chanCount) {
            // Validate Min/Max values. Although the channel appears as available we still may
            // not have good min/max/trim values for it. Set to defaults if needed.
            if (info->rcMin > _calValidMinValue || info->rcMax < _calValidMaxValue) {
                qCDebug(RemoteControlCalibrationControllerLog) << "_validateAndAdjustCalibrationValues resetting channel" << chan;
                info->rcMin = _calDefaultMinValue;
                info->rcMax = _calDefaultMaxValue;
                info->rcTrim = info->rcMin + ((info->rcMax - info->rcMin) / 2);
            } else {
                switch (_rgChannelInfo[chan].function) {
                case stickFunctionThrottle:
                case stickFunctionYaw:
                case stickFunctionRoll:
                case stickFunctionPitch:
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
            qCDebug(RemoteControlCalibrationControllerLog) << "_validateAndAdjustCalibrationValues resetting unavailable channel" << chan;
            info->rcMin = _calDefaultMinValue;
            info->rcMax = _calDefaultMaxValue;
            info->rcTrim = info->rcMin + ((info->rcMax - info->rcMin) / 2);
            info->reversed = false;
        }
    }
}

void RemoteControlCalibrationController::_startCalibration()
{
    if (_chanCount < _chanMinimum) {
        qCWarning(RemoteControlCalibrationControllerLog) << "Call to RemoteControlCalibrationController::_startCalibration with _chanCount < _chanMinimum";
        return;
    }

    _resetInternalCalibrationValues();

    // Let the mav known we are starting calibration. This should turn off motors and so forth.
    _vehicle->startCalibration(QGCMAVLink::CalibrationRadio);

    _nextButton->setProperty("text", tr("Next"));
    _cancelButton->setEnabled(true);

    _currentStep = 0;
    _setupCurrentState();
}

void RemoteControlCalibrationController::_stopCalibration()
{
    _currentStep = -1;

    if (_vehicle) {
        // Only PX4 is known to support this command in all versions. For other firmware which may or may not
        // support this we don't show errors on failure.
        _vehicle->stopCalibration(_vehicle->px4Firmware() ? true : false /* showError */);
        _readStoredCalibrationValues();
    } else {
        _resetInternalCalibrationValues();
    }

    if (_statusText) {
        _statusText->setProperty("text", "");
    }
    if (_nextButton) {
        _nextButton->setProperty("text", tr("Calibrate"));
    }
    if (_nextButton) {
        _nextButton->setEnabled(true);
    }
    if (_cancelButton) {
        _cancelButton->setEnabled(false);
    }
    if (_skipButton) {
        _skipButton->setEnabled(false);
    }

    _stickDisplayPositions = { _stickDisplayPositionCentered.horizontal, _stickDisplayPositionCentered.vertical,
                               _stickDisplayPositionCentered.horizontal, _stickDisplayPositionCentered.vertical };
    emit stickDisplayPositionsChanged();
}

void RemoteControlCalibrationController::_saveCurrentRawValues()
{
    for (int i = 0; i < _chanMax; i++) {
        _channelValueSave[i] = _channelRawValue[i];
        qCDebug(RemoteControlCalibrationControllerLog) << "_saveCurrentRawValues channel:value" << i << _channelValueSave[i];
    }
}

void RemoteControlCalibrationController::_loadCalibrationUISettings()
{
    QSettings settings;

    settings.beginGroup(_settingsGroup);
    _transmitterMode = settings.value(_settingsKeyTransmitterMode, 2).toInt();
    settings.endGroup();

    if (_transmitterMode < 1 || _transmitterMode > 4) {
        _transmitterMode = 2;
    }
}

void RemoteControlCalibrationController::_saveCalibrationUISettings()
{
    QSettings settings;

    settings.beginGroup(_settingsGroup);
    settings.setValue(_settingsKeyTransmitterMode, _transmitterMode);
    settings.endGroup();
}

int RemoteControlCalibrationController::rollChannelValue()
{
    if (_rgFunctionChannelMapping[stickFunctionRoll] != _chanMax) {
        return _channelRawValue[stickFunctionRoll];
    } else {
        return 1500;
    }
}

int RemoteControlCalibrationController::pitchChannelValue()
{
    if (_rgFunctionChannelMapping[stickFunctionPitch] != _chanMax) {
        return _channelRawValue[stickFunctionPitch];
    } else {
        return 1500;
    }
}

int RemoteControlCalibrationController::yawChannelValue()
{
    if (_rgFunctionChannelMapping[stickFunctionYaw] != _chanMax) {
        return _channelRawValue[stickFunctionYaw];
    } else {
        return 1500;
    }
}

int RemoteControlCalibrationController::throttleChannelValue()
{
    if (_rgFunctionChannelMapping[stickFunctionThrottle] != _chanMax) {
        return _channelRawValue[stickFunctionThrottle];
    } else {
        return 1500;
    }
}

bool RemoteControlCalibrationController::rollChannelMapped()
{
    return (_rgFunctionChannelMapping[stickFunctionRoll] != _chanMax);
}

bool RemoteControlCalibrationController::pitchChannelMapped()
{
    return (_rgFunctionChannelMapping[stickFunctionPitch] != _chanMax);
}

bool RemoteControlCalibrationController::yawChannelMapped()
{
    return (_rgFunctionChannelMapping[stickFunctionYaw] != _chanMax);
}

bool RemoteControlCalibrationController::throttleChannelMapped()
{
    return (_rgFunctionChannelMapping[stickFunctionThrottle] != _chanMax);
}

bool RemoteControlCalibrationController::rollChannelReversed()
{
    if (_rgFunctionChannelMapping[stickFunctionRoll] != _chanMax) {
        return _rgChannelInfo[_rgFunctionChannelMapping[stickFunctionRoll]].reversed;
    } else {
        return false;
    }
}

bool RemoteControlCalibrationController::pitchChannelReversed()
{
    if (_rgFunctionChannelMapping[stickFunctionPitch] != _chanMax) {
        return _rgChannelInfo[_rgFunctionChannelMapping[stickFunctionPitch]].reversed;
    } else {
        return false;
    }
}

bool RemoteControlCalibrationController::yawChannelReversed()
{
    if (_rgFunctionChannelMapping[stickFunctionYaw] != _chanMax) {
        return _rgChannelInfo[_rgFunctionChannelMapping[stickFunctionYaw]].reversed;
    } else {
        return false;
    }
}

bool RemoteControlCalibrationController::throttleChannelReversed()
{
    if (_rgFunctionChannelMapping[stickFunctionThrottle] != _chanMax) {
        return _rgChannelInfo[_rgFunctionChannelMapping[stickFunctionThrottle]].reversed;
    } else {
        return false;
    }
}

void RemoteControlCalibrationController::setTransmitterMode(int mode)
{
    if (mode < 1 || mode > 4) {
        qCWarning(RemoteControlCalibrationControllerLog) << "Invalid transmitter mode set:" << mode;
        mode = 2;
    }
    if (_transmitterMode != mode) {
        _transmitterMode = mode;
        emit transmitterModeChanged();
    }
}

void RemoteControlCalibrationController::_signalAllAttitudeValueChanges()
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

void RemoteControlCalibrationController::copyTrims()
{
    _vehicle->startCalibration(QGCMAVLink::CalibrationCopyTrims);
}

QString RemoteControlCalibrationController::_stickFunctionToString(StickFunction function)
{
    switch (function) {
    case stickFunctionRoll:
        return tr("Roll");
    case stickFunctionPitch:
        return tr("Pitch");
    case stickFunctionYaw:
        return tr("Yaw");
    case stickFunctionThrottle:
        return tr("Throttle");
    default:
        return tr("Unknown");
    }
}

void RemoteControlCalibrationController::setCenteredThrottle(bool centered)
{
    if (_centeredThrottle != centered) {
        _centeredThrottle = centered;
        emit centeredThrottleChanged(centered);
    }
}
