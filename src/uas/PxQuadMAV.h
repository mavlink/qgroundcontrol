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

#ifndef PXQUADMAV_H
#define PXQUADMAV_H

#include "UAS.h"

class PxQuadMAV : public UAS
{
    Q_OBJECT
    Q_INTERFACES(UASInterface)
public:
    PxQuadMAV(MAVLinkProtocol* mavlink, int id);
public slots:
    /** @brief Receive a MAVLink message from this MAV */
    void receiveMessage(LinkInterface* link, mavlink_message_t message);
#if defined(QGC_PROTOBUF_ENABLED)
    /** @brief Receive a Protobuf message from this MAV */
    void receiveExtendedMessage(LinkInterface* link, std::tr1::shared_ptr<google::protobuf::Message> message);
#endif
    /** @brief Send a command to an onboard process */
    void sendProcessCommand(int watchdogId, int processId, unsigned int command);
signals:
    void watchdogReceived(int systemId, int watchdogId, unsigned int processCount);
    void processReceived(int systemId, int watchdogId, int processId, QString name, QString arguments, int timeout);
    void processChanged(int systemId, int watchdogId, int processId, int state, bool muted, int crashed, int pid);
};

#endif // PXQUADMAV_H
