#pragma once

#include "WaitStateBase.h"
#include "QGCMAVLink.h"

#include <cstdint>
#include <functional>

class Vehicle;

/// Waits for the specified MAVLink message from the vehicle
/// Filters by message ID and optional predicate before advancing
class WaitForMavlinkMessageState : public WaitStateBase
{
    Q_OBJECT
    Q_DISABLE_COPY(WaitForMavlinkMessageState)

public:
    using Predicate = std::function<bool(const mavlink_message_t &message)>;

    /// @param parent Parent state
    /// @param messageId MAVLink message ID to wait for
    /// @param timeoutMsecs Timeout in milliseconds to wait for message, 0 to wait forever
    /// @param predicate Optional predicate which further filters received messages
    WaitForMavlinkMessageState(QState *parent, uint32_t messageId, int timeoutMsecs, Predicate predicate = Predicate());

    /// @return The message ID being waited for
    uint32_t messageId() const { return _messageId; }

protected:
    void connectWaitSignal() override;
    void disconnectWaitSignal() override;

private slots:
    void _messageReceived(const mavlink_message_t &message);

private:
    uint32_t _messageId = 0U;
    Predicate _predicate;
};
