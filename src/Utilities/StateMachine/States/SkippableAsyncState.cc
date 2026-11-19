#include "SkippableAsyncState.h"

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

        // The machine takes the transition once the state is entered: Qt queues a signal that a state emits
        // while it is entered
        complete();
        return;
    }

    qCDebug(QGCStateMachineLog) << "Condition not met, executing" << stateName();
    if (_setupFunction) {
        _setupFunction(this);
    }
    checkWaitCanEnd();
}

void SkippableAsyncState::onWaitRestarted()
{
    if (_setupFunction) {
        _setupFunction(this);
    }
}
