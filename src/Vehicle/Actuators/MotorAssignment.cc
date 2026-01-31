#include "MotorAssignment.h"
#include "MotorAssignmentStateMachine.h"
#include "QmlObjectListModel.h"

MotorAssignment::MotorAssignment(QObject* parent, Vehicle* vehicle, QmlObjectListModel* actuators)
    : QObject(parent)
    , _stateMachine(new MotorAssignmentStateMachine(vehicle, actuators, this))
{
    connect(_stateMachine, &MotorAssignmentStateMachine::activeChanged,
            this, &MotorAssignment::_onStateMachineActiveChanged);
    connect(_stateMachine, &MotorAssignmentStateMachine::messageChanged,
            this, &MotorAssignment::_onStateMachineMessageChanged);
    connect(_stateMachine, &MotorAssignmentStateMachine::assignmentComplete,
            this, &MotorAssignment::_onAssignmentComplete);
    connect(_stateMachine, &MotorAssignmentStateMachine::assignmentAborted,
            this, &MotorAssignment::_onAssignmentAborted);
    connect(_stateMachine, &MotorAssignmentStateMachine::assignmentError,
            this, &MotorAssignment::_onAssignmentError);
}

MotorAssignment::~MotorAssignment()
{
    if (_stateMachine->isRunning()) {
        _stateMachine->stop();
    }
}

bool MotorAssignment::initAssignment(int selectedActuatorIdx, int firstMotorsFunction, int numMotors)
{
    bool result = _stateMachine->initialize(selectedActuatorIdx, firstMotorsFunction, numMotors);
    _message = _stateMachine->message();
    emit messageChanged();
    return result;
}

void MotorAssignment::start()
{
    _stateMachine->startAssignment();
}

void MotorAssignment::selectMotor(int motorIndex)
{
    _stateMachine->selectMotor(motorIndex);
}

void MotorAssignment::spinCurrentMotor()
{
    // In the new state machine, spinning happens automatically on state entry
    // This method is kept for API compatibility but the state machine handles timing
}

void MotorAssignment::abort()
{
    _stateMachine->abortAssignment();
}

bool MotorAssignment::active() const
{
    return _stateMachine->isActive();
}

void MotorAssignment::_onStateMachineActiveChanged()
{
    emit activeChanged();
}

void MotorAssignment::_onStateMachineMessageChanged()
{
    _message = _stateMachine->message();
    emit messageChanged();
}

void MotorAssignment::_onAssignmentComplete()
{
    emit activeChanged();
}

void MotorAssignment::_onAssignmentAborted()
{
    emit onAbort();
    emit activeChanged();
}

void MotorAssignment::_onAssignmentError(const QString& error)
{
    _message = error;
    emit messageChanged();
    emit onAbort();
    emit activeChanged();
}
