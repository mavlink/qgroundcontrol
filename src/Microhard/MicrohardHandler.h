/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCLoggingCategory.h"

#include <QHostAddress>
#include <QTcpSocket>

#define MICROHARD_SETTINGS_PORT   23

Q_DECLARE_LOGGING_CATEGORY(MicrohardLog)

class MicrohardHandler : public QObject
{
    Q_OBJECT
public:

    explicit MicrohardHandler           (QObject* parent = nullptr);
    ~MicrohardHandler                   ();
    virtual bool start                  () = 0;
    virtual bool close                  ();

protected:
    virtual void _start                 (uint16_t port, QHostAddress addr = QHostAddress::AnyIPv4);

protected slots:
    virtual void    _readBytes          () = 0;
    virtual void    _testConnection     ();

signals:
    void connected                      (int status);
    void rssiUpdated                    (int rssi);

protected:
    QTcpSocket*     _tcpSocket  = nullptr;
};
