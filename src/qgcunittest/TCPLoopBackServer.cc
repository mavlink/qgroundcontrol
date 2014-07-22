/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#include "TCPLoopBackServer.h"

/// @file
///     @brief Simple TCP loop back server
///
///     @author Don Gagne <don@thegagnes.com>

TCPLoopBackServer::TCPLoopBackServer(QHostAddress hostAddress, quint16 port) :
    _hostAddress(hostAddress),
    _port(port),
    _tcpSocket(NULL)
{
    moveToThread(this);
    start(HighPriority);
}
    
void TCPLoopBackServer::run(void)
{
    // Start the server side
    _tcpServer = new QTcpServer(this);
    Q_CHECK_PTR(_tcpServer);

    bool connected = QObject::connect(_tcpServer, SIGNAL(newConnection()), this, SLOT(_newConnection()));
    Q_ASSERT(connected);
    Q_UNUSED(connected); // Fix initialized-but-not-referenced warning on release builds

    Q_ASSERT(_tcpServer->listen(_hostAddress, _port));

    // Fall into main event loop
    exec();
}

void TCPLoopBackServer::_newConnection(void)
{
    Q_ASSERT(_tcpServer);
    _tcpSocket = _tcpServer->nextPendingConnection();
    Q_ASSERT(_tcpSocket);
    bool connected = QObject::connect(_tcpSocket, SIGNAL(readyRead()), this, SLOT(_readBytes()));
    Q_ASSERT(connected);
    Q_UNUSED(connected); // Fix initialized-but-not-referenced warning on release builds
}

void TCPLoopBackServer::_readBytes(void)
{
    Q_ASSERT(_tcpSocket);
    QByteArray bytesIn = _tcpSocket->read(_tcpSocket->bytesAvailable());
    Q_ASSERT(_tcpSocket->write(bytesIn) == bytesIn.count());
    Q_ASSERT(_tcpSocket->waitForBytesWritten(1000));
}
