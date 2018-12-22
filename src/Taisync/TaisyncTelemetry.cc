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

//-----------------------------------------------------------------------------
TaisyncTelemetry::TaisyncTelemetry(QObject* parent)
    : TaisyncHandler(parent)
{
}

//-----------------------------------------------------------------------------
bool
TaisyncTelemetry::close()
{
    if(_mavlinkChannel >= 0) {
        qgcApp()->toolbox()->linkManager()->_freeMavlinkChannel(_mavlinkChannel);
        _mavlinkChannel = -1;
    }
    if(TaisyncHandler::close()) {
        qCDebug(TaisyncLog) << "Close Taisync Telemetry";
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
bool TaisyncTelemetry::start()
{
    qCDebug(TaisyncLog) << "Start Taisync Telemetry";
    if(_start(TAISYNC_TELEM_PORT)) {
        _mavlinkChannel = qgcApp()->toolbox()->linkManager()->_reserveMavlinkChannel();
        _heartbeatTimer.setInterval(1000);
        _heartbeatTimer.setSingleShot(false);
        connect(&_heartbeatTimer, &QTimer::timeout, this, &TaisyncTelemetry::_sendGCSHeartbeat);
        _heartbeatTimer.start();
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
void
TaisyncTelemetry::writeBytes(QByteArray bytes)
{
    if(_tcpSocket) {
        _tcpSocket->write(bytes);
    }
}

//-----------------------------------------------------------------------------
void
TaisyncTelemetry::_newConnection()
{
    TaisyncHandler::_newConnection();
    qCDebug(TaisyncLog) << "New Taisync Temeletry Connection";
}

//-----------------------------------------------------------------------------
void
TaisyncTelemetry::_readBytes()
{
    _heartbeatTimer.stop();
    while(_tcpSocket->bytesAvailable()) {
        QByteArray bytesIn = _tcpSocket->read(_tcpSocket->bytesAvailable());
        emit bytesReady(bytesIn);
    }
}

//-----------------------------------------------------------------------------
void
TaisyncTelemetry::_sendGCSHeartbeat()
{
    //-- TODO: This is temporary. We should not have to send out heartbeats for a link to start.
    if(_tcpSocket) {
        MAVLinkProtocol* pMavlinkProtocol = qgcApp()->toolbox()->mavlinkProtocol();
        if(pMavlinkProtocol && _mavlinkChannel >= 0) {
            qCDebug(TaisyncLog) << "Taisync heartbeat out";
            mavlink_message_t message;
            mavlink_msg_heartbeat_pack_chan(
                static_cast<uint8_t>(pMavlinkProtocol->getSystemId()),
                static_cast<uint8_t>(pMavlinkProtocol->getComponentId()),
                static_cast<uint8_t>(_mavlinkChannel),
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
}
