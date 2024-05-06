/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QTimer>
#include <QtCore/QLoggingCategory>

#include "QGCToolbox.h"
#include "QmlObjectListModel.h"
#include "ADSBVehicle.h"

Q_DECLARE_LOGGING_CATEGORY(ADSBVehicleManagerLog)

class ADSBTCPLink;

class ADSBVehicleManager : public QGCTool
{
    Q_OBJECT
    Q_PROPERTY(QmlObjectListModel* adsbVehicles READ adsbVehicles CONSTANT)

public:
    ADSBVehicleManager(QGCApplication* app, QGCToolbox* toolbox);

    void setToolbox(QGCToolbox* toolbox) final;

    QmlObjectListModel* adsbVehicles(void) { return &_adsbVehicles; }

public slots:
    void adsbVehicleUpdate(const ADSBVehicle::ADSBVehicleInfo_t vehicleInfo);
    void _tcpError(const QString errorMsg);

private slots:
    void _cleanupStaleVehicles(void);

private:
    QmlObjectListModel _adsbVehicles;
    QMap<uint32_t, ADSBVehicle*> _adsbICAOMap;
    QTimer _adsbVehicleCleanupTimer;
    ADSBTCPLink* _tcpLink = nullptr;
};
