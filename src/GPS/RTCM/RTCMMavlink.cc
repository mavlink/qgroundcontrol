/*=====================================================================

 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

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
