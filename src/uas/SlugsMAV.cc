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

#include "SlugsMAV.h"

#include <QDebug>

SlugsMAV::SlugsMAV(MAVLinkProtocol* mavlink, int id) :
        UAS(mavlink, id)//,
        // Place other initializers here
{
}

/**
 * This function is called by MAVLink once a complete, uncorrupted (CRC check valid)
 * mavlink packet is received.
 *
 * @param link Hardware link the message came from (e.g. /dev/ttyUSB0 or UDP port).
 *             messages can be sent back to the system via this link
 * @param message MAVLink message, as received from the MAVLink protocol stack
 */
void SlugsMAV::receiveMessage(LinkInterface* link, mavlink_message_t message)
{
    // Let UAS handle the default message set
    UAS::receiveMessage(link, message);

    // Handle your special messages mavlink_message_t* msg = &message;
    switch (message.msgid)
    {
    case MAVLINK_MSG_ID_HEARTBEAT:
        {
            qDebug() << "SLUGS RECEIVED HEARTBEAT";
            break;
        }

#ifdef MAVLINK_ENABLED_SLUGS_MESSAGES

    case MAVLINK_MSG_ID_CPU_LOAD:
        {
            mavlink_cpu_load_t cpu_load;
            quint64 time = getUnixTime(0);
            mavlink_msg_cpu_load_decode(&message,&cpu_load);

                emit valueChanged(uasId, tr("SensorDSC Load"), cpu_load.sensLoad, time);
                emit valueChanged(uasId, tr("ControlDSC Load"), cpu_load.ctrlLoad, time);
                emit valueChanged(uasId, tr("Battery Volt"), cpu_load.batVolt, time);

            break;
        }
    case MAVLINK_MSG_ID_AIR_DATA:
        {
            mavlink_air_data_t air_data;
            quint64 time = getUnixTime(0);
            mavlink_msg_air_data_decode(&message,&air_data);
                emit valueChanged(uasId, tr("Dynamic Pressure"), air_data.dynamicPressure,time);
                emit valueChanged(uasId, tr("Static Pressure"),air_data.staticPressure, time);
                emit valueChanged(uasId, tr("Temp"),air_data.temperature,time);

            break;
        }
    case MAVLINK_MSG_ID_SENSOR_BIAS:
        {
            mavlink_sensor_bias_t sensor_bias;
            quint64 time = getUnixTime(0);
            mavlink_msg_sensor_bias_decode(&message,&sensor_bias);
                emit valueChanged(uasId,tr("Ax Bias"),sensor_bias.axBias, time);
                emit valueChanged(uasId,tr("Ay Bias"),sensor_bias.ayBias,time);
                emit valueChanged(uasId,tr("Az Bias"),sensor_bias.azBias,time);
                emit valueChanged(uasId,tr("Gx Bias"),sensor_bias.gxBias,time);
                emit valueChanged(uasId,tr("Gy Bias"),sensor_bias.gyBias,time);
                emit valueChanged(uasId,tr("Gz Bias"),sensor_bias.gzBias,time);

            break;
        }
    case MAVLINK_MSG_ID_DIAGNOSTIC:
        {
            mavlink_diagnostic_t diagnostic;
            quint64 time = getUnixTime(0);
            mavlink_msg_diagnostic_decode(&message,&diagnostic);
                emit valueChanged(uasId,tr("Diag F1"),diagnostic.diagFl1,time);
                emit valueChanged(uasId,tr("Diag F2"),diagnostic.diagFl2,time);
                emit valueChanged(uasId,tr("Diag F3"),diagnostic.diagFl3,time);
                emit valueChanged(uasId,tr("Diag S1"),diagnostic.diagSh1,time);
                emit valueChanged(uasId,tr("Diag S2"),diagnostic.diagSh2,time);
                emit valueChanged(uasId,tr("Diag S3"),diagnostic.diagSh3,time);

            break;
        }
    case MAVLINK_MSG_ID_PILOT_CONSOLE:
        {
            mavlink_pilot_console_t pilot;
            quint64 time = getUnixTime(0);
            mavlink_msg_pilot_console_decode(&message,&pilot);
                emit valueChanged(uasId,"dt",pilot.dt,time);
                emit valueChanged(uasId,"dla",pilot.dla,time);
                emit valueChanged(uasId,"dra",pilot.dra,time);
                emit valueChanged(uasId,"dr",pilot.dr,time);
                emit valueChanged(uasId,"de",pilot.de,time);

            break;
        }
    case MAVLINK_MSG_ID_PWM_COMMANDS:
        {
            mavlink_pwm_commands_t pwm;
            quint64 time = getUnixTime(0);
            mavlink_msg_pwm_commands_decode(&message,&pwm);
                emit valueChanged(uasId,"dt_c",pwm.dt_c,time);
                emit valueChanged(uasId,"dla_c",pwm.dla_c,time);
                emit valueChanged(uasId,"dra_c",pwm.dra_c,time);
                emit valueChanged(uasId,"dr_c",pwm.dr_c,time);
                emit valueChanged(uasId,"dle_c",pwm.dle_c,time);
                emit valueChanged(uasId,"dre_c",pwm.dre_c,time);
                emit valueChanged(uasId,"dlf_c",pwm.dlf_c,time);
                emit valueChanged(uasId,"drf_c",pwm.drf_c,time);
                emit valueChanged(uasId,"da1_c",pwm.aux1,time);
                emit valueChanged(uasId,"da2_c",pwm.aux2,time);


            break;
        }
#endif

    default:
        qDebug() << "\nSLUGS RECEIVED MESSAGE WITH ID" << message.msgid;
        break;
    }
}
