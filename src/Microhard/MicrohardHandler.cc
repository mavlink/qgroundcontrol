/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
bool
MicrohardHandler::close()
{
    bool res = false;
    if(_tcpSocket) {
        qCDebug(MicrohardLog) << "Close Microhard TCP socket on port" << _tcpSocket->localPort();
        _tcpSocket->close();
        _tcpSocket->deleteLater();
        _tcpSocket = nullptr;
        res = true;
    }
    return res;
}

//-----------------------------------------------------------------------------
void
MicrohardHandler::_start(uint16_t port, QHostAddress addr)
{
    close();
    _tcpSocket = new QTcpSocket();
    QObject::connect(_tcpSocket, &QIODevice::readyRead, this, &MicrohardHandler::_readBytes);
    qCDebug(MicrohardLog) << "Connecting to" << addr;
    _tcpSocket->connectToHost(addr, port);
    QTimer::singleShot(1000, this, &MicrohardHandler::_testConnection);
}

//-----------------------------------------------------------------------------
void
MicrohardHandler::_testConnection()
{
    if(_tcpSocket) {
        if(_tcpSocket->state() == QAbstractSocket::ConnectedState) {
            qCDebug(MicrohardLog) << "Connected";
            return;
        }
        emit connected(0);
        close();
    }
}
