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

#define TAISYNC_VIDEO_UDP_PORT  5600
#define TAISYNC_VIDEO_TCP_PORT  8000
#define TAISYNC_SETTINGS_PORT   8200
#define TAISYNC_TELEM_PORT      8400

Q_DECLARE_LOGGING_CATEGORY(TaisyncLog)

class TaisyncHandler : public QObject
{
    Q_OBJECT
public:

    explicit TaisyncHandler             (QObject* parent = nullptr);
    ~TaisyncHandler                     ();
    virtual void start                  () = 0;
    virtual void close                  ();

protected:
    virtual void _start                 (uint16_t port);

protected slots:
    virtual void    _newConnection      ();
    virtual void    _socketDisconnected ();
    virtual void    _readBytes          () = 0;

protected:
    QTcpServer*     _tcpServer = nullptr;
    QTcpSocket*     _tcpSocket = nullptr;
};
