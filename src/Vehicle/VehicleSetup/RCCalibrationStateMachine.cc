#include "RCCalibrationStateMachine.h"
#include "RemoteControlCalibrationController.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QTimer>

QGC_LOGGING_CATEGORY(RCCalibrationStateMachineLog, "RCCalibrationStateMachine")

RCCalibrationStateMachine::RCCalibrationStateMachine(RemoteControlCalibrationController* controller, QObject* parent)
    : QGCStateMachine("RCCalibration", nullptr, parent)
    , _controller(controller)
    , _stickDetectChannel(0)
    , _stickDetectValue(0)
    , _stickDetectSettleStarted(false)
{
    _buildStateMachine();
    _setupTransitions();

    connect(this, &QStateMachine::runningChanged, this, [this]() {
        emit stepChanged();
    });
}

void RCCalibrationStateMachine::_buildStateMachine()
{
    // Create all states
    _idleState = new QGCState("Idle", this);
    _stickNeutralState = new QGCState("StickNeutral", this);
    _throttleUpState = new QGCState("ThrottleUp", this);
    _throttleDownState = new QGCState("ThrottleDown", this);
    _yawRightState = new QGCState("YawRight", this);
    _yawLeftState = new QGCState("YawLeft", this);
    _rollRightState = new QGCState("RollRight", this);
    _rollLeftState = new QGCState("RollLeft", this);
    _pitchUpState = new QGCState("PitchUp", this);
    _pitchDownState = new QGCState("PitchDown", this);
    _pitchCenterState = new QGCState("PitchCenter", this);
    _switchMinMaxState = new QGCState("SwitchMinMax", this);
    _completeState = new QGCFinalState("Complete", this);

    // Set initial state
    setInitialState(_idleState);

    // Configure state entry callbacks
    connect(_idleState, &QAbstractState::entered, this, [this]() {
        qCDebug(RCCalibrationStateMachineLog) << "Entered Idle state";
        _currentInputMode = InputMode::None;
        _currentStickFunction = stickFunctionMax;
    });

    connect(_stickNeutralState, &QAbstractState::entered, this, [this]() {
        qCDebug(RCCalibrationStateMachineLog) << "Entered StickNeutral state";
        _currentInputMode = InputMode::CenterWaitBegin;
        _currentStickFunction = stickFunctionMax;
        _controller->_setupCurrentState();
        emit stepChanged();
    });

    connect(_throttleUpState, &QAbstractState::entered, this, [this]() {
        qCDebug(RCCalibrationStateMachineLog) << "Entered ThrottleUp state";
        _currentInputMode = InputMode::StickDetect;
        _currentStickFunction = stickFunctionThrottle;
        _controller->_setupCurrentState();
        emit stepChanged();
    });

    connect(_throttleDownState, &QAbstractState::entered, this, [this]() {
        qCDebug(RCCalibrationStateMachineLog) << "Entered ThrottleDown state";
        _currentInputMode = InputMode::StickMin;
        _currentStickFunction = stickFunctionThrottle;
        _controller->_setupCurrentState();
        emit stepChanged();
    });

    connect(_yawRightState, &QAbstractState::entered, this, [this]() {
        qCDebug(RCCalibrationStateMachineLog) << "Entered YawRight state";
        _currentInputMode = InputMode::StickDetect;
        _currentStickFunction = stickFunctionYaw;
        _controller->_setupCurrentState();
        emit stepChanged();
    });

    connect(_yawLeftState, &QAbstractState::entered, this, [this]() {
        qCDebug(RCCalibrationStateMachineLog) << "Entered YawLeft state";
        _currentInputMode = InputMode::StickMin;
        _currentStickFunction = stickFunctionYaw;
        _controller->_setupCurrentState();
        emit stepChanged();
    });

    connect(_rollRightState, &QAbstractState::entered, this, [this]() {
        qCDebug(RCCalibrationStateMachineLog) << "Entered RollRight state";
        _currentInputMode = InputMode::StickDetect;
        _currentStickFunction = stickFunctionRoll;
        _controller->_setupCurrentState();
        emit stepChanged();
    });

    connect(_rollLeftState, &QAbstractState::entered, this, [this]() {
        qCDebug(RCCalibrationStateMachineLog) << "Entered RollLeft state";
        _currentInputMode = InputMode::StickMin;
        _currentStickFunction = stickFunctionRoll;
        _controller->_setupCurrentState();
        emit stepChanged();
    });

    connect(_pitchUpState, &QAbstractState::entered, this, [this]() {
        qCDebug(RCCalibrationStateMachineLog) << "Entered PitchUp state";
        _currentInputMode = InputMode::StickDetect;
        _currentStickFunction = stickFunctionPitch;
        _controller->_setupCurrentState();
        emit stepChanged();
    });

    connect(_pitchDownState, &QAbstractState::entered, this, [this]() {
        qCDebug(RCCalibrationStateMachineLog) << "Entered PitchDown state";
        _currentInputMode = InputMode::StickMin;
        _currentStickFunction = stickFunctionPitch;
        _controller->_setupCurrentState();
        emit stepChanged();
    });

    connect(_pitchCenterState, &QAbstractState::entered, this, [this]() {
        qCDebug(RCCalibrationStateMachineLog) << "Entered PitchCenter state";
        _currentInputMode = InputMode::CenterWait;
        _currentStickFunction = stickFunctionPitch;
        _controller->_setupCurrentState();
        emit stepChanged();
    });

    connect(_switchMinMaxState, &QAbstractState::entered, this, [this]() {
        qCDebug(RCCalibrationStateMachineLog) << "Entered SwitchMinMax state";
        _currentInputMode = InputMode::SwitchMinMax;
        _currentStickFunction = stickFunctionMax;
        _controller->_setupCurrentState();
        emit stepChanged();
    });

    connect(_completeState, &QAbstractState::entered, this, [this]() {
        qCDebug(RCCalibrationStateMachineLog) << "Calibration complete";
        _currentInputMode = InputMode::None;
        emit calibrationComplete();
    });
}

void RCCalibrationStateMachine::_setupTransitions()
{
    // Idle -> StickNeutral (on start event)
    _idleState->addTransition(new MachineEventTransition("start", _stickNeutralState));

    // StickNeutral -> ThrottleUp (on next_clicked after trims saved)
    _stickNeutralState->addTransition(new MachineEventTransition("next_clicked", _throttleUpState));

    // ThrottleUp -> ThrottleDown (on stick_settled)
    _throttleUpState->addTransition(new MachineEventTransition("stick_settled", _throttleDownState));

    // ThrottleDown -> YawRight (on stick_settled)
    _throttleDownState->addTransition(new MachineEventTransition("stick_settled", _yawRightState));

    // YawRight -> YawLeft (on stick_settled)
    _yawRightState->addTransition(new MachineEventTransition("stick_settled", _yawLeftState));

    // YawLeft -> RollRight (on stick_settled)
    _yawLeftState->addTransition(new MachineEventTransition("stick_settled", _rollRightState));

    // RollRight -> RollLeft (on stick_settled)
    _rollRightState->addTransition(new MachineEventTransition("stick_settled", _rollLeftState));

    // RollLeft -> PitchUp (on stick_settled)
    _rollLeftState->addTransition(new MachineEventTransition("stick_settled", _pitchUpState));

    // PitchUp -> PitchDown (on stick_settled)
    _pitchUpState->addTransition(new MachineEventTransition("stick_settled", _pitchDownState));

    // PitchDown -> PitchCenter (on stick_settled)
    _pitchDownState->addTransition(new MachineEventTransition("stick_settled", _pitchCenterState));

    // PitchCenter -> SwitchMinMax (on stick_settled)
    _pitchCenterState->addTransition(new MachineEventTransition("stick_settled", _switchMinMaxState));

    // SwitchMinMax -> Complete (on next_clicked)
    _switchMinMaxState->addTransition(new MachineEventTransition("next_clicked", _completeState));

    // Cancel from any state -> Idle
    _stickNeutralState->addTransition(new MachineEventTransition("cancel", _idleState));
    _throttleUpState->addTransition(new MachineEventTransition("cancel", _idleState));
    _throttleDownState->addTransition(new MachineEventTransition("cancel", _idleState));
    _yawRightState->addTransition(new MachineEventTransition("cancel", _idleState));
    _yawLeftState->addTransition(new MachineEventTransition("cancel", _idleState));
    _rollRightState->addTransition(new MachineEventTransition("cancel", _idleState));
    _rollLeftState->addTransition(new MachineEventTransition("cancel", _idleState));
    _pitchUpState->addTransition(new MachineEventTransition("cancel", _idleState));
    _pitchDownState->addTransition(new MachineEventTransition("cancel", _idleState));
    _pitchCenterState->addTransition(new MachineEventTransition("cancel", _idleState));
    _switchMinMaxState->addTransition(new MachineEventTransition("cancel", _idleState));
}

void RCCalibrationStateMachine::startCalibration()
{
    qCDebug(RCCalibrationStateMachineLog) << "Starting calibration";

    // Reset detection state
    _stickDetectChannel = _controller->_chanMax;
    _stickDetectSettleStarted = false;

    if (!isRunning()) {
        start();
    }

    // Post start event to transition from Idle to StickNeutral
    QTimer::singleShot(0, this, [this]() {
        postEvent("start");
        emit calibrationStarted();
    });
}

void RCCalibrationStateMachine::cancelCalibration()
{
    qCDebug(RCCalibrationStateMachineLog) << "Cancelling calibration";
    postEvent("cancel");
    emit calibrationCancelled();
}

void RCCalibrationStateMachine::nextButtonPressed()
{
    qCDebug(RCCalibrationStateMachineLog) << "Next button pressed";

    if (isStateActive(_stickNeutralState)) {
        // Save trims before advancing
        _saveAllTrims();
        postEvent("next_clicked");
    } else if (isStateActive(_switchMinMaxState)) {
        // Save calibration and complete
        _saveCalibrationValues();
        postEvent("next_clicked");
    }
}

void RCCalibrationStateMachine::processChannelInput(int channel, int value)
{
    if (_currentInputMode == InputMode::None) {
        return;
    }

    switch (_currentInputMode) {
    case InputMode::CenterWaitBegin:
        _inputCenterWaitBegin(_currentStickFunction, channel, value);
        break;
    case InputMode::StickDetect:
        _inputStickDetect(_currentStickFunction, channel, value);
        break;
    case InputMode::StickMin:
        _inputStickMin(_currentStickFunction, channel, value);
        break;
    case InputMode::CenterWait:
        _inputCenterWait(_currentStickFunction, channel, value);
        break;
    case InputMode::SwitchMinMax:
        _inputSwitchMinMax(_currentStickFunction, channel, value);
        break;
    case InputMode::None:
        break;
    }
}

bool RCCalibrationStateMachine::isCalibrating() const
{
    return isRunning() && !isStateActive(_idleState);
}

void RCCalibrationStateMachine::_inputCenterWaitBegin(StickFunction /*stickFunction*/, int channel, int value)
{
    if (_controller->_joystickMode) {
        // Track deadband adjustments in joystick mode
        int newDeadband = abs(value) * 1.1; // add 10% on top for fudge factor
        if (newDeadband > _controller->_rgChannelInfo[channel].deadband) {
            _controller->_rgChannelInfo[channel].deadband = qMin(newDeadband, _controller->_calValidMaxValue);
            qCDebug(RCCalibrationStateMachineLog) << "Channel:" << channel << "Deadband:" << _controller->_rgChannelInfo[channel].deadband;

            auto stickFunction = _controller->_rgChannelInfo[channel].stickFunction;
            if (stickFunction != stickFunctionMax) {
                _controller->_emitDeadbandChanged(static_cast<RemoteControlCalibrationController::StickFunction>(stickFunction));
            }
        }
    }

    // Enable next button
    _controller->_nextButton->setEnabled(true);
}

bool RCCalibrationStateMachine::_stickSettleComplete(int value)
{
    // We are waiting for the stick to settle out to a max position
    if (abs(_stickDetectValue - value) > _controller->_calSettleDelta) {
        // Stick is moving too much to consider stopped
        qCDebug(RCCalibrationStateMachineLog) << "_stickSettleComplete Still moving, _stickDetectValue:value" << _stickDetectValue << value;

        _stickDetectValue = value;
        _stickDetectSettleStarted = false;
    } else {
        // Stick is still positioned within the specified small range
        if (_stickDetectSettleStarted) {
            // We have already started waiting
            if (_stickDetectSettleElapsed.elapsed() > _controller->_stickDetectSettleMSecs) {
                // Stick has stayed positioned in one place long enough, detection is complete.
                return true;
            }
        } else {
            // Start waiting for the stick to stay settled
            qCDebug(RCCalibrationStateMachineLog) << "_stickSettleComplete Starting settle timer, _stickDetectValue:value" << _stickDetectValue << value;

            _stickDetectSettleStarted = true;
            _stickDetectSettleElapsed.start();
        }
    }

    return false;
}

void RCCalibrationStateMachine::_inputStickDetect(StickFunction stickFunction, int channel, int value)
{
    // If this channel is already used in a mapping we can't use it again
    if (_controller->_rgChannelInfo[channel].stickFunction != stickFunctionMax) {
        return;
    }

    qCDebug(RCCalibrationStateMachineLog) << "_inputStickDetect function:channel:value" << stickFunction << channel << value;

    if (_stickDetectChannel == _controller->_chanMax) {
        // We have not detected enough movement on a channel yet
        if (abs(_controller->_channelValueSave[channel] - value) > _controller->_calMoveDelta) {
            // Stick has moved far enough to consider it as being selected for the function
            qCDebug(RCCalibrationStateMachineLog) << "_inputStickDetect Starting settle wait";

            _stickDetectChannel = channel;
            _stickDetectValue = value;
        }
    } else if (channel == _stickDetectChannel) {
        if (_stickSettleComplete(value)) {
            auto *const info = &_controller->_rgChannelInfo[channel];

            // Map the channel to the function
            _controller->_rgFunctionChannelMapping[stickFunction] = channel;
            info->stickFunction = static_cast<RemoteControlCalibrationController::StickFunction>(stickFunction);

            // A non-reversed channel should show a higher value than center.
            info->channelReversed = value < _controller->_channelValueSave[channel];
            if (info->channelReversed) {
                _controller->_rgChannelInfo[channel].channelMin = value;
            } else {
                _controller->_rgChannelInfo[channel].channelMax = value;
            }

            qCDebug(RCCalibrationStateMachineLog) << "_inputStickDetect Settle complete, reversed:min:max" << info->channelReversed << info->channelMin << info->channelMax;

            _controller->_signalAllAttitudeValueChanges();

            _advanceState();
        }
    }
}

void RCCalibrationStateMachine::_inputStickMin(StickFunction stickFunction, int channel, int value)
{
    // We only care about the channel mapped to the function we are working on
    if (_controller->_rgFunctionChannelMapping[stickFunction] != channel) {
        return;
    }

    qCDebug(RCCalibrationStateMachineLog) << "_inputStickMin function:channel:value" << stickFunction << channel << value;

    if (_stickDetectChannel == _controller->_chanMax) {
        // Setup up to detect stick being pegged to extreme position
        const bool movedSignificantly = abs(_controller->_channelValueSave[channel] - value) > _controller->_calMoveDelta;

        if (_controller->_rgChannelInfo[channel].channelReversed) {
            // Reversed channel: "min" is actually in the positive direction
            const bool alreadyAtExtreme = abs(value - _controller->_calValidMaxValue) < _controller->_calSettleDelta;

            if ((movedSignificantly && value > _controller->_calCenterPoint) || alreadyAtExtreme) {
                qCDebug(RCCalibrationStateMachineLog) << "_inputStickMin Movement detected, starting settle wait";
                _stickDetectChannel = channel;
                _stickDetectValue = value;
            }
        } else {
            // Normal channel: "min" is in the negative direction
            const bool alreadyAtExtreme = abs(value - _controller->_calValidMinValue) < _controller->_calSettleDelta;

            if ((movedSignificantly && value < _controller->_calCenterPoint) || alreadyAtExtreme) {
                qCDebug(RCCalibrationStateMachineLog) << "_inputStickMin Movement detected, starting settle wait";
                _stickDetectChannel = channel;
                _stickDetectValue = value;
            }
        }
    } else {
        // We are waiting for the selected channel to settle out
        if (_stickSettleComplete(value)) {
            const auto *const info = &_controller->_rgChannelInfo[channel];

            // Stick detection is complete. Stick should be at extreme position.
            if (info->channelReversed) {
                _controller->_rgChannelInfo[channel].channelMax = value;
            } else {
                _controller->_rgChannelInfo[channel].channelMin = value;
            }

            qCDebug(RCCalibrationStateMachineLog) << "_inputStickMin Settle complete";

            // Check if this is throttle and set trim accordingly
            if (stickFunction == stickFunctionThrottle) {
                _controller->_rgChannelInfo[channel].channelTrim = value;
            }

            _advanceState();
        }
    }
}

void RCCalibrationStateMachine::_inputCenterWait(StickFunction stickFunction, int channel, int value)
{
    // We only care about the channel mapped to the function we are working on
    if (_controller->_rgFunctionChannelMapping[stickFunction] != channel) {
        return;
    }

    qCDebug(RCCalibrationStateMachineLog) << "_inputCenterWait function:channel:value" << stickFunction << channel << value;

    if (_stickDetectChannel == _controller->_chanMax) {
        // Sticks have not yet moved close enough to center
        if (abs(_controller->_calCenterPoint - value) < _controller->_calRoughCenterDelta) {
            // Stick has moved close enough to center that we can start waiting for it to settle
            qCDebug(RCCalibrationStateMachineLog) << "_inputCenterWait Center detected. Waiting for settle.";
            _stickDetectChannel = channel;
            _stickDetectValue = value;
        }
    } else {
        if (_stickSettleComplete(value)) {
            _advanceState();
        }
    }
}

void RCCalibrationStateMachine::_inputSwitchMinMax(StickFunction /*stickFunction*/, int channel, int value)
{
    // If the channel is mapped we already have min/max
    if (_controller->_rgChannelInfo[channel].stickFunction != stickFunctionMax) {
        return;
    }

    if (abs(_controller->_calCenterPoint - value) > _controller->_calMoveDelta) {
        // Stick has moved far enough from center to consider for min/max
        if (value < _controller->_calCenterPoint) {
            const int minValue = qMin(_controller->_rgChannelInfo[channel].channelMin, value);
            qCDebug(RCCalibrationStateMachineLog) << "setting min channel:min" << channel << minValue;
            _controller->_rgChannelInfo[channel].channelMin = minValue;
        } else {
            int maxValue = qMax(_controller->_rgChannelInfo[channel].channelMax, value);
            qCDebug(RCCalibrationStateMachineLog) << "setting max channel:max" << channel << maxValue;
            _controller->_rgChannelInfo[channel].channelMax = maxValue;
        }
    }
}

void RCCalibrationStateMachine::_advanceState()
{
    qCDebug(RCCalibrationStateMachineLog) << "Advancing state";

    // Reset detection state for next step
    _stickDetectChannel = _controller->_chanMax;
    _stickDetectSettleStarted = false;
    _controller->_saveCurrentRawValues();

    // Post event to trigger state transition
    postEvent("stick_settled");
}

void RCCalibrationStateMachine::_saveAllTrims()
{
    // We save all trims as the first step. At this point no channels are mapped but it should still
    // allow us to get good trims for the roll/pitch/yaw/throttle even though we don't know which
    // channels they are yet.
    for (int i = 0; i < _controller->_chanCount; i++) {
        qCDebug(RCCalibrationStateMachineLog) << "_saveAllTrims channel trim" << i << _controller->_channelRawValue[i];
        _controller->_rgChannelInfo[i].channelTrim = _controller->_channelRawValue[i];
    }

    // Reset detection state for next step
    _stickDetectChannel = _controller->_chanMax;
    _stickDetectSettleStarted = false;
    _controller->_saveCurrentRawValues();
}

void RCCalibrationStateMachine::_saveCalibrationValues()
{
    qCDebug(RCCalibrationStateMachineLog) << "Saving calibration values";
    _controller->_saveStoredCalibrationValues();
    emit _controller->calibrationCompleted();
}
