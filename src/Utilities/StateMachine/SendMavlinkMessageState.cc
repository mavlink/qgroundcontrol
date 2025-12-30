/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SendMavlinkMessageState.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"
#include "QGCLoggingCategory.h"

SendMavlinkMessageState::SendMavlinkMessageState(QState *parent, MessageEncoder encoder, int retryCount)
    : QGCState(QStringLiteral("SendMavlinkMessageState"), parent)
    , _encoder(std::move(encoder))
    , _retryCount(retryCount)
{
    connect(this, &QState::entered, this, &SendMavlinkMessageState::_sendMessage);
}

void SendMavlinkMessageState::_sendMessage()
{
    if (++_runCount > _retryCount + 1  /* +1 for initial attempt */) {
        qCDebug(QGCStateMachineLog) << "Exceeded maximum retries" << stateName();
        emit error();
        return;
    }

    if (!_encoder) {
        qCDebug(QGCStateMachineLog) << "No MAVLink message encoder configured" << stateName();
        emit error();
        return;
    }

    SharedLinkInterfacePtr sharedLink = vehicle()->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCWarning(QGCStateMachineLog) << "No active link available to send MAVLink message" << stateName();
        emit error();
        return;
    }

    mavlink_message_t message{};

    const uint8_t systemId = MAVLinkProtocol::instance()->getSystemId();
    const uint8_t componentId = MAVLinkProtocol::getComponentId();
    const uint8_t channel = sharedLink->mavlinkChannel();

    _encoder(systemId, channel, &message);
    if (!vehicle()->sendMessageOnLinkThreadSafe(sharedLink.get(), message)) {
        qCWarning(QGCStateMachineLog) << "Failed to send MAVLink message" << stateName();
        emit error();
        return;
    }

    emit advance();
}
