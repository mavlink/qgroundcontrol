/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MicrohardHandler.h"
#include "SettingsManager.h"
#include "QGCApplication.h"
#include "VideoManager.h"


QGC_LOGGING_CATEGORY(MicrohardLog,     "MicrohardLog")
QGC_LOGGING_CATEGORY(MicrohardVerbose, "MicrohardVerbose")

//-----------------------------------------------------------------------------
MicrohardHandler::MicrohardHandler(QObject* parent)
    : QObject (parent)
{
}

//-----------------------------------------------------------------------------
MicrohardHandler::~MicrohardHandler()
{
    close();
}

//-----------------------------------------------------------------------------
bool MicrohardHandler::close()
{
    bool res = (_tcpSocket || _tcpServer);
    if(_tcpSocket) {
        qCDebug(MicrohardLog) << "Close Microhard TCP socket on port" << _tcpSocket->localPort();
        _tcpSocket->close();
        _tcpSocket->deleteLater();
        _tcpSocket = nullptr;
    }
    if(_tcpServer) {
        qCDebug(MicrohardLog) << "Close Microhard TCP server on port" << _tcpServer->serverPort();;
        _tcpServer->close();
        _tcpServer->deleteLater();
        _tcpServer = nullptr;
    }
    return res;
}

//-----------------------------------------------------------------------------
bool
MicrohardHandler::_start(uint16_t port, QHostAddress addr)
{
    close();
    _serverMode = addr == QHostAddress::AnyIPv4;
    if(_serverMode) {
        if(!_tcpServer) {
            qCDebug(MicrohardLog) << "Listen for Microhard TCP on port" << port;
            _tcpServer = new QTcpServer(this);
            QObject::connect(_tcpServer, &QTcpServer::newConnection, this, &MicrohardHandler::_newConnection);
            _tcpServer->listen(QHostAddress::AnyIPv4, port);
        }
    } else {
        _tcpSocket = new QTcpSocket();
        QObject::connect(_tcpSocket, &QIODevice::readyRead, this, &MicrohardHandler::_readBytes);
        qCDebug(MicrohardLog) << "Connecting to" << addr;
        _tcpSocket->connectToHost(addr, port);
        if (!_tcpSocket->waitForConnected(1000)) {
            close();
            return false;
        }
        emit connected();
    }
    return true;
}

//-----------------------------------------------------------------------------
void
MicrohardHandler::_newConnection()
{
    qCDebug(MicrohardLog) << "New Microhard TCP Connection on port" << _tcpServer->serverPort();
    if(_tcpSocket) {
        _tcpSocket->close();
        _tcpSocket->deleteLater();
    }
    _tcpSocket = _tcpServer->nextPendingConnection();
    if(_tcpSocket) {
        QObject::connect(_tcpSocket, &QIODevice::readyRead, this, &MicrohardHandler::_readBytes);
        QObject::connect(_tcpSocket, &QAbstractSocket::disconnected, this, &MicrohardHandler::_socketDisconnected);
        emit connected();
    } else {
        qCWarning(MicrohardLog) << "New Microhard TCP Connection provided no socket";
    }
}

//-----------------------------------------------------------------------------
void
MicrohardHandler::_socketDisconnected()
{
    qCDebug(MicrohardLog) << "Microhard TCP Connection Closed on port" << _tcpSocket->localPort();
    if(_tcpSocket) {
        _tcpSocket->close();
        _tcpSocket->deleteLater();
        _tcpSocket = nullptr;
    }
    emit disconnected();
}
