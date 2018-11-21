/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TaisyncUSBHandler.h"
#include "SettingsManager.h"
#include "QGCApplication.h"
#include "VideoManager.h"

//-----------------------------------------------------------------------------
TaisyncUSBHandler::TaisyncUSBHandler(QObject* parent)
    : QObject (parent)
{
}

//-----------------------------------------------------------------------------
TaisyncUSBHandler::~TaisyncUSBHandler()
{
}

//-----------------------------------------------------------------------------
void
TaisyncUSBHandler::startVideo()
{
    _udpSocket = new QUdpSocket(this);
    _tcpVideoServer = new QTcpServer(this);
    QObject::connect(_tcpVideoServer, &QTcpServer::newConnection, this, &TaisyncUSBHandler::_newVideoConnection);
    _tcpVideoServer->listen(QHostAddress::Any, TAISYNC_USB_VIDEO_PORT);
}

//-----------------------------------------------------------------------------
void
TaisyncUSBHandler::startTelemetry()
{
    _tcpDataServer = new QTcpServer(this);
    QObject::connect(_tcpDataServer, &QTcpServer::newConnection, this, &TaisyncUSBHandler::_newDataConnection);
    _tcpDataServer->listen(QHostAddress::Any, TAISYNC_USB_DATA_PORT);
}

//-----------------------------------------------------------------------------
void
TaisyncUSBHandler::_newVideoConnection()
{
    _tcpVideoSocket = _tcpVideoServer->nextPendingConnection();
    QObject::connect(_tcpVideoSocket, &QIODevice::readyRead, this, &TaisyncUSBHandler::_readVideoBytes);
}

//-----------------------------------------------------------------------------
void
TaisyncUSBHandler::_readVideoBytes()
{
    QByteArray bytesIn = _tcpVideoSocket->read(_tcpVideoSocket->bytesAvailable());
    _udpSocket->writeDatagram(bytesIn, QHostAddress::LocalHost, TAISYNC_USB_UDP_PORT);
}

//-----------------------------------------------------------------------------
void
TaisyncUSBHandler::_newDataConnection()
{
    _tcpDataSocket = _tcpDataServer->nextPendingConnection();
    QObject::connect(_tcpDataSocket, &QIODevice::readyRead, this, &TaisyncUSBHandler::_readDataBytes);
}

//-----------------------------------------------------------------------------
void
TaisyncUSBHandler::_readDataBytes()
{
    QByteArray bytesIn = _tcpDataSocket->read(_tcpDataSocket->bytesAvailable());
}



