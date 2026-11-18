#include "CompletionWaitState.h"

#include <utility>

CompletionWaitState::CompletionWaitState(const QString& stateName, QState* parent, int timeoutMsecs)
    : WaitStateBase(stateName, parent, timeoutMsecs)
{}

void CompletionWaitState::disconnectWaitSignal()
{
    for (const QMetaObject::Connection& connection : std::as_const(_completionConnections)) {
        disconnect(connection);
    }
    _completionConnections.clear();
}
