/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCToolbox.h"
#include "QmlObjectListModel.h"

#include <QThread>
#include <QTcpSocket>
#include <QTimer>
#include <QGeoCoordinate>

class NTRIPSettings;

class NTRIPTCPLink : public QThread
{
    Q_OBJECT

public:
    NTRIPTCPLink(const QString& hostAddress, int port, QObject* parent);
    ~NTRIPTCPLink();

signals:
    void error(const QString errorMsg);

protected:
    void run(void) final;

private slots:
    void _readBytes(void);

private:
    void _hardwareConnect(void);
    void _parseLine(const QString& line);

    QString         _hostAddress;
    int             _port;
    QTcpSocket*     _socket =   nullptr;
};

class NTRIP : public QGCTool {
    Q_OBJECT
    
public:
    NTRIP(QGCApplication* app, QGCToolbox* toolbox);

    // QGCTool overrides
    void setToolbox(QGCToolbox* toolbox) final;

public slots:
    void _tcpError          (const QString errorMsg);

private slots:

private:
    NTRIPTCPLink*                    _tcpLink = nullptr;
};
