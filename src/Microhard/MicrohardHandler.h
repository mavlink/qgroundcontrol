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

#define MICROHARD_SETTINGS_PORT   23

Q_DECLARE_LOGGING_CATEGORY(MicrohardLog)
Q_DECLARE_LOGGING_CATEGORY(MicrohardVerbose)

class MicrohardHandler : public QObject
{
    Q_OBJECT
public:

    explicit MicrohardHandler           (QObject* parent = nullptr);
    ~MicrohardHandler                   ();
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
