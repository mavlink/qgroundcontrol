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
    : TaisyncHandler(parent)
{
}

//-----------------------------------------------------------------------------
void
TaisyncVideoReceiver::close()
{
    TaisyncHandler::close();
    qCDebug(TaisyncVideoReceiverLog) << "Close Taisync Video Receiver";
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
    _start(TAISYNC_VIDEO_TCP_PORT);
}

//-----------------------------------------------------------------------------
void
TaisyncVideoReceiver::_readBytes()
{
    QByteArray bytesIn = _tcpSocket->read(_tcpSocket->bytesAvailable());
    _udpVideoSocket->writeDatagram(bytesIn, QHostAddress::LocalHost, TAISYNC_VIDEO_UDP_PORT);
}
