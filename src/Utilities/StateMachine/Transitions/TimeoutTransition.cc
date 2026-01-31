#include "TimeoutTransition.h"

#include <QtStateMachine/QState>
#include <QtStateMachine/QStateMachine>

TimeoutTransition::TimeoutTransition(int timeoutMsecs, QAbstractState* target)
    : QGCSignalTransition(this, &TimeoutTransition::timeout)
    , _timeoutMsecs(timeoutMsecs)
{
    if (target) {
        setTargetState(target);
    }

    _timer.setSingleShot(true);
    connect(&_timer, &QTimer::timeout, this, &TimeoutTransition::timeout);
}

void TimeoutTransition::_onAddedToState()
{
    // Connect to source state's entered/exited signals for timer control
    if (auto* source = qobject_cast<QState*>(sourceState())) {
        connect(source, &QState::entered, this, &TimeoutTransition::_onSourceStateEntered);
        connect(source, &QState::exited, this, &TimeoutTransition::_onSourceStateExited);
    }
}

void TimeoutTransition::_onSourceStateEntered()
{
    _timer.start(_timeoutMsecs);
}

void TimeoutTransition::_onSourceStateExited()
{
    _timer.stop();
}

bool TimeoutTransition::event(QEvent* e)
{
    // Detect when transition is added to a state (parent changes)
    if (e->type() == QEvent::ParentChange) {
        _onAddedToState();
    }
    return QGCSignalTransition::event(e);
}

void TimeoutTransition::onTransition(QEvent* event)
{
    Q_UNUSED(event);
    // Timer already stopped by _onSourceStateExited or will be stopped when we leave
}
