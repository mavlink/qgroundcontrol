#include "VehicleMessageWaitState.h"

#include <utility>

#include "MAVLinkProtocol.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

void VehicleMessageWaitState::setRequest(MessageEncoder encoder, int retryCount)
{
    _requestEncoder = std::move(encoder);
    _requestRetryCount = retryCount;
}

void VehicleMessageWaitState::connectWaitSignal()
{
    Vehicle* const sourceVehicle = vehicleOrWarn();
    if (!sourceVehicle) {
        waitFailed();
        return;
    }

    connect(sourceVehicle, &Vehicle::mavlinkMessageReceived, this, &VehicleMessageWaitState::handleMessage,
            Qt::UniqueConnection);
}

void VehicleMessageWaitState::disconnectWaitSignal()
{
    if (Vehicle* const sourceVehicle = vehicle()) {
        disconnect(sourceVehicle, &Vehicle::mavlinkMessageReceived, this, &VehicleMessageWaitState::handleMessage);
    }
}

void VehicleMessageWaitState::onWaitEntered()
{
    _requestRetriesDone = 0;
    _sendRequest();
}

void VehicleMessageWaitState::onWaitRestarted()
{
    _sendRequest();
}

void VehicleMessageWaitState::onWaitTimeout()
{
    if (_requestEncoder && (_requestRetriesDone < _requestRetryCount)) {
        _requestRetriesDone++;
        qCDebug(QGCStateMachineLog) << "Sending request again" << stateName() << "retry" << _requestRetriesDone;
        restartWait();
        return;
    }

    WaitStateBase::onWaitTimeout();
}

void VehicleMessageWaitState::_sendRequest()
{
    // Without a vehicle the wait has failed when its signal was to be connected
    Vehicle* const targetVehicle = vehicle();
    if (!_requestEncoder || waitEnded() || !targetVehicle) {
        return;
    }

    const SharedLinkInterfacePtr sharedLink = targetVehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCWarning(QGCStateMachineLog) << "No active link available to send MAVLink message" << stateName();
        waitFailed();
        return;
    }

    mavlink_message_t message{};
    _requestEncoder(MAVLinkProtocol::instance()->getSystemId(), sharedLink->mavlinkChannel(), &message);
    if (!targetVehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), message)) {
        qCWarning(QGCStateMachineLog) << "Failed to send MAVLink message" << stateName();
        waitFailed();
    }
}
