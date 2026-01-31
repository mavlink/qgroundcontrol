#include "MotorAssignmentStateMachine.h"
#include "ActuatorOutputs.h"
#include "Fact.h"
#include "QmlObjectListModel.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

#include <QtCore/QTimer>

#include <vector>

QGC_LOGGING_CATEGORY(MotorAssignmentLog, "Vehicle.Actuators.MotorAssignment")

MotorAssignmentStateMachine::MotorAssignmentStateMachine(Vehicle* vehicle, QmlObjectListModel* actuators, QObject* parent)
    : QGCStateMachine("MotorAssignment", vehicle, parent)
    , _vehicle(vehicle)
    , _actuators(actuators)
{
    _buildStateMachine();

    connect(this, &QStateMachine::runningChanged, this, &MotorAssignmentStateMachine::activeChanged);
}

void MotorAssignmentStateMachine::_buildStateMachine()
{
    // Create states
    _idleState = new QGCState("Idle", this);
    _initializedState = new QGCState("Initialized", this);
    _assigningMotorsState = new QGCState("AssigningMotors", this);
    _spinningMotorState = new QGCState("SpinningMotor", this);
    _waitingForSelectionState = new QGCState("WaitingForSelection", this);
    _applyingAssignmentState = new QGCState("ApplyingAssignment", this);
    _completeState = new QGCFinalState("Complete", this);
    _errorState = new QGCState("Error", this);

    // Idle state - entry point
    setInitialState(_idleState);

    // Initialized state - waiting for user to confirm
    connect(_initializedState, &QAbstractState::entered, this, [this]() {
        qCDebug(MotorAssignmentLog) << "Waiting for user confirmation";
    });

    // Assigning motors state - clear existing and assign to selected output
    connect(_assigningMotorsState, &QAbstractState::entered, this, [this]() {
        qCDebug(MotorAssignmentLog) << "Assigning motors to selected output";
        _clearAndAssignMotors();
        // Transition happens via signal after assignment
    });

    // Spinning motor state - send MAVLink command
    connect(_spinningMotorState, &QAbstractState::entered, this, [this]() {
        int motorIndex = _selectedMotors.size();
        qCDebug(MotorAssignmentLog) << "Spinning motor" << motorIndex;
        emit motorSpinStarted(motorIndex);
        _spinMotor(motorIndex);
    });

    // Waiting for selection state
    connect(_waitingForSelectionState, &QAbstractState::entered, this, [this]() {
        qCDebug(MotorAssignmentLog) << "Waiting for user to select motor" << _selectedMotors.size();
    });

    // Applying assignment state - finalize motor ordering
    connect(_applyingAssignmentState, &QAbstractState::entered, this, [this]() {
        qCDebug(MotorAssignmentLog) << "Applying final motor assignment";
        _applyFinalAssignment();
    });

    // Complete state
    connect(_completeState, &QAbstractState::entered, this, [this]() {
        qCDebug(MotorAssignmentLog) << "Motor assignment complete";
        emit assignmentComplete();
    });

    // Error state
    connect(_errorState, &QAbstractState::entered, this, [this]() {
        qCWarning(MotorAssignmentLog) << "Motor assignment error:" << _message;
        emit assignmentError(_message);
    });

    // Note: Transitions are added dynamically during initialize() and runtime
    // because they depend on configuration (e.g., whether motor reassignment is needed)
}

bool MotorAssignmentStateMachine::initialize(int selectedActuatorIdx, int firstMotorsFunction, int numMotors)
{
    if (isRunning()) {
        qCWarning(MotorAssignmentLog) << "Cannot initialize while running";
        return false;
    }

    _selectedActuatorIdx = selectedActuatorIdx;
    _firstMotorsFunction = firstMotorsFunction;
    _numMotors = numMotors;
    _selectedMotors.clear();
    _assignMotors = false;

    _gatherFunctionFacts();

    if (!_validateConfiguration()) {
        return false;
    }

    return true;
}

void MotorAssignmentStateMachine::_gatherFunctionFacts()
{
    _functionFacts.clear();
    for (int groupIdx = 0; groupIdx < _actuators->count(); groupIdx++) {
        auto group = qobject_cast<ActuatorOutputs::ActuatorOutput*>(_actuators->get(groupIdx));
        _functionFacts.append(QList<Fact*>{});
        group->getAllChannelFunctions(_functionFacts.last());
    }
}

bool MotorAssignmentStateMachine::_validateConfiguration()
{
    // Check the current configuration for these cases:
    // - no motors assigned: ask to assign to selected output
    // - there are motors, but none assigned to the selected output: ask to remove and assign to selected output
    // - partially assigned motors: abort with error
    // - all assigned to current output: nothing further to do

    std::vector<bool> selectedAssignment(_numMotors);
    std::vector<bool> otherAssignment(_numMotors);

    int index = 0;
    for (auto& group : _functionFacts) {
        bool selected = index == _selectedActuatorIdx;
        for (auto& fact : group) {
            int currentFunction = fact->rawValue().toInt();
            if (currentFunction >= _firstMotorsFunction && currentFunction < _firstMotorsFunction + _numMotors) {
                if (selected) {
                    selectedAssignment[currentFunction - _firstMotorsFunction] = true;
                } else {
                    otherAssignment[currentFunction - _firstMotorsFunction] = true;
                }
            }
        }
        ++index;
    }

    int numAssigned = 0;
    int numAssignedToSelected = 0;
    for (int i = 0; i < _numMotors; ++i) {
        numAssigned += selectedAssignment[i] || otherAssignment[i];
        numAssignedToSelected += selectedAssignment[i];
    }

    QString selectedActuatorOutputName = qobject_cast<ActuatorOutputs::ActuatorOutput*>(
        _actuators->get(_selectedActuatorIdx))->label();

    QString extraMessage;
    if (numAssigned == 0 && _functionFacts[_selectedActuatorIdx].size() >= _numMotors) {
        _assignMotors = true;
        extraMessage = tr(
R"(<br />No motors are assigned yet.
By saying yes, all motors will be assigned to the first %1 channels of the selected output (%2)
 (you can also first assign all motors, then start the identification).<br />)")
            .arg(_numMotors).arg(selectedActuatorOutputName);

    } else if (numAssigned > 0 && numAssignedToSelected == 0 && _functionFacts[_selectedActuatorIdx].size() >= _numMotors) {
        _assignMotors = true;
        extraMessage = tr(
R"(<br />Motors are currently assigned to a different output.
By saying yes, all motors will be reassigned to the first %1 channels of the selected output (%2).<br />)")
            .arg(_numMotors).arg(selectedActuatorOutputName);

    } else if (numAssigned < _numMotors) {
        _message = tr("Not all motors are assigned yet. Either clear all existing assignments or assign all motors to an output.");
        emit messageChanged();
        return false;
    }

    _message = tr(
R"(This will automatically spin individual motors at 15%% thrust.<br /><br />
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

    return true;
}

void MotorAssignmentStateMachine::startAssignment()
{
    if (isRunning()) {
        qCWarning(MotorAssignmentLog) << "Already running";
        return;
    }

    _selectedMotors.clear();

    // Set up transitions before starting
    // Idle -> AssigningMotors or SpinningMotor
    _idleState->addTransition(new MachineEventTransition("start_with_assign", _assigningMotorsState));
    _idleState->addTransition(new MachineEventTransition("start_direct", _spinningMotorState));

    // AssigningMotors -> SpinningMotor (after delay for ESC init)
    _assigningMotorsState->addTransition(new MachineEventTransition("assign_complete", _spinningMotorState));

    // SpinningMotor -> WaitingForSelection (after MAVLink ack)
    _spinningMotorState->addTransition(new MachineEventTransition("spin_complete", _waitingForSelectionState));

    // SpinningMotor -> Error (on MAVLink failure)
    _spinningMotorState->addTransition(new MachineEventTransition("spin_error", _errorState));

    // WaitingForSelection -> SpinningMotor (more motors to go)
    _waitingForSelectionState->addTransition(new MachineEventTransition("next_motor", _spinningMotorState));

    // WaitingForSelection -> ApplyingAssignment (all motors selected)
    _waitingForSelectionState->addTransition(new MachineEventTransition("all_selected", _applyingAssignmentState));

    // ApplyingAssignment -> Complete
    _applyingAssignmentState->addTransition(new MachineEventTransition("apply_complete", _completeState));

    // Any state -> Error on abort
    _spinningMotorState->addTransition(new MachineEventTransition("abort", _errorState));
    _waitingForSelectionState->addTransition(new MachineEventTransition("abort", _errorState));

    // Start the machine
    start();

    // Trigger the appropriate initial transition
    QTimer::singleShot(0, this, [this]() {
        if (_assignMotors) {
            postEvent("start_with_assign");
        } else {
            postEvent("start_direct");
        }
    });
}

void MotorAssignmentStateMachine::selectMotor(int motorIndex)
{
    if (!isRunning() || !configuration().contains(_waitingForSelectionState)) {
        qCDebug(MotorAssignmentLog) << "Not waiting for selection";
        return;
    }

    _selectedMotors.append(motorIndex);
    qCDebug(MotorAssignmentLog) << "Motor" << _selectedMotors.size() - 1 << "selected as" << motorIndex;

    if (_selectedMotors.size() == _numMotors) {
        // All motors selected, apply assignment
        postEvent("all_selected");
    } else {
        // More motors to go, spin next after delay
        QTimer::singleShot(SpinTimeoutDefaultMs, this, [this]() {
            if (isRunning()) {
                postEvent("next_motor");
            }
        });
    }
}

void MotorAssignmentStateMachine::abortAssignment()
{
    if (!isRunning()) {
        return;
    }

    _message = tr("Assignment aborted");
    postEvent("abort");
    emit assignmentAborted();
}

bool MotorAssignmentStateMachine::isActive() const
{
    return isRunning() && !configuration().contains(_idleState);
}

void MotorAssignmentStateMachine::_clearAndAssignMotors()
{
    // Clear all motors from all outputs
    for (auto& group : _functionFacts) {
        for (auto& fact : group) {
            int currentFunction = fact->rawValue().toInt();
            if (currentFunction >= _firstMotorsFunction && currentFunction < _firstMotorsFunction + _numMotors) {
                fact->setRawValue(0);
            }
        }
    }

    // Assign motors to selected output
    int index = 0;
    for (auto& fact : _functionFacts[_selectedActuatorIdx]) {
        fact->setRawValue(_firstMotorsFunction + index);
        if (++index >= _numMotors) {
            break;
        }
    }

    // Wait for ESCs to initialize, then continue
    QTimer::singleShot(SpinTimeoutAfterAssignMs, this, [this]() {
        if (isRunning()) {
            postEvent("assign_complete");
        }
    });
}

void MotorAssignmentStateMachine::_applyFinalAssignment()
{
    // Apply the final motor ordering based on user selections
    for (auto& group : _functionFacts) {
        for (auto& fact : group) {
            int currentFunction = fact->rawValue().toInt();
            if (currentFunction >= _firstMotorsFunction && currentFunction < _firstMotorsFunction + _numMotors) {
                // This is set to a motor -> update based on selection order
                fact->setRawValue(_firstMotorsFunction + _selectedMotors[currentFunction - _firstMotorsFunction]);
            }
        }
    }

    // Transition to complete
    QTimer::singleShot(0, this, [this]() {
        postEvent("apply_complete");
    });
}

void MotorAssignmentStateMachine::_spinMotor(int motorIndex)
{
    qCDebug(MotorAssignmentLog) << "Sending actuator test function:" << (_firstMotorsFunction + motorIndex) << "value: 0.15";

    Vehicle::MavCmdAckHandlerInfo_t handlerInfo = {};
    handlerInfo.resultHandler = _ackHandlerEntry;
    handlerInfo.resultHandlerData = this;

    _vehicle->sendMavCommandWithHandler(
        &handlerInfo,
        MAV_COMP_ID_AUTOPILOT1,
        MAV_CMD_ACTUATOR_TEST,
        0.15f,                              // value (15% thrust)
        0.5f,                               // timeout
        0,                                  // unused
        0,                                  // unused
        1000 + _firstMotorsFunction + motorIndex,  // function
        0,                                  // unused
        0);

    _commandInProgress = true;
}

void MotorAssignmentStateMachine::_ackHandlerEntry(void* resultHandlerData, int /*compId*/,
                                                    const mavlink_command_ack_t& ack,
                                                    Vehicle::MavCmdResultFailureCode_t failureCode)
{
    auto* self = static_cast<MotorAssignmentStateMachine*>(resultHandlerData);
    self->_handleSpinAck(static_cast<MAV_RESULT>(ack.result), failureCode);
}

void MotorAssignmentStateMachine::_handleSpinAck(MAV_RESULT result, Vehicle::MavCmdResultFailureCode_t failureCode)
{
    _commandInProgress = false;

    if (failureCode != Vehicle::MavCmdResultFailureNoResponseToCommand && result != MAV_RESULT_ACCEPTED) {
        _message = tr("Actuator test command failed");
        postEvent("spin_error");
        return;
    }

    // Success - transition to waiting for selection
    postEvent("spin_complete");
}

void MotorAssignmentStateMachine::_onStateChanged()
{
    emit activeChanged();
}
