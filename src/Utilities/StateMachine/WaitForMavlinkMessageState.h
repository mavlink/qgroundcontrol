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

/// Waits for the specified MAVLink message from the vehicle
///     signals timeout() - if the message is not received within the specified timeout period
class WaitForMavlinkMessageState : public QGCState
{
    Q_OBJECT

public:
    /// @return true: Predicate matched, false: no match
    using Predicate = std::function<bool(WaitForMavlinkMessageState * state, const mavlink_message_t &message)>;

    /// @param messageId MAVLink message ID to wait for
    /// @param timeoutMsecs Timeout in milliseconds to wait for message, 0 to wait forever
    /// @param predicate Predicate which further filters received messages
    WaitForMavlinkMessageState(uint32_t messageId, int timeoutMsecs, Predicate predicate, QState *parentState);

signals:
    void timeout();

private slots:
    void _onEntered();
    void _onExited();
    void _messageReceived(const mavlink_message_t &message);
    void _onTimeout();

private:
    uint32_t _messageId = 0U;
    Predicate _predicate;
    int _timeoutMsecs = 0;
    QTimer _timeoutTimer;
};
