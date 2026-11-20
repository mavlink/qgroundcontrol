#include "RetryTransition.h"
#include "QGCState.h"
#include "WaitStateBase.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(RetryTransitionLog, "Utilities.StateMachine.RetryTransition")

void RetryTransition::reset()
{
    _retryCount = 0;
    _updateTargetState();
}

void RetryTransition::_updateTargetState()
{
    setTargetState((_retryCount < _maxRetries) ? nullptr : _target.data());
}

bool RetryTransition::eventTest(QEvent* event)
{
    // The source state is known once the transition has been added to it. That is early enough here: the count
    // changes only in onTransition().
    if (!_resetOnEntry && sourceState()) {
        _resetOnEntry = connect(sourceState(), &QAbstractState::entered, this, &RetryTransition::reset);
    }

    return QSignalTransition::eventTest(event);
}

void RetryTransition::onTransition(QEvent* event)
{
    Q_UNUSED(event);

    QString stateName = QStringLiteral("unknown");
    if (auto* state = qobject_cast<QGCState*>(sourceState())) {
        stateName = state->stateName();
    }

    if (_retryCount >= _maxRetries) {
        qCWarning(RetryTransitionLog) << stateName << "timeout after" << _maxRetries << "retries, advancing";
        // Qt has worked out where this transition leads, so the target stays until the state is entered again
        _retryCount = 0;
        return;
    }

    _retryCount++;
    qCWarning(RetryTransitionLog) << stateName << "timeout, retry" << _retryCount << "of" << _maxRetries;
    // In effect from the next signal on
    _updateTargetState();

    if (auto* waitState = qobject_cast<WaitStateBase*>(sourceState())) {
        waitState->restartWait();
    }

    if (_retryAction) {
        _retryAction();
    }
}
