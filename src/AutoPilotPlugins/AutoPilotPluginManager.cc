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

#include "AutoPilotPluginManager.h"
#include "PX4/PX4AutoPilotPlugin.h"
#include "Generic/GenericAutoPilotPlugin.h"
#include "QGCApplication.h"
#include "UASManager.h"

IMPLEMENT_QGC_SINGLETON(AutoPilotPluginManager, AutoPilotPluginManager)

AutoPilotPluginManager::AutoPilotPluginManager(QObject* parent) :
    QGCSingleton(parent)
{
    UASManagerInterface* uasMgr = UASManager::instance();
    Q_ASSERT(uasMgr);
    
    // We need to track uas coming and going so that we can instantiate plugins for each uas
    connect(uasMgr, &UASManagerInterface::UASCreated, this, &AutoPilotPluginManager::_uasCreated);
    connect(uasMgr, &UASManagerInterface::UASDeleted, this, &AutoPilotPluginManager::_uasDeleted);
}

AutoPilotPluginManager::~AutoPilotPluginManager()
{
#ifdef QT_DEBUG
    foreach(MAV_AUTOPILOT mavType, _pluginMap.keys()) {
        Q_ASSERT_X(_pluginMap[mavType].count() == 0, "AutoPilotPluginManager", "LinkManager::_shutdown should have already closed all uas");
    }
#endif
    _pluginMap.clear();
    
    PX4AutoPilotPlugin::clearStaticData();
    GenericAutoPilotPlugin::clearStaticData();
}

/// Create the plugin for this uas
void AutoPilotPluginManager::_uasCreated(UASInterface* uas)
{
    Q_ASSERT(uas);
    
    MAV_AUTOPILOT autopilotType = static_cast<MAV_AUTOPILOT>(uas->getAutopilotType());
    int uasId = uas->getUASID();
    Q_ASSERT(uasId != 0);
    
    if (_pluginMap.contains(autopilotType)) {
        Q_ASSERT_X(!_pluginMap[autopilotType].contains(uasId), "AutoPilotPluginManager", "Either we have duplicate UAS ids, or a UAS was not removed correctly.");
    }

    AutoPilotPlugin* plugin;
    switch (autopilotType) {
        case MAV_AUTOPILOT_PX4:
            plugin = new PX4AutoPilotPlugin(uas, this);
            Q_CHECK_PTR(plugin);
            _pluginMap[MAV_AUTOPILOT_PX4][uasId] = plugin;
            break;
        case MAV_AUTOPILOT_GENERIC:
        default:
            plugin = new GenericAutoPilotPlugin(uas, this);
            Q_CHECK_PTR(plugin);
            _pluginMap[MAV_AUTOPILOT_GENERIC][uasId] = plugin;
    }
}

/// Destroy the plugin associated with this uas
void AutoPilotPluginManager::_uasDeleted(UASInterface* uas)
{
    Q_ASSERT(uas);
    
    MAV_AUTOPILOT autopilotType = _installedAutopilotType(static_cast<MAV_AUTOPILOT>(uas->getAutopilotType()));
    int uasId = uas->getUASID();
    Q_ASSERT(uasId != 0);
    
    Q_ASSERT(_pluginMap.contains(autopilotType));
    Q_ASSERT(_pluginMap[autopilotType].contains(uasId));
    delete _pluginMap[autopilotType][uasId];
    _pluginMap[autopilotType].remove(uasId);
}

AutoPilotPlugin* AutoPilotPluginManager::getInstanceForAutoPilotPlugin(UASInterface* uas)
{
    Q_ASSERT(uas);
    
    MAV_AUTOPILOT autopilotType = _installedAutopilotType(static_cast<MAV_AUTOPILOT>(uas->getAutopilotType()));
    int uasId = uas->getUASID();
    Q_ASSERT(uasId != 0);
    
    Q_ASSERT(_pluginMap.contains(autopilotType));
    Q_ASSERT(_pluginMap[autopilotType].contains(uasId));
    
    return _pluginMap[autopilotType][uasId];
}

QList<AutoPilotPluginManager::FullMode_t> AutoPilotPluginManager::getModes(int autopilotType) const
{
    switch (autopilotType) {
        case MAV_AUTOPILOT_PX4:
            return PX4AutoPilotPlugin::getModes();
        case MAV_AUTOPILOT_GENERIC:
        default:
            return GenericAutoPilotPlugin::getModes();
    }
}

QString AutoPilotPluginManager::getShortModeText(uint8_t baseMode, uint32_t customMode, int autopilotType) const
{
    switch (autopilotType) {
        case MAV_AUTOPILOT_PX4:
            return PX4AutoPilotPlugin::getShortModeText(baseMode, customMode);
        case MAV_AUTOPILOT_GENERIC:
        default:
            return GenericAutoPilotPlugin::getShortModeText(baseMode, customMode);
    }
}

/// If autopilot is not an installed plugin, returns MAV_AUTOPILOT_GENERIC
MAV_AUTOPILOT AutoPilotPluginManager::_installedAutopilotType(MAV_AUTOPILOT autopilot)
{
    return _pluginMap.contains(autopilot) ? autopilot : MAV_AUTOPILOT_GENERIC;
}
