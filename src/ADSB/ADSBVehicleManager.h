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
#include "ADSBVehicle.h"

#include <QThread>
#include <QTcpSocket>
#include <QTimer>
#include <QGeoCoordinate>

class ADSBVehicleManagerSettings;

class ADSBTCPLink : public QThread
{
    Q_OBJECT

public:
    ADSBTCPLink(const QString& hostAddress, int port, QObject* parent);
    ~ADSBTCPLink();

signals:
    void adsbVehicleUpdate(const ADSBVehicle::VehicleInfo_t vehicleInfo);
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

class ADSBVehicleManager : public QGCTool {
    Q_OBJECT
    
public:
    ADSBVehicleManager(QGCApplication* app, QGCToolbox* toolbox);

    Q_PROPERTY(QmlObjectListModel* adsbVehicles READ adsbVehicles CONSTANT)

    QmlObjectListModel* adsbVehicles(void) { return &_adsbVehicles; }

    // QGCTool overrides
    void setToolbox(QGCToolbox* toolbox) final;

public slots:
    void adsbVehicleUpdate  (const ADSBVehicle::VehicleInfo_t vehicleInfo);
    void _tcpError          (const QString errorMsg);

private slots:
    void _cleanupStaleVehicles(void);

private:
    QmlObjectListModel              _adsbVehicles;
    QMap<uint32_t, ADSBVehicle*>    _adsbICAOMap;
    QTimer                          _adsbVehicleCleanupTimer;
    ADSBTCPLink*                    _tcpLink = nullptr;
};
