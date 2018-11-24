/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCLoggingCategory.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>

#define TAISYNC_USB_TELEM_PORT  8400

Q_DECLARE_LOGGING_CATEGORY(TaisyncTelemetryLog)

class TaisyncTelemetry : public QObject
{
    Q_OBJECT
public:

    explicit TaisyncTelemetry           (QObject* parent = nullptr);
    ~TaisyncTelemetry                   ();
    void startTelemetry                 ();
    void close                          ();

private slots:
    void    _newTelemetryConnection     ();
    void    _readTelemetryBytes         ();
    void    _telemetrySocketDisconnected();
    void    _readBytes                  ();

private:
    QTcpServer*     _tcpTelemetryServer = nullptr;
    QTcpSocket*     _tcpTelemetrySocket = nullptr;
    QUdpSocket*     _udpTelemetrySocket = nullptr;
};
