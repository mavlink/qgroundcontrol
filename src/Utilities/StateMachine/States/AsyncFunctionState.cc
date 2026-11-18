#include "AsyncFunctionState.h"

AsyncFunctionState::AsyncFunctionState(const QString& stateName, QState* parent, SetupFunction setupFunction,
                                       int timeoutMsecs)
    : CompletionWaitState(stateName, parent, timeoutMsecs)
    , _setupFunction(std::move(setupFunction))
{
}

void AsyncFunctionState::onWaitEntered()
{
    if (_setupFunction) {
        _setupFunction(this);
    }

    if (!hasCompletionConnection() && timeoutMsecs() == 0) {
        qCCritical(QGCStateMachineLog) << stateName()
            << "has no completion connection and no timeout - state may hang indefinitely";
    }
}
