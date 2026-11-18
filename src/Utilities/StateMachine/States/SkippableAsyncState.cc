#include "SkippableAsyncState.h"

#include <QtCore/QMetaObject>

SkippableAsyncState::SkippableAsyncState(const QString& stateName, QState* parent, SkipPredicate skipPredicate,
                                         SetupFunction setupFunc, SkipAction skipAction, int timeoutMsecs)
    : CompletionWaitState(stateName, parent, timeoutMsecs)
    , _skipPredicate(std::move(skipPredicate))
    , _setupFunction(std::move(setupFunc))
    , _skipAction(std::move(skipAction))
{
}

void SkippableAsyncState::onWaitEntered()
{
    if (_skipPredicate && _skipPredicate()) {
        qCDebug(QGCStateMachineLog) << "Skip condition met, skipping" << stateName();

        if (_skipAction) {
            _skipAction();
        }

        // Defer signal emission to avoid firing during state entry
        // This ensures the state machine finishes entering before transitions are evaluated
        QMetaObject::invokeMethod(this, &SkippableAsyncState::skipped, Qt::QueuedConnection);
        return;
    }

    qCDebug(QGCStateMachineLog) << "Condition not met, executing" << stateName();
    if (_setupFunction) {
        _setupFunction(this);
    }
}
