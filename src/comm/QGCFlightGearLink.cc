/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009 - 2011 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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
 *   @brief Definition of UDP connection (server) for unmanned vehicles
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QTimer>
#include <QList>
#include <QDebug>
#include <QMutexLocker>
#include <iostream>
#include "QGCFlightGearLink.h"
#include "QGC.h"
#include <QHostInfo>

QGCFlightGearLink::QGCFlightGearLink(QString remoteHost, QHostAddress host, quint16 port)
{
    this->host = host;
    this->port = port;
    this->connectState = false;
    this->currentPort = 49000;

    // Set unique ID and add link to the list of links
    this->name = tr("FlightGear Link (port:%1)").arg(port);
    setRemoteHost(remoteHost);
    connect(&refreshTimer, SIGNAL(timeout()), this, SLOT(sendUAVUpdate()));
    refreshTimer.start(20); // 50 Hz UAV -> Simulation update rate
}

QGCFlightGearLink::~QGCFlightGearLink()
{
    disconnect();
}

/**
 * @brief Runs the thread
 *
 **/
void QGCFlightGearLink::run()
{
//    forever
//    {
//        QGC::SLEEP::msleep(5000);
//    }
    exec();
}

void QGCFlightGearLink::setPort(int port)
{
    this->port = port;
    disconnectSimulation();
    connectSimulation();
}

/**
 * @param host Hostname in standard formatting, e.g. localhost:14551 or 192.168.1.1:14551
 */
void QGCFlightGearLink::setRemoteHost(const QString& host)
{
    //qDebug() << "UDP:" << "ADDING HOST:" << host;
    if (host.contains(":"))
    {
        //qDebug() << "HOST: " << host.split(":").first();
        QHostInfo info = QHostInfo::fromName(host.split(":").first());
        if (info.error() == QHostInfo::NoError)
        {
            // Add host
            QList<QHostAddress> hostAddresses = info.addresses();
            QHostAddress address;
            for (int i = 0; i < hostAddresses.size(); i++)
            {
                // Exclude loopback IPv4 and all IPv6 addresses
                if (!hostAddresses.at(i).toString().contains(":"))
                {
                    address = hostAddresses.at(i);
                }
            }
            currentHost = address;
            //qDebug() << "Address:" << address.toString();
            // Set port according to user input
            currentPort = host.split(":").last().toInt();
        }
    }
    else
    {
        QHostInfo info = QHostInfo::fromName(host);
        if (info.error() == QHostInfo::NoError)
        {
            // Add host
            currentHost = info.addresses().first();
        }
    }
}

void QGCFlightGearLink::updateGlobalPosition(quint64 time, double lat, double lon, double alt)
{

}

void QGCFlightGearLink::sendUAVUpdate()
{
    // 37.613548,-122.357246,-9999.000000,0.000000,0.424000,297.899994,0.000000\n
    // magnetos,aileron,elevator,rudder,throttle\n

    float magnetos = 3.0f;
    float aileron = 0.0f;
    float elevator = 0.0f;
    float rudder = 0.0f;
    float throttle = 90.0f;

    QString state("%1,%2,%3,%4,%5\n");
    state = state.arg(magnetos).arg(aileron).arg(elevator).arg(rudder).arg(throttle);
    writeBytes(state.toAscii().constData(), state.length());
}

void QGCFlightGearLink::writeBytes(const char* data, qint64 size)
{
//#define QGCFlightGearLink_DEBUG
#ifdef QGCFlightGearLink_DEBUG
        QString bytes;
        QString ascii;
        for (int i=0; i<size; i++)
        {
            unsigned char v = data[i];
            bytes.append(QString().sprintf("%02x ", v));
            if (data[i] > 31 && data[i] < 127)
            {
                ascii.append(data[i]);
            }
            else
            {
                ascii.append(219);
            }
        }
        qDebug() << "Sent" << size << "bytes to" << currentHost.toString() << ":" << currentPort << "data:";
        qDebug() << bytes;
        qDebug() << "ASCII:" << ascii;
#endif
        socket->writeDatagram(data, size, currentHost, currentPort);
}

/**
 * @brief Read a number of bytes from the interface.
 *
 * @param data Pointer to the data byte array to write the bytes to
 * @param maxLength The maximum number of bytes to write
 **/
void QGCFlightGearLink::readBytes()
{
    const qint64 maxLength = 65536;
    static char data[maxLength];
    QHostAddress sender;
    quint16 senderPort;

    unsigned int s = socket->pendingDatagramSize();
    if (s > maxLength) std::cerr << __FILE__ << __LINE__ << " UDP datagram overflow, allowed to read less bytes than datagram size" << std::endl;
    socket->readDatagram(data, maxLength, &sender, &senderPort);

    // FIXME TODO Check if this method is better than retrieving the data by individual processes
    QByteArray b(data, s);
    //emit bytesReceived(this, b);
	
	// Print string
	qDebug() << "FG LINK GOT:" << QString(b);

//    // Echo data for debugging purposes
//    std::cerr << __FILE__ << __LINE__ << "Received datagram:" << std::endl;
//    int i;
//    for (i=0; i<s; i++)
//    {
//        unsigned int v=data[i];
//        fprintf(stderr,"%02x ", v);
//    }
//    std::cerr << std::endl;
}


/**
 * @brief Get the number of bytes to read.
 *
 * @return The number of bytes to read
 **/
qint64 QGCFlightGearLink::bytesAvailable()
{
    return socket->pendingDatagramSize();
}

/**
 * @brief Disconnect the connection.
 *
 * @return True if connection has been disconnected, false if connection couldn't be disconnected.
 **/
bool QGCFlightGearLink::disconnectSimulation()
{
    delete socket;
    socket = NULL;

    connectState = false;

    emit flightGearDisconnected();
    emit flightGearConnected(false);
    return !connectState;
}

/**
 * @brief Connect the connection.
 *
 * @return True if connection has been established, false if connection couldn't be established.
 **/
bool QGCFlightGearLink::connectSimulation()
{
    socket = new QUdpSocket(this);

    //Check if we are using a multicast-address
//    bool multicast = false;
//    if (host.isInSubnet(QHostAddress("224.0.0.0"),4))
//    {
//        multicast = true;
//        connectState = socket->bind(port, QUdpSocket::ShareAddress);
//    }
//    else
//    {
    connectState = socket->bind(host, port);
//    }

    //Provides Multicast functionality to UdpSocket
    /* not working yet
    if (multicast)
    {
        int sendingFd = socket->socketDescriptor();

        if (sendingFd != -1)
        {
            // set up destination address
            struct sockaddr_in sendAddr;
            memset(&sendAddr,0,sizeof(sendAddr));
            sendAddr.sin_family=AF_INET;
            sendAddr.sin_addr.s_addr=inet_addr(HELLO_GROUP);
            sendAddr.sin_port=htons(port);

            // set TTL
            unsigned int ttl = 1; // restricted to the same subnet
            if (setsockopt(sendingFd, IPPROTO_IP, IP_MULTICAST_TTL, (unsigned int*)&ttl, sizeof(ttl) ) < 0)
            {
                std::cout << "TTL failed\n";
            }
        }
    }
    */

    //QObject::connect(socket, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));
    QObject::connect(socket, SIGNAL(readyRead()), this, SLOT(readBytes()));

    emit flightGearConnected(connectState);
    if (connectState) {
        emit flightGearConnected();
        connectionStartTime = QGC::groundTimeUsecs()/1000;
    }

    start(HighPriority);
    return connectState;
}

/**
 * @brief Check if connection is active.
 *
 * @return True if link is connected, false otherwise.
 **/
bool QGCFlightGearLink::isConnected()
{
    return connectState;
}

QString QGCFlightGearLink::getName()
{
    return name;
}

void QGCFlightGearLink::setName(QString name)
{
    this->name = name;
//    emit nameChanged(this->name);
}
