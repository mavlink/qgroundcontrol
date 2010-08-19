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
 *   @brief Brief Description
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QDebug>
#include <QTime>

#include <MG.h>
#include <AS4Protocol.h>
#include <UASInterface.h>
#include <UASManager.h>
#include <UASInterface.h>
#include <UAS.h>
#include <configuration.h>
#include <LinkManager.h>
#include <inttypes.h>
#include <iostream>


AS4Protocol::AS4Protocol()
{

    // Start heartbeat timer, emitting a heartbeat at the configured rate
    heartbeatRate = 1; ///< SAE AS-4 has a fixed heartbeat rate of 1 hz.
//    heartbeatTimer = new QTimer(this);
//    connect(heartbeatTimer, SIGNAL(timeout()), this, SLOT());
//    heartbeatTimer->start(1000/heartbeatRate);
/*
    // Start the node manager
    configData = new FileLoader("nodeManager.conf");
    handler = new MyHandler();

        try
        {
                nodeManager = new NodeManager(configData, handler);
                qDebug() << "SAE AS-4 NODE MANAGER constructed";
        }
        catch(char *exceptionString)
        {
                printf("%s", exceptionString);
                printf("Terminating Program...\n");
        }
        catch(...)
        {
                printf("Node Manager Construction Failed. Terminating Program...\n");
        }
        */

}

AS4Protocol::~AS4Protocol()
{
//        delete nodeManager;
//        delete handler;
//        delete configData;
}

void AS4Protocol::run()
{
}


/**
 * @brief Receive bytes from a communication interface.
 *
 * The bytes copied by calling the LinkInterface::readBytes() method.
 *
 * @param link The interface to read from
 * @see LinkInterface
 **/
void AS4Protocol::receiveBytes(LinkInterface* link)
{
//    receiveMutex.lock();
    // Prepare buffer
    //static const int maxlen = 4096 * 100;
    //static char buffer[maxlen];

    qint64 bytesToRead = link->bytesAvailable();

    // Get all data at once, let link read the bytes in the buffer array
    //link->readBytes(buffer, maxlen);
//
//    /*
//    // Debug output
//    std::cerr << "receive buffer: ";
//    for (int i = 0; i < bytesToRead; i++)
//    {
//            std::cerr << std::hex << static_cast<unsigned char>(buffer[i]);
//    }
//    std::cerr << std::dec << " length: " << bytesToRead;
//    */
//
//    qDebug() << __FILE__ << __LINE__ << ": buffer size:" << maxlen << "bytes:" << bytesToRead;
//

    for (int position = 0; position < bytesToRead; position++)
    {
    }
//    receiveMutex.unlock();

}


/**
 * @brief Get the human-readable name of this protocol.
 *
 * @return The name of this protocol
 **/
QString AS4Protocol::getName()
{
    return QString(tr("SAE AS-4"));
}

void AS4Protocol::setHeartbeatRate(int rate)
{
    heartbeatRate = rate;
    heartbeatTimer->setInterval(1000/heartbeatRate);
}

int AS4Protocol::getHeartbeatRate()
{
    return heartbeatRate;
}
