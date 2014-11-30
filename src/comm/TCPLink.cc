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

#include <QTimer>
#include <QList>
#include <QDebug>
#include <QMutexLocker>
#include <iostream>
#include "TCPLink.h"
#include "LinkManager.h"
#include "QGC.h"
#include <QHostInfo>
#include <QSignalSpy>

/// @file
///     @brief TCP link type for SITL support
///
///     @author Don Gagne <don@thegagnes.com>

TCPLink::TCPLink(QHostAddress hostAddress, quint16 socketPort) :
    _hostAddress(hostAddress),
    _port(socketPort),
    _socket(NULL),
    _socketIsConnected(false)
{
    // We're doing it wrong - because the Qt folks got the API wrong:
    // http://blog.qt.digia.com/blog/2010/06/17/youre-doing-it-wrong/
    moveToThread(this);

    _linkId = getNextLinkId();
    _resetName();
    
    qDebug() << "TCP Created " << _name;
}

TCPLink::~TCPLink()
{
    _disconnect();

    // Tell the thread to exit
    quit();
    // Wait for it to exit
    wait();
}

void TCPLink::run()
{
    _hardwareConnect();

	exec();
}

void TCPLink::setHostAddress(QHostAddress hostAddress)
{
    bool reconnect = false;
    
	if (this->isConnected()) {
		_disconnect();
		reconnect = true;
	}
    
	_hostAddress = hostAddress;
    _resetName();
    
	if (reconnect) {
		_connect();
	}
}

void TCPLink::setHostAddress(const QString& hostAddress)
{
    setHostAddress(QHostAddress(hostAddress));
}

void TCPLink::setPort(int port)
{
    bool reconnect = false;
    
	if (this->isConnected()) {
		_disconnect();
		reconnect = true;
	}
    
	_port = port;
    _resetName();
    
	if (reconnect) {
		_connect();
	}
}

#ifdef TCPLINK_READWRITE_DEBUG
void TCPLink::_writeDebugBytes(const char *data, qint16 size)
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
    qDebug() << "Sent" << size << "bytes to" << _hostAddress.toString() << ":" << _port << "data:";
    qDebug() << bytes;
    qDebug() << "ASCII:" << ascii;
}
#endif

void TCPLink::writeBytes(const char* data, qint64 size)
{
#ifdef TCPLINK_READWRITE_DEBUG
    _writeDebugBytes(data, size);
#endif
    _socket->write(data, size);

    // Log the amount and time written out for future data rate calculations.
    QMutexLocker dataRateLocker(&dataRateMutex);
    logDataRateToBuffer(outDataWriteAmounts, outDataWriteTimes, &outDataIndex, size, QDateTime::currentMSecsSinceEpoch());
}

/**
 * @brief Read a number of bytes from the interface.
 *
 * @param data Pointer to the data byte array to write the bytes to
 * @param maxLength The maximum number of bytes to write
 **/
void TCPLink::readBytes()
{
    qint64 byteCount = _socket->bytesAvailable();
    
    if (byteCount)
    {
        QByteArray buffer;
        buffer.resize(byteCount);
        
        _socket->read(buffer.data(), buffer.size());
        
        emit bytesReceived(this, buffer);

        // Log the amount and time received for future data rate calculations.
        QMutexLocker dataRateLocker(&dataRateMutex);
        logDataRateToBuffer(inDataWriteAmounts, inDataWriteTimes, &inDataIndex, byteCount, QDateTime::currentMSecsSinceEpoch());

#ifdef TCPLINK_READWRITE_DEBUG
        writeDebugBytes(buffer.data(), buffer.size());
#endif
    }
}

/**
 * @brief Disconnect the connection.
 *
 * @return True if connection has been disconnected, false if connection couldn't be disconnected.
 **/
bool TCPLink::_disconnect(void)
{
	quit();
	wait();
    
    if (_socket)
	{
        _socketIsConnected = false;
		_socket->deleteLater(); // Make sure delete happens on correct thread
		_socket = NULL;

        emit disconnected();
        emit connected(false);
	}
    
    return true;
}

/**
 * @brief Connect the connection.
 *
 * @return True if connection has been established, false if connection couldn't be established.
 **/
bool TCPLink::_connect(void)
{
	if (isRunning())
	{
		quit();
		wait();
	}

    start(HighPriority);

    return true;
}

bool TCPLink::_hardwareConnect(void)
{
    Q_ASSERT(_socket == NULL);
	_socket = new QTcpSocket();
    
    QSignalSpy errorSpy(_socket, SIGNAL(error(QAbstractSocket::SocketError)));
    
    _socket->connectToHost(_hostAddress, _port);
    
    QObject::connect(_socket, SIGNAL(readyRead()), this, SLOT(readBytes()));
    QObject::connect(_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(_socketError(QAbstractSocket::SocketError)));
    
    // Give the socket a second to connect to the other side otherwise error out
    if (!_socket->waitForConnected(1000))
    {
        // Whether a failed connection emits an error signal or not is platform specific.
        // So in cases where it is not emitted, we emit one ourselves.
        if (errorSpy.count() == 0) {
            emit communicationError(getName(), "Connection failed");
        }
        delete _socket;
        _socket = NULL;
        return false;
    }
    
    _socketIsConnected = true;
    emit connected(true);
    emit connected();

    return true;
}

void TCPLink::_socketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    emit communicationError(getName(), "Error on socket: " + _socket->errorString());
}

/**
 * @brief Check if connection is active.
 *
 * @return True if link is connected, false otherwise.
 **/
bool TCPLink::isConnected() const
{
    return _socketIsConnected;
}

int TCPLink::getId() const
{
    return _linkId;
}

QString TCPLink::getName() const
{
    return _name;
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

void TCPLink::_resetName(void)
{
    _name = QString("TCP Link (host:%1 port:%2)").arg(_hostAddress.toString()).arg(_port);
    emit nameChanged(_name);
}

void TCPLink::waitForBytesWritten(int msecs)
{
    Q_ASSERT(_socket);
    _socket->waitForBytesWritten(msecs);
}

void TCPLink::waitForReadyRead(int msecs)
{
    Q_ASSERT(_socket);
    _socket->waitForReadyRead(msecs);
}
