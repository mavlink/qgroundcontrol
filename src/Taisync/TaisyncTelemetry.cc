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
}

//-----------------------------------------------------------------------------
void
TaisyncTelemetry::start()
{
    qCDebug(TaisyncTelemetryLog) << "Start Taisync Telemetry";
    _start(TAISYNC_TELEM_PORT);
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
    qCDebug(TaisyncTelemetryLog) << "New Taisync Temeletry Connection";
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
