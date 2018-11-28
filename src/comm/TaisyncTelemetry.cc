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
    : QObject (parent)
{
}

//-----------------------------------------------------------------------------
TaisyncTelemetry::~TaisyncTelemetry()
{
    close();
}

//-----------------------------------------------------------------------------
void
TaisyncTelemetry::close()
{
    qCDebug(TaisyncTelemetryLog) << "Close Taisync Telemetry";
    if(_tcpTelemetrySocket) {
        _tcpTelemetrySocket->close();
        _tcpTelemetrySocket->deleteLater();
        _tcpTelemetrySocket = nullptr;
    }
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
    QObject::connect(_udpTelemetrySocket, &QUdpSocket::readyRead, this, &TaisyncTelemetry::_readBytes);
    _udpTelemetrySocket->bind(QHostAddress::LocalHost, 0, QUdpSocket::ShareAddress);
    _tcpTelemetryServer = new QTcpServer(this);
    QObject::connect(_tcpTelemetryServer, &QTcpServer::newConnection, this, &TaisyncTelemetry::_newTelemetryConnection);
    _tcpTelemetryServer->listen(QHostAddress::AnyIPv4, TAISYNC_USB_TELEM_PORT);
    _heartbeatTimer.setInterval(1000);
    _heartbeatTimer.setSingleShot(false);
    connect(&_heartbeatTimer, &QTimer::timeout, this, &TaisyncTelemetry::_sendGCSHeartbeat);
    _heartbeatTimer.start();
}

//-----------------------------------------------------------------------------
void
TaisyncTelemetry::_newTelemetryConnection()
{
    qCDebug(TaisyncTelemetryLog) << "New Taisync Temeletry Connection";
    if(_tcpTelemetrySocket) {
        _tcpTelemetrySocket->close();
        _tcpTelemetrySocket->deleteLater();
    }
    _tcpTelemetrySocket = _tcpTelemetryServer->nextPendingConnection();
    QObject::connect(_tcpTelemetrySocket, &QIODevice::readyRead, this, &TaisyncTelemetry::_readTelemetryBytes);
    _heartbeatTimer.start();
}

//-----------------------------------------------------------------------------
void
TaisyncTelemetry::_telemetrySocketDisconnected()
{
    qCDebug(TaisyncTelemetryLog) << "Taisync Telemetry Connection Closed";
    if(_tcpTelemetrySocket) {
        _tcpTelemetrySocket->close();
        _tcpTelemetrySocket->deleteLater();
        _tcpTelemetrySocket = nullptr;
    }
}

//-----------------------------------------------------------------------------
void
TaisyncTelemetry::_readTelemetryBytes()
{
    QByteArray bytesIn = _tcpTelemetrySocket->read(_tcpTelemetrySocket->bytesAvailable());
    _udpTelemetrySocket->writeDatagram(bytesIn, QHostAddress::LocalHost, 14550);
    qCDebug(TaisyncTelemetryLog) << "Taisync telemetry data:" << bytesIn.size();
    _heartbeatTimer.stop();
}

//-----------------------------------------------------------------------------
void
TaisyncTelemetry::_readBytes()
{
    if (!_udpTelemetrySocket) {
        return;
    }
    qCDebug(TaisyncTelemetryLog) << "UDP from QGC";
    while (_udpTelemetrySocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<int>(_udpTelemetrySocket->pendingDatagramSize()));
        _udpTelemetrySocket->readDatagram(datagram.data(), datagram.size());
        if(_tcpTelemetrySocket) {
            _tcpTelemetrySocket->write(datagram);
        }
    }
}

//-----------------------------------------------------------------------------
void
TaisyncTelemetry::_sendGCSHeartbeat(void)
{
    if(_tcpTelemetrySocket) {
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
        _tcpTelemetrySocket->write(reinterpret_cast<const char*>(buffer), len);
    }
}
