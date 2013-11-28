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
 *   @brief Definition of TCP connection (server) for unmanned vehicles
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QTimer>
#include <QList>
#include <QDebug>
#include <QMutexLocker>
#include <iostream>
#include "TCPLink.h"
#include "LinkManager.h"
#include "QGC.h"
#include <QHostInfo>
//#include <netinet/in.h>

TCPLink::TCPLink(QHostAddress host, quint16 port)
: socket(NULL)
{
    // FIXFIX: host and port and hard-wired
    this->host = "127.0.0.1";
    this->port = 5760;
    
    this->socketIsConnected = false;
    
    // Set unique ID and add link to the list of links
    this->id = getNextLinkId();
    // FIXFIX: What about host name?
	this->name = tr("TCP Link (port:%1)").arg(this->port);
	emit nameChanged(this->name);
    
    qDebug() << "TCP Created " << name;
}

TCPLink::~TCPLink()
{
    disconnect();
	this->deleteLater();
}

/**
 * @brief Runs the thread
 *
 **/
void TCPLink::run()
{
	exec();
}

void TCPLink::setAddress(QHostAddress host)
{
    bool reconnect(false);
	if(this->isConnected())
	{
		disconnect();
		reconnect = true;
	}
	this->host = host;
	if(reconnect)
	{
		connect();
	}
}

void TCPLink::setPort(int port)
{
	bool reconnect(false);
	if(this->isConnected())
	{
		disconnect();
		reconnect = true;
	}
    this->port = port;
	this->name = tr("TCP Link (port:%1)").arg(this->port);
	emit nameChanged(this->name);
	if(reconnect)
	{
		connect();
	}
}

void TCPLink::writeBytes(const char* data, qint64 size)
{
    //#define TCPLINK_DEBUG
#ifdef TCPLINK_DEBUG
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
    socket->write(data, size);
}

/**
 * @brief Read a number of bytes from the interface.
 *
 * @param data Pointer to the data byte array to write the bytes to
 * @param maxLength The maximum number of bytes to write
 **/
void TCPLink::readBytes()
{
    qint64 byteCount = socket->bytesAvailable();
    
    if (byteCount)
    {
        QByteArray buffer;
        buffer.resize(byteCount);
        
        socket->read(buffer.data(), buffer.size());
        
        emit bytesReceived(this, buffer);
        
        //        // Echo data for debugging purposes
        //        std::cerr << __FILE__ << __LINE__ << "Received datagram:" << std::endl;
        //        int i;
        //        for (i=0; i<s; i++)
        //        {
        //            unsigned int v=data[i];
        //            fprintf(stderr,"%02x ", v);
        //        }
        //        std::cerr << std::endl;
        
    }
}


/**
 * @brief Get the number of bytes to read.
 *
 * @return The number of bytes to read
 **/
qint64 TCPLink::bytesAvailable()
{
    return socket->bytesAvailable();
}

/**
 * @brief Disconnect the connection.
 *
 * @return True if connection has been disconnected, false if connection couldn't be disconnected.
 **/
bool TCPLink::disconnect()
{
	this->quit();
	this->wait();
    
    if (socket)
	{
        if (socketIsConnected) {
            socket->disconnect();
            socketIsConnected = false;
        }
		delete socket;
		socket = NULL;
	}
    
    return true;
}

/**
 * @brief Connect the connection.
 *
 * @return True if connection has been established, false if connection couldn't be established.
 **/
bool TCPLink::connect()
{
	if(this->isRunning())
	{
		this->quit();
		this->wait();
	}
    bool connected = this->hardwareConnect();
    start(HighPriority);
    return connected;
}

bool TCPLink::hardwareConnect(void)
{
	socket = new QTcpSocket();
    
    socket->connectToHost(host, port);
    
    QObject::connect(socket, SIGNAL(readyRead()), this, SLOT(readBytes()));
    //QObject::connect(socket, SIGNAL(connected()), this, SLOT(socketConnected()));
    socketIsConnected = true;
    
    connectionStartTime = QGC::groundTimeUsecs()/1000;
	
    return true;
}

void TCPLink::socketConnected()
{
    socketIsConnected = true;
}

/**
 * @brief Check if connection is active.
 *
 * @return True if link is connected, false otherwise.
 **/
bool TCPLink::isConnected() const
{
    return socketIsConnected;
}

int TCPLink::getId() const
{
    return id;
}

QString TCPLink::getName() const
{
    return name;
}

void TCPLink::setName(QString name)
{
    this->name = name;
    emit nameChanged(this->name);
}


qint64 TCPLink::getNominalDataRate() const
{
    return 54000000; // 54 Mbit
}

qint64 TCPLink::getTotalUpstream()
{
    statisticsMutex.lock();
    qint64 totalUpstream = bitsSentTotal / ((QGC::groundTimeUsecs()/1000 - connectionStartTime) / 1000);
    statisticsMutex.unlock();
    return totalUpstream;
}

qint64 TCPLink::getCurrentUpstream()
{
    return 0; // TODO
}

qint64 TCPLink::getMaxUpstream()
{
    return 0; // TODO
}

qint64 TCPLink::getBitsSent() const
{
    return bitsSentTotal;
}

qint64 TCPLink::getBitsReceived() const
{
    return bitsReceivedTotal;
}

qint64 TCPLink::getTotalDownstream()
{
    statisticsMutex.lock();
    qint64 totalDownstream = bitsReceivedTotal / ((QGC::groundTimeUsecs()/1000 - connectionStartTime) / 1000);
    statisticsMutex.unlock();
    return totalDownstream;
}

qint64 TCPLink::getCurrentDownstream()
{
    return 0; // TODO
}

qint64 TCPLink::getMaxDownstream()
{
    return 0; // TODO
}

bool TCPLink::isFullDuplex() const
{
    return true;
}

int TCPLink::getLinkQuality() const
{
    /* This feature is not supported with this interface */
    return -1;
}
