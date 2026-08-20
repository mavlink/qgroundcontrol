#include "RTCMMavlink.h"

#include <QtCore/QByteArray>
#include <QtCore/QSet>
#include <QtCore/QThread>
#include <algorithm>
#include <cstring>

#include "LinkInterface.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

QGC_LOGGING_CATEGORY(RTCMMavlinkLog, "GPS.RTCMMavlink")

// Compile-time check that our constants match the MAVLink message definition.
static_assert(RTCMMavlink::kFragmentLen == MAVLINK_MSG_GPS_RTCM_DATA_FIELD_DATA_LEN);

RTCMMavlink::RTCMMavlink(QObject* parent) : QObject(parent)
{
    qCDebug(RTCMMavlinkLog) << this;
}

RTCMMavlink::~RTCMMavlink()
{
    qCDebug(RTCMMavlinkLog) << this;
}

uint8_t RTCMMavlink::_makeFlags(bool fragmented, uint8_t fragmentId, uint8_t sequenceId)
{
    uint8_t flags = static_cast<uint8_t>((sequenceId & 0x1FU) << 3);
    if (fragmented) {
        flags |= 0x01U;
        flags |= static_cast<uint8_t>((fragmentId & 0x03U) << 1);
    }
    return flags;
}

RTCMMavlink::PackResult RTCMMavlink::pack(QByteArrayView data, uint8_t sequenceId)
{
    PackResult result;
    result.nextSequenceId = sequenceId;

    if (data.isEmpty()) {
        return result;
    }

    // Larger than the 4-fragment reassembly window: stream unfragmented chunks so
    // the vehicle's RTCM framer can rebuild frames from the inject stream. Do not
    // invent fragment IDs beyond 0..3 (would clobber the sequence field).
    if (data.size() > kMaxAssembledLen) {
        qsizetype start = 0;
        while (start < data.size()) {
            const qsizetype length = std::min(data.size() - start, kFragmentLen);
            GpsRtcmPacket packet;
            packet.flags = _makeFlags(false, 0, result.nextSequenceId);
            packet.data = data.mid(start, length).toByteArray();
            result.packets.append(std::move(packet));
            ++result.nextSequenceId;
            start += length;
        }
        return result;
    }

    if (data.size() <= kFragmentLen) {
        GpsRtcmPacket packet;
        packet.flags = _makeFlags(false, 0, sequenceId);
        packet.data = data.toByteArray();
        result.packets.append(std::move(packet));
        ++result.nextSequenceId;
        return result;
    }

    // Fragmented: 181..720 bytes. Fragment ID is only 2 bits (0..3).
    uint8_t fragmentId = 0;
    qsizetype start = 0;
    while (start < data.size()) {
        const qsizetype length = std::min(data.size() - start, kFragmentLen);
        GpsRtcmPacket packet;
        packet.flags = _makeFlags(true, fragmentId, sequenceId);
        packet.data = data.mid(start, length).toByteArray();
        result.packets.append(std::move(packet));
        ++fragmentId;
        start += length;
    }

    // Exact multiple of 180 with fewer than 4 fragments: MAVLink requires a final
    // zero-length fragment so receivers know the message is complete. (All four
    // full fragments complete by the "all fragments present" rule without this.)
    // See ArduPilot AP_GPS::handle_gps_rtcm_fragment and PX4 GpsRtcmMessageAssembler.
    if ((data.size() % kFragmentLen) == 0 && fragmentId < kMaxFragments) {
        GpsRtcmPacket terminator;
        terminator.flags = _makeFlags(true, fragmentId, sequenceId);
        terminator.data.clear();
        result.packets.append(std::move(terminator));
    }

    ++result.nextSequenceId;
    return result;
}

void RTCMMavlink::RTCMDataUpdate(QByteArrayView data)
{
    if (data.isEmpty()) {
        return;
    }

    _rateTracker.recordBytes(data.size());
    if (_rateTracker.rateUpdated()) {
        qCDebug(RTCMMavlinkLog) << QStringLiteral("RTCM bandwidth: %1 kB/s").arg(_rateTracker.kBps(), 0, 'f', 3);
        emit bandwidthChanged();
    }

    const PackResult packed = pack(data, _sequenceId);
    _sequenceId = packed.nextSequenceId;

    for (const GpsRtcmPacket& packet : packed.packets) {
        mavlink_gps_rtcm_data_t gpsRtcmData{};
        gpsRtcmData.flags = packet.flags;
        gpsRtcmData.len = static_cast<uint8_t>(packet.data.size());
        if (!packet.data.isEmpty()) {
            (void) memcpy(gpsRtcmData.data, packet.data.constData(), static_cast<size_t>(packet.data.size()));
        }
        _sendMessageOnAllLinks(gpsRtcmData);
    }
}

void RTCMMavlink::sendSimulatedData(const std::atomic_bool& requestStop)
{
    constexpr int kMessageLengths[] = {30, 170, 240};
    const QByteArray payload(kMessageLengths[2], '\0');
    while (!requestStop) {
        for (const int length : kMessageLengths) {
            RTCMDataUpdate(QByteArrayView(payload).first(length));
            QThread::msleep(4);
        }
        QThread::msleep(100);
    }
}

void RTCMMavlink::_sendMessageOnAllLinks(const mavlink_gps_rtcm_data_t& data)
{
    QmlObjectListModel* const vehicles = MultiVehicleManager::instance()->vehicles();
    QSet<const LinkInterface*> sentLinks;
    for (qsizetype i = 0; i < vehicles->count(); i++) {
        Vehicle* const vehicle = qobject_cast<Vehicle*>(vehicles->get(i));
        if (!vehicle) {
            continue;
        }
        const SharedLinkInterfacePtr sharedLink = vehicle->vehicleLinkManager()->primaryLink().lock();
        if (!sharedLink || !sharedLink->isConnected()) {
            continue;
        }
        // RTCM corrections are broadcast data. Send only once per link so that vehicles
        // which share the same link don't cause duplicate sends. UDP links in particular
        // send each write to all connected endpoints on the link. Send directly on the
        // link rather than through a Vehicle so the send is not tied to whichever vehicle
        // happens to be first on a shared link.
        if (sentLinks.contains(sharedLink.get())) {
            continue;
        }
        (void) sentLinks.insert(sharedLink.get());

        mavlink_message_t message{};
        (void) mavlink_msg_gps_rtcm_data_encode_chan(MAVLinkProtocol::instance()->getSystemId(),
                                                     MAVLinkProtocol::getComponentId(), sharedLink->mavlinkChannel(),
                                                     &message, &data);
        sharedLink->sendMessageThreadSafe(message);
    }
}
