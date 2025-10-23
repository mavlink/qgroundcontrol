/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCState.h"
#include "QGCMAVLink.h"

#include <cstdint>
#include <functional>

class Vehicle;

/// Sends the specified MAVLink message to the vehicle
class SendMavlinkMessageState : public QGCState
{
    Q_OBJECT

public:
    using MessageEncoder = std::function<void (uint8_t systemId, uint8_t channel, mavlink_message_t *message)>;

    /// @param encoder Function which encodes the MAVLink message to send
    /// @param retryCount Number of times to retry sending the message on failure
    SendMavlinkMessageState(QState *parent, MessageEncoder encoder, int retryCount);

private slots:
    void _sendMessage();

private:
    MessageEncoder _encoder;
    int _retryCount = 0;
    int _runCount = 0;
};
