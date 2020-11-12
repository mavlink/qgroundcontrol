/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCLoggingCategory.h"

#include <QTcpServer>
#include <QTcpSocket>

#if defined(__ios__) || defined(__android__)
#define TAISYNC_VIDEO_UDP_PORT      5600
#define TAISYNC_VIDEO_TCP_PORT      8000
#define TAISYNC_SETTINGS_PORT       8200
#define TAISYNC_TELEM_PORT          8400
#define TAISYNC_TELEM_TARGET_PORT   14550
#else
#define TAISYNC_SETTINGS_PORT   80
#endif

Q_DECLARE_LOGGING_CATEGORY(TaisyncLog)
Q_DECLARE_LOGGING_CATEGORY(TaisyncVerbose)

class TaisyncHandler : public QObject
{
    Q_OBJECT
public:

    explicit TaisyncHandler             (QObject* parent = nullptr);
    ~TaisyncHandler                     ();
    virtual bool start                  () = 0;
    virtual bool close                  ();
    virtual bool isServerRunning        () { return (_serverMode && _tcpServer); }

protected:
    virtual bool    _start              (uint16_t port, QHostAddress addr = QHostAddress::AnyIPv4);

protected slots:
    virtual void    _newConnection      ();
    virtual void    _socketDisconnected ();
    virtual void    _readBytes          () = 0;

signals:
    void connected                      ();
    void disconnected                   ();

protected:
    bool            _serverMode = true;
    QTcpServer*     _tcpServer  = nullptr;
    QTcpSocket*     _tcpSocket  = nullptr;
};
