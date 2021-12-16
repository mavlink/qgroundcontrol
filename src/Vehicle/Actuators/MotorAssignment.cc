/****************************************************************************
 *
 * (c) 2021 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MotorAssignment.h"
#include "ActuatorOutputs.h"

#include "QGCApplication.h"

#include <vector>

MotorAssignment::MotorAssignment(QObject* parent, Vehicle* vehicle, QmlObjectListModel* actuators)
        : QObject(parent), _vehicle(vehicle), _actuators(actuators)
{
    _spinTimer.setInterval(1000);
    _spinTimer.setSingleShot(true);
    connect(&_spinTimer, &QTimer::timeout, this, &MotorAssignment::spinTimeout);
}

bool MotorAssignment::initAssignment(int selectedActuatorIdx, int firstMotorsFunction, int numMotors)
{
    if (_state != State::Idle) {
        qCWarning(ActuatorsConfigLog) << "Already init/running";
    }

    // gather all the function facts
    _functionFacts.clear();
    for (int groupIdx = 0; groupIdx < _actuators->count(); groupIdx++) {
        auto group = qobject_cast<ActuatorOutputs::ActuatorOutput*>(_actuators->get(groupIdx));
        _functionFacts.append(QList<Fact*>{});
        group->getAllChannelFunctions(_functionFacts.last());
    }

    // Check the current configuration for these cases:
    // - no motors assigned: ask to assign to selected output
    // - there are motors, but none assigned to the selected output: ask to remove and assign to selected output
    // - partially assigned motors: abort with error
    // - all assigned to current output: nothing further to do

    int index = 0;
    std::vector<bool> selectedAssignment(numMotors);
    std::vector<bool> otherAssignment(numMotors);
    for (auto& group : _functionFacts) {
        bool selected = index == selectedActuatorIdx;
        for (auto& fact : group) {
            int currentFunction = fact->rawValue().toInt();
            if (currentFunction >= firstMotorsFunction && currentFunction < firstMotorsFunction + numMotors) {
                if (selected) {
                    selectedAssignment[currentFunction - firstMotorsFunction] = true;
                } else {
                    otherAssignment[currentFunction - firstMotorsFunction] = true;
                }
            }
        }
        ++index;
    }
    int numAssigned = 0;
    int numAssignedToSelected = 0;
    for (int i = 0; i < numMotors; ++i) {
        numAssigned += selectedAssignment[i] || otherAssignment[i];
        numAssignedToSelected += selectedAssignment[i];
    }

    QString selectedActuatorOutputName = qobject_cast<ActuatorOutputs::ActuatorOutput*>(_actuators->get(selectedActuatorIdx))->label();

    _assignMotors = false;
    QString extraMessage;
    if (numAssigned == 0 && _functionFacts[selectedActuatorIdx].size() >= numMotors) {
        _assignMotors = true;
        extraMessage = tr(
R"(<br />No motors are assigned yet.
By saying yes, all motors will be assigned to the first %1 channels of the selected output (%2)
 (you can also first assign all motors, then start the identification).<br />)").arg(numMotors).arg(selectedActuatorOutputName);

    } else if (numAssigned > 0 && numAssignedToSelected == 0 && _functionFacts[selectedActuatorIdx].size() >= numMotors) {
        _assignMotors = true;
        extraMessage = tr(
R"(<br />Motors are currently assigned to a different output.
By saying yes, all motors will be reassigned to the first %1 channels of the selected output (%2).<br />)")
            .arg(numMotors).arg(selectedActuatorOutputName);

    } else if (numAssigned < numMotors) {
        _message = tr("Not all motors are assigned yet. Either clear all existing assignments or assign all motors to an output.");
        emit messageChanged();
        return false;
    }

    _message = tr(
R"(This will automatically spin individual motors at 15% thrust.<br /><br />
<b>Warning: Only proceed if you removed all propellers</b>.<br />
%1
<br />
The procedure is as following:<br />
- After confirming, the first motor starts to spin for 0.5 seconds.<br />
- Then click on the motor that was spinning.<br />
- The above steps are repeated for all motors.<br />
- The motor output functions will automatically be reassigned by the selected order.<br />
<br />
Do you wish to proceed?)").arg(extraMessage);
    emit messageChanged();

    _selectedActuatorIdx = selectedActuatorIdx;
    _firstMotorsFunction = firstMotorsFunction;
    _numMotors = numMotors;
    _selectedMotors.clear();
    _state = State::Init;
    emit activeChanged();
    return true;
}

void MotorAssignment::start()
{
    if (_state != State::Init) {
        qCWarning(ActuatorsConfigLog) << "Invalid state";
        return;
    }

    if (_assignMotors) {
        // clear all motors
        for (auto& group : _functionFacts) {
            for (auto& fact : group) {
                int currentFunction = fact->rawValue().toInt();
                if (currentFunction >= _firstMotorsFunction && currentFunction < _firstMotorsFunction + _numMotors) {
                    fact->setRawValue(0);
                }
            }
        }
        // assign to selected output
        int index = 0;
        for (auto& fact : _functionFacts[_selectedActuatorIdx]) {
            fact->setRawValue(_firstMotorsFunction + index);
            if (++index >= _numMotors) {
                break;
            }
        }
    }
    _state = State::Running;
    emit activeChanged();
    _spinTimer.start();
}

void MotorAssignment::selectMotor(int motorIndex)
{
    if (_state != State::Running) {
        qCDebug(ActuatorsConfigLog) << "Not running";
        return;
    }

    _selectedMotors.append(motorIndex);
    if (_selectedMotors.size() == _numMotors) {

        // finished, change functions
        for (auto& group : _functionFacts) {
            for (auto& fact : group) {
                int currentFunction = fact->rawValue().toInt();
                if (currentFunction >= _firstMotorsFunction && currentFunction < _firstMotorsFunction + _numMotors) {
                    // this is set to a motor -> update
                    fact->setRawValue(_firstMotorsFunction + _selectedMotors[currentFunction - _firstMotorsFunction]);
                }
            }
        }

        _state = State::Idle;
        emit activeChanged();
    } else {
        // spin the next motor after some time
        _spinTimer.start();
    }
}

void MotorAssignment::spinCurrentMotor()
{
    if (!_commandInProgress) {
        _spinTimer.stop();
        int motorIndex = _selectedMotors.size();
        sendMavlinkRequest(_firstMotorsFunction + motorIndex, 0.15f);
    }
}

void MotorAssignment::abort()
{
    _spinTimer.stop();
    _state = State::Idle;
    emit activeChanged();
    emit onAbort();
}

void MotorAssignment::spinTimeout()
{
    spinCurrentMotor();
}

void MotorAssignment::ackHandlerEntry(void* resultHandlerData, int compId, MAV_RESULT commandResult, uint8_t progress,
        Vehicle::MavCmdResultFailureCode_t failureCode)
{
    MotorAssignment* motorAssignment = (MotorAssignment*)resultHandlerData;
    motorAssignment->ackHandler(commandResult, failureCode);
}

void MotorAssignment::ackHandler(MAV_RESULT commandResult, Vehicle::MavCmdResultFailureCode_t failureCode)
{
    _commandInProgress = false;
    if (failureCode != Vehicle::MavCmdResultFailureNoResponseToCommand && commandResult != MAV_RESULT_ACCEPTED) {
        abort();
        qgcApp()->showAppMessage(tr("Actuator test command failed"));
    }
}

void MotorAssignment::sendMavlinkRequest(int function, float value)
{
    qCDebug(ActuatorsConfigLog) << "Sending actuator test function:" << function << "value:" << value;

    _vehicle->sendMavCommandWithHandler(
            ackHandlerEntry,                  // Ack callback
            this,                             // Ack callback data
            MAV_COMP_ID_AUTOPILOT1,           // the ID of the autopilot
            MAV_CMD_ACTUATOR_TEST,            // the mavlink command
            value,                            // value
            0.5f,                             // timeout
            0,                                // unused parameter
            0,                                // unused parameter
            1000+function,                    // function
            0,                                // unused parameter
            0);
    _commandInProgress = true;
}
