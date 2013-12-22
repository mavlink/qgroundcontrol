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

TCPLink::TCPLink(QHostAddress hostAddress, quint16 socketPort) :
    host(hostAddress),
    port(socketPort),
    socket(NULL),
    socketIsConnected(false)

{
    // Set unique ID and add link to the list of links
    this->id = getNextLinkId();
	this->name = tr("TCP Link (port:%1)").arg(this->port);
	emit nameChanged(this->name);
    
    qDebug() << "TCP Created " << this->name;
}

TCPLink::~TCPLink()
{
    disconnect();
	this->deleteLater();
}

void TCPLink::run()
{
	exec();
}

void TCPLink::setAddress(const QString &text)
{
    setAddress(QHostAddress(text));
}

void TCPLink::setAddress(QHostAddress host)
{
    bool reconnect(false);
	if (this->isConnected())
	{
		disconnect();
		reconnect = true;
	}
	this->host = host;
	if (reconnect)
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

#ifdef TCPLINK_READWRITE_DEBUG
void TCPLink::writeDebugBytes(const char *data, qint16 size)
{
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
    qDebug() << "Sent" << size << "bytes to" << host.toString() << ":" << port << "data:";
    qDebug() << bytes;
    qDebug() << "ASCII:" << ascii;
}
#endif

void TCPLink::writeBytes(const char* data, qint64 size)
{
#ifdef TCPLINK_READWRITE_DEBUG
    writeDebugBytes(data, size);
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

#ifdef TCPLINK_READWRITE_DEBUG
        writeDebugBytes(buffer.data(), buffer.size());
#endif
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
        socket->disconnect();
        socketIsConnected = false;
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
    QObject::connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));
    
    // Give the socket a second to connect to the other side otherwise error out
    if (!socket->waitForConnected(1000))
    {
        emit communicationError(getName(), "connection failed");
        return false;
    }
    
    socketIsConnected = true;
    emit connected(true);

    return true;
}

void TCPLink::socketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    emit communicationError(getName(), "Error on socket: " + socket->errorString());
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


qint64 TCPLink::getConnectionSpeed() const
{
    return 54000000; // 54 Mbit
}

qint64 TCPLink::getCurrentInDataRate() const
{
    return 0;
}

qint64 TCPLink::getCurrentOutDataRate() const
{
    return 0;
}
