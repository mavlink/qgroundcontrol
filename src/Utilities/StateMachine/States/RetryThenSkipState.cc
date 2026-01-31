#include "RetryThenSkipState.h"
#include "QGCLoggingCategory.h"

RetryThenSkipState::RetryThenSkipState(const QString& stateName, QState* parent,
                                       Action action, int maxRetries, int retryDelayMsecs)
    : QGCState(stateName, parent)
    , _action(std::move(action))
    , _maxRetries(maxRetries)
    , _retryDelayMsecs(retryDelayMsecs)
{
    _retryTimer.setSingleShot(true);
    connect(&_retryTimer, &QTimer::timeout, this, &RetryThenSkipState::_executeAction);
}

void RetryThenSkipState::onEnter()
{
    _currentAttempt = 0;
    _wasSkipped = false;
    _executeAction();
}

void RetryThenSkipState::_executeAction()
{
    _currentAttempt++;
    int totalAttempts = _maxRetries + 1;

    qCDebug(QGCStateMachineLog) << stateName() << "attempt" << _currentAttempt
                                 << "of" << totalAttempts;

    bool success = false;
    if (_action) {
        success = _action();
    }

    if (success) {
        qCDebug(QGCStateMachineLog) << stateName() << "succeeded on attempt" << _currentAttempt;
        _wasSkipped = false;
        emit succeeded();
        emit advance();
    } else if (_currentAttempt <= _maxRetries) {
        // More retries available
        qCDebug(QGCStateMachineLog) << stateName() << "failed, retrying in"
                                     << _retryDelayMsecs << "ms";
        emit retrying(_currentAttempt + 1, totalAttempts);
        _retryTimer.start(_retryDelayMsecs);
    } else {
        // All retries exhausted - skip and continue
        qCDebug(QGCStateMachineLog) << stateName() << "all retries exhausted, skipping";
        _wasSkipped = true;
        emit skipped();
        emit advance();  // Continue instead of error
    }
}
