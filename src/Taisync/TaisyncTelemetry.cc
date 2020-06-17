/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
    return _start(TAISYNC_TELEM_PORT);
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
    while(_tcpSocket->bytesAvailable()) {
        QByteArray bytesIn = _tcpSocket->read(_tcpSocket->bytesAvailable());
        emit bytesReady(bytesIn);
    }
}
