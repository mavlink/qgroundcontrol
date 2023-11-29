/****************************************************************************
 *
 * (c) 2021 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ActuatorTesting.h"
#include "Common.h"

#include "QGCApplication.h"

#include <QDebug>

#include <cmath>

using namespace ActuatorTesting;

ActuatorTest::ActuatorTest(Vehicle* vehicle)
    : _vehicle(vehicle)
{
    _watchdogTimer.setInterval(100);
    _watchdogTimer.setSingleShot(false);
    connect(&_watchdogTimer, &QTimer::timeout, this, &ActuatorTest::watchdogTimeout);
    _watchdogTimer.start();
}

ActuatorTest::~ActuatorTest()
{
    _actuators->clearAndDeleteContents();
    delete _allMotorsActuator;
}

void ActuatorTest::updateFunctions(const QList<Actuator*> &actuators)
{
    _actuators->clearAndDeleteContents();
    _allMotorsActuator->deleteLater();
    _allMotorsActuator = nullptr;

    Actuator* motorActuator{nullptr};
    for (const auto& actuator : actuators) {
        if (actuator->isMotor()) {
            motorActuator = actuator;
        }
        _actuators->append(actuator);
    }
    if (motorActuator) {
        _allMotorsActuator = new Actuator(this, tr("All Motors"), motorActuator->min(), motorActuator->max(), motorActuator->defaultValue(),
                motorActuator->function(), true);
    }
    resetStates();

    emit actuatorsChanged();
}

void ActuatorTest::resetStates()
{
    _states.clear();
    _currentState = -1;
    for (int i = 0; i < _actuators->count(); ++i) {
        _states.append(ActuatorState{});
    }
}

void ActuatorTest::watchdogTimeout()
{
    for (int i = 0; i < _states.size(); ++i) {
        if (_states[i].state == ActuatorState::State::Active) {
            if (_states[i].lastUpdated.elapsed() > 100) {
                qCWarning(ActuatorsConfigLog) << "Stopping actuator due to timeout:" << i;
                _states[i].state = ActuatorState::State::StopRequest;
            }
        }
    }
    sendNext();
}

void ActuatorTest::setChannelTo(int index, float value)
{
    if (!_active || index >= _states.size()) {
        return;
    }
    qCDebug(ActuatorsConfigLog) << "setting actuator: index:" << index << "value:" << value;

    _states[index].value = value;
    _states[index].state = ActuatorState::State::Active;
    _states[index].lastUpdated.start();
    sendNext();
}

void ActuatorTest::stopControl(int index)
{
    if (index >= _states.size() || index < -1) {
        return;
    }
    qCDebug(ActuatorsConfigLog) << "stop actuator control: index:" << index;

    if (index == -1) {
        for (int i = 0; i < _states.size(); ++i) {
            if (_states[i].state == ActuatorState::State::Active) {
                _states[i].state = ActuatorState::State::StopRequest;
            }
        }
    } else {
        if (_states[index].state == ActuatorState::State::Active) {
            _states[index].state = ActuatorState::State::StopRequest;
        }
    }
    sendNext();
}

void ActuatorTest::setActive(bool active)
{
    qCDebug(ActuatorsConfigLog) << "setting active: " << active;
    if (!active) {
        stopControl(-1);
    }
    _active = active;
}

void ActuatorTest::ackHandlerEntry(void* resultHandlerData, int compId, MAV_RESULT commandResult, uint8_t progress,
        Vehicle::MavCmdResultFailureCode_t failureCode)
{
    ActuatorTest* actuatorTest = (ActuatorTest*)resultHandlerData;
    actuatorTest->ackHandler(commandResult, failureCode);
}

void ActuatorTest::ackHandler(MAV_RESULT commandResult, Vehicle::MavCmdResultFailureCode_t failureCode)
{
    // upon receiving an (n)ack, continuously cycle through the active actuators, one at a time
    _commandInProgress = false;
    if (failureCode == Vehicle::MavCmdResultFailureNoResponseToCommand) {
        // on timeout, just try the next one
        sendNext();
    } else if (commandResult == MAV_RESULT_ACCEPTED) {
        if (_currentState != -1 && _states[_currentState].state == ActuatorState::State::Stopping) {
            _states[_currentState].state = ActuatorState::State::NotActive;
        }
        sendNext();
    } else { // failure
        if (_currentState != -1) {
            _states[_currentState].state = ActuatorState::State::NotActive;
        }

        if (!_hadFailure) {
            QString message;
            if (commandResult == MAV_RESULT_TEMPORARILY_REJECTED) {
                message = tr("Actuator test command temporarily rejected");
            } else if (commandResult == MAV_RESULT_DENIED) {
                message = tr("Actuator test command denied");
            } else if (commandResult == MAV_RESULT_UNSUPPORTED) {
                message = tr("Actuator test command not supported");
            } else {
                message = tr("Actuator test command failed");
            }
            qgcApp()->showAppMessage(message);
            _hadFailure = true;
            emit hadFailureChanged();
        }
        sendNext();
    }
}

void ActuatorTest::sendNext()
{
    if (_commandInProgress) {
        return;
    }

    // find the next actuator not in state NotActive
    for (int i = 0; i < _states.size(); ++i) {
        _currentState = (_currentState + 1) % _states.size();
        Actuator* actuator = _actuators->value<Actuator*>(_currentState);
        if (_states[_currentState].state == ActuatorState::State::Active) {
            sendMavlinkRequest(actuator->function(), _states[_currentState].value, 1.f);
            break;
        } else if (_states[_currentState].state == ActuatorState::State::StopRequest) {
            _states[_currentState].state = ActuatorState::State::Stopping;
            sendMavlinkRequest(actuator->function(), NAN, 0.f);
            break;
        }
    }
}

void ActuatorTest::sendMavlinkRequest(int function, float value, float timeout)
{
    qCDebug(ActuatorsConfigLog) << "Sending actuator test function:" << function << "value:" << value;

    // TODO: consider using a lower command timeout

    _vehicle->sendMavCommandWithHandler(
            ackHandlerEntry,                  // Ack callback
            this,                             // Ack callback data
            MAV_COMP_ID_AUTOPILOT1,           // the ID of the autopilot
            MAV_CMD_ACTUATOR_TEST,            // the mavlink command
            value,                            // value
            timeout,                          // timeout
            0,                                // unused parameter
            0,                                // unused parameter
            1000+function,                    // function
            0,                                // unused parameter
            0);
    _commandInProgress = true;
}
