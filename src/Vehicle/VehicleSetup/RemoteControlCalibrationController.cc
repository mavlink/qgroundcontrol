#include "RemoteControlCalibrationController.h"
#include "Fact.h"
#include "ParameterManager.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

#include <QtCore/QSettings>
#include <algorithm>

QGC_LOGGING_CATEGORY(RemoteControlCalibrationControllerLog, "RemoteControl.RemoteControlCalibrationController")
QGC_LOGGING_CATEGORY(RemoteControlCalibrationControllerVerboseLog, "RemoteControl.RemoteControlCalibrationController:verbose")

static constexpr const char *msgBeginThrottleDown = QT_TR_NOOP(
        "* Lower the Throttle stick all the way down as shown in diagram\n"
        "* Please ensure all motor power is disconnected AND all props are removed from the vehicle.\n"
        "* Click Next to continue"
);
static constexpr const char *msgBeginThrottleCenter = QT_TR_NOOP(
    "* Center all sticks as shown in diagram.\n"
    "* Please ensure all motor power is disconnected from the vehicle.\n"
    "* Click Next to continue"
);
static constexpr const char *msgThrottleUp =            QT_TR_NOOP("Move the Throttle stick all the way up and hold it there...");
static constexpr const char *msgThrottleDown =          QT_TR_NOOP("Move the Throttle stick all the way down and leave it there...");
static constexpr const char *msgYawLeft =               QT_TR_NOOP("Move the Yaw stick all the way to the left and hold it there...");
static constexpr const char *msgYawRight =              QT_TR_NOOP("Move the Yaw stick all the way to the right and hold it there...");
static constexpr const char *msgRollLeft =              QT_TR_NOOP("Move the Roll stick all the way to the left and hold it there...");
static constexpr const char *msgRollRight =             QT_TR_NOOP("Move the Roll stick all the way to the right and hold it there...");
static constexpr const char *msgPitchDown =             QT_TR_NOOP("Move the Pitch stick all the way down and hold it there...");
static constexpr const char *msgPitchUp =               QT_TR_NOOP("Move the Pitch stick all the way up and hold it there...");
static constexpr const char *msgPitchCenter =           QT_TR_NOOP("Allow the Pitch stick to move back to center...");
static constexpr const char *msgSwitchMinMaxRC =        QT_TR_NOOP("Move all the transmitter switches and/or dials back and forth to their extreme positions.");
static constexpr const char *msgSwitchMinMaxJoystick =  QT_TR_NOOP("Move all sticks to their extreme positions.");
static constexpr const char *msgComplete =              QT_TR_NOOP("All settings have been captured. Click Next to write the new parameters to your board.");

RemoteControlCalibrationController::RemoteControlCalibrationController(QObject *parent)
    : FactPanelController(parent)
    , _stateMachine{
        // stickFunction,           stepFunction,                   channelInputFn,                                             nextButtonFn
        { stickFunctionMax,         StateMachineStepStickNeutral,   &RemoteControlCalibrationController::_inputCenterWaitBegin, &RemoteControlCalibrationController::_saveAllTrims },
        { stickFunctionThrottle,    StateMachineStepThrottleUp,     &RemoteControlCalibrationController::_inputStickDetect,     nullptr },
        { stickFunctionThrottle,    StateMachineStepThrottleDown,   &RemoteControlCalibrationController::_inputStickMin,        nullptr },
        { stickFunctionYaw,         StateMachineStepYawRight,       &RemoteControlCalibrationController::_inputStickDetect,     nullptr },
        { stickFunctionYaw,         StateMachineStepYawLeft,        &RemoteControlCalibrationController::_inputStickMin,        nullptr },
        { stickFunctionRoll,        StateMachineStepRollRight,      &RemoteControlCalibrationController::_inputStickDetect,     nullptr },
        { stickFunctionRoll,        StateMachineStepRollLeft,       &RemoteControlCalibrationController::_inputStickMin,        nullptr },
        { stickFunctionPitch,       StateMachineStepPitchUp,        &RemoteControlCalibrationController::_inputStickDetect,     nullptr },
        { stickFunctionPitch,       StateMachineStepPitchDown,      &RemoteControlCalibrationController::_inputStickMin,        nullptr },
        { stickFunctionPitch,       StateMachineStepPitchCenter,    &RemoteControlCalibrationController::_inputCenterWait,      nullptr },
        { stickFunctionMax,         StateMachineStepSwitchMinMax,   &RemoteControlCalibrationController::_inputSwitchMinMax,    &RemoteControlCalibrationController::_advanceState },
        { stickFunctionMax,         StateMachineStepComplete,       nullptr,                                                    &RemoteControlCalibrationController::_saveCalibrationValues },
    }
{
    // qCDebug(RemoteControlCalibrationControllerLog) << Q_FUNC_INFO << this;

    _resetInternalCalibrationValues();
    _loadCalibrationUISettings();

    _stickDisplayPositions = { _stickDisplayPositionCentered.horizontal, _stickDisplayPositionCentered.vertical,
                               _stickDisplayPositionCentered.horizontal, _stickDisplayPositionCentered.vertical };

    if (_vehicle->rover()) {
        _centeredThrottle = true;
    }

    _stepFunctionToMsgStringMap = {
        { StateMachineStepStickNeutral,    msgBeginThrottleCenter }, // Adjusted based on throttle centered or not
        { StateMachineStepThrottleUp,      msgThrottleUp },
        { StateMachineStepThrottleDown,    msgThrottleDown },
        { StateMachineStepYawRight,        msgYawRight },
        { StateMachineStepYawLeft,         msgYawLeft },
        { StateMachineStepRollRight,       msgRollRight },
        { StateMachineStepRollLeft,        msgRollLeft },
        { StateMachineStepPitchUp,         msgPitchUp },
        { StateMachineStepPitchDown,       msgPitchDown },
        { StateMachineStepPitchCenter,     msgPitchCenter },
        { StateMachineStepSwitchMinMax,    msgSwitchMinMaxRC }, // Adjusted based on joystick mode or not
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
    if (_currentStep >= _stateMachine.size()) {
        _stopCalibration();
        return;
    }

    _setupCurrentState();
}

void RemoteControlCalibrationController::_setupCurrentState()
{
    auto state = _getStateMachineEntry(_currentStep);

    _stepFunctionToMsgStringMap[StateMachineStepStickNeutral] = _centeredThrottle ? msgBeginThrottleCenter : msgBeginThrottleDown;
    _stepFunctionToMsgStringMap[StateMachineStepSwitchMinMax] = _joystickMode ? msgSwitchMinMaxJoystick : msgSwitchMinMaxRC;

    BothSticksDisplayPositions defaultPositions = { _stickDisplayPositionCentered, _stickDisplayPositionCentered };
    BothSticksDisplayPositions bothStickPositions = _centeredThrottle
        ? _bothStickDisplayPositionThrottleCenteredMap.value(state.stepFunction).value(_transmitterMode, defaultPositions)
        : _bothStickDisplayPositionThrottleDownMap.value(state.stepFunction).value(_transmitterMode, defaultPositions);

    _statusText->setProperty("text", _stepFunctionToMsgStringMap.value(state.stepFunction, QString()));
    _stickDisplayPositions = { bothStickPositions.leftStick.horizontal, bothStickPositions.leftStick.vertical,
                               bothStickPositions.rightStick.horizontal, bothStickPositions.rightStick.vertical };
    qDebug() << "_setupCurrentState stepFunction:" << state.stepFunction << "leftStick(h,v):" << bothStickPositions.leftStick.horizontal << bothStickPositions.leftStick.vertical
             << "rightStick(h,v):" << bothStickPositions.rightStick.horizontal << bothStickPositions.rightStick.vertical;
    emit stickDisplayPositionsChanged();

    _stickDetectChannel = _chanMax;
    _stickDetectSettleStarted = false;

    _saveCurrentRawValues();

    _nextButton->setEnabled(state.nextButtonFn != nullptr);
}

void RemoteControlCalibrationController::rawChannelValuesChanged(QVector<int> channelValues)
{
    auto channelCount = channelValues.size();
    if (channelCount > _chanMax) {
        qCWarning(RemoteControlCalibrationControllerLog) << "Too many channels:" << channelCount << ", max is" << _chanMax;
        channelCount = _chanMax;
    }

    qCDebug(RemoteControlCalibrationControllerVerboseLog) << "channelValues" << channelValues;

    for (int channel=0; channel<channelCount; channel++) {
        const int channelValue = channelValues[channel];
        const ChannelInfo &channelInfo = _rgChannelInfo[channel];
        const int adjustedValue = _adjustChannelRawValue(channelInfo, channelValue);

        _channelRawValue[channel] = channelValue;
        emit rawChannelValueChanged(channel, channelValue);

        // Signal attitude rc values to Qml if mapped
        if (channelInfo.stickFunction != stickFunctionMax) {
            switch (channelInfo.stickFunction) {
            case stickFunctionRoll:
                emit adjustedRollChannelValueChanged(adjustedValue);
                break;
            case stickFunctionPitch:
                emit adjustedPitchChannelValueChanged(adjustedValue);
                break;
            case stickFunctionYaw:
                emit adjustedYawChannelValueChanged(adjustedValue);
                break;
            case stickFunctionThrottle:
                emit adjustedThrottleChannelValueChanged(adjustedValue);
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
                (this->*state.channelInputFn)(state.stickFunction, channel, channelValue);
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
        _rgChannelInfo[i].channelTrim = _channelRawValue[i];
    }
    _advanceState();
}

void RemoteControlCalibrationController::_inputCenterWaitBegin(StickFunction /*stickFunction*/, int channel, int value)
{
    if (_joystickMode) {
        // Track deadband adjustments in joystick mode
        int newDeadband = abs(value) * 1.1; // add 10% on top for fudge factor
        if (newDeadband > _rgChannelInfo[channel].deadband) {
            _rgChannelInfo[channel].deadband = qMin(newDeadband, _calValidMaxValue  );
            qCDebug(RemoteControlCalibrationControllerLog) << "Channel:" << channel << "Deadband:" << _rgChannelInfo[channel].deadband;
            StickFunction stickFunction = _rgChannelInfo[channel].stickFunction;
            if (stickFunction != stickFunctionMax) {
                _emitDeadbandChanged(stickFunction);
            }
        }
    }

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

void RemoteControlCalibrationController::_inputStickDetect(StickFunction stickFunction, int channel, int value)
{
    // If this channel is already used in a mapping we can't use it again
    if (_rgChannelInfo[channel].stickFunction != stickFunctionMax) {
        return;
    }

    qCDebug(RemoteControlCalibrationControllerVerboseLog) << "_inputStickDetect function:channel:value" << _stickFunctionToString(stickFunction) << channel << value;

    if (_stickDetectChannel == _chanMax) {
        // We have not detected enough movement on a channel yet
        // Note: We intentionally require movement here (not just being at extreme) because
        // this function is called for ALL channels, and some (like triggers) may rest at
        // extreme positions. Requiring movement ensures we detect the correct channel.

        if (abs(_channelValueSave[channel] - value) > _calMoveDelta) {
            // Stick has moved far enough to consider it as being selected for the function

            qCDebug(RemoteControlCalibrationControllerLog) << "_inputStickDetect Starting settle wait";

            // Setup up to detect stick being pegged to min or max value
            _stickDetectChannel = channel;
            _stickDetectValue = value;
        }
    } else if (channel == _stickDetectChannel) {
        if (_stickSettleComplete(value)) {
            ChannelInfo *const info = &_rgChannelInfo[channel];

            // Map the channel to the function
            _rgFunctionChannelMapping[stickFunction] = channel;
            info->stickFunction = stickFunction;

            // A non-reversed channel should show a higher value than center.
            info->channelReversed = value < _channelValueSave[channel];
            if (info->channelReversed) {
                _rgChannelInfo[channel].channelMin = value;
            } else {
                _rgChannelInfo[channel].channelMax = value;
            }

            qCDebug(RemoteControlCalibrationControllerLog) << "_inputStickDetect Settle complete, reversed:min:max" << info->channelReversed << info->channelMin << info->channelMax;

            _signalAllAttitudeValueChanges();

            _advanceState();
        }
    }
}

void RemoteControlCalibrationController::_inputStickMin(StickFunction stickFunction, int channel, int value)
{
    // We only care about the channel mapped to the function we are working on
    if (_rgFunctionChannelMapping[stickFunction] != channel) {
        return;
    }

    qCDebug(RemoteControlCalibrationControllerVerboseLog) << "_inputStickMin function:channel:value" << _stickFunctionToString(stickFunction) << channel << value;

    if (_stickDetectChannel == _chanMax) {
        // Setup up to detect stick being pegged to extreme position
        // Check both: 1) significant movement from saved position, AND 2) value is in the expected half of range
        const bool movedSignificantly = abs(_channelValueSave[channel] - value) > _calMoveDelta;

        if (_rgChannelInfo[channel].channelReversed) {
            // Reversed channel: "min" is actually in the positive direction
            // Also check if already at the max extreme (user moved stick before step started)
            const bool alreadyAtExtreme = abs(value - _calValidMaxValue) < _calSettleDelta;

            if ((movedSignificantly && value > _calCenterPoint) || alreadyAtExtreme) {
                qCDebug(RemoteControlCalibrationControllerLog) << "_inputStickMin Movement detected, starting settle wait"
                                                               << "movedSignificantly:" << movedSignificantly
                                                               << "alreadyAtExtreme:" << alreadyAtExtreme;
                _stickDetectChannel = channel;
                _stickDetectValue = value;
            }
        } else {
            // Normal channel: "min" is in the negative direction
            // Also check if already at the min extreme (user moved stick before step started)
            const bool alreadyAtExtreme = abs(value - _calValidMinValue) < _calSettleDelta;

            if ((movedSignificantly && value < _calCenterPoint) || alreadyAtExtreme) {
                qCDebug(RemoteControlCalibrationControllerLog) << "_inputStickMin Movement detected, starting settle wait"
                                                               << "movedSignificantly:" << movedSignificantly
                                                               << "alreadyAtExtreme:" << alreadyAtExtreme;
                _stickDetectChannel = channel;
                _stickDetectValue = value;
            }
        }
    } else {
        // We are waiting for the selected channel to settle out
        if (_stickSettleComplete(value)) {
            const ChannelInfo *const info = &_rgChannelInfo[channel];

            // Stick detection is complete. Stick should be at extreme position.
            if (info->channelReversed) {
                _rgChannelInfo[channel].channelMax = value;
            } else {
                _rgChannelInfo[channel].channelMin = value;
            }

            qCDebug(RemoteControlCalibrationControllerLog) << "_inputStickMin Settle complete";

            // Check if this is throttle and set trim accordingly
            if (stickFunction == stickFunctionThrottle) {
                _rgChannelInfo[channel].channelTrim = value;
            }
            // XXX to support configs which can reverse they need to check a reverse
            // flag here and not do this.

            _advanceState();
        }
    }
}

void RemoteControlCalibrationController::_inputCenterWait(StickFunction stickFunction, int channel, int value)
{
    // We only care about the channel mapped to the function we are working on
    if (_rgFunctionChannelMapping[stickFunction] != channel) {
        return;
    }

    qCDebug(RemoteControlCalibrationControllerLog) << "_inputCenterWait function:channel:value" << _stickFunctionToString(stickFunction) << channel << value;
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

void RemoteControlCalibrationController::_inputSwitchMinMax(StickFunction /*stickFunction*/, int channel, int value)
{
    // If the channel is mapped we already have min/max
    if (_rgChannelInfo[channel].stickFunction != stickFunctionMax) {
        return;
    }

    if (abs(_calCenterPoint - value) > _calMoveDelta) {
        // Stick has moved far enough from center to consider for min/max
        if (value < _calCenterPoint) {
            const int minValue = qMin(_rgChannelInfo[channel].channelMin, value);

            qCDebug(RemoteControlCalibrationControllerLog) << "setting min channel:min" << channel << minValue;

            _rgChannelInfo[channel].channelMin = minValue;
        } else {
            int maxValue = qMax(_rgChannelInfo[channel].channelMax, value);

            qCDebug(RemoteControlCalibrationControllerLog) << "setting max channel:max" << channel << maxValue;

            _rgChannelInfo[channel].channelMax = maxValue;
        }
    }
}

void RemoteControlCalibrationController::_resetInternalCalibrationValues()
{
    // Set all raw channels to not reversed and center point values
    for (int i = 0; i < _chanMax; i++) {
        ChannelInfo *const info = &_rgChannelInfo[i];
        info->stickFunction = stickFunctionMax;
        info->channelReversed = false;
        info->channelMin = RemoteControlCalibrationController::_calCenterPoint;
        info->channelMax = RemoteControlCalibrationController::_calCenterPoint;
        info->channelTrim = RemoteControlCalibrationController::_calCenterPoint;
        info->deadband = 0;
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
            if (info->channelMin > _calValidMinValue || info->channelMax < _calValidMaxValue) {
                qCDebug(RemoteControlCalibrationControllerLog) << "_validateAndAdjustCalibrationValues resetting channel" << chan;
                info->channelMin = _calDefaultMinValue;
                info->channelMax = _calDefaultMaxValue;
                info->channelTrim = info->channelMin + ((info->channelMax - info->channelMin) / 2);
            } else {
                switch (_rgChannelInfo[chan].stickFunction) {
                case stickFunctionThrottle:
                case stickFunctionYaw:
                case stickFunctionRoll:
                case stickFunctionPitch:
                    // Make sure trim is within min/max
                    if (info->channelTrim < info->channelMin) {
                        info->channelTrim = info->channelMin;
                    } else if (info->channelTrim > info->channelMax) {
                        info->channelTrim = info->channelMax;
                    }
                    break;
                default:
                    // Non-attitude control channels have calculated trim
                    info->channelTrim = info->channelMin + ((info->channelMax - info->channelMin) / 2);
                    break;
                }

            }
        } else {
            // Unavailable channels are set to defaults
            qCDebug(RemoteControlCalibrationControllerLog) << "_validateAndAdjustCalibrationValues resetting unavailable channel" << chan;
            info->channelMin = _calDefaultMinValue;
            info->channelMax = _calDefaultMaxValue;
            info->channelTrim = info->channelMin + ((info->channelMax - info->channelMin) / 2);
            info->channelReversed = false;
            info->deadband = 0;
            info->stickFunction = stickFunctionMax;

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

    if (!_calibrating) {
        _calibrating = true;
        emit calibratingChanged(true);
    }

    _nextButton->setProperty("text", tr("Next"));
    _cancelButton->setEnabled(true);

    _currentStep = 0;
    _setupCurrentState();
}

void RemoteControlCalibrationController::_stopCalibration()
{
    _currentStep = -1;

    if (_vehicle) {
        _readStoredCalibrationValues();
    }

    if (_calibrating) {
        _calibrating = false;
        emit calibratingChanged(false);
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

    _stickDisplayPositions = { _stickDisplayPositionCentered.horizontal, _stickDisplayPositionCentered.vertical,
                               _stickDisplayPositionCentered.horizontal, _stickDisplayPositionCentered.vertical };
    emit stickDisplayPositionsChanged();
}

void RemoteControlCalibrationController::_saveCurrentRawValues()
{
    for (int i = 0; i < _chanMax; i++) {
        _channelValueSave[i] = _channelRawValue[i];
        qCDebug(RemoteControlCalibrationControllerVerboseLog) << "_saveCurrentRawValues channel:value" << i << _channelValueSave[i];
    }
}

/// Adjust raw channel value for reversal if needed
int RemoteControlCalibrationController::_adjustChannelRawValue(const ChannelInfo& info, int rawValue) const
{
    if (!info.channelReversed) {
        return rawValue;
    }

    const int invertedValue = info.channelMin + info.channelMax - rawValue;
    return std::clamp(invertedValue, info.channelMin, info.channelMax);
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

int RemoteControlCalibrationController::adjustedRollChannelValue()
{
    int channel = _rgFunctionChannelMapping[stickFunctionRoll];
    if (channel != _chanMax) {
        return _adjustChannelRawValue(_rgChannelInfo[channel], _channelRawValue[channel]);
    } else {
        return _calCenterPoint;
    }
}

int RemoteControlCalibrationController::adjustedPitchChannelValue()
{
    int channel = _rgFunctionChannelMapping[stickFunctionPitch];
    if (channel != _chanMax) {
        return _adjustChannelRawValue(_rgChannelInfo[channel], _channelRawValue[channel]);
    } else {
        return _calCenterPoint;
    }
}

int RemoteControlCalibrationController::adjustedYawChannelValue()
{
    int channel = _rgFunctionChannelMapping[stickFunctionYaw];
    if (channel != _chanMax) {
        return _adjustChannelRawValue(_rgChannelInfo[channel], _channelRawValue[channel]);
    } else {
        return _calCenterPoint;
    }
}

int RemoteControlCalibrationController::adjustedThrottleChannelValue()
{
    int channel = _rgFunctionChannelMapping[stickFunctionThrottle];
    if (channel != _chanMax) {
        return _adjustChannelRawValue(_rgChannelInfo[channel], _channelRawValue[channel]);
    } else {
        return _calCenterPoint;
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
        return _rgChannelInfo[_rgFunctionChannelMapping[stickFunctionRoll]].channelReversed;
    } else {
        return false;
    }
}

bool RemoteControlCalibrationController::pitchChannelReversed()
{
    if (_rgFunctionChannelMapping[stickFunctionPitch] != _chanMax) {
        return _rgChannelInfo[_rgFunctionChannelMapping[stickFunctionPitch]].channelReversed;
    } else {
        return false;
    }
}

bool RemoteControlCalibrationController::yawChannelReversed()
{
    if (_rgFunctionChannelMapping[stickFunctionYaw] != _chanMax) {
        return _rgChannelInfo[_rgFunctionChannelMapping[stickFunctionYaw]].channelReversed;
    } else {
        return false;
    }
}

bool RemoteControlCalibrationController::throttleChannelReversed()
{
    if (_rgFunctionChannelMapping[stickFunctionThrottle] != _chanMax) {
        return _rgChannelInfo[_rgFunctionChannelMapping[stickFunctionThrottle]].channelReversed;
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

    _emitDeadbandChanged(stickFunctionRoll);
    _emitDeadbandChanged(stickFunctionPitch);
    _emitDeadbandChanged(stickFunctionYaw);
    _emitDeadbandChanged(stickFunctionThrottle);
}

int RemoteControlCalibrationController::rollDeadband()
{
    return _deadbandForFunction(stickFunctionRoll);
}

int RemoteControlCalibrationController::pitchDeadband()
{
    return _deadbandForFunction(stickFunctionPitch);
}

int RemoteControlCalibrationController::yawDeadband()
{
    return _deadbandForFunction(stickFunctionYaw);
}

int RemoteControlCalibrationController::throttleDeadband()
{
    return _deadbandForFunction(stickFunctionThrottle);
}

void RemoteControlCalibrationController::copyTrims()
{
    _vehicle->startCalibration(QGCMAVLink::CalibrationCopyTrims);
}

QString RemoteControlCalibrationController::_stickFunctionToString(StickFunction stickFunction)
{
    switch (stickFunction) {
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

void RemoteControlCalibrationController::setJoystickMode(bool joystickMode)
{
    if (_joystickMode == joystickMode) {
        return;
    }

    _joystickMode = joystickMode;
    setCenteredThrottle(joystickMode);
    emit joystickModeChanged(_joystickMode);
}

int RemoteControlCalibrationController::_deadbandForFunction(StickFunction stickFunction) const
{
    if (stickFunction < 0 || stickFunction >= stickFunctionMax) {
        return 0;
    }

    int channel = _rgFunctionChannelMapping[stickFunction];
    if (channel != _chanMax) {
        return _rgChannelInfo[channel].deadband;
    }

    return 0;
}

void RemoteControlCalibrationController::_emitDeadbandChanged(StickFunction stickFunction)
{
    const int deadband = _deadbandForFunction(stickFunction);
    switch (stickFunction) {
    case stickFunctionRoll:
        emit rollDeadbandChanged(deadband);
        break;
    case stickFunctionPitch:
        emit pitchDeadbandChanged(deadband);
        break;
    case stickFunctionYaw:
        emit yawDeadbandChanged(deadband);
        break;
    case stickFunctionThrottle:
        emit throttleDeadbandChanged(deadband);
        break;
    default:
        break;
    }
}

void RemoteControlCalibrationController::_saveCalibrationValues()
{
    _saveStoredCalibrationValues();
    emit calibrationCompleted();
    _advanceState();
}
