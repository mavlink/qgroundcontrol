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

#ifndef MAVLINKLIGHTPROTOCOL_H
#define MAVLINKLIGHTPROTOCOL_H

#include <inttypes.h>
#include <QByteArray>
#include <MAVLinkProtocol.h>

#define MAVLINK_LIGHT_MAX_PAYLOAD_LEN 50

// Not part of message struct, but sent on link
// uint8_t stx1;
// uint8_t stx2;
typedef struct {
    uint8_t msgid;   ///< ID of message in payload
    uint8_t payload[MAVLINK_LIGHT_MAX_PAYLOAD_LEN]; ///< Payload data, ALIGNMENT IMPORTANT ON MCU
    uint8_t ck_a;    ///< Checksum high byte
    uint8_t ck_b;    ///< Checksum low byte
} mavlink_light_message_t;

class MAVLinkLightProtocol : public MAVLinkProtocol
{
Q_OBJECT
public:
    explicit MAVLinkLightProtocol();
    QString getName();

signals:
    /** @brief Message received and directly copied via signal */
    void messageReceived(LinkInterface* link, mavlink_light_message_t message);

public slots:
    void sendMessage(mavlink_light_message_t message);
    void sendMessage(LinkInterface* link, mavlink_light_message_t message);
    void receiveBytes(LinkInterface* link, QByteArray b);

};

#endif // MAVLINKLIGHTPROTOCOL_H
