/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "RTCMMavlink.h"

#include "MultiVehicleManager.h"
#include "Vehicle.h"

#include <cstdio>

RTCMMavlink::RTCMMavlink(QGCToolbox& toolbox)
    : _toolbox(toolbox)
{
    _bandwidthTimer.start();
}

void RTCMMavlink::RTCMDataUpdate(QByteArray message)
{
    /* statistics */
    _bandwidthByteCounter += message.size();
    qint64 elapsed = _bandwidthTimer.elapsed();
    if (elapsed > 1000) {
        printf("RTCM bandwidth: %.2f kB/s\n", (float) _bandwidthByteCounter / elapsed * 1000.f / 1024.f);
        _bandwidthTimer.restart();
        _bandwidthByteCounter = 0;
    }

    const int maxMessageLength = MAVLINK_MSG_GPS_RTCM_DATA_FIELD_DATA_LEN;
    mavlink_gps_rtcm_data_t mavlinkRtcmData;
    memset(&mavlinkRtcmData, 0, sizeof(mavlink_gps_rtcm_data_t));

    if (message.size() < maxMessageLength) {
        mavlinkRtcmData.len = message.size();
        mavlinkRtcmData.flags = (_sequenceId & 0x1F) << 3;
        memcpy(&mavlinkRtcmData.data, message.data(), message.size());
        sendMessageToVehicle(mavlinkRtcmData);
    } else {
        // We need to fragment

        uint8_t fragmentId = 0;         // Fragment id indicates the fragment within a set
        int start = 0;
        while (start < message.size()) {
            int length = std::min(message.size() - start, maxMessageLength);
            mavlinkRtcmData.flags = 1;                      // LSB set indicates message is fragmented
            mavlinkRtcmData.flags |= fragmentId++ << 1;     // Next 2 bits are fragment id
            mavlinkRtcmData.flags |= (_sequenceId & 0x1F) << 3;     // Next 5 bits are sequence id
            mavlinkRtcmData.len = length;
            memcpy(&mavlinkRtcmData.data, message.data() + start, length);
            sendMessageToVehicle(mavlinkRtcmData);
            start += length;
        }
    }
    ++_sequenceId;
}

void RTCMMavlink::sendMessageToVehicle(const mavlink_gps_rtcm_data_t& msg)
{
    QmlObjectListModel& vehicles = *_toolbox.multiVehicleManager()->vehicles();
    MAVLinkProtocol* mavlinkProtocol = _toolbox.mavlinkProtocol();
    for (int i = 0; i < vehicles.count(); i++) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehicles[i]);
        mavlink_message_t message;
        mavlink_msg_gps_rtcm_data_encode_chan(mavlinkProtocol->getSystemId(),
                                              mavlinkProtocol->getComponentId(),
                                              vehicle->priorityLink()->mavlinkChannel(),
                                              &message,
                                              &msg);
        vehicle->sendMessageOnLink(vehicle->priorityLink(), message);
    }
}
