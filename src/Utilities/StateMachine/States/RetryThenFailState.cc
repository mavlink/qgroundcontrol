#include "RetryThenFailState.h"
#include "QGCLoggingCategory.h"

RetryThenFailState::RetryThenFailState(const QString& stateName, QState* parent,
                                       Action action, int maxRetries, int retryDelayMsecs)
    : QGCState(stateName, parent)
    , _action(std::move(action))
    , _maxRetries(maxRetries)
    , _retryDelayMsecs(retryDelayMsecs)
{
    _retryTimer.setSingleShot(true);
    connect(&_retryTimer, &QTimer::timeout, this, &RetryThenFailState::_executeAction);
}

void RetryThenFailState::onEnter()
{
    _currentAttempt = 0;
    _executeAction();
}

void RetryThenFailState::_executeAction()
{
    _currentAttempt++;

    qCDebug(QGCStateMachineLog) << stateName() << "attempt" << _currentAttempt
                                 << "of" << totalAttempts();

    bool success = false;
    if (_action) {
        success = _action();
    }

    if (success) {
        qCDebug(QGCStateMachineLog) << stateName() << "succeeded on attempt" << _currentAttempt;
        emit succeeded();
        emit advance();
    } else if (_currentAttempt <= _maxRetries) {
        // More retries available
        qCDebug(QGCStateMachineLog) << stateName() << "failed, retrying in"
                                     << _retryDelayMsecs << "ms";
        emit retrying(_currentAttempt + 1, totalAttempts());
        _retryTimer.start(_retryDelayMsecs);
    } else {
        // All retries exhausted
        qCDebug(QGCStateMachineLog) << stateName() << "all retries exhausted";
        emit exhausted();
        emit error();
    }
}
