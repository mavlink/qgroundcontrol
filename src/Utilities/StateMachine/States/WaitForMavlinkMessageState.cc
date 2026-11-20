#include "WaitForMavlinkMessageState.h"

#include <utility>

WaitForMavlinkMessageState::WaitForMavlinkMessageState(QState* parent, uint32_t messageId, int timeoutMsecs,
                                                       Predicate predicate)
    : VehicleMessageWaitState(QStringLiteral("WaitForMavlinkMessageState:%1").arg(messageId), parent, timeoutMsecs)
    , _messageId(messageId)
    , _predicate(std::move(predicate))
{
}

void WaitForMavlinkMessageState::handleMessage(const mavlink_message_t& message)
{
    if (message.msgid != _messageId) {
        return;
    }
    if (_predicate && !_predicate(message)) {
        return;
    }

    qCDebug(QGCStateMachineLog) << "Received expected message id" << _messageId << stateName();
    waitComplete();
}
