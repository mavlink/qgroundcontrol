/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TaisyncVideoReceiver.h"
#include "SettingsManager.h"
#include "QGCApplication.h"
#include "VideoManager.h"


QGC_LOGGING_CATEGORY(TaisyncVideoReceiverLog, "TaisyncVideoReceiverLog")

//-----------------------------------------------------------------------------
TaisyncVideoReceiver::TaisyncVideoReceiver(QObject* parent)
    : QObject (parent)
{
}

//-----------------------------------------------------------------------------
TaisyncVideoReceiver::~TaisyncVideoReceiver()
{
    close();
}

//-----------------------------------------------------------------------------
void
TaisyncVideoReceiver::close()
{
    qCDebug(TaisyncVideoReceiverLog) << "Close Taisync Video Receiver";
    if(_tcpVideoSocket) {
        _tcpVideoSocket->close();
        _tcpVideoSocket->deleteLater();
        _tcpVideoSocket = nullptr;
    }
    if(_udpVideoSocket) {
        _udpVideoSocket->close();
        _udpVideoSocket->deleteLater();
        _udpVideoSocket = nullptr;
    }
}

//-----------------------------------------------------------------------------
void
TaisyncVideoReceiver::startVideo()
{
    qCDebug(TaisyncVideoReceiverLog) << "Start Taisync Video Receiver";
    _udpVideoSocket = new QUdpSocket(this);
    _tcpVideoServer = new QTcpServer(this);
    QObject::connect(_tcpVideoServer, &QTcpServer::newConnection, this, &TaisyncVideoReceiver::_newVideoConnection);
    _tcpVideoServer->listen(QHostAddress::AnyIPv4, TAISYNC_USB_VIDEO_PORT);
}

//-----------------------------------------------------------------------------
void
TaisyncVideoReceiver::_newVideoConnection()
{
    qCDebug(TaisyncVideoReceiverLog) << "New Taisync Video Connection";
    if(_tcpVideoSocket) {
        _tcpVideoSocket->close();
        _tcpVideoSocket->deleteLater();
    }
    _tcpVideoSocket = _tcpVideoServer->nextPendingConnection();
    QObject::connect(_tcpVideoSocket, &QIODevice::readyRead, this, &TaisyncVideoReceiver::_readVideoBytes);
    QObject::connect(_tcpVideoSocket, &QAbstractSocket::disconnected, this, &TaisyncVideoReceiver::_videoSocketDisconnected);
}

//-----------------------------------------------------------------------------
void
TaisyncVideoReceiver::_videoSocketDisconnected()
{
    qCDebug(TaisyncVideoReceiverLog) << "Taisync Video Connection Closed";
    if(_tcpVideoSocket) {
        _tcpVideoSocket->close();
        _tcpVideoSocket->deleteLater();
        _tcpVideoSocket = nullptr;
    }
}

//-----------------------------------------------------------------------------
void
TaisyncVideoReceiver::_readVideoBytes()
{
    QByteArray bytesIn = _tcpVideoSocket->read(_tcpVideoSocket->bytesAvailable());
    _udpVideoSocket->writeDatagram(bytesIn, QHostAddress::LocalHost, TAISYNC_USB_UDP_PORT);
  //qCDebug(TaisyncVideoReceiverLog) << "Taisync video data:" << bytesIn.size();
}
