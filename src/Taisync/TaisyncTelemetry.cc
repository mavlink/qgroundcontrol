/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TaisyncTelemetry.h"
#include "SettingsManager.h"
#include "QGCApplication.h"
#include "VideoManager.h"


QGC_LOGGING_CATEGORY(TaisyncTelemetryLog, "TaisyncTelemetryLog")

//-----------------------------------------------------------------------------
TaisyncTelemetry::TaisyncTelemetry(QObject* parent)
    : TaisyncHandler(parent)
{
}

//-----------------------------------------------------------------------------
void
TaisyncTelemetry::close()
{
    TaisyncHandler::close();
    qCDebug(TaisyncTelemetryLog) << "Close Taisync Telemetry";
    if(_udpTelemetrySocket) {
        _udpTelemetrySocket->close();
        _udpTelemetrySocket->deleteLater();
        _udpTelemetrySocket = nullptr;
    }
}

//-----------------------------------------------------------------------------
void
TaisyncTelemetry::startTelemetry()
{
    qCDebug(TaisyncTelemetryLog) << "Start Taisync Telemetry";
    _udpTelemetrySocket = new QUdpSocket(this);
    _udpTelemetrySocket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption,    64 * 1024);
    _udpTelemetrySocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 64 * 1024);
    QObject::connect(_udpTelemetrySocket, &QUdpSocket::readyRead, this, &TaisyncTelemetry::_readUDPBytes);
    _udpTelemetrySocket->bind(QHostAddress::LocalHost, 0, QUdpSocket::ShareAddress);
    _start(TAISYNC_TELEM_PORT);
    _heartbeatTimer.setInterval(1000);
    _heartbeatTimer.setSingleShot(false);
    connect(&_heartbeatTimer, &QTimer::timeout, this, &TaisyncTelemetry::_sendGCSHeartbeat);
    _heartbeatTimer.start();
}

//-----------------------------------------------------------------------------
void
TaisyncTelemetry::_newConnection()
{
    TaisyncHandler::_newConnection();
    qCDebug(TaisyncTelemetryLog) << "New Taisync Temeletry Connection";
    _heartbeatTimer.start();
}

//-----------------------------------------------------------------------------
void
TaisyncTelemetry::_readBytes()
{
    QByteArray bytesIn = _tcpSocket->read(_tcpSocket->bytesAvailable());
    _udpTelemetrySocket->writeDatagram(bytesIn, QHostAddress::LocalHost, 14550);
    qCDebug(TaisyncTelemetryLog) << "Taisync telemetry data:" << bytesIn.size();
    _heartbeatTimer.stop();
}

//-----------------------------------------------------------------------------
void
TaisyncTelemetry::_readUDPBytes()
{
    if (!_udpTelemetrySocket) {
        return;
    }
    qCDebug(TaisyncTelemetryLog) << "UDP from QGC";
    while (_udpTelemetrySocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<int>(_udpTelemetrySocket->pendingDatagramSize()));
        _udpTelemetrySocket->readDatagram(datagram.data(), datagram.size());
        if(_tcpSocket) {
            _tcpSocket->write(datagram);
        }
    }
}

//-----------------------------------------------------------------------------
void
TaisyncTelemetry::_sendGCSHeartbeat(void)
{
    //-- TODO: This is temporary. We should not have to send out heartbeats for a link to start.
    if(_tcpSocket) {
        qCDebug(TaisyncTelemetryLog) << "Taisync heartbeat out";
        MAVLinkProtocol* pMavlinkProtocol = qgcApp()->toolbox()->mavlinkProtocol();
        mavlink_message_t message;
        mavlink_msg_heartbeat_pack_chan(
            static_cast<uint8_t>(pMavlinkProtocol->getSystemId()),
            static_cast<uint8_t>(pMavlinkProtocol->getComponentId()),
            30,                      // TODO: Need to use a properly allocated channel
            &message,
            MAV_TYPE_GCS,            // MAV_TYPE
            MAV_AUTOPILOT_INVALID,   // MAV_AUTOPILOT
            MAV_MODE_MANUAL_ARMED,   // MAV_MODE
            0,                       // custom mode
            MAV_STATE_ACTIVE);       // MAV_STATE
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        int len = mavlink_msg_to_send_buffer(buffer, &message);
        _tcpSocket->write(reinterpret_cast<const char*>(buffer), len);
    }
}
