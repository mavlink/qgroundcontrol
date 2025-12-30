/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "WaitForMavlinkMessageState.h"

#include "MultiVehicleManager.h"
#include "Vehicle.h"

#include "QGCLoggingCategory.h"

#include <QString>
#include <utility>
WaitForMavlinkMessageState::WaitForMavlinkMessageState(QState *parent, uint32_t messageId, int timeoutMsecs, Predicate predicate)
    : QGCState(QStringLiteral("WaitForMavlinkMessageState"), parent)
    , _messageId(messageId)
    , _predicate(std::move(predicate))
    , _timeoutMsecs(timeoutMsecs > 0 ? timeoutMsecs : 0)
{
    connect(this, &QState::entered, this, &WaitForMavlinkMessageState::_onEntered);
    connect(this, &QState::exited, this, &WaitForMavlinkMessageState::_onExited);
    _timeoutTimer.setSingleShot(true);
    connect(&_timeoutTimer, &QTimer::timeout, this, &WaitForMavlinkMessageState::_onTimeout);
}

void WaitForMavlinkMessageState::_onEntered()
{
    connect(vehicle(), &Vehicle::mavlinkMessageReceived, this, &WaitForMavlinkMessageState::_messageReceived, Qt::UniqueConnection);

    if (_timeoutMsecs > 0) {
        _timeoutTimer.start(_timeoutMsecs);
    }
}

void WaitForMavlinkMessageState::_onExited()
{
    disconnect(vehicle(), &Vehicle::mavlinkMessageReceived, this, &WaitForMavlinkMessageState::_messageReceived);

    _timeoutTimer.stop();
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

    disconnect(vehicle(), &Vehicle::mavlinkMessageReceived, this, &WaitForMavlinkMessageState::_messageReceived);
    _timeoutTimer.stop();

    emit advance();
}

void WaitForMavlinkMessageState::_onTimeout()
{
    qCDebug(QGCStateMachineLog) << "Timeout waiting for message id" << _messageId << stateName();
    disconnect(vehicle(), &Vehicle::mavlinkMessageReceived, this, &WaitForMavlinkMessageState::_messageReceived);

    emit timeout();
}
