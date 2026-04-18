#include "TimeoutTransition.h"

#include <QtStateMachine/QState>
#include <QtStateMachine/QStateMachine>

TimeoutTransition::TimeoutTransition(int timeoutMsecs, QAbstractState* target)
    : QGCSignalTransition(this, SIGNAL(timeout()))
    , _timeoutMsecs(timeoutMsecs)
{
    if (target) {
        setTargetState(target);
    }

    _timer.setSingleShot(true);
    connect(&_timer, &QTimer::timeout, this, &TimeoutTransition::timeout);
}

void TimeoutTransition::attachToSourceState(QState* source)
{
    if (!source || _sourceAttached) {
        return;
    }

    connect(source, &QState::entered, this, &TimeoutTransition::_onSourceStateEntered, Qt::UniqueConnection);
    connect(source, &QState::exited, this, &TimeoutTransition::_onSourceStateExited, Qt::UniqueConnection);
    _sourceAttached = true;
}

void TimeoutTransition::_onAddedToState()
{
    attachToSourceState(qobject_cast<QState*>(sourceState()));
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
    if (e->type() == QEvent::ParentChange) {
        _onAddedToState();

        // sourceState() may not be finalized during the immediate ParentChange
        // callback in some addTransition paths; retry once on the next turn.
        if (!_sourceAttached) {
            QTimer::singleShot(0, this, [this]() { _onAddedToState(); });
        }
    }
    return QGCSignalTransition::event(e);
}

void TimeoutTransition::onTransition(QEvent* event)
{
    Q_UNUSED(event);
}
