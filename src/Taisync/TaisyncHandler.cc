/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TaisyncHandler.h"
#include "SettingsManager.h"
#include "QGCApplication.h"
#include "VideoManager.h"


QGC_LOGGING_CATEGORY(TaisyncLog,     "TaisyncLog")
QGC_LOGGING_CATEGORY(TaisyncVerbose, "TaisyncVerbose")

//-----------------------------------------------------------------------------
TaisyncHandler::TaisyncHandler(QObject* parent)
    : QObject (parent)
{
}

//-----------------------------------------------------------------------------
TaisyncHandler::~TaisyncHandler()
{
    close();
}

//-----------------------------------------------------------------------------
bool TaisyncHandler::close()
{
    bool res = (_tcpSocket || _tcpServer);
    if(_tcpSocket) {
        qCDebug(TaisyncLog) << "Close Taisync TCP socket on port" << _tcpSocket->localPort();
        _tcpSocket->close();
        _tcpSocket->deleteLater();
        _tcpSocket = nullptr;
    }
    if(_tcpServer) {
        qCDebug(TaisyncLog) << "Close Taisync TCP server on port" << _tcpServer->serverPort();;
        _tcpServer->close();
        _tcpServer->deleteLater();
        _tcpServer = nullptr;
    }
    return res;
}

//-----------------------------------------------------------------------------
bool
TaisyncHandler::_start(uint16_t port, QHostAddress addr)
{
    close();
    _serverMode = addr == QHostAddress::AnyIPv4;
    if(_serverMode) {
        if(!_tcpServer) {
            qCDebug(TaisyncLog) << "Listen for Taisync TCP on port" << port;
            _tcpServer = new QTcpServer(this);
            QObject::connect(_tcpServer, &QTcpServer::newConnection, this, &TaisyncHandler::_newConnection);
            _tcpServer->listen(QHostAddress::AnyIPv4, port);
        }
    } else {
        _tcpSocket = new QTcpSocket();
        QObject::connect(_tcpSocket, &QIODevice::readyRead, this, &TaisyncHandler::_readBytes);
        qCDebug(TaisyncLog) << "Connecting to" << addr;
        _tcpSocket->connectToHost(addr, port);
        //-- TODO: This has to be removed. It's blocking the main thread.
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
TaisyncHandler::_newConnection()
{
    qCDebug(TaisyncLog) << "New Taisync TCP Connection on port" << _tcpServer->serverPort();
    if(_tcpSocket) {
        _tcpSocket->close();
        _tcpSocket->deleteLater();
    }
    _tcpSocket = _tcpServer->nextPendingConnection();
    if(_tcpSocket) {
        QObject::connect(_tcpSocket, &QIODevice::readyRead, this, &TaisyncHandler::_readBytes);
        QObject::connect(_tcpSocket, &QAbstractSocket::disconnected, this, &TaisyncHandler::_socketDisconnected);
        emit connected();
    } else {
        qCWarning(TaisyncLog) << "New Taisync TCP Connection provided no socket";
    }
}

//-----------------------------------------------------------------------------
void
TaisyncHandler::_socketDisconnected()
{
    qCDebug(TaisyncLog) << "Taisync TCP Connection Closed on port" << _tcpSocket->localPort();
    if(_tcpSocket) {
        _tcpSocket->close();
        _tcpSocket->deleteLater();
        _tcpSocket = nullptr;
        emit disconnected();
    }
}
