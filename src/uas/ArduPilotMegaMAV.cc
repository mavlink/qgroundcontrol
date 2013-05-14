/*=====================================================================
QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

/**
 * @file
 *   @brief Implementation of class MainWindow
 *   @author Your Name here
 */

#include "ArduPilotMegaMAV.h"
ArduPilotMegaMAV::ArduPilotMegaMAV(MAVLinkProtocol* mavlink, int id) :
    UAS(mavlink, id)//,
    // place other initializers here
{
    //This does not seem to work. Manually request each stream type at a specified rate.
    // Ask for all streams at 4 Hz
    //enableAllDataTransmission(4);
    enableExtendedSystemStatusTransmission(2);
    enablePositionTransmission(3);
    enableExtra1Transmission(10);
    enableExtra2Transmission(10);
    enableExtra3Transmission(2);
    enableRawSensorDataTransmission(2);
    enableRCChannelDataTransmission(2);
}

/**
 * This function is called by MAVLink once a complete, uncorrupted (CRC check valid)
 * mavlink packet is received.
 *
 * @param link Hardware link the message came from (e.g. /dev/ttyUSB0 or UDP port).
 *             messages can be sent back to the system via this link
 * @param message MAVLink message, as received from the MAVLink protocol stack
 */
void ArduPilotMegaMAV::receiveMessage(LinkInterface* link, mavlink_message_t message)
{
    // Let UAS handle the default message set
    UAS::receiveMessage(link, message);

    if (message.sysid == uasId) {
        // Handle your special messages
        switch (message.msgid) {
        case MAVLINK_MSG_ID_HEARTBEAT:
        {
            //qDebug() << "ARDUPILOT RECEIVED HEARTBEAT";
            break;
        }
        default:
            //qDebug() << "\nARDUPILOT RECEIVED MESSAGE WITH ID" << message.msgid;
            break;
        }
    }
}
void ArduPilotMegaMAV::setMountConfigure(unsigned char mode, bool stabilize_roll,bool stabilize_pitch,bool stabilize_yaw)
{
    //Only supported by APM
    mavlink_message_t msg;
    mavlink_msg_mount_configure_pack(255,1,&msg,this->uasId,1,mode,stabilize_roll,stabilize_pitch,stabilize_yaw);
    sendMessage(msg);
}
void ArduPilotMegaMAV::setMountControl(double pa,double pb,double pc,bool islatlong)
{
    mavlink_message_t msg;
    if (islatlong)
    {
        mavlink_msg_mount_control_pack(255,1,&msg,this->uasId,1,pa*10000000.0,pb*10000000.0,pc*10000000.0,0);
    }
    else
    {
        mavlink_msg_mount_control_pack(255,1,&msg,this->uasId,1,pa,pb,pc,0);
    }
    sendMessage(msg);
}
