#include "RTCMMavlink.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(RTCMMavlinkLog, "GPS.RTCMMavlink")

RTCMMavlink::RTCMMavlink(QObject *parent)
    : QObject(parent)
    , _sender(&RTCMMavlink::_defaultSendToVehicles)
{
    _bandwidthTimer.start();
}

RTCMMavlink::~RTCMMavlink() = default;

void RTCMMavlink::RTCMDataUpdate(QByteArrayView data)
{
    _calculateBandwidth(data.size());

    mavlink_gps_rtcm_data_t gpsRtcmData{};

    static constexpr qsizetype maxMessageLength = MAVLINK_MSG_GPS_RTCM_DATA_FIELD_DATA_LEN;
    if (data.size() < maxMessageLength) {
        gpsRtcmData.len = data.size();
        gpsRtcmData.flags = (_sequenceId & 0x1FU) << 3;
        (void) memcpy(&gpsRtcmData.data, data.data(), data.size());
        _sender(gpsRtcmData);
    } else {
        uint8_t fragmentId = 0;
        qsizetype start = 0;
        while (start < data.size()) {
            gpsRtcmData.flags = 0x01U; // LSB set indicates message is fragmented
            gpsRtcmData.flags |= (fragmentId++ & 0x03U) << 1; // Next 2 bits are fragment id
            gpsRtcmData.flags |= (_sequenceId & 0x1FU) << 3; // Next 5 bits are sequence id

            const qsizetype length = std::min(data.size() - start, maxMessageLength);
            gpsRtcmData.len = length;

            (void) memcpy(gpsRtcmData.data, data.constData() + start, length);
            _sender(gpsRtcmData);

            start += length;
        }
    }

    ++_sequenceId;
}

void RTCMMavlink::_defaultSendToVehicles(const mavlink_gps_rtcm_data_t &data)
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

void RTCMMavlink::_calculateBandwidth(qsizetype bytes)
{
    if (!_bandwidthTimer.isValid()) {
        return;
    }

    _bandwidthByteCounter += bytes;
    _totalBytesSent += static_cast<quint64>(bytes);
    emit totalBytesSentChanged();

    const qint64 elapsed = _bandwidthTimer.elapsed();
    if (elapsed > 1000) {
        _bandwidthKBps = (static_cast<double>(_bandwidthByteCounter) / elapsed * 1000.0) / 1024.0;
        qCDebug(RTCMMavlinkLog) << QStringLiteral("RTCM bandwidth: %1 kB/s").arg(_bandwidthKBps, 0, 'f', 1);
        emit bandwidthChanged();
        (void) _bandwidthTimer.restart();
        _bandwidthByteCounter = 0;
    }
}
