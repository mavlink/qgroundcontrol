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

#define TAISYNC_USB_UDP_PORT    5000
#define TAISYNC_USB_VIDEO_PORT  8000

Q_DECLARE_LOGGING_CATEGORY(TaisyncVideoReceiverLog)

class TaisyncVideoReceiver : public QObject
{
    Q_OBJECT
public:

    explicit TaisyncVideoReceiver       (QObject* parent = nullptr);
    ~TaisyncVideoReceiver               ();
    void startVideo                     ();
    void close                          ();

private slots:
    void    _newVideoConnection         ();
    void    _readVideoBytes             ();
    void    _videoSocketDisconnected    ();

private:
    QTcpServer*     _tcpVideoServer     = nullptr;
    QTcpSocket*     _tcpVideoSocket     = nullptr;
    QUdpSocket*     _udpVideoSocket     = nullptr;
};
