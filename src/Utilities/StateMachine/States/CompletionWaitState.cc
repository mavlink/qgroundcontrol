#include "CompletionWaitState.h"

#include <utility>

CompletionWaitState::CompletionWaitState(const QString& stateName, QState* parent, int timeoutMsecs)
    : WaitStateBase(stateName, parent, timeoutMsecs)
{}

void CompletionWaitState::checkWaitCanEnd() const
{
    // Not for a setup function that has completed or failed the state already, which removes the connection
    if (!waitEnded() && _completionConnections.isEmpty() && (timeoutMsecs() == 0)) {
        qCCritical(QGCStateMachineLog) << stateName()
                                       << "has no completion connection and no timeout - state may hang indefinitely";
    }
}

void CompletionWaitState::disconnectWaitSignal()
{
    for (const QMetaObject::Connection& connection : std::as_const(_completionConnections)) {
        disconnect(connection);
    }
    for (const QMetaObject::Connection& connection : std::as_const(_otherConnections)) {
        disconnect(connection);
    }
    _completionConnections.clear();
    _otherConnections.clear();
}
