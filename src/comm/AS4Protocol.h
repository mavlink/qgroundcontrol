/*=====================================================================
 
PIXHAWK Micro Air Vehicle Flying Robotics Toolkit
 
(c) 2009 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>
 
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

#ifndef AS4PROTOCOL_H_
#define AS4PROTOCOL_H_

#include <QObject>
#include <QMutex>
#include <QString>
#include <QTimer>
#include <QByteArray>
#include <ProtocolInterface.h>
#include <LinkInterface.h>
#include <protocol.h>
/*#include <openJaus.h>

class MyHandler : public EventHandler
{
public:
    ~MyHandler()
    {

    }

    void handleEvent(NodeManagerEvent *e)
    {
        SystemTreeEvent *treeEvent;
        ErrorEvent *errorEvent;
        JausMessageEvent *messageEvent;
        DebugEvent *debugEvent;
        ConfigurationEvent *configEvent;

        switch(e->getType())
        {
                        case NodeManagerEvent::SystemTreeEvent:
            treeEvent = (SystemTreeEvent *)e;
            printf("%s\n", treeEvent->toString().c_str());
            delete e;
            break;

                        case NodeManagerEvent::ErrorEvent:
            errorEvent = (ErrorEvent *)e;
            printf("%s\n", errorEvent->toString().c_str());
            delete e;
            break;

                        case NodeManagerEvent::JausMessageEvent:
            messageEvent = (JausMessageEvent *)e;
            // If you turn this on, the system gets spam-y this is very useful for debug purposes
            if(messageEvent->getJausMessage()->commandCode != JAUS_REPORT_HEARTBEAT_PULSE)
            {
                //printf("%s\n", messageEvent->toString().c_str());
            }
            else
            {
                //printf("%s\n", messageEvent->toString().c_str());
            }
            delete e;
            break;

                        case NodeManagerEvent::DebugEvent:
            debugEvent = (DebugEvent *)e;
            //printf("%s\n", debugEvent->toString().c_str());
            delete e;
            break;

                        case NodeManagerEvent::ConfigurationEvent:
            configEvent = (ConfigurationEvent *)e;
            printf("%s\n", configEvent->toString().c_str());
            delete e;
            break;

                        default:
            delete e;
            break;
        }
    }
};*/

/**
 * SAE AS-4 Nodemanager
 *
 **/
class AS4Protocol : public ProtocolInterface {
    Q_OBJECT

public:
    AS4Protocol();
    ~AS4Protocol();

    void run();
    QString getName();
    int getHeartbeatRate();

public slots:
    void receiveBytes(LinkInterface* link);
    /**
         * @brief Set the rate at which heartbeats are emitted
         *
         * The default rate is 1 Hertz.
         *
         * @param rate heartbeat rate in hertz (times per second)
         */
    void setHeartbeatRate(int rate);

    /**
         * @brief Send an extra heartbeat to all connected units
         *
         * The heartbeat is sent out of order and does not reset the
         * periodic heartbeat emission. It will be just sent in addition.
         */
//    void sendHeartbeat();

protected:
    QTimer* heartbeatTimer;
    int heartbeatRate;
    QMutex receiveMutex;
//    NodeManager* nodeManager;
//    MyHandler* handler;
//    FileLoader* configData;

signals:

};

#endif // AS4PROTOCOL_H_
