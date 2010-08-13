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
 *   @brief Example implementation of a different protocol than the default MAVLink.
 *   @author Lorenz Meier <mail@qgroundcontrol.org>
 */

#include "MAVLinkLightProtocol.h"
#include "UASManager.h"
#include "ArduPilotMAV.h"
#include "LinkManager.h"

MAVLinkLightProtocol::MAVLinkLightProtocol() :
    MAVLinkProtocol()
{
}


/**
 * @param message message to send
 */
void MAVLinkLightProtocol::sendMessage(mavlink_light_message_t message)
{
    // Get all links connected to this unit
    QList<LinkInterface*> links = LinkManager::instance()->getLinksForProtocol(this);

    // Emit message on all links that are currently connected
    QList<LinkInterface*>::iterator i;
    for (i = links.begin(); i != links.end(); ++i)
    {
        sendMessage(*i, message);
    }
}

/**
 * @param link the link to send the message over
 * @param message message to send
 */
void MAVLinkLightProtocol::sendMessage(LinkInterface* link, mavlink_light_message_t message)
{
    // Create buffer
    uint8_t buffer[100]; // MAXIMUM PACKET LENGTH, INCLUDING STX BYTES
    // Write message into buffer, prepending start sign
    //int len = mavlink_msg_to_send_buffer(buffer, &message);


    // FIXME TO SEND BUFFER FUNCTION MISSING
    Q_UNUSED(message);
    int len = 0;

    // If link is connected
    if (link->isConnected())
    {
        // Send the portion of the buffer now occupied by the message
        link->writeBytes((const char*)buffer, len);
    }
}

/**
 * The bytes are copied by calling the LinkInterface::readBytes() method.
 * This method parses all incoming bytes and constructs a MAVLink packet.
 * It can handle multiple links in parallel, as each link has it's own buffer/
 * parsing state machine.
 * @param link The interface to read from
 * @see LinkInterface
 **/
void MAVLinkLightProtocol::receiveBytes(LinkInterface* link)
{
    receiveMutex.lock();
    // Prepare buffer
    static const int maxlen = 4096 * 100;
    static char buffer[maxlen];
    qint64 bytesToRead = link->bytesAvailable();

    // Get all data at once, let link read the bytes in the buffer array
    link->readBytes(buffer, maxlen);

    // Run through all bytes
    for (int position = 0; position < bytesToRead; position++)
    {
        mavlink_light_message_t msg;
        // FIXME PARSE
        if (1 == 0/* parsing returned a message */)
        {

            int sysid = 0; // system id from message, or always null if only one MAV is supported
            UASInterface* uas = UASManager::instance()->getUASForId(sysid);

            // Check and (if necessary) create UAS object
            if (uas == NULL)
            {
                ArduPilotMAV* mav = new ArduPilotMAV(this, sysid); // FIXME change to msg.sysid if this field exists
                // Connect this robot to the UAS object
                // it is IMPORTANT here to use the right object type,
                // else the slot of the parent object is called (and thus the special
                // packets never reach their goal)
                connect(this, SIGNAL(messageReceived(LinkInterface*, mavlink_message_t)), mav, SLOT(receiveMessage(LinkInterface*, mavlink_message_t)));
                uas = mav;
                // Make UAS aware that this link can be used to communicate with the actual robot
                uas->addLink(link);
                // Now add UAS to "official" list, which makes the whole application aware of it
                UASManager::instance()->addUAS(uas);
            }

            // The packet is emitted as a whole, as it is only 255 - 261 bytes short
            // kind of inefficient, but no issue for a groundstation pc.
            // It buys as reentrancy for the whole code over all threads
            emit messageReceived(link, msg);
        }
    }
    receiveMutex.unlock();
}

/**
 * @return The name of this protocol
 **/
QString MAVLinkLightProtocol::getName()
{
    return QString(tr("MAVLinkLight protocol"));
}
