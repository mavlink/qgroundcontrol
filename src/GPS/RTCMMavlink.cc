/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "RTCMMavlink.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(RTCMMavlinkLog, "qgc.gps.rtcmmavlink")

RTCMMavlink::RTCMMavlink(QObject *parent)
    : QObject(parent)
{
    // qCDebug(RTCMMavlinkLog) << Q_FUNC_INFO << this;

    _bandwidthTimer.start();
}

RTCMMavlink::~RTCMMavlink()
{
    // qCDebug(RTCMMavlinkLog) << Q_FUNC_INFO << this;
}

void RTCMMavlink::RTCMDataUpdate(QByteArrayView data)
{
#ifdef QT_DEBUG
    _calculateBandwith(data.size());
#endif

    mavlink_gps_rtcm_data_t gpsRtcmData{};

    static constexpr qsizetype maxMessageLength = MAVLINK_MSG_GPS_RTCM_DATA_FIELD_DATA_LEN;
    if (data.size() < maxMessageLength) {
        gpsRtcmData.len = data.size();
        gpsRtcmData.flags = (_sequenceId & 0x1FU) << 3;
        (void) memcpy(&gpsRtcmData.data, data.data(), data.size());
        _sendMessageToVehicle(gpsRtcmData);
    } else {
        uint8_t fragmentId = 0;
        qsizetype start = 0;
        while (start < data.size()) {
            gpsRtcmData.flags = 0x01U; // LSB set indicates message is fragmented
            gpsRtcmData.flags |= fragmentId++ << 1; // Next 2 bits are fragment id
            gpsRtcmData.flags |= (_sequenceId & 0x1FU) << 3; // Next 5 bits are sequence id

            const qsizetype length = std::min(data.size() - start, maxMessageLength);
            gpsRtcmData.len = length;

            (void) memcpy(gpsRtcmData.data, data.constData() + start, length);
            _sendMessageToVehicle(gpsRtcmData);

            start += length;
        }
    }

    ++_sequenceId;
}

void RTCMMavlink::_sendMessageToVehicle(const mavlink_gps_rtcm_data_t &data)
{
    QmlObjectListModel* const vehicles = MultiVehicleManager::instance()->vehicles();
    for (qsizetype i = 0; i < vehicles->count(); i++) {
        Vehicle* const vehicle = qobject_cast<Vehicle*>(vehicles->get(i));
        const SharedLinkInterfacePtr sharedLink = vehicle->vehicleLinkManager()->primaryLink().lock();
        if (sharedLink) {
            mavlink_message_t message;
            (void) mavlink_msg_gps_rtcm_data_encode_chan(
                MAVLinkProtocol::instance()->getSystemId(),
                MAVLinkProtocol::getComponentId(),
                sharedLink->mavlinkChannel(),
                &message,
                &data
            );
            (void) vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), message);
        }
    }
}

void RTCMMavlink::_calculateBandwith(qsizetype bytes)
{
    if (!_bandwidthTimer.isValid()) {
        return;
    }

    _bandwidthByteCounter += bytes;

    const qint64 elapsed = _bandwidthTimer.elapsed();
    if (elapsed > 1000) {
        qCDebug(RTCMMavlinkLog) << QStringLiteral("RTCM bandwidth: %1 kB/s").arg(((_bandwidthByteCounter / elapsed) * 1000.f) / 1024.f);
        (void) _bandwidthTimer.restart();
        _bandwidthByteCounter = 0;
    }
}
