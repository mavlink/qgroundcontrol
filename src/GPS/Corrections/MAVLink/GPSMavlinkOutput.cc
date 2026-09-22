#include "GPSMavlinkOutput.h"

#include <cstring>
#include <memory>

#include <QtCore/QHash>
#include <QtCore/QSet>

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
        quint64 nextSession = 0;
    };

    return [state = std::make_shared<State>()]() {
        QList<RTCMMavlink::Output> outputs;
        QSet<LinkInterface*> seen;
        auto* vehicles = MultiVehicleManager::instance()->vehicles();
        for (qsizetype index = 0; index < vehicles->count(); ++index) {
            auto* vehicle = qobject_cast<Vehicle*>(vehicles->get(index));
            if (!vehicle) {
                continue;
            }
            const auto link = vehicle->vehicleLinkManager()->primaryLink().lock();
            if (!link || !link->isConnected() || link->isLogReplay() || seen.contains(link.get())) {
                continue;
            }
            seen.insert(link.get());
            auto& connection = state->connections[link.get()];
            if (connection.link.lock() != link) {
                connection = {link, ++state->nextSession};
            }
            outputs.append({QStringLiteral("mavlink/%1").arg(connection.session), connection.session,
                            [link](const GpsRtcmPacket& packet) {
                                if (packet.data.size() > RTCMMavlinkPacket::kFragmentLen) {
                                    qCWarning(GPSMavlinkOutputLog) << "RTCM packet exceeds MAVLink payload limit";
                                    return false;
                                }
                                if (!link->isConnected() || !link->mavlinkChannelIsSet()) {
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
                                                                      MAVLinkProtocol::getComponentId(),
                                                                      link->mavlinkChannel(), &message, &payload);
                                link->sendMessageThreadSafe(message);
                                return true;
                            }});
        }
        state->connections.removeIf([](auto it) {
            const auto link = it->link.lock();
            return !link || !link->isConnected();
        });
        return outputs;
    };
}
