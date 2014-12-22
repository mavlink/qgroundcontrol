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

#include "UASInterface.h"
#include "VehicleComponent.h"
#include "AutoPilotPlugin.h"
#include "QGCSingleton.h"
#include "QGCMAVLink.h"


/// AutoPilotPlugin manager is a singleton which maintains the list of AutoPilotPlugin objects.

class AutoPilotPluginManager : public QGCSingleton
{
    Q_OBJECT
    
    DECLARE_QGC_SINGLETON(AutoPilotPluginManager, AutoPilotPluginManager)

public:
    /// Returns the singleton AutoPilotPlugin instance for the specified uas.
    ///     @param uas Uas to get plugin for
    AutoPilotPlugin* getInstanceForAutoPilotPlugin(UASInterface* uas);
    
    typedef struct {
        uint8_t baseMode;
        uint32_t customMode;
    } FullMode_t;

    /// Returns the list of modes which are available for the specified autopilot type.
    QList<FullMode_t> getModes(int autopilotType) const;
    
    /// @brief Returns a human readable short description for the specified mode.
    QString getShortModeText(uint8_t baseMode, uint32_t customMode, int autopilotType) const;

private slots:
    void _uasCreated(UASInterface* uas);
    void _uasDeleted(UASInterface* uas);

private:
    /// All access to singleton is through AutoPilotPluginManager::instance
    AutoPilotPluginManager(QObject* parent = NULL);
    ~AutoPilotPluginManager();
    
    MAV_AUTOPILOT _installedAutopilotType(MAV_AUTOPILOT autopilot);
    
    QMap<MAV_AUTOPILOT, QMap<int, AutoPilotPlugin*> > _pluginMap; ///< Map of AutoPilot plugins _pluginMap[MAV_TYPE][UASid]
};

#endif
