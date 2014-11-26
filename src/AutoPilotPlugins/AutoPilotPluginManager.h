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

#ifndef AUTOPILOTPLUGINMANAGER_H
#define AUTOPILOTPLUGINMANAGER_H

#include <QObject>
#include <QList>
#include <QString>

#include "UASInterface.h"
#include "VehicleComponent.h"
#include "AutoPilotPlugin.h"
#include "QGCSingleton.h"

/// @file
///     @brief The AutoPilotPlugin manager is a singleton which maintains the list of AutoPilotPlugin objects.
///
///     @author Don Gagne <don@thegagnes.com>

class AutoPilotPluginManager : public QGCSingleton
{
    Q_OBJECT

public:
    /// @brief Returns the AutoPilotPluginManager singleton
    static AutoPilotPluginManager* instance(void);
    
    virtual void deleteInstance(void);
    
    /// @brief Returns the singleton AutoPilot instance for the specified auto pilot type.
    /// @param autopilotType Specified using the MAV_AUTOPILOT_* values.
    AutoPilotPlugin* getInstanceForAutoPilotPlugin(int autopilotType);
    
private:
    /// All access to singleton is through AutoPilotPluginManager::instance
    AutoPilotPluginManager(QObject* parent = NULL);
    
    static AutoPilotPluginManager* _instance;
    
    QMap<int, AutoPilotPlugin*> _pluginMap;
};

#endif
