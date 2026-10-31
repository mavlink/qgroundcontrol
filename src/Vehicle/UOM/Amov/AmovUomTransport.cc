#include "AmovUomTransport.h"

#include <cstring>

#include <QtCore/QRandomGenerator>

#include "MAVLinkLib.h"
#include "MAVLinkProtocol.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

AmovUomTransport::AmovUomTransport(Vehicle* vehicle, QObject* parent)
    : QObject(parent)
    , _vehicle(vehicle)
{
    _deadline.setSingleShot(true);
    _deadline.setInterval(3000);
    connect(&_deadline, &QTimer::timeout, this, [this]() {
        _pending = 0;
        emit failed(4, tr("RID response timed out"));
    });
    reset();
}

void AmovUomTransport::reset()
{
    _deadline.stop();
    _pending = 0;
    _session = QRandomGenerator::global()->generate64();
    if (!_session) {
        _session = 1;
    }
}

bool AmovUomTransport::send(quint8 operation, quint8 node, const QByteArray& payload)
{
    if (!_vehicle || busy() || payload.size() > 192 || operation > 1 ||
        (operation == 0 && (node || !payload.isEmpty())) || (operation == 1 && (!node || node > 127))) {
        return false;
    }
    // The deployed bridge envelope has 8-bit addresses, even when QGC supports MAVLink TARGET32.
    if (_vehicle->id() > 255 || MAVLinkProtocol::instance()->getSystemId() > 255) {
        return false;
    }
    const auto link = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!link || !link->isConnected() || link->isLogReplay() || _vehicle->vehicleLinkManager()->communicationLost()) {
        return false;
    }
    mavlink_amov_device_request_t request{};
    request.session_id = _session;
    request.request_id = ++_nextRequest;
    if (!request.request_id) {
        request.request_id = ++_nextRequest;
    }
    request.target_system = _vehicle->id();
    request.target_component = _vehicle->defaultComponentId();
    request.protocol_version = 1;
    request.operation = operation;
    request.node_id = node;
    request.payload_length = static_cast<quint8>(payload.size());
    std::memcpy(request.payload, payload.constData(), payload.size());
    mavlink_get_channel_status(link->mavlinkChannel())->flags &= ~MAVLINK_STATUS_FLAG_OUT_MAVLINK1;
    mavlink_message_t message{};
    mavlink_msg_amov_device_request_encode_chan(MAVLinkProtocol::instance()->getSystemId(),
                                                MAVLinkProtocol::getComponentId(), link->mavlinkChannel(), &message,
                                                &request);
    _pending = request.request_id;
    _operation = operation;
    _node = node;
    _deadline.start();
    const bool sent = _vehicle->sendMessageOnLinkThreadSafe(link.get(), message);
    std::memset(request.payload, 0, sizeof(request.payload));
    std::memset(&message, 0, sizeof(message));
    if (!sent) {
        _pending = 0;
        _deadline.stop();
    }
    return sent;
}

void AmovUomTransport::receive(const mavlink_message_t& message)
{
    if (!_vehicle || !_pending || message.msgid != MAVLINK_MSG_ID_AMOV_DEVICE_RESPONSE ||
        message.sysid != _vehicle->id() || message.compid != _vehicle->defaultComponentId()) {
        return;
    }
    mavlink_amov_device_response_t response{};
    mavlink_msg_amov_device_response_decode(&message, &response);
    if (response.session_id != _session || response.request_id != _pending || response.node_id != _node ||
        response.operation != _operation || response.target_system != MAVLinkProtocol::instance()->getSystemId() ||
        response.target_component != MAVLinkProtocol::getComponentId()) {
        return;
    }
    _deadline.stop();
    _pending = 0;
    if (response.protocol_version != 1 || response.payload_length > sizeof(response.payload)) {
        emit failed(2, tr("Invalid RID transport response"));
    } else if (response.status != 0) {
        QString detail;
        switch (response.status) {
            case 1:
                detail = tr("Flight-controller firmware does not support RID management. Install matching firmware.");
                break;
            case 2:
                detail = tr("Flight controller rejected an invalid RID management request");
                break;
            case 3:
                detail = tr("RID management is busy; try again after the current operation");
                break;
            case 4:
                detail = tr("RID did not respond before the deadline");
                break;
            case 5:
                detail = tr("RID is unavailable; check the device and CAN connection");
                break;
            default:
                detail = tr("Unexpected RID transport status (%1)").arg(response.status);
                break;
        }
        emit failed(response.status, detail);
    } else {
        emit completed(response.operation, response.node_id,
                       QByteArray(reinterpret_cast<const char*>(response.payload), response.payload_length));
    }
}
