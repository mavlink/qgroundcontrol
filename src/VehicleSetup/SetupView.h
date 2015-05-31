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

#ifndef SETUPVIEW_H
#define SETUPVIEW_H

#include "UASInterface.h"
#include "VehicleComponent.h"
#include "AutoPilotPlugin.h"
#include "QGCQmlWidgetHolder.h"

#include <QWidget>

/// @file
///     @brief This class is used to display the UI for the VehicleComponent objects.
///     @author Don Gagne <don@thegagnes.com>

class SetupView : public QGCQmlWidgetHolder
{
    Q_OBJECT

public:
    explicit SetupView(QWidget* parent = 0);
    ~SetupView();
    
    Q_PROPERTY(AutoPilotPlugin* autopilot READ autopilot NOTIFY autopilotChanged)
    Q_PROPERTY (bool showFirmware MEMBER _showFirmware CONSTANT)
    
#ifdef UNITTEST_BUILD
    void showFirmware(void);
    void showParameters(void);
    void showSummary(void);
    void showVehicleComponentSetup(VehicleComponent* vehicleComponent);
#endif
    
    AutoPilotPlugin* autopilot(void);
    
signals:
    void autopilotChanged(AutoPilotPlugin* autopilot);
    
private slots:
    void _setActiveUAS(UASInterface* uas);
    void _pluginReadyChanged(bool pluginReady);

private:
    UASInterface*                   _uasCurrent;
    bool                            _initComplete;      ///< true: parameters are ready and ui has been setup
    QSharedPointer<AutoPilotPlugin> _autopilot;         // Shared pointer to prevent shutdown ordering problems
    AutoPilotPlugin*                _readyAutopilot;    // NULL if autopilot is not yet read
    bool                            _showFirmware;
};

#endif
