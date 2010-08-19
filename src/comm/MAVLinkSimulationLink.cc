/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009, 2010 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Implementation of simulated system link
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <cmath>
#include <QTime>
#include <QImage>
#include <QDebug>
#include "MG.h"
#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "MAVLinkSimulationLink.h"
// MAVLINK includes
#include <mavlink.h>
#include "QGC.h"

/**
 * Create a simulated link. This link is connected to an input and output file.
 * The link sends one line at a time at the specified sendrate. The timing of
 * the sendrate is free of drift, which means it is stable on the long run.
 * However, small deviations are mixed in to vary the sendrate slightly
 * in order to simulate disturbances on a real communication link.
 *
 * @param readFile The file with the messages to read (must be in ASCII format, line breaks can be Unix or Windows style)
 * @param writeFile The received messages are written to that file
 * @param rate The rate at which the messages are sent (in intervals of milliseconds)
 **/
MAVLinkSimulationLink::MAVLinkSimulationLink(QString readFile, QString writeFile, int rate) :
        readyBytes(0),
        timeOffset(0)
{
    this->rate = rate;
    _isConnected = false;

    onboardParams = QMap<QString, float>();
    onboardParams.insert("PID_ROLL_K_P", 0.5f);
    onboardParams.insert("PID_PITCH_K_P", 0.5f);
    onboardParams.insert("PID_YAW_K_P", 0.5f);
    onboardParams.insert("PID_XY_K_P", 0.5f);
    onboardParams.insert("PID_ALT_K_P", 0.5f);
    onboardParams.insert("SYS_TYPE", 1);
    onboardParams.insert("SYS_ID", systemId);

    // Comments on the variables can be found in the header file

    simulationFile = new QFile(readFile, this);
    if (simulationFile->exists() && simulationFile->open(QIODevice::ReadOnly | QIODevice::Text))
    {
        simulationHeader = simulationFile->readLine();
    }
    receiveFile = new QFile(writeFile, this);
    lastSent = MG::TIME::getGroundTimeNow() * 1000;

    if (simulationFile->exists())
    {
        this->name = "Simulation: " + QFileInfo(simulationFile->fileName()).fileName();
    }
    else
    {
        this->name = "MAVLink simulation link";
    }

    // Initialize the pseudo-random number generator
    srand(QTime::currentTime().msec());
    maxTimeNoise = 0;
    this->id = getNextLinkId();
    LinkManager::instance()->add(this);

    // Open packet log
    mavlinkLogFile = new QFile(MAVLinkProtocol::getLogfileName());
    mavlinkLogFile->open(QIODevice::ReadOnly);

    x = 0;
    y = 0;
    z = 0;
    yaw = 0;
}

MAVLinkSimulationLink::~MAVLinkSimulationLink()
{
    //TODO Check destructor
    //    fileStream->flush();
    //    outStream->flush();
    delete simulationFile;
}

void MAVLinkSimulationLink::run()
{

    status.mode = MAV_MODE_UNINIT;
    status.status = MAV_STATE_UNINIT;
    status.vbat = 0;
    status.motor_block = 1;
    status.packet_drop = 0;

    forever
    {

        static quint64 last = 0;

        if (MG::TIME::getGroundTimeNow() - last >= rate)
        {
            if (_isConnected)
            {
                mainloop();

                // FIXME Hacky code to read from packet log file
//                readyBufferMutex.lock();
//                for (int i = 0; i < streampointer; i++)
//                {
//                    readyBuffer.enqueue(*(stream + i));
//                }
//                readyBufferMutex.unlock();

                readBytes();
            }
            last = MG::TIME::getGroundTimeNow();
        }
        MG::SLEEP::msleep(2);

    }
}

void MAVLinkSimulationLink::enqueue(uint8_t* stream, uint8_t* index, mavlink_message_t* msg)
{
    // Allocate buffer with packet data
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    unsigned int bufferlength = mavlink_msg_to_send_buffer(buf, msg);
    //add data into datastream
    memcpy(stream+(*index),buf, bufferlength);
    (*index) += bufferlength;
}

void MAVLinkSimulationLink::mainloop()
{

    // Test for encoding / decoding packets

    // Test data stream
    const int streamlength = 4096;
    int streampointer = 0;
    //const int testoffset = 0;
    uint8_t stream[streamlength] = {};

    // Fake system values

    static float fullVoltage = 4.2 * 3;
    static float emptyVoltage = 3.35 * 3;
    static float voltage = fullVoltage;
    static float drainRate = 0.025; // x.xx% of the capacity is linearly drained per second

    mavlink_attitude_t attitude;
    memset(&attitude, 0, sizeof(mavlink_attitude_t));
    #ifdef MAVLINK_ENABLED_PIXHAWK_MESSAGES
      mavlink_raw_aux_t rawAuxValues;
      memset(&rawAuxValues, 0, sizeof(mavlink_raw_aux_t));
    #endif
    mavlink_raw_imu_t rawImuValues;
    memset(&rawImuValues, 0, sizeof(mavlink_raw_imu_t));

    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    int bufferlength;
    int messageSize;
    mavlink_message_t msg;

    // Timers
    static unsigned int rate1hzCounter = 1;
    static unsigned int rate10hzCounter = 1;
    static unsigned int rate50hzCounter = 1;

    // Vary values

    // VOLTAGE
    // The battery is drained constantly
    voltage = voltage - ((fullVoltage - emptyVoltage) * drainRate / rate);
    if (voltage < 3.550 * 3) voltage = 3.550 * 3;

    static int state = 0;

    if (state == 0)
    {
        // BOOT
        // Pack message and get size of encoded byte string
        messageSize = mavlink_msg_boot_pack(systemId, componentId, &msg, version);
        // Allocate buffer with packet data
        bufferlength = mavlink_msg_to_send_buffer(buffer, &msg);
        //add data into datastream
        memcpy(stream+streampointer,buffer, bufferlength);
        streampointer += bufferlength;
        state++;
    }


    // 50 HZ TASKS
    if (rate50hzCounter == 1000 / rate / 40)
    {
        if (simulationFile->isOpen())
        {
            if (simulationFile->atEnd())
            {
                // We reached the end of the file, start from scratch
                simulationFile->reset();
                simulationHeader = simulationFile->readLine();
            }

            // Data was made available, read one line
            // first entry is the timestamp
            QString values = QString(simulationFile->readLine());
            QStringList parts = values.split("\t");
            QStringList keys = simulationHeader.split("\t");
            //qDebug() << simulationHeader;
            //qDebug() << values;
            bool ok;
            static quint64 lastTime = 0;
            static quint64 baseTime = 0;
            quint64 time = QString(parts.first()).toLongLong(&ok, 10);
	    // FIXME Remove multiplicaton by 1000
	    time *= 1000;

            if (ok)
            {
                if (timeOffset == 0)
                {
                    timeOffset = time;
                    baseTime = time;
                }

                if (lastTime > time)
                {
                    // We have wrapped around in the logfile
                    // Add the measurement time interval to the base time
                    baseTime += lastTime - timeOffset;
                }
                lastTime = time;

                time = time - timeOffset + baseTime;

                // Gather individual measurement values
                for (int i = 1; i < (parts.size() - 1); ++i)
                {
                    // Get one data field
                    bool res;
                    double d = QString(parts.at(i)).toDouble(&res);
                    if (!res) d = 0;

                    //qDebug() << "TIME" << time << "VALUE" << d;
                    //emit valueChanged(220, keys.at(i), d, MG::TIME::getGroundTimeNow());

                    if (keys.value(i, "") == "Accel._X")
                    {
                        rawImuValues.xacc = d;
                    }

                    if (keys.value(i, "") == "Accel._Y")
                    {
                        rawImuValues.yacc = d;
                    }

                    if (keys.value(i, "") == "Accel._Z")
                    {
                        rawImuValues.zacc = d;
                    }
                    if (keys.value(i, "") == "Gyro_Phi")
                    {
                        rawImuValues.xgyro = d;
                    }

                    if (keys.value(i, "") == "Gyro_Theta")
                    {
                        rawImuValues.ygyro = d;
                    }

                    if (keys.value(i, "") == "Gyro_Psi")
                    {
                        rawImuValues.zgyro = d;
                    }
#ifdef MAVLINK_ENABLED_PIXHAWK_MESSAGES
                    if (keys.value(i, "") == "Pressure")
                    {
                        rawAuxValues.baro = d;
                    }

                    if (keys.value(i, "") == "Battery")
                    {
                        rawAuxValues.vbat = d;
                    }
#endif
                    if (keys.value(i, "") == "roll_IMU")
                    {
                        attitude.roll = d;
                    }

                    if (keys.value(i, "") == "pitch_IMU")
                    {
                        attitude.pitch = d;
                    }

                    if (keys.value(i, "") == "yaw_IMU")
                    {
                        attitude.yaw = d;
                    }

                    //Accel._X	Accel._Y	Accel._Z	Battery	Bottom_Rotor	CPU_Load	Ground_Dist.	Gyro_Phi	Gyro_Psi	Gyro_Theta	Left_Servo	Mag._X	Mag._Y	Mag._Z	Pressure	Right_Servo	Temperature	Top_Rotor	pitch_IMU	roll_IMU	yaw_IMU

                }
                // Send out packets


                // ATTITUDE
                attitude.usec = time;
                // Pack message and get size of encoded byte string
                mavlink_msg_attitude_encode(systemId, componentId, &msg, &attitude);
                // Allocate buffer with packet data
                bufferlength = mavlink_msg_to_send_buffer(buffer, &msg);
                //add data into datastream
                memcpy(stream+streampointer,buffer, bufferlength);
                streampointer += bufferlength;

                // IMU
                rawImuValues.usec = time;
                rawImuValues.xmag = 0;
                rawImuValues.ymag = 0;
                rawImuValues.zmag = 0;
                // Pack message and get size of encoded byte string
                mavlink_msg_raw_imu_encode(systemId, componentId, &msg, &rawImuValues);
                // Allocate buffer with packet data
                bufferlength = mavlink_msg_to_send_buffer(buffer, &msg);
                //add data into datastream
                memcpy(stream+streampointer,buffer, bufferlength);
                streampointer += bufferlength;

                //qDebug() << "ATTITUDE" << "BUF LEN" << bufferlength << "POINTER" << streampointer;

                //qDebug() << "REALTIME" << MG::TIME::getGroundTimeNow() << "ONBOARDTIME" << attitude.msec << "ROLL" << attitude.roll;

            }

        }

        rate50hzCounter = 1;
    }


    // 10 HZ TASKS
    if (rate10hzCounter == 1000 / rate / 9)
    {
        rate10hzCounter = 1;

        // Move X Position
        x += sin(QGC::groundTimeUsecs()) * 0.1f;
        y += sin(QGC::groundTimeUsecs()) * 0.1f;
        z += sin(QGC::groundTimeUsecs()) * 0.1f;

        x = (x > 1.0f) ? 1.0f : x;
        y = (y > 1.0f) ? 1.0f : y;
        z = (z > 1.0f) ? 1.0f : z;
        // Send back new setpoint
        mavlink_message_t ret;
        mavlink_msg_local_position_setpoint_pack(systemId, componentId, &ret, spX, spY, spZ, spYaw);
        bufferlength = mavlink_msg_to_send_buffer(buffer, &msg);
        //add data into datastream
        memcpy(stream+streampointer,buffer, bufferlength);
        streampointer += bufferlength;
    }

    // 1 HZ TASKS
    if (rate1hzCounter == 1000 / rate / 1)
    {
        // STATE
        static int statusCounter = 0;
        if (statusCounter == 100)
        {
            status.mode = (status.mode + 1) % MAV_MODE_TEST3;
            statusCounter = 0;
        }
        statusCounter++;

        status.vbat = voltage * 1000; // millivolts

        // Pack message and get size of encoded byte string
        messageSize = mavlink_msg_sys_status_encode(systemId, componentId, &msg, &status);
        // Allocate buffer with packet data
        bufferlength = mavlink_msg_to_send_buffer(buffer, &msg);
        //add data into datastream
        memcpy(stream+streampointer,buffer, bufferlength);
        streampointer += bufferlength;

        // Pack debug text message
        mavlink_statustext_t text;
        text.severity = 0;
        strcpy((char*)(text.text), "DEBUG MESSAGE TEXT");
        mavlink_msg_statustext_encode(systemId, componentId, &msg, &text);
        bufferlength = mavlink_msg_to_send_buffer(buffer, &msg);
        memcpy(stream+streampointer, buffer, bufferlength);
        streampointer += bufferlength;

        /*
        // Pack message and get size of encoded byte string
        messageSize = mavlink_msg_boot_pack(systemId, componentId, &msg, version);
        // Allocate buffer with packet data
        bufferlength = mavlink_msg_to_send_buffer(buffer, &msg);
        //add data into datastream
        memcpy(stream+streampointer,buffer, bufferlength);
        streampointer += bufferlength;*/

        // HEARTBEAT

        static int typeCounter = 0;
        uint8_t mavType = typeCounter % (OCU);
        typeCounter++;

        // Pack message and get size of encoded byte string
        messageSize = mavlink_msg_heartbeat_pack(systemId, componentId, &msg, mavType, MAV_AUTOPILOT_PIXHAWK);
        // Allocate buffer with packet data
        bufferlength = mavlink_msg_to_send_buffer(buffer, &msg);
        //add data into datastream
        memcpy(stream+streampointer,buffer, bufferlength);
        streampointer += bufferlength;


        // Send controller states
        uint8_t attControl = 1;
        uint8_t posXYControl = 1;
        uint8_t posZControl = 0;
        uint8_t posYawControl = 1;

        uint8_t gpsLock = 2;
        uint8_t visLock = 3;
        uint8_t posLock = qMax(gpsLock, visLock);

        #ifdef MAVLINK_ENABLED_PIXHAWK_MESSAGES
        messageSize = mavlink_msg_control_status_pack(systemId, componentId, &msg, posLock, visLock, gpsLock, attControl, posXYControl, posZControl, posYawControl);
        #endif

        bufferlength = mavlink_msg_to_send_buffer(buffer, &msg);
        memcpy(stream+streampointer, buffer, bufferlength);
        streampointer += bufferlength;


        /*
        // HEARTBEAT VEHICLE 2

        // Pack message and get size of encoded byte string
        messageSize = mavlink_msg_heartbeat_pack(42, componentId, &msg, MAV_FIXED_WING);
        // Allocate buffer with packet data
        bufferlength = mavlink_msg_to_send_buffer(buffer, &msg);
        //add data into datastream
        memcpy(stream+streampointer,buffer, bufferlength);
        streampointer += bufferlength;

        // STATUS VEHICLE 2
        sys_status_t status2;
        status2.mode = MAV_MODE_LOCKED;
        status2.vbat = voltage;
        status2.status = MAV_STATE_STANDBY;

        // Pack message and get size of encoded byte string
        messageSize = mavlink_msg_sys_status_encode(systemId, componentId, &msg, &status);
        // Allocate buffer with packet data
        bufferlength = mavlink_msg_to_send_buffer(buffer, &msg);
        //add data into datastream
        memcpy(stream+streampointer,buffer, bufferlength);
        streampointer += bufferlength;
        */
        //qDebug() << "BOOT" << "BUF LEN" << bufferlength << "POINTER" << streampointer;

        // AUX STATUS
        #ifdef MAVLINK_ENABLED_PIXHAWK_MESSAGES
        rawAuxValues.vbat = voltage;
#endif

        rate1hzCounter = 1;
    }

    // FULL RATE TASKS
    // Default is 50 Hz

    /*
    // 50 HZ TASKS
    if (rate50hzCounter == 1000 / rate / 50)
    {

        //streampointer = 0;

        // Attitude

        // Pack message and get size of encoded byte string
        messageSize = mavlink_msg_attitude_pack(systemId, componentId, &msg, usec, roll, pitch, yaw, 0, 0, 0);
        // Allocate buffer with packet data
        bufferlength = mavlink_msg_to_send_buffer(buffer, &msg);
        //add data into datastream
        memcpy(stream+streampointer,buffer, bufferlength);
        streampointer += bufferlength;

        rate50hzCounter = 1;
    }*/

    readyBufferMutex.lock();
    for (int i = 0; i < streampointer; i++)
    {
        readyBuffer.enqueue(*(stream + i));
    }
    readyBufferMutex.unlock();

    // Increment counters after full main loop
    rate1hzCounter++;
    rate10hzCounter++;
    rate50hzCounter++;
}


qint64 MAVLinkSimulationLink::bytesAvailable()
{
    readyBufferMutex.lock();
    qint64 size = readyBuffer.size();
    readyBufferMutex.unlock();
    return size;
}

void MAVLinkSimulationLink::writeBytes(const char* data, qint64 size)
{
    qDebug() << "Simulation received " << size << " bytes from groundstation: ";

    // Increase write counter
    //bitsSentTotal += size * 8;

    // Parse bytes
    mavlink_message_t msg;
    mavlink_status_t comm;

    uint8_t stream[2048];
    int streampointer = 0;
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    int bufferlength = 0;

    // Output all bytes as hex digits
    int i;
    for (i=0; i<size; i++)
    {
        if (mavlink_parse_char(this->id, data[i], &msg, &comm))
        {
            // MESSAGE RECEIVED!

            switch (msg.msgid)
            {
                // SET THE SYSTEM MODE
            case MAVLINK_MSG_ID_SET_MODE:
                {
                    mavlink_set_mode_t mode;
                    mavlink_msg_set_mode_decode(&msg, &mode);
                    // Set mode indepent of mode.target
                    status.mode = mode.mode;
                }
                break;
            case MAVLINK_MSG_ID_LOCAL_POSITION_SETPOINT_SET:
                {
                    mavlink_local_position_setpoint_set_t set;
                    mavlink_msg_local_position_setpoint_set_decode(&msg, &set);
                    spX = set.x;
                    spY = set.y;
                    spZ = set.z;
                    spYaw = set.yaw;

                    // Send back new setpoint
                    mavlink_message_t ret;
                    mavlink_msg_local_position_setpoint_pack(systemId, componentId, &ret, spX, spY, spZ, spYaw);
                    bufferlength = mavlink_msg_to_send_buffer(buffer, &msg);
                    //add data into datastream
                    memcpy(stream+streampointer,buffer, bufferlength);
                    streampointer += bufferlength;
                }
                break;
                // EXECUTE OPERATOR ACTIONS
            case MAVLINK_MSG_ID_ACTION:
                {
                    mavlink_action_t action;
                    mavlink_msg_action_decode(&msg, &action);

                    qDebug() << "SIM" << "received action" << action.action << "for system" << action.target;

                    switch (action.action)
                    {
                    case MAV_ACTION_LAUNCH:
                        status.status = MAV_STATE_ACTIVE;
                        status.mode = MAV_MODE_AUTO;
                        break;
                    case MAV_ACTION_RETURN:
                        status.status = MAV_STATE_ACTIVE;
                        break;
                    case MAV_ACTION_MOTORS_START:
                        status.status = MAV_STATE_ACTIVE;
                        status.mode = MAV_MODE_LOCKED;
                        break;
                    case MAV_ACTION_MOTORS_STOP:
                        status.status = MAV_STATE_STANDBY;
                        status.mode = MAV_MODE_LOCKED;
                        break;
                    case MAV_ACTION_EMCY_KILL:
                        status.status = MAV_STATE_EMERGENCY;
                        status.mode = MAV_MODE_MANUAL;
                        break;
                    case MAV_ACTION_SHUTDOWN:
                        status.status = MAV_STATE_POWEROFF;
                        status.mode = MAV_MODE_LOCKED;
                        break;
                    }
                }
                break;
#ifdef MAVLINK_ENABLED_PIXHAWK_MESSAGES
            case MAVLINK_MSG_ID_MANUAL_CONTROL:
                {
                  #ifdef MAVLINK_ENABLED_PIXHAWK_MESSAGES
                    mavlink_manual_control_t control;
                    mavlink_msg_manual_control_decode(&msg, &control);
                    qDebug() << "\n" << "ROLL:" << control.roll << "PITCH:" << control.pitch;
                  #endif
                }
                break;
#endif
            case MAVLINK_MSG_ID_PARAM_REQUEST_LIST:
                {
                    qDebug() << "GCS REQUESTED PARAM LIST FROM SIMULATION";
                    mavlink_param_request_list_t read;
                    mavlink_msg_param_request_list_decode(&msg, &read);
                    if (read.target_system == systemId)
                    {
                        // Output all params
                        // Iterate through all components, through all parameters and emit them
                        QMap<QString, float>::iterator i;
                        // Iterate through all components / subsystems
                        int j = 0;
                        for (i = onboardParams.begin(); i != onboardParams.end(); ++i)
                        {
                            // Pack message and get size of encoded byte string
                            mavlink_msg_param_value_pack(systemId, componentId, &msg, (int8_t*)i.key().toStdString().c_str(), i.value(), onboardParams.size(), j);
                            // Allocate buffer with packet data
                            bufferlength = mavlink_msg_to_send_buffer(buffer, &msg);
                            //add data into datastream
                            memcpy(stream+streampointer,buffer, bufferlength);
                            streampointer+=bufferlength;
                            j++;
                        }

                        /*
                        // Pack message and get size of encoded byte string
                        mavlink_msg_param_value_pack(systemId, componentId, &msg, (int8_t*)"ROLL_K_P", 0.5f);
                        // Allocate buffer with packet data
                        bufferlength = mavlink_msg_to_send_buffer(buffer, &msg);
                        //add data into datastream
                        memcpy(stream+streampointer,buffer, bufferlength);
                        streampointer+=bufferlength;

                        // Pack message and get size of encoded byte string
                        mavlink_msg_param_value_pack(systemId, componentId, &msg, (int8_t*)"PITCH_K_P", 0.6f);
                        // Allocate buffer with packet data
                        bufferlength = mavlink_msg_to_send_buffer(buffer, &msg);
                        //add data into datastream
                        memcpy(stream+streampointer,buffer, bufferlength);
                        streampointer+=bufferlength;

                        // Pack message and get size of encoded byte string
                        mavlink_msg_param_value_pack(systemId, componentId, &msg, (int8_t*)"YAW_K_P", 0.8f);
                        // Allocate buffer with packet data
                        bufferlength = mavlink_msg_to_send_buffer(buffer, &msg);
                        //add data into datastream
                        memcpy(stream+streampointer,buffer, bufferlength);
                        streampointer+=bufferlength;*/

                        qDebug() << "SIMULATION SENT PARAMETERS TO GCS";
                    }
                }
                break;
            case MAVLINK_MSG_ID_PARAM_SET:
                {
                    qDebug() << "SIMULATION RECEIVED COMMAND TO SET PARAMETER";
                    mavlink_param_set_t set;
                    mavlink_msg_param_set_decode(&msg, &set);
                    if (set.target_system == systemId)
                    {
                        QString key = QString((char*)set.param_id);
                        if (onboardParams.contains(key))
                        {
                            onboardParams.remove(key);
                            onboardParams.insert(key, set.param_value);

                            // Pack message and get size of encoded byte string
                            mavlink_msg_param_value_pack(systemId, componentId, &msg, (int8_t*)key.toStdString().c_str(), set.param_value, onboardParams.size(), 0);
                            // Allocate buffer with packet data
                            bufferlength = mavlink_msg_to_send_buffer(buffer, &msg);
                            //add data into datastream
                            memcpy(stream+streampointer,buffer, bufferlength);
                            streampointer+=bufferlength;
                        }
                    }
                }
                break;
            }


        }
        unsigned char v=data[i];
        fprintf(stderr,"%02x ", v);
    }
    fprintf(stderr,"\n");

    readyBufferMutex.lock();
    for (int i = 0; i < streampointer; i++)
    {
        readyBuffer.enqueue(*(stream + i));
    }
    readyBufferMutex.unlock();

    // Update comm status
    status.packet_drop = comm.packet_rx_drop_count;

}


void MAVLinkSimulationLink::readBytes() {
    // Lock concurrent resource readyBuffer
    readyBufferMutex.lock();
    const qint64 maxLength = 2048;
    char data[maxLength];
    qint64 len = maxLength;
    if (maxLength > readyBuffer.size()) len = readyBuffer.size();

    for (unsigned int i = 0; i < len; i++)
    {
        *(data + i) = readyBuffer.takeFirst();
    }

    QByteArray b(data, len);
    emit bytesReceived(this, b);

    readyBufferMutex.unlock();

    //    if (len > 0)
    //    {
    //        qDebug() << "Simulation sent " << len << " bytes to groundstation: ";
    //
    //        /* Increase write counter */
    //        //bitsSentTotal += size * 8;
    //
    //        //Output all bytes as hex digits
    //        int i;
    //        for (i=0; i<len; i++)
    //        {
    //            unsigned int v=data[i];
    //            fprintf(stderr,"%02x ", v);
    //        }
    //        fprintf(stderr,"\n");
    //    }
}

/**
 * Disconnect the connection.
 *
 * @return True if connection has been disconnected, false if connection
 * couldn't be disconnected.
 **/
bool MAVLinkSimulationLink::disconnect() {

    if(isConnected()) {
        //        timer->stop();

        _isConnected = false;

        emit disconnected();

        //exit();
    }

    return true;
}

/**
 * Connect the link.
 *
 * @return True if connection has been established, false if connection
 * couldn't be established.
 **/
bool MAVLinkSimulationLink::connect()
{
    _isConnected = true;

    start(LowPriority);
    //    timer->start(rate);
    return true;
}

/**
 * Connect the link.
 *
 * @param connect true connects the link, false disconnects it
 * @return True if connection has been established, false if connection
 * couldn't be established.
 **/
bool MAVLinkSimulationLink::connectLink(bool connect)
{
    _isConnected = connect;

    if(connect)
    {
        this->connect();
    }

    return true;
}

/**
 * Check if connection is active.
 *
 * @return True if link is connected, false otherwise.
 **/
bool MAVLinkSimulationLink::isConnected() {
    return _isConnected;
}

int MAVLinkSimulationLink::getId()
{
    return id;
}

QString MAVLinkSimulationLink::getName()
{
    return name;
}

qint64 MAVLinkSimulationLink::getNominalDataRate() {
    /* 100 Mbit is reasonable fast and sufficient for all embedded applications */
    return 100000000;
}

qint64 MAVLinkSimulationLink::getTotalUpstream() {
    return 0;
    //TODO Add functionality here
    // @todo Add functionality here
}

qint64 MAVLinkSimulationLink::getShortTermUpstream() {
    return 0;
}

qint64 MAVLinkSimulationLink::getCurrentUpstream() {
    return 0;
}

qint64 MAVLinkSimulationLink::getMaxUpstream() {
    return 0;
}

qint64 MAVLinkSimulationLink::getBitsSent() {
    return 0;
}

qint64 MAVLinkSimulationLink::getBitsReceived() {
    return 0;
}

qint64 MAVLinkSimulationLink::getTotalDownstream() {
    return 0;
}

qint64 MAVLinkSimulationLink::getShortTermDownstream() {
    return 0;
}

qint64 MAVLinkSimulationLink::getCurrentDownstream() {
    return 0;
}

qint64 MAVLinkSimulationLink::getMaxDownstream() {
    return 0;
}

bool MAVLinkSimulationLink::isFullDuplex() {
    /* Full duplex is no problem when running in pure software, but this is a serial simulation */
    return false;
}

int MAVLinkSimulationLink::getLinkQuality() {
    /* The Link quality is always perfect when running in software */
    return 100;
}
