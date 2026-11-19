#include "DelayState.h"

#include <chrono>

#include <QtStateMachine/QStateMachine>

DelayState::DelayState(QState* parentState, int delayMsecs)
    : QGCState("DelayState", parentState)
    , _delayMsecs(delayMsecs)
{
    connect(this, &QState::entered, this, &DelayState::_startDelay);
    connect(this, &QState::exited, this, &DelayState::_stopDelay);
}

void DelayState::_startDelay()
{
    qCDebug(QGCStateMachineLog) << stateName() << "Starting delay of" << _delayMsecs << "ms";

    // Qt exits no state of a machine that stops. The machine is not known before the state is entered.
    const QStateMachine* const qtMachine = QAbstractState::machine();  // clazy:exclude=skipped-base-method
    if (qtMachine) {
        connect(qtMachine, &QStateMachine::stopped, this, &DelayState::_stopDelay, Qt::UniqueConnection);
    }

    _delayTask.emplace(scheduler(), this);
    _delayTask->schedule(std::chrono::milliseconds(_delayMsecs), [this]() { emit advance(); });
}

void DelayState::_stopDelay()
{
    _delayTask.reset();
}
