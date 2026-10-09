#include "RTCMMAVLink.h"

#include <cstring>
#include <memory>
#include <utility>

#include "LinkInterface.h"
#include "MAVLinkLib.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

QGC_LOGGING_CATEGORY(RTCMMAVLinkLog, "GPS.Corrections.RTCMMavlink")
static_assert(RTCMMAVLinkPacket::kFragmentLen == MAVLINK_MSG_GPS_RTCM_DATA_FIELD_DATA_LEN);

namespace {

RTCMMAVLink::Output makeOutput(const std::shared_ptr<LinkInterface>& link)
{
    // Outputs hold the link weakly so they never extend its lifetime.
    return [weakLink = std::weak_ptr<LinkInterface>(link)](const GPSRTCMPacket& packet) {
        if (packet.data.size() > RTCMMAVLinkPacket::kFragmentLen) {
            qCWarning(RTCMMAVLinkLog) << "RTCM packet exceeds MAVLink payload limit";
            return false;
        }
        const auto target = weakLink.lock();
        if (!target || !target->isConnected() || !target->mavlinkChannelIsSet()) {
            return false;
        }
        mavlink_gps_rtcm_data_t payload{};
        payload.flags = packet.flags;
        payload.len = static_cast<uint8_t>(packet.data.size());
        if (!packet.data.isEmpty()) {
            std::memcpy(payload.data, packet.data.constData(), packet.data.size());
        }
        mavlink_message_t message{};
        mavlink_msg_gps_rtcm_data_encode_chan(MAVLinkProtocol::instance()->getSystemId(),
                                              MAVLinkProtocol::getComponentId(), target->mavlinkChannel(), &message,
                                              &payload);
        target->sendMessageThreadSafe(message);
        return true;
    };
}

}  // namespace

RTCMMAVLink::RTCMMAVLink()
    : _outputProvider(vehicleLinkOutputs())
{
    qCDebug(RTCMMAVLinkLog) << this;
}

RTCMMAVLink::~RTCMMAVLink()
{
    qCDebug(RTCMMAVLinkLog) << this;
}

RTCMMAVLink::OutputProvider RTCMMAVLink::vehicleLinkOutputs()
{
    return []() {
        QList<std::shared_ptr<LinkInterface>> links;
        QList<Output> outputs;
        auto* manager = MultiVehicleManager::instance();
        if (!manager) {
            return outputs;
        }
        auto* vehicles = manager->vehicles();
        for (int index = 0; index < vehicles->count(); ++index) {
            auto* vehicle = vehicles->value<Vehicle*>(index);
            if (!vehicle || !vehicle->vehicleLinkManager()) {
                continue;
            }
            auto link = vehicle->vehicleLinkManager()->primaryLink().lock();
            if (!link || !link->isConnected() || link->isLogReplay() || links.contains(link)) {
                continue;
            }
            outputs.append(makeOutput(link));
            links.append(std::move(link));
        }
        return outputs;
    };
}

void RTCMMAVLink::setOutputProvider(OutputProvider provider)
{
    _outputProvider = std::move(provider);
}

quint64 RTCMMAVLink::submitToOutputs(QByteArrayView data)
{
    if (data.isEmpty()) {
        return 0;
    }
    const auto packed = RTCMMAVLinkPacket::pack(data, _sequenceId);
    _sequenceId = packed.nextSequenceId;
    const auto outputs = _outputProvider ? _outputProvider() : QList<Output>();
    quint64 submitted = 0;
    for (const auto& output : outputs) {
        if (!output) {
            continue;
        }
        for (const auto& packet : packed.packets) {
            if (!output(packet)) {
                break;
            }
            submitted += packet.data.size();
        }
    }
    _submittedBytes += submitted;
    return submitted;
}
