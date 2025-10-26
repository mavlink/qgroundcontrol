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

/// Waits for the _HASH_CHECK to come through as a PARAM_VALUE message
///     signals notFound() - parameter hashing is not in effect
class WaitForHashCheckParamValue : public QGCState
{
    Q_OBJECT

public:
    /// @param timeoutMsecs Timeout in milliseconds to wait for _HASH_CHECK, 0 to wait forever
    WaitForHashCheckParamValue(QState *parent, int timeoutMsecs);

signals:
    /// Signal emitted if either _HASH_CHECK is not received as the first PARAM_VALUE or no PARAM_VALUE
    /// messages are received within the specified timeout period
    void notFound();    

private slots:
    void _onEntered();
    void _onExited();
    void _messageReceived(const mavlink_message_t &message);
    void _onTimeout();

private:
    uint32_t _messageId = 0U;
    int _timeoutMsecs = 0;
    QTimer _timeoutTimer;
};
