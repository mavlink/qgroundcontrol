#include "CompletionWaitState.h"

CompletionWaitState::CompletionWaitState(const QString& stateName, QState* parent, int timeoutMsecs)
    : WaitStateBase(stateName, parent, timeoutMsecs)
{}

void CompletionWaitState::disconnectWaitSignal()
{
    if (_completionConnection) {
        disconnect(_completionConnection);
        _completionConnection = {};
    }
}
