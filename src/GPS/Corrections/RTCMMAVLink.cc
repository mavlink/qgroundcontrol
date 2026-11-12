#include "RTCMMAVLink.h"

#include <cstring>
#include <memory>
#include <utility>

#include "LinkConfiguration.h"
#include "LinkInterface.h"
#include "MAVLinkLib.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"
#include "VehicleGPSFactGroup.h"
#include "VehicleLinkManager.h"

QGC_LOGGING_CATEGORY(RTCMMAVLinkLog, "GPS.Corrections.RTCMMAVLink")
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

/// Calls @a visit with each live vehicle that @a selection sends corrections to, and its primary link: every vehicle,
/// or the active vehicle alone, whose primary link is connected and neither a log replay nor a high-latency link.
template <typename Visit>
void visitCorrectionVehicles(GPSCorrectionSettings::CorrectionVehicles selection, Visit&& visit)
{
    auto* manager = MultiVehicleManager::instance();
    if (!manager) {
        return;
    }
    // Compared, never dereferenced: no vehicle matches while none is active, or while the active one is being
    // removed and has already left the list.
    const Vehicle* const activeVehicle = manager->activeVehicle();
    auto* vehicles = manager->vehicles();
    for (int index = 0; index < vehicles->count(); ++index) {
        auto* vehicle = vehicles->value<Vehicle*>(index);
        if (!vehicle || !vehicle->vehicleLinkManager()) {
            continue;
        }
        if (selection == GPSCorrectionSettings::ActiveVehicle && vehicle != activeVehicle) {
            continue;
        }
        auto link = vehicle->vehicleLinkManager()->primaryLink().lock();
        // Corrections are kilobytes per second, which a high-latency link such as a satellite modem cannot carry.
        if (!link || !link->isConnected() || link->isLogReplay() || link->linkConfiguration()->isHighLatency()) {
            continue;
        }
        visit(vehicle, std::move(link));
    }
}

}  // namespace

RTCMMAVLink::RTCMMAVLink()
    : _outputProvider([this]() { return vehicleLinkOutputs(_vehicles)(); })
{
    qCDebug(RTCMMAVLinkLog) << this;
}

RTCMMAVLink::~RTCMMAVLink()
{
    qCDebug(RTCMMAVLinkLog) << this;
}

RTCMMAVLink::OutputProvider RTCMMAVLink::vehicleLinkOutputs(GPSCorrectionSettings::CorrectionVehicles selection)
{
    return [selection]() {
        QList<std::shared_ptr<LinkInterface>> links;
        QList<Output> outputs;
        visitCorrectionVehicles(selection, [&links, &outputs](const Vehicle*, std::shared_ptr<LinkInterface> link) {
            if (!links.contains(link)) {
                outputs.append(makeOutput(link));
                links.append(std::move(link));
            }
        });
        return outputs;
    };
}

QList<GPSCorrectionLossAlert::Vehicle> RTCMMAVLink::vehicleFixes(GPSCorrectionSettings::CorrectionVehicles selection)
{
    QList<GPSCorrectionLossAlert::Vehicle> fixes;
    visitCorrectionVehicles(selection, [&fixes](Vehicle* vehicle, const std::shared_ptr<LinkInterface>&) {
        auto* gps = vehicle->gpsFactGroup();
        const int fixType = gps ? gps->lock()->rawValue().toInt() : GPS_FIX_TYPE_NO_GPS;
        fixes.append({.id = static_cast<int>(vehicle->id()),
                      .rtkFix = fixType == GPS_FIX_TYPE_RTK_FLOAT || fixType == GPS_FIX_TYPE_RTK_FIXED});
    });
    return fixes;
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
