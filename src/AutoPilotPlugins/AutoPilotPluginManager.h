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
#include "QGCApplication.h"
#include "AutoPilotPlugin.h"

/// @file
///     @brief The AutoPilotPlugin manager is a singleton which maintains the list of AutoPilotPlugin objects.
///
///     @author Don Gagne <don@thegagnes.com>

class AutoPilotPluginManager : public QObject
{
    Q_OBJECT

public:
    
    /// @brief Returns the singleton AutoPilot instance for the specified auto pilot type.
    /// @param autopilotType Specified using the MAV_AUTOPILOT_* values.
    AutoPilotPlugin* getInstanceForAutoPilotPlugin(int autopilotType);
    
private:
    /// @brief Only QGCQpplication is allowed to call constructor. All access to singleton is through
    ///         QGCApplication::singletonAutoPilotPluginManager.
    AutoPilotPluginManager(QObject* parent = NULL);
    
    /// @brief Only QGCQpplication is allowed to call constructor. All access to singleton is through
    ///         QGCApplication::singletonAutoPilotPluginManager.
    friend class QGCApplication;
    
    QMap<int, AutoPilotPlugin*> _pluginMap;
};

#endif
