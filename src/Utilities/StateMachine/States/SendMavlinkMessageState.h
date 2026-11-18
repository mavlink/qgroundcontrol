#pragma once

#include "QGCState.h"
#include "MAVLinkMessageType.h"

#include <cstdint>
#include <functional>

/// \brief Sends the specified MAVLink message to the vehicle
///
/// Each entry sends the message once and emits advance(). A machine retries by entering the state again; once it
/// has been entered more often than the retry count allows in one run of the machine, the state emits error().
///
class SendMavlinkMessageState : public QGCState
{
    Q_OBJECT
    Q_DISABLE_COPY(SendMavlinkMessageState)

public:
    using MessageEncoder = std::function<void(quint32 systemId, uint8_t channel, mavlink_message_t* message)>;

    /// @param encoder Function which encodes the MAVLink message to send
    /// @param retryCount Number of times the message is sent again in one run of the machine
    SendMavlinkMessageState(QState *parent, MessageEncoder encoder, int retryCount);

private slots:
    void _sendMessage();

    void _resetRunCount() { _runCount = 0; }

private:
    MessageEncoder _encoder;
    int _retryCount = 0;
    int _runCount = 0;
};
