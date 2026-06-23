#include "RetryTransition.h"
#include "QGCState.h"
#include "WaitStateBase.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(RetryTransitionLog, "Utilities.StateMachine.RetryTransition")

bool RetryTransition::eventTest(QEvent* event)
{
    if (!QGCSignalTransition::eventTest(event)) {
        return false;
    }

    // Get source state name for logging
    QString stateName = QStringLiteral("unknown");
    if (auto* state = qobject_cast<QGCState*>(sourceState())) {
        stateName = state->stateName();
    }

    if (_retryCount < _maxRetries) {
        _retryCount++;
        qCDebug(RetryTransitionLog) << stateName << "timeout, retry" << _retryCount << "of" << _maxRetries;

        if (auto* waitState = qobject_cast<WaitStateBase*>(sourceState())) {
            waitState->restartWait();
        }

        if (_retryAction) {
            _retryAction();
        }

        // Block the transition - stay in current state
        return false;
    }

    // Max retries exhausted - allow transition
    qCWarning(RetryTransitionLog) << stateName << "timeout after" << _maxRetries << "retries, advancing";
    return true;
}

void RetryTransition::onTransition(QEvent* event)
{
    Q_UNUSED(event);

    // Reset for potential future use (if state is re-entered)
    _retryCount = 0;
}
