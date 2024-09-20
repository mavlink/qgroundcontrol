/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>

#include "ADSB.h"

Q_DECLARE_LOGGING_CATEGORY(ADSBVehicleManagerLog)

class ADSBTCPLink;
class ADSBVehicle;
class QmlObjectListModel;
class QTimer;
class ADSBVehicleManagerSettings;

class ADSBVehicleManager : public QObject
{
    Q_OBJECT
    Q_MOC_INCLUDE("QmlObjectListModel.h")

    Q_PROPERTY(const QmlObjectListModel *adsbVehicles READ adsbVehicles CONSTANT)

public:
    ADSBVehicleManager(ADSBVehicleManagerSettings *settings, QObject *parent = nullptr);
    ~ADSBVehicleManager();

    static ADSBVehicleManager *instance();

    const QmlObjectListModel *adsbVehicles() const { return _adsbVehicles; }

public slots:
    void adsbVehicleUpdate(const ADSB::VehicleInfo_t &vehicleInfo);

private slots:
    void _cleanupStaleVehicles();
    void _linkError(const QString &errorMsg, bool stopped = false);

private:
    void _start(const QString &hostAddress, quint16 port);
    void _stop();

    ADSBVehicleManagerSettings *_adsbSettings = nullptr;
    QTimer *_adsbVehicleCleanupTimer = nullptr;
    QmlObjectListModel *_adsbVehicles = nullptr;

    QMap<uint32_t, ADSBVehicle*> _adsbICAOMap;
    ADSBTCPLink *_adsbTcpLink = nullptr;
};
