#include "SubMachineState.h"
#include "QGCStateMachine.h"
#include "QGCLoggingCategory.h"

SubMachineState::SubMachineState(const QString& stateName, QState* parent, MachineFactory factory)
    : QGCState(stateName, parent)
    , _factory(std::move(factory))
{
}

void SubMachineState::onEntry(QEvent* event)
{
    QGCState::onEntry(event);

    if (!_factory) {
        qCWarning(QGCStateMachineLog) << stateName() << "no machine factory provided";
        emit error();
        return;
    }

    _childMachine = _factory(this);
    if (!_childMachine) {
        qCWarning(QGCStateMachineLog) << stateName() << "factory returned null machine";
        emit error();
        return;
    }

    qCDebug(QGCStateMachineLog) << stateName() << "starting child machine:" << _childMachine->machineName();

    connect(_childMachine, &QStateMachine::finished, this, &SubMachineState::_onChildFinished);
    connect(_childMachine, &QGCStateMachine::error, this, &SubMachineState::_onChildError);

    _childMachine->start();
}

void SubMachineState::onExit(QEvent* event)
{
    if (_childMachine && _childMachine->isRunning()) {
        qCDebug(QGCStateMachineLog) << stateName() << "stopping child machine on exit";
        _childMachine->stop();
    }

    if (_childMachine) {
        _childMachine->deleteLater();
        _childMachine = nullptr;
    }

    QGCState::onExit(event);
}

void SubMachineState::_onChildFinished()
{
    qCDebug(QGCStateMachineLog) << stateName() << "child machine finished";
    emit advance();
}

void SubMachineState::_onChildError()
{
    qCDebug(QGCStateMachineLog) << stateName() << "child machine error";
    emit childError();
    emit error();
}
