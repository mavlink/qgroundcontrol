#include "ActuatorTesting.h"
#include "ActuatorTestingStateMachine.h"
#include "QGCApplication.h"

using namespace ActuatorTesting;

ActuatorTest::ActuatorTest(Vehicle* vehicle)
    : _vehicle(vehicle)
    , _stateMachine(new ActuatorTestingStateMachine(vehicle, _actuators, this))
{
    connect(_stateMachine, &ActuatorTestingStateMachine::commandFailed,
            this, &ActuatorTest::_onCommandFailed);
    connect(_stateMachine, &ActuatorTestingStateMachine::hadFailureChanged,
            this, &ActuatorTest::hadFailureChanged);
}

ActuatorTest::~ActuatorTest()
{
    _actuators->clearAndDeleteContents();
    delete _allMotorsActuator;
}

void ActuatorTest::updateFunctions(const QList<Actuator*>& actuators)
{
    _actuators->clearAndDeleteContents();

    if (_allMotorsActuator) {
        _allMotorsActuator->deleteLater();
    }

    _allMotorsActuator = nullptr;

    Actuator* motorActuator = nullptr;
    for (const auto& actuator : actuators) {
        if (actuator->isMotor()) {
            motorActuator = actuator;
        }
        _actuators->append(actuator);
    }

    if (motorActuator) {
        _allMotorsActuator = new Actuator(this, tr("All Motors"),
                                          motorActuator->min(), motorActuator->max(),
                                          motorActuator->defaultValue(),
                                          motorActuator->function(), true);
    }

    _stateMachine->resetStates();
    emit actuatorsChanged();
}

void ActuatorTest::setActive(bool active)
{
    _stateMachine->setActive(active);
}

void ActuatorTest::setChannelTo(int index, float value)
{
    _stateMachine->setChannelTo(index, value);
}

void ActuatorTest::stopControl(int index)
{
    _stateMachine->stopControl(index);
}

bool ActuatorTest::hadFailure() const
{
    return _stateMachine->hadFailure();
}

void ActuatorTest::_onCommandFailed(const QString& message)
{
    qgcApp()->showAppMessage(message);
}
