/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>

#define TAISYNC_USB_UDP_PORT    5000
#define TAISYNC_USB_VIDEO_PORT  8000
#define TAISYNC_USB_DATA_PORT   8200

class TaisyncUSBHandler : public QObject
{
    Q_OBJECT
public:

    explicit TaisyncUSBHandler  (QObject* parent = nullptr);
    ~TaisyncUSBHandler          ();
    void startVideo             ();
    void startTelemetry         ();

private slots:
    void    _newVideoConnection ();
    void    _readVideoBytes     ();
    void    _newDataConnection  ();
    void    _readDataBytes      ();

private:
    QTcpServer*     _tcpVideoServer = nullptr;
    QTcpSocket*     _tcpVideoSocket = nullptr;
    QTcpServer*     _tcpDataServer  = nullptr;
    QTcpSocket*     _tcpDataSocket  = nullptr;
    QUdpSocket*     _udpSocket      = nullptr;

};
