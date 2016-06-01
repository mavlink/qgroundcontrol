/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "RTCMMavlink.h"

#include "MultiVehicleManager.h"
#include "MAVLinkProtocol.h"
#include "Vehicle.h"

#include <cstdio>

RTCMMavlink::RTCMMavlink(QGCToolbox& toolbox)
    : _toolbox(toolbox)
{
    _bandwidthTimer.start();
}

void RTCMMavlink::RTCMDataUpdate(QByteArray message)
{
    Q_ASSERT(message.size() <= 110); //mavlink message uses a fixed-size buffer

    /* statistics */
    _bandwidthByteCounter += message.size();
    qint64 elapsed = _bandwidthTimer.elapsed();
    if (elapsed > 1000) {
        printf("RTCM bandwidth: %.2f kB/s\n", (float) _bandwidthByteCounter / elapsed * 1000.f / 1024.f);
        _bandwidthTimer.restart();
        _bandwidthByteCounter = 0;
    }
    QmlObjectListModel& vehicles = *_toolbox.multiVehicleManager()->vehicles();
    MAVLinkProtocol* mavlinkProtocol = _toolbox.mavlinkProtocol();
    mavlink_gps_inject_data_t mavlinkRtcmData;
    memset(&mavlinkRtcmData, 0, sizeof(mavlink_gps_inject_data_t));

    mavlinkRtcmData.len = message.size();
    memcpy(&mavlinkRtcmData.data, message.data(), message.size());

    for (int i = 0; i < vehicles.count(); i++) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehicles[i]);
        mavlink_message_t message;
        mavlink_msg_gps_inject_data_encode(mavlinkProtocol->getSystemId(),
                mavlinkProtocol->getComponentId(), &message, &mavlinkRtcmData);
        vehicle->sendMessage(message);
    }
}
