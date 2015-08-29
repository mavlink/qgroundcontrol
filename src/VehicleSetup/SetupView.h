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
    
#ifdef UNITTEST_BUILD
    void showFirmware(void);
    void showParameters(void);
    void showSummary(void);
    void showVehicleComponentSetup(VehicleComponent* vehicleComponent);
#endif
};

#endif
