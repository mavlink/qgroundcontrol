/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef PowerComponentController_H
#define PowerComponentController_H

#include <QObject>
#include <QQuickItem>

#include "UASInterface.h"
#include "FactPanelController.h"

/// Power Component MVC Controller for PowerComponent.qml.
class PowerComponentController : public FactPanelController
{
    Q_OBJECT
    
public:
    PowerComponentController(void);
    
    Q_INVOKABLE void calibrateEsc(void);
    Q_INVOKABLE void busConfigureActuators(void);
    Q_INVOKABLE void stopBusConfigureActuators(void);
    
signals:
    void oldFirmware(void);
    void newerFirmware(void);
    void incorrectFirmwareRevReporting(void);
    void connectBattery(void);
    void disconnectBattery(void);
    void batteryConnected(void);
    void calibrationFailed(const QString& errorMessage);
    void calibrationSuccess(const QStringList& warningMessages);
    
private slots:
    void _handleUASTextMessage(int uasId, int compId, int severity, QString text);
    
private:
    void _stopCalibration(void);
    void _stopBusConfig(void);
    
    QStringList _warningMessages;
    static const int _neededFirmwareRev = 1;
};

#endif
