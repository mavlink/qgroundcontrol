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
    if(_tcpVideoSocket) {
        _tcpVideoSocket->close();
        _tcpVideoSocket->deleteLater();
    }
    if(_udpSocket) {
        _udpSocket->close();
        _udpSocket->deleteLater();
    }
}

//-----------------------------------------------------------------------------
void
TaisyncVideoReceiver::startVideo()
{
    qCDebug(TaisyncVideoReceiverLog) << "Start Taisync Video";
    _udpSocket = new QUdpSocket(this);
    _tcpVideoServer = new QTcpServer(this);
    QObject::connect(_tcpVideoServer, &QTcpServer::newConnection, this, &TaisyncVideoReceiver::_newVideoConnection);
    _tcpVideoServer->listen(QHostAddress::Any, TAISYNC_USB_VIDEO_PORT);
}

//-----------------------------------------------------------------------------
void
TaisyncVideoReceiver::_newVideoConnection()
{
    qCDebug(TaisyncVideoReceiverLog) << "New Taisync Connection";
    _tcpVideoSocket = _tcpVideoServer->nextPendingConnection();
    QObject::connect(_tcpVideoSocket, &QIODevice::readyRead, this, &TaisyncVideoReceiver::_readVideoBytes);
}

//-----------------------------------------------------------------------------
void
TaisyncVideoReceiver::_readVideoBytes()
{
    QByteArray bytesIn = _tcpVideoSocket->read(_tcpVideoSocket->bytesAvailable());
    _udpSocket->writeDatagram(bytesIn, QHostAddress::LocalHost, TAISYNC_USB_UDP_PORT);
  //qCDebug(TaisyncVideoReceiverLog) << "Taisync video data:" << bytesIn.size();
}
