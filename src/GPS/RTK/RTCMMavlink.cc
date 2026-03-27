#include "RTCMMavlink.h"
#include "GPSRtk.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

#include <QtCore/QThread>

QGC_LOGGING_CATEGORY(RTCMMavlinkLog, "GPS.RTCMMavlink")

RTCMMavlink::RTCMMavlink(QObject *parent)
    : QObject(parent)
    , _sender(&RTCMMavlink::_defaultSendToVehicles)
{
    // Decay bandwidth display to 0 when no data arrives for 2 seconds
    _bandwidthDecayTimer.setInterval(2000);
    _bandwidthDecayTimer.setSingleShot(true);
    connect(&_bandwidthDecayTimer, &QTimer::timeout, this, [this]() {
        if (_rateTracker.kBps() != 0.0) {
            _rateTracker.reset();
            emit bandwidthChanged();
        }
    });
}

RTCMMavlink::~RTCMMavlink() = default;

void RTCMMavlink::addGpsDevice(GPSRtk *device)
{
    if (device && !_gpsDevices.contains(device)) {
        _gpsDevices.append(device);
    }
}

void RTCMMavlink::removeGpsDevice(GPSRtk *device)
{
    _gpsDevices.removeAll(device);
}

void RTCMMavlink::routeToBaseStation(QByteArrayView data)
{
    const QByteArray bytes = data.toByteArray();
    for (auto it = _gpsDevices.begin(); it != _gpsDevices.end(); ) {
        if (*it) {
            (*it)->injectRTCMData(bytes);
            ++it;
        } else {
            it = _gpsDevices.erase(it); // prune dead QPointers
        }
    }
}

void RTCMMavlink::setMessageSender(QObject *context, MessageSender sender)
{
    _senderContext = context;
    _useSenderContext = true;
    _sender = std::move(sender);
}

void RTCMMavlink::RTCMDataUpdate(QByteArrayView data)
{
    if (Q_UNLIKELY(QThread::currentThread() != thread())) {
        qCCritical(RTCMMavlinkLog) << "RTCMDataUpdate called from wrong thread — dropping data";
        return;
    }
    _rateTracker.recordBytes(data.size());
    if (_rateTracker.rateUpdated()) {
        emit bandwidthChanged();
        emit totalBytesSentChanged();
    }
    _bandwidthDecayTimer.start();

    if (_useSenderContext && _senderContext.isNull()) {
        qCWarning(RTCMMavlinkLog) << "Message sender context destroyed, skipping send";
        return;
    }

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
        if (!vehicle) {
            continue;
        }
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

// _calculateBandwidth removed — replaced by DataRateTracker
