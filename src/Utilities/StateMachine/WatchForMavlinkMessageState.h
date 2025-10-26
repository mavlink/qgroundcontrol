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

#include <QtCore/QTimer>

#include <cstdint>
#include <functional>

class Vehicle;

/// Allows for processing on all mavlink messages with the specified message id
///     signals timeout() - if the message is not received within the specified timeout period
class WatchForMavlinkMessageState : public QGCState
{
    Q_OBJECT

public:
    /// @return true: continue processing, false: stop processing further messages
    using MessageProcessor = std::function<bool(WatchForMavlinkMessageState *state, const mavlink_message_t &message)>;

    /// @param messageId MAVLink message ID to wait for
    /// @param timeoutMsecs Timeout in milliseconds to wait for message, 0 to watch forever
    WatchForMavlinkMessageState(uint32_t messageId, int timeoutMsecs, MessageProcessor processor, QState *parentState);

signals:
    void timeout();

private slots:
    void _onEntered();
    void _onExited();
    void _messageReceived(const mavlink_message_t &message);
    void _onTimeout();

private:
    uint32_t _messageId = 0U;
    MessageProcessor _processor;
    int _timeoutMsecs = 0;
    QTimer _timeoutTimer;
};
