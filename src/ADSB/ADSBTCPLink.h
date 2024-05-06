/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QThread>
#include <QtCore/QLoggingCategory>

#include "ADSBVehicle.h"

Q_DECLARE_LOGGING_CATEGORY(ADSBTCPLinkLog)

class QTcpSocket;

class ADSBTCPLink : public QThread
{
    Q_OBJECT

public:
    ADSBTCPLink(const QString& hostAddress, int port, QObject* parent);
    ~ADSBTCPLink();

signals:
    void adsbVehicleUpdate(const ADSBVehicle::ADSBVehicleInfo_t vehicleInfo);
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
    void _parseAndEmitCallsign(ADSBVehicle::ADSBVehicleInfo_t &adsbInfo, QStringList values);
    void _parseAndEmitLocation(ADSBVehicle::ADSBVehicleInfo_t &adsbInfo, QStringList values);
    void _parseAndEmitHeading(ADSBVehicle::ADSBVehicleInfo_t &adsbInfo, QStringList values);
};
