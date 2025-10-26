/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "WatchForMavlinkMessageState.h"

WatchForMavlinkMessageState::WatchForMavlinkMessageState(QState *parent, uint32_t messageId, int timeoutMsecs)
    : QGCState(QStringLiteral("WatchForMavlinkMessageState"), parent)
    , _messageId(messageId)
    , _timeoutMsecs(timeoutMsecs > 0 ? timeoutMsecs : 0)
{
    connect(this, &QState::entered, this, &WatchForMavlinkMessageState::_onEntered);
    connect(this, &QState::exited, this, &WatchForMavlinkMessageState::_onExited);

    _timeoutTimer.setSingleShot(true);
    _timeoutTimer.setInterval(_timeoutMsecs);

    connect(&_timeoutTimer, &QTimer::timeout, this, &WatchForMavlinkMessageState::_onTimeout);
}

void WatchForMavlinkMessageState::_onEntered()
{
    connect(vehicle(), &Vehicle::mavlinkMessageReceived, this, &WatchForMavlinkMessageState::_messageReceived);

    if (_timeoutMsecs > 0) {
        _timeoutTimer.start();
    }
}

void WatchForMavlinkMessageState::_onExited()
{
    disconnect(vehicle(), &Vehicle::mavlinkMessageReceived, this, &WatchForMavlinkMessageState::_messageReceived);
    _timeoutTimer.stop();
}

void WatchForMavlinkMessageState::_messageReceived(const mavlink_message_t &message)
{
    if (message.msgid != _messageId) {
        return;
    }

    qCDebug(QGCStateMachineLog) << "Received expected message id" << _messageId << stateName();
    _timeoutTimer.start();
    
    if (!_processor(message)) {
        qCDebug(QGCStateMachineLog) << "Stopping further processing of message id" << _messageId << stateName();
        emit advance();
    }
}

void WatchForMavlinkMessageState::_onTimeout()
{
    qCDebug(QGCStateMachineLog) << "Timeout waiting for message id" << _messageId << stateName();
    emit timeout();
}
