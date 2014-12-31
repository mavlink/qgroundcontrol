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
#include "UDPLink.h"
#include "LinkManager.h"
#include "QGC.h"
#include <QHostInfo>
//#include <netinet/in.h>

UDPLink::UDPLink(QHostAddress host, quint16 port) :
    socket(NULL)
{
    // We're doing it wrong - because the Qt folks got the API wrong:
    // http://blog.qt.digia.com/blog/2010/06/17/youre-doing-it-wrong/
    moveToThread(this);

    this->host = host;
    this->port = port;
    this->connectState = false;
    // Set unique ID and add link to the list of links
    this->id = getNextLinkId();
	this->name = tr("UDP Link (port:%1)").arg(this->port);
	emit nameChanged(this->name);
    // LinkManager::instance()->add(this);
    qDebug() << "UDP Created " << name;
}

UDPLink::~UDPLink()
{
    _disconnect();

    // Tell the thread to exit
    quit();
    // Wait for it to exit
    wait();

	this->deleteLater();
}

/**
 * @brief Runs the thread
 *
 **/
void UDPLink::run()
{
    hardwareConnect();

    exec();
}

void UDPLink::setAddress(QHostAddress host)
{
    bool reconnect(false);
	if(this->isConnected())
	{
		_disconnect();
		reconnect = true;
	}
	this->host = host;
	if(reconnect)
	{
		_connect();
	}
}

void UDPLink::setPort(int port)
{
	bool reconnect(false);
	if(this->isConnected())
	{
		_disconnect();
		reconnect = true;
	}
    this->port = port;
	this->name = tr("UDP Link (port:%1)").arg(this->port);
	emit nameChanged(this->name);
	if(reconnect)
	{
		_connect();
	}
}

/**
 * @param host Hostname in standard formatting, e.g. localhost:14551 or 192.168.1.1:14551
 */
void UDPLink::addHost(const QString& host)
{
    qDebug() << "UDP:" << "ADDING HOST:" << host;
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
            hosts.append(address);
            //qDebug() << "Address:" << address.toString();
            // Set port according to user input
            ports.append(host.split(":").last().toInt());
        }
    }
    else
    {
        QHostInfo info = QHostInfo::fromName(host);
        if (info.error() == QHostInfo::NoError)
        {
            // Add host
            hosts.append(info.addresses().first());
            // Set port according to default (this port)
            ports.append(port);
        }
    }
}

void UDPLink::removeHost(const QString& hostname)
{
    QString host = hostname;
    if (host.contains(":")) host = host.split(":").first();
    host = host.trimmed();
    QHostInfo info = QHostInfo::fromName(host);
    QHostAddress address;
    QList<QHostAddress> hostAddresses = info.addresses();
    for (int i = 0; i < hostAddresses.size(); i++)
    {
        // Exclude loopback IPv4 and all IPv6 addresses
        if (!hostAddresses.at(i).toString().contains(":"))
        {
            address = hostAddresses.at(i);
        }
    }
    for (int i = 0; i < hosts.count(); ++i)
    {
        if (hosts.at(i) == address)
        {
            hosts.removeAt(i);
            ports.removeAt(i);
        }
    }
}

void UDPLink::writeBytes(const char* data, qint64 size)
{
    // Broadcast to all connected systems
    for (int h = 0; h < hosts.size(); h++)
    {
        QHostAddress currentHost = hosts.at(h);
        quint16 currentPort = ports.at(h);
//#define UDPLINK_DEBUG
#ifdef UDPLINK_DEBUG
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

        // Log the amount and time written out for future data rate calculations.
        QMutexLocker dataRateLocker(&dataRateMutex);
        logDataRateToBuffer(outDataWriteAmounts, outDataWriteTimes, &outDataIndex, size, QDateTime::currentMSecsSinceEpoch());
    }
}

/**
 * @brief Read a number of bytes from the interface.
 *
 * @param data Pointer to the data byte array to write the bytes to
 * @param maxLength The maximum number of bytes to write
 **/
void UDPLink::readBytes()
{
    while (socket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(socket->pendingDatagramSize());

        QHostAddress sender;
        quint16 senderPort;
        socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        // FIXME TODO Check if this method is better than retrieving the data by individual processes
        emit bytesReceived(this, datagram);

        // Log this data reception for this timestep
        QMutexLocker dataRateLocker(&dataRateMutex);
        logDataRateToBuffer(inDataWriteAmounts, inDataWriteTimes, &inDataIndex, datagram.length(), QDateTime::currentMSecsSinceEpoch());


//        // Echo data for debugging purposes
//        std::cerr << __FILE__ << __LINE__ << "Received datagram:" << std::endl;
//        int i;
//        for (i=0; i<s; i++)
//        {
//            unsigned int v=data[i];
//            fprintf(stderr,"%02x ", v);
//        }
//        std::cerr << std::endl;


        // Add host to broadcast list if not yet present
        if (!hosts.contains(sender))
        {
            hosts.append(sender);
            ports.append(senderPort);
            //        ports->insert(sender, senderPort);
        }
        else
        {
            int index = hosts.indexOf(sender);
            ports.replace(index, senderPort);
        }
    }
}

/**
 * @brief Disconnect the connection.
 *
 * @return True if connection has been disconnected, false if connection couldn't be disconnected.
 **/
bool UDPLink::_disconnect(void)
{
	this->quit();
	this->wait();

    if (socket) {
		// Make sure delete happen on correct thread
		socket->deleteLater();
		socket = NULL;
        emit disconnected();
	}

    connectState = false;

    return !connectState;
}

/**
 * @brief Connect the connection.
 *
 * @return True if connection has been established, false if connection couldn't be established.
 **/
bool UDPLink::_connect(void)
{
	if(this->isRunning())
	{
		this->quit();
		this->wait();
	}
    bool connected = true;
    start(HighPriority);
    return connected;
}

bool UDPLink::hardwareConnect(void)
{
	socket = new QUdpSocket();

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

    if (connectState) {
        emit connected();
    }
	return connectState;
}


/**
 * @brief Check if connection is active.
 *
 * @return True if link is connected, false otherwise.
 **/
bool UDPLink::isConnected() const
{
    return connectState;
}

int UDPLink::getId() const
{
    return id;
}

QString UDPLink::getName() const
{
    return name;
}

void UDPLink::setName(QString name)
{
    this->name = name;
    emit nameChanged(this->name);
}


qint64 UDPLink::getConnectionSpeed() const
{
    return 54000000; // 54 Mbit
}

qint64 UDPLink::getCurrentInDataRate() const
{
    return 0;
}

qint64 UDPLink::getCurrentOutDataRate() const
{
    return 0;
}
