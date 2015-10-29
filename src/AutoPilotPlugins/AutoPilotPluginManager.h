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

/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef AUTOPILOTPLUGINMANAGER_H
#define AUTOPILOTPLUGINMANAGER_H

#include <QObject>
#include <QList>
#include <QString>

#include "AutoPilotPlugin.h"
#include "QGCSingleton.h"
#include "Vehicle.h"

/// AutoPilotPlugin manager is a singleton which maintains the list of AutoPilotPlugin objects.

class AutoPilotPluginManager : public QGCSingleton
{
    Q_OBJECT
    
    DECLARE_QGC_SINGLETON(AutoPilotPluginManager, AutoPilotPluginManager)

public:
    AutoPilotPlugin* newAutopilotPluginForVehicle(Vehicle* vehicle);
    
private:
    /// All access to singleton is through AutoPilotPluginManager::instance
    AutoPilotPluginManager(QObject* parent = NULL);
};

#endif
