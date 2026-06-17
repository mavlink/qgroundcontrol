#include "RTCMMavlink.h"

#include <QtCore/QByteArray>
#include <QtCore/QThread>

#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

QGC_LOGGING_CATEGORY(RTCMMavlinkLog, "GPS.RTCMMavlink")

RTCMMavlink::RTCMMavlink(QObject* parent) : QObject(parent)
{
    qCDebug(RTCMMavlinkLog) << this;
}

RTCMMavlink::~RTCMMavlink()
{
    qCDebug(RTCMMavlinkLog) << this;
}

void RTCMMavlink::RTCMDataUpdate(QByteArrayView data)
{
    _rateTracker.recordBytes(data.size());
    if (_rateTracker.rateUpdated()) {
        qCDebug(RTCMMavlinkLog) << QStringLiteral("RTCM bandwidth: %1 kB/s").arg(_rateTracker.kBps(), 0, 'f', 3);
        emit bandwidthChanged();
    }

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
            gpsRtcmData.flags = 0x01U;                        // LSB set indicates message is fragmented
            gpsRtcmData.flags |= fragmentId++ << 1;           // Next 2 bits are fragment id
            gpsRtcmData.flags |= (_sequenceId & 0x1FU) << 3;  // Next 5 bits are sequence id

            const qsizetype length = std::min(data.size() - start, maxMessageLength);
            gpsRtcmData.len = length;

            (void) memcpy(gpsRtcmData.data, data.constData() + start, length);
            _sendMessageToVehicle(gpsRtcmData);

            start += length;
        }
    }

    ++_sequenceId;
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

void RTCMMavlink::_sendMessageToVehicle(const mavlink_gps_rtcm_data_t& data)
{
    QmlObjectListModel* const vehicles = MultiVehicleManager::instance()->vehicles();
    for (qsizetype i = 0; i < vehicles->count(); i++) {
        Vehicle* const vehicle = qobject_cast<Vehicle*>(vehicles->get(i));
        if (!vehicle) {
            continue;
        }
        const SharedLinkInterfacePtr sharedLink = vehicle->vehicleLinkManager()->primaryLink().lock();
        if (sharedLink) {
            mavlink_message_t message;
            (void) mavlink_msg_gps_rtcm_data_encode_chan(MAVLinkProtocol::instance()->getSystemId(),
                                                         MAVLinkProtocol::getComponentId(),
                                                         sharedLink->mavlinkChannel(), &message, &data);
            (void) vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), message);
        }
    }
}
