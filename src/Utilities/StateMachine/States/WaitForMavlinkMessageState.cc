#include "WaitForMavlinkMessageState.h"
#include "Vehicle.h"
#include "QGCLoggingCategory.h"

#include <utility>

WaitForMavlinkMessageState::WaitForMavlinkMessageState(QState *parent, uint32_t messageId, int timeoutMsecs, Predicate predicate)
    : WaitStateBase(QStringLiteral("WaitForMavlinkMessageState:%1").arg(messageId), parent, timeoutMsecs)
    , _messageId(messageId)
    , _predicate(std::move(predicate))
{
}

void WaitForMavlinkMessageState::connectWaitSignal()
{
    connect(vehicle(), &Vehicle::mavlinkMessageReceived, this, &WaitForMavlinkMessageState::_messageReceived, Qt::UniqueConnection);
}

void WaitForMavlinkMessageState::disconnectWaitSignal()
{
    if (vehicle()) {
        disconnect(vehicle(), &Vehicle::mavlinkMessageReceived, this, &WaitForMavlinkMessageState::_messageReceived);
    }
}

void WaitForMavlinkMessageState::_messageReceived(const mavlink_message_t &message)
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
