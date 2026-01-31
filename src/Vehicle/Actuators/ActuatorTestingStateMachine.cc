#include "ActuatorTestingStateMachine.h"
#include "ActuatorTesting.h"
#include "QmlObjectListModel.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

#include <cmath>

QGC_LOGGING_CATEGORY(ActuatorTestingLog, "Vehicle.Actuators.ActuatorTesting")

ActuatorTestingStateMachine::ActuatorTestingStateMachine(Vehicle* vehicle, QmlObjectListModel* actuators, QObject* parent)
    : QGCStateMachine("ActuatorTesting", vehicle, parent)
    , _vehicle(vehicle)
    , _actuators(actuators)
{
    _buildStateMachine();

    // Set up watchdog timer for timeout detection
    _watchdogTimer.setInterval(WatchdogIntervalMs);
    _watchdogTimer.setSingleShot(false);
    connect(&_watchdogTimer, &QTimer::timeout, this, &ActuatorTestingStateMachine::_watchdogTimeout);
}

void ActuatorTestingStateMachine::_buildStateMachine()
{
    // Create states
    _idleState = new QGCState("Idle", this);
    _pollingState = new QGCState("Polling", this);
    _sendingState = new QGCState("Sending", this);
    _waitingAckState = new QGCState("WaitingAck", this);

    setInitialState(_idleState);

    // Idle state - waiting for activation
    connect(_idleState, &QAbstractState::entered, this, [this]() {
        qCDebug(ActuatorTestingLog) << "Actuator testing idle";
        _watchdogTimer.stop();
    });

    // Polling state - actively monitoring actuators
    connect(_pollingState, &QAbstractState::entered, this, [this]() {
        qCDebug(ActuatorTestingLog) << "Actuator testing active, polling started";
        _watchdogTimer.start();
    });

    connect(_pollingState, &QAbstractState::exited, this, [this]() {
        _watchdogTimer.stop();
    });

    // Sending state - sending a command
    connect(_sendingState, &QAbstractState::entered, this, [this]() {
        qCDebug(ActuatorTestingLog) << "Sending command for actuator" << _currentIndex;
    });

    // Waiting for ack state
    connect(_waitingAckState, &QAbstractState::entered, this, [this]() {
        qCDebug(ActuatorTestingLog) << "Waiting for ack from actuator" << _currentIndex;
    });

    // Transitions
    // Idle -> Polling (on activate)
    _idleState->addTransition(new MachineEventTransition("activate", _pollingState));

    // Polling -> Idle (on deactivate)
    _pollingState->addTransition(new MachineEventTransition("deactivate", _idleState));

    // Polling -> Sending (when we have something to send)
    _pollingState->addTransition(new MachineEventTransition("send_command", _sendingState));

    // Sending -> WaitingAck (command sent)
    _sendingState->addTransition(new MachineEventTransition("command_sent", _waitingAckState));

    // WaitingAck -> Polling (ack received)
    _waitingAckState->addTransition(new MachineEventTransition("ack_received", _pollingState));

    // WaitingAck -> Polling (timeout, try next)
    _waitingAckState->addTransition(new MachineEventTransition("timeout", _pollingState));
}

void ActuatorTestingStateMachine::setActive(bool active)
{
    qCDebug(ActuatorTestingLog) << "Setting active:" << active;

    if (active == _testingActive) {
        return;
    }

    _testingActive = active;

    if (active) {
        if (!isRunning()) {
            start();
        }
        QTimer::singleShot(0, this, [this]() {
            postEvent("activate");
        });
    } else {
        // Stop all actuators before deactivating
        stopControl(-1);
        QTimer::singleShot(0, this, [this]() {
            postEvent("deactivate");
        });
    }
}

void ActuatorTestingStateMachine::setChannelTo(int index, float value)
{
    if (!_testingActive || index < 0 || index >= _states.size()) {
        return;
    }

    qCDebug(ActuatorTestingLog) << "Setting actuator" << index << "to" << value;

    ActuatorState oldState = _states[index].state;
    _states[index].value = value;
    _states[index].state = ActuatorState::Active;
    _states[index].lastUpdated.start();

    if (oldState != ActuatorState::Active) {
        emit actuatorStateChanged(index, ActuatorState::Active);
    }

    _sendNext();
}

void ActuatorTestingStateMachine::stopControl(int index)
{
    if (index < -1 || index >= _states.size()) {
        return;
    }

    qCDebug(ActuatorTestingLog) << "Stop control for actuator:" << index;

    if (index == -1) {
        // Stop all
        for (int i = 0; i < _states.size(); ++i) {
            if (_states[i].state == ActuatorState::Active) {
                _states[i].state = ActuatorState::StopRequest;
                emit actuatorStateChanged(i, ActuatorState::StopRequest);
            }
        }
    } else {
        if (_states[index].state == ActuatorState::Active) {
            _states[index].state = ActuatorState::StopRequest;
            emit actuatorStateChanged(index, ActuatorState::StopRequest);
        }
    }

    _sendNext();
}

void ActuatorTestingStateMachine::resetStates()
{
    _states.clear();
    _currentIndex = -1;

    if (_actuators) {
        for (int i = 0; i < _actuators->count(); ++i) {
            _states.append(ActuatorStateData{});
        }
    }

    qCDebug(ActuatorTestingLog) << "Reset states for" << _states.size() << "actuators";
}

ActuatorTestingStateMachine::ActuatorState ActuatorTestingStateMachine::actuatorState(int index) const
{
    if (index < 0 || index >= _states.size()) {
        return ActuatorState::NotActive;
    }
    return _states[index].state;
}

bool ActuatorTestingStateMachine::hasActiveActuators() const
{
    for (const auto& state : _states) {
        if (state.state != ActuatorState::NotActive) {
            return true;
        }
    }
    return false;
}

void ActuatorTestingStateMachine::_watchdogTimeout()
{
    // Check for stale actuators (not updated within timeout period)
    for (int i = 0; i < _states.size(); ++i) {
        if (_states[i].state == ActuatorState::Active) {
            if (_states[i].lastUpdated.elapsed() > ActuatorTimeoutMs) {
                qCWarning(ActuatorTestingLog) << "Stopping actuator due to timeout:" << i;
                _states[i].state = ActuatorState::StopRequest;
                emit actuatorStateChanged(i, ActuatorState::StopRequest);
            }
        }
    }

    _sendNext();
}

void ActuatorTestingStateMachine::_sendNext()
{
    if (_commandInProgress || _states.isEmpty()) {
        return;
    }

    // Find the next actuator that needs attention (not in NotActive state)
    for (int i = 0; i < _states.size(); ++i) {
        _currentIndex = (_currentIndex + 1) % _states.size();

        auto* actuator = _actuators->value<ActuatorTesting::Actuator*>(_currentIndex);
        if (!actuator) {
            continue;
        }

        if (_states[_currentIndex].state == ActuatorState::Active) {
            // Send control command
            postEvent("send_command");
            _sendMavlinkRequest(actuator->function(), _states[_currentIndex].value, 1.0f);
            return;

        } else if (_states[_currentIndex].state == ActuatorState::StopRequest) {
            // Send stop command (NAN value, 0 timeout)
            _states[_currentIndex].state = ActuatorState::Stopping;
            emit actuatorStateChanged(_currentIndex, ActuatorState::Stopping);

            postEvent("send_command");
            _sendMavlinkRequest(actuator->function(), NAN, 0.0f);
            return;
        }
    }
}

void ActuatorTestingStateMachine::_sendMavlinkRequest(int function, float value, float timeout)
{
    qCDebug(ActuatorTestingLog) << "Sending actuator test - function:" << function << "value:" << value;

    Vehicle::MavCmdAckHandlerInfo_t handlerInfo = {};
    handlerInfo.resultHandler = _ackHandlerEntry;
    handlerInfo.resultHandlerData = this;

    _vehicle->sendMavCommandWithHandler(
        &handlerInfo,
        MAV_COMP_ID_AUTOPILOT1,
        MAV_CMD_ACTUATOR_TEST,
        value,
        timeout,
        0,
        0,
        1000 + function,
        0,
        0);

    _commandInProgress = true;
    postEvent("command_sent");
}

void ActuatorTestingStateMachine::_ackHandlerEntry(void* resultHandlerData, int /*compId*/,
                                                    const mavlink_command_ack_t& ack,
                                                    Vehicle::MavCmdResultFailureCode_t failureCode)
{
    auto* self = static_cast<ActuatorTestingStateMachine*>(resultHandlerData);
    self->_handleAck(static_cast<MAV_RESULT>(ack.result), failureCode);
}

void ActuatorTestingStateMachine::_handleAck(MAV_RESULT result, Vehicle::MavCmdResultFailureCode_t failureCode)
{
    _commandInProgress = false;

    if (failureCode == Vehicle::MavCmdResultFailureNoResponseToCommand) {
        // Timeout - try next actuator
        qCDebug(ActuatorTestingLog) << "Command timeout for actuator" << _currentIndex;
        postEvent("timeout");
        _sendNext();
        return;
    }

    if (result == MAV_RESULT_ACCEPTED) {
        // Success
        if (_currentIndex >= 0 && _currentIndex < _states.size()) {
            if (_states[_currentIndex].state == ActuatorState::Stopping) {
                _states[_currentIndex].state = ActuatorState::NotActive;
                emit actuatorStateChanged(_currentIndex, ActuatorState::NotActive);
            }
        }
        postEvent("ack_received");
        _sendNext();
        return;
    }

    // Failure
    if (_currentIndex >= 0 && _currentIndex < _states.size()) {
        _states[_currentIndex].state = ActuatorState::NotActive;
        emit actuatorStateChanged(_currentIndex, ActuatorState::NotActive);
    }

    if (!_hadFailure) {
        QString message;
        switch (result) {
        case MAV_RESULT_TEMPORARILY_REJECTED:
            message = tr("Actuator test command temporarily rejected");
            break;
        case MAV_RESULT_DENIED:
            message = tr("Actuator test command denied");
            break;
        case MAV_RESULT_UNSUPPORTED:
            message = tr("Actuator test command not supported");
            break;
        default:
            message = tr("Actuator test command failed");
            break;
        }

        qCWarning(ActuatorTestingLog) << message;
        emit commandFailed(message);
        _hadFailure = true;
        emit hadFailureChanged();
    }

    postEvent("ack_received");
    _sendNext();
}
