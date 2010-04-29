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
#include "MAVLinkSimulationLink.h"
// MAVLINK includes
#include <mavlink.h>

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

    // Comments on the variables can be found in the header file

    simulationFile = new QFile(readFile, this);
    if (simulationFile->exists() && simulationFile->open(QIODevice::ReadOnly | QIODevice::Text))
    {
        simulationHeader = simulationFile->readLine();
    }
    receiveFile = new QFile(writeFile, this);
    lastSent = MG::TIME::getGroundTimeNow();

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
}

MAVLinkSimulationLink::~MAVLinkSimulationLink()
{
    //TODO Check destructor
    //    fileStream->flush();
    //    outStream->flush();
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
                emit bytesReady(this);
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
    unsigned int bufferlength = message_to_send_buffer(buf, msg);
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
    static float drainRate = 0.0025; // x.xx% of the capacity is linearly drained per second

    attitude_t attitude;
    raw_aux_t rawAuxValues;
    raw_imu_t rawImuValues;

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
        messageSize = message_boot_pack(systemId, componentId, &msg, version);
        // Allocate buffer with packet data
        bufferlength = message_to_send_buffer(buffer, &msg);
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

                    if (keys.value(i, "") == "Pressure")
                    {
                        rawAuxValues.baro = d;
                    }

                    if (keys.value(i, "") == "Battery")
                    {
                        rawAuxValues.vbat = d;
                    }

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
                attitude.msec = time;
                // Pack message and get size of encoded byte string
                message_attitude_encode(systemId, componentId, &msg, &attitude);
                // Allocate buffer with packet data
                bufferlength = message_to_send_buffer(buffer, &msg);
                //add data into datastream
                memcpy(stream+streampointer,buffer, bufferlength);
                streampointer += bufferlength;

                // IMU
                rawImuValues.msec = time;
                rawImuValues.xmag = 0;
                rawImuValues.ymag = 0;
                rawImuValues.zmag = 0;
                // Pack message and get size of encoded byte string
                message_raw_imu_encode(systemId, componentId, &msg, &rawImuValues);
                // Allocate buffer with packet data
                bufferlength = message_to_send_buffer(buffer, &msg);
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

        status.vbat = voltage;

        // Pack message and get size of encoded byte string
        messageSize = message_sys_status_encode(systemId, componentId, &msg, &status);
        // Allocate buffer with packet data
        bufferlength = message_to_send_buffer(buffer, &msg);
        //add data into datastream
        memcpy(stream+streampointer,buffer, bufferlength);
        streampointer += bufferlength;

        // Pack debug text message
        statustext_t text;
        text.severity = 0;
        strcpy((char*)(text.text), "DEBUG MESSAGE TEXT");
        message_statustext_encode(systemId, componentId, &msg, &text);
        bufferlength = message_to_send_buffer(buffer, &msg);
        memcpy(stream+streampointer, buffer, bufferlength);
        streampointer += bufferlength;

        /*
        // Pack message and get size of encoded byte string
        messageSize = message_boot_pack(systemId, componentId, &msg, version);
        // Allocate buffer with packet data
        bufferlength = message_to_send_buffer(buffer, &msg);
        //add data into datastream
        memcpy(stream+streampointer,buffer, bufferlength);
        streampointer += bufferlength;*/

        // HEARTBEAT

        static int typeCounter = 0;
        uint8_t mavType = typeCounter % (OCU);
        typeCounter++;

        // Pack message and get size of encoded byte string
        messageSize = message_heartbeat_pack(systemId, componentId, &msg, mavType);
        // Allocate buffer with packet data
        bufferlength = message_to_send_buffer(buffer, &msg);
        //add data into datastream
        memcpy(stream+streampointer,buffer, bufferlength);
        streampointer += bufferlength;


        /*
        // HEARTBEAT VEHICLE 2

        // Pack message and get size of encoded byte string
        messageSize = message_heartbeat_pack(42, componentId, &msg, MAV_FIXED_WING);
        // Allocate buffer with packet data
        bufferlength = message_to_send_buffer(buffer, &msg);
        //add data into datastream
        memcpy(stream+streampointer,buffer, bufferlength);
        streampointer += bufferlength;

        // STATUS VEHICLE 2
        sys_status_t status2;
        status2.mode = MAV_MODE_LOCKED;
        status2.vbat = voltage;
        status2.status = MAV_STATE_STANDBY;

        // Pack message and get size of encoded byte string
        messageSize = message_sys_status_encode(systemId, componentId, &msg, &status);
        // Allocate buffer with packet data
        bufferlength = message_to_send_buffer(buffer, &msg);
        //add data into datastream
        memcpy(stream+streampointer,buffer, bufferlength);
        streampointer += bufferlength;
        */
        //qDebug() << "BOOT" << "BUF LEN" << bufferlength << "POINTER" << streampointer;

        // AUX STATUS
        rawAuxValues.vbat = voltage;

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
        messageSize = message_attitude_pack(systemId, componentId, &msg, usec, roll, pitch, yaw, 0, 0, 0);
        // Allocate buffer with packet data
        bufferlength = message_to_send_buffer(buffer, &msg);
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
    mavlink_message_t ret;
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
                    set_mode_t mode;
                    message_set_mode_decode(&msg, &mode);
                    // Set mode indepent of mode.target
                    status.mode = mode.mode;
                }
                // EXECUTE OPERATOR ACTIONS
            case MAVLINK_MSG_ID_ACTION:
                {
                    action_t action;
                    message_action_decode(&msg, &action);
                    switch (action.action)
                    {
                    case MAV_ACTION_LAUNCH:
                        status.status = MAV_STATE_ACTIVE;
                        status.mode = MAV_MODE_AUTO;
                        break;
                    case MAV_ACTION_RETURN:
                        status.status = MAV_STATE_LANDING;
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

            case MAVLINK_MSG_ID_MANUAL_CONTROL:
                {
                    manual_control_t control;
                    message_manual_control_decode(&msg, &control);
                    qDebug() << "\n" << "ROLL:" << control.roll << "PITCH:" << control.pitch;
                }
                break;
            case MAVLINK_MSG_ID_PARAM_REQUEST_LIST:
                {
                    param_request_list_t read;
                    message_param_request_list_decode(&msg, &read);
                    // Output all params

                    // Pack message and get size of encoded byte string
                    message_param_value_pack(systemId, componentId, &msg, (int8_t*)"ROLL_K_P", 0.5f);
                    // Allocate buffer with packet data
                    bufferlength = message_to_send_buffer(buffer, &msg);

                    //add data into datastream
                    memcpy(stream+streampointer,buffer, bufferlength);
                    streampointer+=bufferlength;
                }
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


void MAVLinkSimulationLink::readBytes(char* const data, qint64 maxLength) {
    // Lock concurrent resource readyBuffer
    readyBufferMutex.lock();
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
