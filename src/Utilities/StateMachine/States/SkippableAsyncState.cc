#include "SkippableAsyncState.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QMetaObject>

SkippableAsyncState::SkippableAsyncState(const QString& stateName,
                                         QState* parent,
                                         SkipPredicate skipPredicate,
                                         SetupFunction setupFunc,
                                         SkipAction skipAction,
                                         int timeoutMsecs)
    : WaitStateBase(stateName, parent, timeoutMsecs)
    , _skipPredicate(std::move(skipPredicate))
    , _setupFunction(std::move(setupFunc))
    , _skipAction(std::move(skipAction))
{
}

void SkippableAsyncState::connectWaitSignal()
{
    // Connection is set up dynamically via connectToCompletion() in the setup function
}

void SkippableAsyncState::disconnectWaitSignal()
{
    if (_completionConnection) {
        disconnect(_completionConnection);
        _completionConnection = {};
    }
}

void SkippableAsyncState::onWaitEntered()
{
    _wasSkipped = false;

    // Evaluate skip predicate
    if (_skipPredicate && _skipPredicate()) {
        qCDebug(QGCStateMachineLog) << "Skip condition met, skipping" << stateName();
        _wasSkipped = true;

        // Execute skip action if provided
        if (_skipAction) {
            _skipAction();
        }

        // Defer signal emission to avoid firing during state entry
        // This ensures the state machine finishes entering before transitions are evaluated
        QMetaObject::invokeMethod(this, &SkippableAsyncState::skipped, Qt::QueuedConnection);
        return;
    }

    // Not skipped - execute the setup function
    qCDebug(QGCStateMachineLog) << "Condition not met, executing" << stateName();
    if (_setupFunction) {
        _setupFunction(this);
    }
}
