#include "SendMavlinkMessageState.h"

#include <QtStateMachine/QStateMachine>

#include "MAVLinkProtocol.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

SendMavlinkMessageState::SendMavlinkMessageState(QState *parent, MessageEncoder encoder, int retryCount)
    : QGCState(QStringLiteral("SendMavlinkMessageState"), parent)
    , _encoder(std::move(encoder))
    , _retryCount(retryCount)
{
    connect(this, &QState::entered, this, &SendMavlinkMessageState::_sendMessage);
}

void SendMavlinkMessageState::_sendMessage()
{
    // _runCount is kept across the entries of one run of the machine, which are the retries, and starts again
    // with the next run. The machine is not known before the state is entered.
    if (const QStateMachine* const stateMachine = QAbstractState::machine()) {
        connect(stateMachine, &QStateMachine::finished, this, &SendMavlinkMessageState::_resetRunCount,
                Qt::UniqueConnection);
        connect(stateMachine, &QStateMachine::stopped, this, &SendMavlinkMessageState::_resetRunCount,
                Qt::UniqueConnection);
    }

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

    Vehicle* const targetVehicle = vehicleOrWarn();
    if (!targetVehicle) {
        emit error();
        return;
    }

    SharedLinkInterfacePtr sharedLink = targetVehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCWarning(QGCStateMachineLog) << "No active link available to send MAVLink message" << stateName();
        emit error();
        return;
    }

    mavlink_message_t message{};

    const quint32 systemId = MAVLinkProtocol::instance()->getSystemId();
    const uint8_t channel = sharedLink->mavlinkChannel();

    _encoder(systemId, channel, &message);
    if (!targetVehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), message)) {
        qCWarning(QGCStateMachineLog) << "Failed to send MAVLink message" << stateName();
        emit error();
        return;
    }

    emit advance();
}
