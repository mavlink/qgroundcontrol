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
MAVLinkSimulationLink::MAVLinkSimulationLink(QFile* readFile, QFile* writeFile, int rate) :
        readyBytes(0)
{
    this->id = getNextLinkId();
    LinkManager::instance()->add(this);
    this->rate = rate;
    _isConnected = false;

    if (readFile != NULL)
    {
        this->name = "Simulation: " + readFile->fileName();
    }
    else
    {
        this->name = "MAVLink simulation link";
    }

    // Comments on the variables can be found in the header file

    simulationFile = readFile;
    simulationHeader = readFile->readLine();
    receiveFile = writeFile;
    lastSent = MG::TIME::getGroundTimeNow();

    // Initialize the pseudo-random number generator
    srand(QTime::currentTime().msec());
    maxTimeNoise = 0;

    //    timer = new QTimer(this);
    //    QObject::connect(timer, SIGNAL(timeout()), this, SLOT(mainloop()));
    //    _isConnected = false;
    //    this->rate = rate;

}

MAVLinkSimulationLink::~MAVLinkSimulationLink()
{
    //TODO Check destructor
    //    fileStream->flush();
    //    outStream->flush();
}

void MAVLinkSimulationLink::run()
{
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
        msleep((rate / 20));

    }
}

void MAVLinkSimulationLink::mainloop()
{

    // Test for encoding / decoding packets

    // Test data stream
    const int streamlength = 1024;
    int streampointer = 0;
    //const int testoffset = 0;
    uint8_t stream[streamlength];

    // Fake system values
    uint8_t systemId = 220;
    uint8_t componentId = 0;
    uint16_t version = 1000;

    // Fake sensor values
    uint32_t xacc = 0;
    uint32_t yacc = 0;
    uint32_t zacc = 9810;

    uint32_t xgyro = 10;
    uint32_t ygyro = 5;
    uint32_t zgyro = 100;

    uint32_t xmag = 0;
    uint32_t ymag = 1000;
    uint32_t zmag = 500;

    uint32_t pressure = 20000;
    uint32_t grounddist = 500;
    uint32_t temp = 20000;

    static float fullVoltage = 4.2 * 3;
    static float emptyVoltage = 3.35 * 3;
    static float voltage = fullVoltage;
    static float drainRate = 0.0025; // x.xx% of the capacity is linearly drained per second
//    uint32_t chan1 = 1500;
//    uint32_t chan2 = 1500;
//    uint32_t chan3 = 1500;
//    uint32_t chan4 = 1500;
//    uint32_t chan5 = 1500;

    float act1 = 0.1;
    float act2 = 0.2;
    float act3 = 0.3;
    float act4 = 0.4;

    // Fake coordinates

    float roll = 0.0f;
    float pitch = 10.0f;
    float yaw = 60.5f;


    // Fake Marker positions

    uint32_t markerId = 20;
    float confidence = 100.0f;
    float posX = -1.0f;
    float posY = 1.0f;
    float posZ = 2.5f;

    // Fake timestamp

    unsigned int usec = MG::TIME::getGroundTimeNow();

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

    //uint8_t msgbuffer[COMM_MAX_PACKET_LEN];

    // 100 HZ TASKS
    if (rate50hzCounter == 1000 / rate / 100)
    {
        // Read values
        char buf[1024];
        qint64 lineLength = simulationFile->readLine(buf, sizeof(buf));
        if (lineLength != -1)
        {
            // Data is available
        }
        else
        {
            // We reached the end of the file, start from scratch
            file.reset();
            //simulationHeader
        }
    }

    // 1 HZ TASKS
    if (rate1hzCounter == 1000 / rate / 1)
    {
        // BOOT

        // Pack message and get size of encoded byte string
        messageSize = message_boot_pack(systemId, componentId, &msg, version);
        // Allocate buffer with packet data
        bufferlength = message_to_send_buffer(buffer, &msg);
        //add data into datastream
        memcpy(stream+streampointer,buffer, bufferlength);
        streampointer += bufferlength;

        // HEARTBEAT

        // Pack message and get size of encoded byte string
        messageSize = message_heartbeat_pack(systemId, componentId, &msg, MAV_GENERIC);
        // Allocate buffer with packet data
        bufferlength = message_to_send_buffer(buffer, &msg);
        //add data into datastream
        memcpy(stream+streampointer,buffer, bufferlength);
        streampointer += bufferlength;

        readyBufferMutex.lock();
        for (int i = 0; i < streampointer; i++)
        {
            readyBuffer.enqueue(*(stream + i));
        }
        readyBufferMutex.unlock();

        rate1hzCounter = 1;
    }

    // 10 HZ TASKS
    if (rate10hzCounter == 1000 / rate / 10)
    {

        readyBufferMutex.lock();
        for (int i = 0; i < streampointer; i++)
        {
            readyBuffer.enqueue(*(stream + i));
        }
        readyBufferMutex.unlock();
        rate10hzCounter = 1;
    }

    // FULL RATE TASKS
    // Default is 50 Hz

    // 50 HZ TASKS
    if (rate50hzCounter == 1000 / rate / 50)
    {

        streampointer = 0;

        // Attitude

        // Pack message and get size of encoded byte string
        messageSize = message_attitude_pack(systemId, componentId, &msg, usec, roll, pitch, yaw, 0, 0, 0);
        // Allocate buffer with packet data
        bufferlength = message_to_send_buffer(buffer, &msg);
        //add data into datastream
        memcpy(stream+streampointer,buffer, bufferlength);
        streampointer += bufferlength;

        rate50hzCounter = 1;
    }

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

    // Output all bytes as hex digits
    int i;
    for (i=0; i<size; i++)
    {
        unsigned char v=data[i];
        fprintf(stderr,"%02x ", v);
    }
    fprintf(stderr,"\n");
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
 * Set the maximum time deviation noise. This amount (in milliseconds) is
 * the maximum time offset (+/-) from the specified message send rate.
 *
 * @param milliseconds The maximum time offset (in milliseconds)
 *
 * @bug The current implementation might induce one milliseconds additional
 * 		 discrepancy, this will be fixed by multithreading
 **/
void MAVLinkSimulationLink::setMaximumTimeNoise(int milliseconds) {
    maxTimeNoise = milliseconds;
}


/**
 * Add or subtract a pseudo random time offset. The maximum time offset is
 * defined by setMaximumTimeNoise().
 *
 * @see setMaximumTimeNoise()
 **/
void MAVLinkSimulationLink::addTimeNoise() {
    /* Calculate the time deviation */
    if(maxTimeNoise == 0) {
        /* Don't do expensive calculations if no noise is desired */
        timer->setInterval(rate);
    } else {
        /* Calculate random time noise (gauss distribution):
                 *
                 * (1) (2 * rand()) / RAND_MAX: Number between 0 and 2
                 * (induces numerical noise through floating point representation,
                 * ignored here)
                 *
                 * (2) ((2 * rand()) / RAND_MAX) - 1: Number between -1 and 1
                 *
                 * (3) Complete term: Number between -maxTimeNoise and +maxTimeNoise
                 */
        double timeDeviation = (((2 * rand()) / RAND_MAX) - 1) * maxTimeNoise;
        timer->setInterval(static_cast<int>(rate + floor(timeDeviation)));
    }

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

        exit();
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
