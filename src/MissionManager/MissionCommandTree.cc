
/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "MissionCommandTree.h"
#include "FactMetaData.h"
#include "Vehicle.h"
#include "FirmwarePluginManager.h"
#include "QGCApplication.h"
#include "QGroundControlQmlGlobal.h"
#include "MissionCommandUIInfo.h"
#include "MissionCommandList.h"
#include "SettingsManager.h"

#include <QQmlEngine>

MissionCommandTree::MissionCommandTree(QGCApplication* app, QGCToolbox* toolbox, bool unitTest)
    : QGCTool(app, toolbox)
    , _allCommandsCategory(tr("All commands"))
    , _settingsManager(NULL)
    , _unitTest(unitTest)
{
}

void MissionCommandTree::setToolbox(QGCToolbox* toolbox)
{
    QGCTool::setToolbox(toolbox);

    _settingsManager = toolbox->settingsManager();

#ifdef UNITTEST_BUILD
    if (_unitTest) {
        // Load unit testing tree
        _staticCommandTree[MAV_AUTOPILOT_GENERIC][MAV_TYPE_GENERIC] =           new MissionCommandList(":/unittest/MavCmdInfoCommon.json", true, this);
        _staticCommandTree[MAV_AUTOPILOT_GENERIC][MAV_TYPE_FIXED_WING] =        new MissionCommandList(":/unittest/MavCmdInfoFixedWing.json", false, this);
        _staticCommandTree[MAV_AUTOPILOT_GENERIC][MAV_TYPE_QUADROTOR] =         new MissionCommandList(":/unittest/MavCmdInfoMultiRotor.json", false, this);
        _staticCommandTree[MAV_AUTOPILOT_GENERIC][MAV_TYPE_VTOL_QUADROTOR] =    new MissionCommandList(":/unittest/MavCmdInfoVTOL.json", false, this);
        _staticCommandTree[MAV_AUTOPILOT_GENERIC][MAV_TYPE_SUBMARINE] =         new MissionCommandList(":/unittest/MavCmdInfoSub.json", false, this);
        _staticCommandTree[MAV_AUTOPILOT_GENERIC][MAV_TYPE_GROUND_ROVER] =      new MissionCommandList(":/unittest/MavCmdInfoRover.json", false, this);
    } else {
#endif
        // Load all levels of hierarchy
        for (MAV_AUTOPILOT firmwareType: _toolbox->firmwarePluginManager()->supportedFirmwareTypes()) {
            FirmwarePlugin* plugin = _toolbox->firmwarePluginManager()->firmwarePluginForAutopilot(firmwareType, MAV_TYPE_QUADROTOR);

            QList<MAV_TYPE> vehicleTypes;
            vehicleTypes << MAV_TYPE_GENERIC << MAV_TYPE_FIXED_WING << MAV_TYPE_QUADROTOR << MAV_TYPE_VTOL_QUADROTOR << MAV_TYPE_GROUND_ROVER << MAV_TYPE_SUBMARINE;

            for(MAV_TYPE vehicleType: vehicleTypes) {
                QString overrideFile = plugin->missionCommandOverrides(vehicleType);
                if (!overrideFile.isEmpty()) {
                    _staticCommandTree[firmwareType][vehicleType] = new MissionCommandList(overrideFile, firmwareType == MAV_AUTOPILOT_GENERIC && vehicleType == MAV_TYPE_GENERIC /* baseCommandList */, this);
                }
            }
        }
#ifdef UNITTEST_BUILD
    }
#endif
}

MAV_AUTOPILOT MissionCommandTree::_baseFirmwareType(MAV_AUTOPILOT firmwareType) const
{
    if (qgcApp()->toolbox()->firmwarePluginManager()->supportedFirmwareTypes().contains(firmwareType)) {
        return firmwareType;
    } else {
        return MAV_AUTOPILOT_GENERIC;
    }

}

MAV_TYPE MissionCommandTree::_baseVehicleType(MAV_TYPE mavType) const
{
    if (QGCMAVLink::isFixedWing(mavType)) {
        return MAV_TYPE_FIXED_WING;
    } else if (QGCMAVLink::isMultiRotor(mavType)) {
        return MAV_TYPE_QUADROTOR;
    } else if (QGCMAVLink::isVTOL(mavType)) {
        return MAV_TYPE_VTOL_QUADROTOR;
    } else if (QGCMAVLink::isRover(mavType)) {
        return MAV_TYPE_GROUND_ROVER;
    } else if (QGCMAVLink::isSub(mavType)) {
        return MAV_TYPE_SUBMARINE;
    } else {
        return MAV_TYPE_GENERIC;
    }
}

/// Add the next level of the hierarchy to a collapsed tree.
///     @param vehicle Collapsed tree is for this vehicle
///     @param cmdList List of mission commands to collapse into ui info
///     @param collapsedTree Tree we are collapsing into
void MissionCommandTree::_collapseHierarchy(Vehicle*                                vehicle,
                                            const MissionCommandList*               cmdList,
                                            QMap<MAV_CMD, MissionCommandUIInfo*>&   collapsedTree)
{
    MAV_AUTOPILOT   baseFirmwareType;
    MAV_TYPE        baseVehicleType;

    _baseVehicleInfo(vehicle, baseFirmwareType, baseVehicleType);

    for (MAV_CMD command: cmdList->commandIds()) {
        // Only add supported command to tree (MAV_CMD_NAV_LAST is used for planned home position)
        if (!qgcApp()->runningUnitTests()
                && !vehicle->firmwarePlugin()->supportedMissionCommands().isEmpty()
                && !vehicle->firmwarePlugin()->supportedMissionCommands().contains(command)
                && command != MAV_CMD_NAV_LAST) {
            continue;
        }

        MissionCommandUIInfo* uiInfo = cmdList->getUIInfo(command);
        if (uiInfo) {
            if (collapsedTree.contains(command)) {
                collapsedTree[command]->_overrideInfo(uiInfo);
            } else {
                collapsedTree[command] = new MissionCommandUIInfo(*uiInfo);
            }
        }
    }
}

void MissionCommandTree::_buildAvailableCommands(Vehicle* vehicle)
{
    MAV_AUTOPILOT   baseFirmwareType;
    MAV_TYPE        baseVehicleType;

    _baseVehicleInfo(vehicle, baseFirmwareType, baseVehicleType);

    if (_availableCommands.contains(baseFirmwareType) &&
            _availableCommands[baseFirmwareType].contains(baseVehicleType)) {
        // Available commands list already built
        return;
    }

    // Build new available commands list

    QMap<MAV_CMD, MissionCommandUIInfo*>& collapsedTree = _availableCommands[baseFirmwareType][baseVehicleType];

    // Any Firmware, Any Vehicle
    _collapseHierarchy(vehicle, _staticCommandTree[MAV_AUTOPILOT_GENERIC][MAV_TYPE_GENERIC], collapsedTree);

    // Any Firmware, Specific Vehicle
    if (baseVehicleType != MAV_TYPE_GENERIC) {
        _collapseHierarchy(vehicle, _staticCommandTree[MAV_AUTOPILOT_GENERIC][baseVehicleType], collapsedTree);
    }

    // Known Firmware, Any Vehicle
    if (baseFirmwareType != MAV_AUTOPILOT_GENERIC) {
        _collapseHierarchy(vehicle, _staticCommandTree[baseFirmwareType][MAV_TYPE_GENERIC], collapsedTree);

        // Known Firmware, Specific Vehicle
        if (baseVehicleType != MAV_TYPE_GENERIC) {
            _collapseHierarchy(vehicle, _staticCommandTree[baseFirmwareType][baseVehicleType], collapsedTree);
        }
    }

    // Build category list
    QMapIterator<MAV_CMD, MissionCommandUIInfo*> iter(collapsedTree);
    while (iter.hasNext()) {
        iter.next();
        QString newCategory = iter.value()->category();
        if (!_availableCategories[baseFirmwareType][baseVehicleType].contains(newCategory)) {
            _availableCategories[baseFirmwareType][baseVehicleType].append(newCategory);
        }
    }
    _availableCategories[baseFirmwareType][baseVehicleType].append(_allCommandsCategory);
}

QStringList MissionCommandTree::_availableCategoriesForVehicle(Vehicle* vehicle)
{
    MAV_AUTOPILOT   baseFirmwareType;
    MAV_TYPE        baseVehicleType;

    _baseVehicleInfo(vehicle, baseFirmwareType, baseVehicleType);
    _buildAvailableCommands(vehicle);

    return _availableCategories[baseFirmwareType][baseVehicleType];
}

QString MissionCommandTree::friendlyName(MAV_CMD command)
{
    MissionCommandList *    commandList =   _staticCommandTree[MAV_AUTOPILOT_GENERIC][MAV_TYPE_GENERIC];
    MissionCommandUIInfo*   uiInfo =        commandList->getUIInfo(command);

    if (uiInfo) {
        return uiInfo->friendlyName();
    } else {
        return QString("MAV_CMD(%1)").arg((int)command);
    }
}

QString MissionCommandTree::rawName(MAV_CMD command)
{
    MissionCommandList *    commandList =   _staticCommandTree[MAV_AUTOPILOT_GENERIC][MAV_TYPE_GENERIC];
    MissionCommandUIInfo*   uiInfo =        commandList->getUIInfo(command);

    if (uiInfo) {
        return uiInfo->rawName();
    } else {
        return QString("MAV_CMD(%1)").arg((int)command);
    }
}

const QList<MAV_CMD>& MissionCommandTree::allCommandIds(void) const
{
    return _staticCommandTree[MAV_AUTOPILOT_GENERIC][MAV_TYPE_GENERIC]->commandIds();
}

const MissionCommandUIInfo* MissionCommandTree::getUIInfo(Vehicle* vehicle, MAV_CMD command)
{
    MAV_AUTOPILOT   baseFirmwareType;
    MAV_TYPE        baseVehicleType;

    _baseVehicleInfo(vehicle, baseFirmwareType, baseVehicleType);
    _buildAvailableCommands(vehicle);

    const QMap<MAV_CMD, MissionCommandUIInfo*>& infoMap = _availableCommands[baseFirmwareType][baseVehicleType];
    if (infoMap.contains(command)) {
        return infoMap[command];
    } else {
        return NULL;
    }
}

QVariantList MissionCommandTree::getCommandsForCategory(Vehicle* vehicle, const QString& category)
{
    MAV_AUTOPILOT   baseFirmwareType;
    MAV_TYPE        baseVehicleType;

    _baseVehicleInfo(vehicle, baseFirmwareType, baseVehicleType);
    _buildAvailableCommands(vehicle);

    QVariantList list;
    QMap<MAV_CMD, MissionCommandUIInfo*> commandMap = _availableCommands[baseFirmwareType][baseVehicleType];
    for (MAV_CMD command: commandMap.keys()) {
        if (command == MAV_CMD_NAV_LAST) {
            // MAV_CMD_NAV_LAST is used for Mission Settings item. Although we want to be able to get command info for it.
            // The user should not be able to use it as a command.
            continue;
        }
        MissionCommandUIInfo* uiInfo = commandMap[command];
        if (uiInfo->category() == category || category == _allCommandsCategory) {
            list.append(QVariant::fromValue(uiInfo));
        }
    }

    return list;
}

void MissionCommandTree::_baseVehicleInfo(Vehicle* vehicle, MAV_AUTOPILOT& baseFirmwareType, MAV_TYPE& baseVehicleType) const
{
    if (vehicle) {
        baseFirmwareType = _baseFirmwareType(vehicle->firmwareType());
        baseVehicleType = _baseVehicleType(vehicle->vehicleType());
    } else {
        // No Vehicle means offline editing
        baseFirmwareType = _baseFirmwareType((MAV_AUTOPILOT)_settingsManager->appSettings()->offlineEditingFirmwareType()->rawValue().toInt());
        baseVehicleType = _baseVehicleType((MAV_TYPE)_settingsManager->appSettings()->offlineEditingVehicleType()->rawValue().toInt());
    }
}
