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

#ifndef SENSORSCOMPONENTCONTROLLER_H
#define SENSORSCOMPONENTCONTROLLER_H

#include <QObject>
#include <QQuickItem>

#include "UASInterface.h"
#include "AutoPilotPlugin.h"

/// Sensors Component MVC Controller for SensorsComponent.qml.
class SensorsComponentController : public QObject
{
    Q_OBJECT
    
public:
    SensorsComponentController(AutoPilotPlugin* autopilot, QObject* parent = NULL);
    
    Q_PROPERTY(bool fixedWing READ fixedWing CONSTANT)
    
    /// TextArea for log output
    Q_PROPERTY(QQuickItem* statusLog MEMBER _statusLog)
    
    Q_INVOKABLE void calibrateCompass(void);
    Q_INVOKABLE void calibrateGyro(void);
    Q_INVOKABLE void calibrateAccel(void);
    Q_INVOKABLE void calibrateAirspeed(void);
    
    bool fixedWing(void);
    
signals:
    void bogusNotify(void);
    
private slots:
    void _handleUASTextMessage(int uasId, int compId, int severity, QString text);
    
private:
    void _beginTextLogging(void);
    void _appendStatusLog(const QString& text);
    void _refreshParams(void);

    QQuickItem*         _statusLog;         ///< Status log TextArea Qml control
    AutoPilotPlugin*    _autopilot;
};

#endif
