#include "AsyncFunctionState.h"

AsyncFunctionState::AsyncFunctionState(const QString& stateName, QState* parent, SetupFunction setupFunction,
                                       int timeoutMsecs)
    : CompletionWaitState(stateName, parent, timeoutMsecs)
    , _setupFunction(std::move(setupFunction))
{
}

void AsyncFunctionState::setSkipPredicate(SkipPredicate skipPredicate, SkipAction skipAction)
{
    _skipPredicate = std::move(skipPredicate);
    _skipAction = std::move(skipAction);
}

void AsyncFunctionState::onWaitEntered()
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

    if (_setupFunction) {
        _setupFunction(this);
    }
    checkWaitCanEnd();
}

void AsyncFunctionState::onWaitRestarted()
{
    if (_setupFunction) {
        _setupFunction(this);
    }
}
