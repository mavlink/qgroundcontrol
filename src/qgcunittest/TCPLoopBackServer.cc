/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "TCPLoopBackServer.h"

/// @file
///     @brief Simple TCP loop back server
///
///     @author Don Gagne <don@thegagnes.com>

TCPLoopBackServer::TCPLoopBackServer(QHostAddress hostAddress, quint16 port) :
    _hostAddress(hostAddress),
    _port(port),
    _tcpSocket(nullptr)
{
    moveToThread(this);
    start(HighPriority);
}
    
void TCPLoopBackServer::run(void)
{
    // Start the server side
    _tcpServer = new QTcpServer(this);
    Q_CHECK_PTR(_tcpServer);

    bool connected = QObject::connect(_tcpServer, &QTcpServer::newConnection, this, &TCPLoopBackServer::_newConnection);
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
    bool connected = QObject::connect(_tcpSocket, &QIODevice::readyRead, this, &TCPLoopBackServer::_readBytes);
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
