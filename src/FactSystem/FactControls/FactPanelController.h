/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#ifndef FactPanelController_H
#define FactPanelController_H

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include <QObject>
#include <QQuickItem>

#include "UASInterface.h"
#include "AutoPilotPlugin.h"
#include "QGCLoggingCategory.h"

Q_DECLARE_LOGGING_CATEGORY(FactPanelControllerLog)

/// FactPanelController is used in combination with the FactPanel Qml control for handling
/// missing Facts from C++ code.
class FactPanelController : public QObject
{
	Q_OBJECT
	
public:
	FactPanelController(void);
	
    Q_PROPERTY(QQuickItem* factPanel READ factPanel WRITE setFactPanel)
    
    Q_INVOKABLE Fact* getParameterFact(int componentId, const QString& name);
    Q_INVOKABLE bool parameterExists(int componentId, const QString& name);
    
    QQuickItem* factPanel(void);
    void setFactPanel(QQuickItem* panel);
    
protected:
    /// Checks for existence of the specified parameters
    /// @return true: all parameters exists, false: parameters missing and reported
    bool _allParametersExists(int componentId, QStringList names);
    
    /// Report a missing parameter to the FactPanel Qml element
    void _reportMissingParameter(int componentId, const QString& name);
    
    Vehicle*            _vehicle;
    UASInterface*       _uas;
    AutoPilotPlugin*    _autopilot;
    
private slots:
    void _checkForMissingFactPanel(void);
    
private:
    void _notifyPanelMissingParameter(const QString& missingParam);
    void _notifyPanelErrorMsg(const QString& errorMsg);
    void _showInternalError(const QString& errorMsg);

    QQuickItem*         _factPanel;
    QStringList         _delayedMissingParams;
};

#endif