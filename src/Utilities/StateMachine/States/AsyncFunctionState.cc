#include "AsyncFunctionState.h"
#include "QGCLoggingCategory.h"

AsyncFunctionState::AsyncFunctionState(const QString& stateName, QState* parent, SetupFunction setupFunction, int timeoutMsecs)
    : WaitStateBase(stateName, parent, timeoutMsecs)
    , _setupFunction(std::move(setupFunction))
{
}

void AsyncFunctionState::connectWaitSignal()
{
    // Connection is set up dynamically via connectToCompletion() in the setup function
}

void AsyncFunctionState::disconnectWaitSignal()
{
    if (_completionConnection) {
        disconnect(_completionConnection);
        _completionConnection = {};
    }
}

void AsyncFunctionState::onWaitEntered()
{
    if (_setupFunction) {
        _setupFunction(this);
    }
}
