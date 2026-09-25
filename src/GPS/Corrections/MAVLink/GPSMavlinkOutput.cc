#include "GPSMavlinkOutput.h"

#include <algorithm>
#include <cstring>
#include <memory>

#include <QtCore/QHash>
#include <QtCore/QVarLengthArray>

#include "LinkInterface.h"
#include "MAVLinkLib.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

QGC_LOGGING_CATEGORY(GPSMavlinkOutputLog, "GPS.Corrections.GPSMavlinkOutput")
static_assert(RTCMMavlinkPacket::kFragmentLen == MAVLINK_MSG_GPS_RTCM_DATA_FIELD_DATA_LEN);

namespace {

RTCMMavlink::Output makeOutput(const std::shared_ptr<LinkInterface>& link, quint64 session)
{
    // Cached outputs hold the link weakly so they never extend its lifetime.
    return {QStringLiteral("mavlink/%1").arg(session), session,
            [weakLink = std::weak_ptr<LinkInterface>(link)](const GpsRtcmPacket& packet) {
                if (packet.data.size() > RTCMMavlinkPacket::kFragmentLen) {
                    qCWarning(GPSMavlinkOutputLog) << "RTCM packet exceeds MAVLink payload limit";
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
                                                      MAVLinkProtocol::getComponentId(), target->mavlinkChannel(),
                                                      &message, &payload);
                target->sendMessageThreadSafe(message);
                return true;
            }};
}

}  // namespace

RTCMMavlink::OutputProvider createGpsMavlinkOutputProvider()
{
    struct State
    {
        struct Connection
        {
            std::weak_ptr<LinkInterface> link;
            quint64 session = 0;
        };

        QHash<LinkInterface*, Connection> connections;
        /// Links behind the cached outputs, in vehicle order; outputs are rebuilt only when they change.
        QList<std::weak_ptr<LinkInterface>> links;
        QList<RTCMMavlink::Output> outputs;
        quint64 nextSession = 0;
    };

    return [state = std::make_shared<State>()]() {
        QVarLengthArray<std::shared_ptr<LinkInterface>, 4> links;
        auto* vehicles = MultiVehicleManager::instance()->vehicles();
        for (qsizetype index = 0; index < vehicles->count(); ++index) {
            auto* vehicle = qobject_cast<Vehicle*>(vehicles->get(index));
            if (!vehicle) {
                continue;
            }
            auto link = vehicle->vehicleLinkManager()->primaryLink().lock();
            if (!link || !link->isConnected() || link->isLogReplay() ||
                std::find(links.cbegin(), links.cend(), link) != links.cend()) {
                continue;
            }
            links.append(std::move(link));
        }
        const bool unchanged = links.size() == state->links.size() &&
                               std::equal(links.cbegin(), links.cend(), state->links.cbegin(),
                                          [](const auto& link, const auto& cached) { return cached.lock() == link; });
        if (unchanged) {
            return state->outputs;
        }
        state->links.clear();
        state->outputs.clear();
        for (const auto& link : links) {
            auto& connection = state->connections[link.get()];
            if (connection.link.lock() != link) {
                connection = {link, ++state->nextSession};
            }
            state->links.append(link);
            state->outputs.append(makeOutput(link, connection.session));
        }
        state->connections.removeIf([](auto it) {
            const auto link = it->link.lock();
            return !link || !link->isConnected();
        });
        return state->outputs;
    };
}
