#include "RetryState.h"
#include "QGCLoggingCategory.h"

RetryState::RetryState(const QString& stateName, QState* parent,
                       Action action, int maxRetries, int retryDelayMsecs,
                       ExhaustedBehavior exhaustedBehavior)
    : QGCState(stateName, parent)
    , _action(std::move(action))
    , _maxRetries(maxRetries)
    , _retryDelayMsecs(retryDelayMsecs)
    , _exhaustedBehavior(exhaustedBehavior)
{
    _retryTimer.setSingleShot(true);
    connect(&_retryTimer, &QTimer::timeout, this, &RetryState::_executeAction);
}

void RetryState::onEnter()
{
    _currentAttempt = 0;
    _wasSkipped = false;
    _executeAction();
}

void RetryState::_executeAction()
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
        qCDebug(QGCStateMachineLog) << stateName() << "failed, retrying in"
                                     << _retryDelayMsecs << "ms";
        emit retrying(_currentAttempt + 1, totalAttempts());
        _retryTimer.start(_retryDelayMsecs);
    } else {
        qCDebug(QGCStateMachineLog) << stateName() << "all retries exhausted";
        emit exhausted();

        if (_exhaustedBehavior == EmitAdvance) {
            _wasSkipped = true;
            emit skipped();
            emit advance();
        } else {
            emit error();
        }
    }
}
