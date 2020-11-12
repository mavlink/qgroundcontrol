/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TaisyncVideoReceiver.h"
#include "SettingsManager.h"
#include "QGCApplication.h"
#include "VideoManager.h"

//-----------------------------------------------------------------------------
TaisyncVideoReceiver::TaisyncVideoReceiver(QObject* parent)
    : TaisyncHandler(parent)
{
}

//-----------------------------------------------------------------------------
bool
TaisyncVideoReceiver::close()
{
    if(TaisyncHandler::close() || _udpVideoSocket) {
        qCDebug(TaisyncLog) << "Close Taisync Video Receiver";
        if(_udpVideoSocket) {
            _udpVideoSocket->close();
            _udpVideoSocket->deleteLater();
            _udpVideoSocket = nullptr;
        }
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
bool
TaisyncVideoReceiver::start()
{
    qCDebug(TaisyncLog) << "Start Taisync Video Receiver";
    if(_start(TAISYNC_VIDEO_TCP_PORT)) {
        _udpVideoSocket = new QUdpSocket(this);
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
void
TaisyncVideoReceiver::_readBytes()
{
    if(_udpVideoSocket) {
        QByteArray bytesIn = _tcpSocket->read(_tcpSocket->bytesAvailable());
        _udpVideoSocket->writeDatagram(bytesIn, QHostAddress::LocalHost, TAISYNC_VIDEO_UDP_PORT);
    }
}
