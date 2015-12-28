/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

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
