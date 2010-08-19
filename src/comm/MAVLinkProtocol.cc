/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit
Please see our website at <http://pixhawk.ethz.ch>

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
 *   @brief Implementation of the MAVLink protocol
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <inttypes.h>
#include <iostream>

#include <QDebug>
#include <QTime>
#include <QApplication>

#include "MG.h"
#include "MAVLinkProtocol.h"
#include "UASInterface.h"
#include "UASManager.h"
#include "UASInterface.h"
#include "UAS.h"
#include "SlugsMAV.h"
#include "PxQuadMAV.h"
#include "ArduPilotMAV.h"
#include "configuration.h"
#include "LinkManager.h"
#include <mavlink.h>
#include "QGC.h"

/**
 * The default constructor will create a new MAVLink object sending heartbeats at
 * the MAVLINK_HEARTBEAT_DEFAULT_RATE to all connected links.
 */
MAVLinkProtocol::MAVLinkProtocol() :
        heartbeatTimer(new QTimer(this)),
        heartbeatRate(MAVLINK_HEARTBEAT_DEFAULT_RATE),
        m_heartbeatsEnabled(false),
        m_loggingEnabled(false),
        m_logfile(NULL)
{
    start(QThread::LowPriority);
    // Start heartbeat timer, emitting a heartbeat at the configured rate
    connect(heartbeatTimer, SIGNAL(timeout()), this, SLOT(sendHeartbeat()));
    heartbeatTimer->start(1000/heartbeatRate);
    totalReceiveCounter = 0;
    totalLossCounter = 0;
    currReceiveCounter = 0;
    currLossCounter = 0;
    for (int i = 0; i < 256; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            lastIndex[i][j] = -1;
        }
    }
}

MAVLinkProtocol::~MAVLinkProtocol()
{
    if (m_logfile)
    {
        m_logfile->close();
        delete m_logfile;
    }
}



void MAVLinkProtocol::run()
{

}

QString MAVLinkProtocol::getLogfileName()
{
    return QCoreApplication::applicationDirPath()+"/mavlink.log";
}

/**
 * The bytes are copied by calling the LinkInterface::readBytes() method.
 * This method parses all incoming bytes and constructs a MAVLink packet.
 * It can handle multiple links in parallel, as each link has it's own buffer/
 * parsing state machine.
 * @param link The interface to read from
 * @see LinkInterface
 **/
void MAVLinkProtocol::receiveBytes(LinkInterface* link, QByteArray b)
{
    receiveMutex.lock();
    mavlink_message_t message;
    mavlink_status_t status;
    for (int position = 0; position < b.size(); position++)
    {
        unsigned int decodeState = mavlink_parse_char(link->getId(), (uint8_t)(b.at(position)), &message, &status);

        if (decodeState == 1)
        {
            // Log data
            if (m_loggingEnabled)
            {
                uint8_t buf[MAVLINK_MAX_PACKET_LEN];
                quint64 time = MG::TIME::getGroundTimeNowUsecs();
                memcpy(buf, (void*)&time, sizeof(quint64));
                mavlink_msg_to_send_buffer(buf+sizeof(quint64), &message);
                m_logfile->write((const char*) buf);
                qDebug() << "WROTE LOGFILE";
            }

            // ORDER MATTERS HERE!
            // If the matching UAS object does not yet exist, it has to be created
            // before emitting the packetReceived signal
            UASInterface* uas = UASManager::instance()->getUASForId(message.sysid);

            // Check and (if necessary) create UAS object
            if (uas == NULL && message.msgid == MAVLINK_MSG_ID_HEARTBEAT)
            {
                // ORDER MATTERS HERE!
                // The UAS object has first to be created and connected,
                // only then the rest of the application can be made aware
                // of its existence, as it only then can send and receive
                // it's first messages.

                // FIXME Current debugging
                // check if the UAS has the same id like this system
                if (message.sysid == getSystemId())
                {
                    qDebug() << "WARNING\nWARNING\nWARNING\nWARNING\nWARNING\nWARNING\nWARNING\n\n RECEIVED MESSAGE FROM THIS SYSTEM WITH ID" << message.msgid << "FROM COMPONENT" << message.compid;
                }

                // Create a new UAS based on the heartbeat received
                // Todo dynamically load plugin at run-time for MAV
                // WIKISEARCH:AUTOPILOT_TYPE_INSTANTIATION

                // First create new UAS object
                // Decode heartbeat message
                mavlink_heartbeat_t heartbeat;
                mavlink_msg_heartbeat_decode(&message, &heartbeat);
                switch (heartbeat.autopilot)
                {
                case MAV_AUTOPILOT_GENERIC:
                    uas = new UAS(this, message.sysid);
                    // Connect this robot to the UAS object
                    connect(this, SIGNAL(messageReceived(LinkInterface*, mavlink_message_t)), uas, SLOT(receiveMessage(LinkInterface*, mavlink_message_t)));
                    break;
                case MAV_AUTOPILOT_PIXHAWK:
                    {
                    // Fixme differentiate between quadrotor and coaxial here
                    PxQuadMAV* mav = new PxQuadMAV(this, message.sysid);
                    // Connect this robot to the UAS object
                    // it is IMPORTANT here to use the right object type,
                    // else the slot of the parent object is called (and thus the special
                    // packets never reach their goal)
                    connect(this, SIGNAL(messageReceived(LinkInterface*, mavlink_message_t)), mav, SLOT(receiveMessage(LinkInterface*, mavlink_message_t)));
                    uas = mav;
                }
                    break;
                case MAV_AUTOPILOT_SLUGS:
                    {
                    SlugsMAV* mav = new SlugsMAV(this, message.sysid);
                    // Connect this robot to the UAS object
                    // it is IMPORTANT here to use the right object type,
                    // else the slot of the parent object is called (and thus the special
                    // packets never reach their goal)
                    connect(this, SIGNAL(messageReceived(LinkInterface*, mavlink_message_t)), mav, SLOT(receiveMessage(LinkInterface*, mavlink_message_t)));
                    uas = mav;
                }
                    break;
                case MAV_AUTOPILOT_ARDUPILOT:
                    {
                    ArduPilotMAV* mav = new ArduPilotMAV(this, message.sysid);
                    // Connect this robot to the UAS object
                    // it is IMPORTANT here to use the right object type,
                    // else the slot of the parent object is called (and thus the special
                    // packets never reach their goal)
                    connect(this, SIGNAL(messageReceived(LinkInterface*, mavlink_message_t)), mav, SLOT(receiveMessage(LinkInterface*, mavlink_message_t)));
                    uas = mav;
                }
                    break;
                default:
                    uas = new UAS(this, message.sysid);
                    break;
                }


                // Make UAS aware that this link can be used to communicate with the actual robot
                uas->addLink(link);
                // Now add UAS to "official" list, which makes the whole application aware of it
                UASManager::instance()->addUAS(uas);
            }

            // Only count message if UAS exists for this message
            if (uas != NULL)
            {
                // Increase receive counter
                totalReceiveCounter++;
                currReceiveCounter++;
                qint64 lastLoss = totalLossCounter;
                // Update last packet index
                if (lastIndex[message.sysid][message.compid] == -1)
                {
                    lastIndex[message.sysid][message.compid] = message.seq;
                }
                else
                {
                    if (lastIndex[message.sysid][message.compid] == 255)
                    {
                        lastIndex[message.sysid][message.compid] = 0;
                    }
                    else
                    {
                        lastIndex[message.sysid][message.compid]++;
                    }

                    int safeguard = 0;
                    //qDebug() << "SYSID" << message.sysid << "COMPID" << message.compid << "MSGID" << message.msgid << "LASTINDEX" << lastIndex[message.sysid][message.compid] << "SEQ" << message.seq;
                    while(lastIndex[message.sysid][message.compid] != message.seq && safeguard < 255)
                    {
                        if (lastIndex[message.sysid][message.compid] == 255)
                        {
                            lastIndex[message.sysid][message.compid] = 0;
                        }
                        else
                        {
                            lastIndex[message.sysid][message.compid]++;
                        }
                        totalLossCounter++;
                        currLossCounter++;
                        safeguard++;
                    }
                }
                //            if (lastIndex.contains(message.sysid))
                //            {
                //                QMap<int, int>* lastCompIndex = lastIndex.value(message.sysid);
                //                if (lastCompIndex->contains(message.compid))
                //                while (lastCompIndex->value(message.compid, 0)+1 )
                //            }
                //if ()

                // If a new loss was detected or we just hit one 128th packet step
                if (lastLoss != totalLossCounter || (totalReceiveCounter & 0x7F) == 0)
                {
                    // Calculate new loss ratio
                    // Receive loss
                    float receiveLoss = (double)currLossCounter/(double)(currReceiveCounter+currLossCounter);
                    receiveLoss *= 100.0f;
                    // qDebug() << "LOSSCHANGED" << receiveLoss;
                    currLossCounter = 0;
                    currReceiveCounter = 0;
                    emit receiveLossChanged(message.sysid, receiveLoss);
                }

                // The packet is emitted as a whole, as it is only 255 - 261 bytes short
                // kind of inefficient, but no issue for a groundstation pc.
                // It buys as reentrancy for the whole code over all threads
                emit messageReceived(link, message);
            }
        }
    }
    receiveMutex.unlock();
}

/**
 * @return The name of this protocol
 **/
QString MAVLinkProtocol::getName()
{
    return QString(tr("MAVLink protocol"));
}

/** @return System id of this application */
int MAVLinkProtocol::getSystemId()
{
    return MG::SYSTEM::ID;
}

/** @return Component id of this application */
int MAVLinkProtocol::getComponentId()
{
    return MG::SYSTEM::COMPID;
}

/**
 * @param message message to send
 */
void MAVLinkProtocol::sendMessage(mavlink_message_t message)
{
    // Get all links connected to this unit
    QList<LinkInterface*> links = LinkManager::instance()->getLinksForProtocol(this);

    // Emit message on all links that are currently connected
    QList<LinkInterface*>::iterator i;
    for (i = links.begin(); i != links.end(); ++i)
    {
        sendMessage(*i, message);
        //qDebug() << __FILE__ << __LINE__ << "SENT MESSAGE OVER" << ((LinkInterface*)*i)->getName() << "LIST SIZE:" << links.size();
    }
}

/**
 * @param link the link to send the message over
 * @param message message to send
 */
void MAVLinkProtocol::sendMessage(LinkInterface* link, mavlink_message_t message)
{
    // Create buffer
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    // Write message into buffer, prepending start sign
    int len = mavlink_msg_to_send_buffer(buffer, &message);
    // If link is connected
    if (link->isConnected())
    {
        // Send the portion of the buffer now occupied by the message
        link->writeBytes((const char*)buffer, len);
    }
}

/**
 * The heartbeat is sent out of order and does not reset the
 * periodic heartbeat emission. It will be just sent in addition.
 * @return mavlink_message_t heartbeat message sent on serial link
 */
void MAVLinkProtocol::sendHeartbeat()
{
    if (m_heartbeatsEnabled)
    {
        mavlink_message_t beat;
        mavlink_msg_heartbeat_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID,&beat, OCU, MAV_AUTOPILOT_GENERIC);
        sendMessage(beat);
    }
}

/** @param enabled true to enable heartbeats emission at heartbeatRate, false to disable */
void MAVLinkProtocol::enableHeartbeats(bool enabled)
{
    m_heartbeatsEnabled = enabled;
    emit heartbeatChanged(enabled);
}

void MAVLinkProtocol::enableLogging(bool enabled)
{
    if (enabled && !m_loggingEnabled)
    {
       m_logfile = new QFile(getLogfileName());
       m_logfile->open(QIODevice::WriteOnly | QIODevice::Append);
    }
    else
    {
       m_logfile->close();
       delete m_logfile;
       m_logfile = NULL;
    }
    m_loggingEnabled = enabled;
}

bool MAVLinkProtocol::heartbeatsEnabled(void)
{
    return m_heartbeatsEnabled;
}

bool MAVLinkProtocol::loggingEnabled(void)
{
    return m_loggingEnabled;
}

/**
 * The default rate is 1 Hertz.
 *
 * @param rate heartbeat rate in hertz (times per second)
 */
void MAVLinkProtocol::setHeartbeatRate(int rate)
{
    heartbeatRate = rate;
    heartbeatTimer->setInterval(1000/heartbeatRate);
}

/** @return heartbeat rate in Hertz */
int MAVLinkProtocol::getHeartbeatRate()
{
    return heartbeatRate;
}
