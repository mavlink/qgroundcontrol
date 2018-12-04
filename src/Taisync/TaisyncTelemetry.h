/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "TaisyncHandler.h"
#include <QUdpSocket>
#include <QTimer>

Q_DECLARE_LOGGING_CATEGORY(TaisyncTelemetryLog)

class TaisyncTelemetry : public TaisyncHandler
{
    Q_OBJECT
public:

    explicit TaisyncTelemetry           (QObject* parent = nullptr);
    void close                          () override;
    void startTelemetry                 ();

private slots:
    void    _newConnection              () override;
    void    _readBytes                  () override;
    void    _readUDPBytes               ();
    void    _sendGCSHeartbeat           ();

private:
    QTimer          _heartbeatTimer;
    QUdpSocket*     _udpTelemetrySocket = nullptr;
};
