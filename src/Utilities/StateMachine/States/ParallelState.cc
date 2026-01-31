#include "ParallelState.h"
#include "QGCLoggingCategory.h"

#include <QtStateMachine/QFinalState>

ParallelState::ParallelState(const QString& stateName, QState* parent)
    : QGCState(stateName, parent)
{
    setChildMode(QState::ParallelStates);
}

void ParallelState::addParallelState(QAbstractState* state)
{
    state->setParent(this);
}

void ParallelState::onEntry(QEvent* event)
{
    QGCState::onEntry(event);

    _completedChildren = 0;
    _activeChildren = 0;

    const auto children = findChildren<QAbstractState*>(Qt::FindDirectChildrenOnly);
    for (QAbstractState* child : children) {
        _activeChildren++;

        if (auto* finalState = qobject_cast<QFinalState*>(child)) {
            connect(finalState, &QFinalState::entered, this, &ParallelState::_onChildFinished, Qt::UniqueConnection);
        } else if (auto* state = qobject_cast<QState*>(child)) {
            connect(state, &QState::finished, this, &ParallelState::_onChildFinished, Qt::UniqueConnection);
        }
    }

    qCDebug(QGCStateMachineLog) << stateName() << "entered with" << _activeChildren << "parallel children";

    if (_activeChildren == 0) {
        emit allComplete();
        emit advance();
    }
}

void ParallelState::onExit(QEvent* event)
{
    QGCState::onExit(event);
    qCDebug(QGCStateMachineLog) << stateName() << "exited";
}

void ParallelState::_onChildFinished()
{
    _completedChildren++;
    qCDebug(QGCStateMachineLog) << stateName() << "child finished:" << _completedChildren << "/" << _activeChildren;
    _checkAllComplete();
}

void ParallelState::_checkAllComplete()
{
    if (_completedChildren >= _activeChildren && _activeChildren > 0) {
        qCDebug(QGCStateMachineLog) << stateName() << "all parallel children complete";
        emit allComplete();
        emit advance();
    }
}
