/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
#include "MissionCommandUIInfo.h"
#include "MissionCommandList.h"
#include "SettingsManager.h"

#include <QQmlEngine>

MissionCommandTree::MissionCommandTree(QGCApplication* app, QGCToolbox* toolbox, bool unitTest)
    : QGCTool               (app, toolbox)
    , _allCommandsCategory  (tr("All commands"))
    , _settingsManager      (nullptr)
    , _unitTest             (unitTest)
{
}

void MissionCommandTree::setToolbox(QGCToolbox* toolbox)
{
    QGCTool::setToolbox(toolbox);

    _settingsManager = toolbox->settingsManager();

#ifdef UNITTEST_BUILD
    if (_unitTest) {
        // Load unit testing tree
        _staticCommandTree[MAV_AUTOPILOT_GENERIC][QGCMAVLink::VehicleClassGeneric]      = new MissionCommandList(":/unittest/UT-MavCmdInfoCommon.json", true, this);
        _staticCommandTree[MAV_AUTOPILOT_GENERIC][QGCMAVLink::VehicleClassFixedWing]    = new MissionCommandList(":/unittest/UT-MavCmdInfoFixedWing.json", false, this);
        _staticCommandTree[MAV_AUTOPILOT_GENERIC][QGCMAVLink::VehicleClassMultiRotor]   = new MissionCommandList(":/unittest/UT-MavCmdInfoMultiRotor.json", false, this);
        _staticCommandTree[MAV_AUTOPILOT_GENERIC][QGCMAVLink::VehicleClassVTOL]         = new MissionCommandList(":/unittest/UT-MavCmdInfoVTOL.json", false, this);
        _staticCommandTree[MAV_AUTOPILOT_GENERIC][QGCMAVLink::VehicleClassSub]          = new MissionCommandList(":/unittest/UT-MavCmdInfoSub.json", false, this);
        _staticCommandTree[MAV_AUTOPILOT_GENERIC][QGCMAVLink::VehicleClassRoverBoat]    = new MissionCommandList(":/unittest/UT-MavCmdInfoRover.json", false, this);
    } else {
#endif
        // Load all levels of hierarchy
        for (const QGCMAVLink::FirmwareClass_t firmwareClass: _toolbox->firmwarePluginManager()->supportedFirmwareClasses()) {
            FirmwarePlugin* plugin = _toolbox->firmwarePluginManager()->firmwarePluginForAutopilot(QGCMAVLink::firmwareClassToAutopilot(firmwareClass), MAV_TYPE_QUADROTOR);

            for (const QGCMAVLink::VehicleClass_t vehicleClass: QGCMAVLink::allVehicleClasses()) {
                QString overrideFile = plugin->missionCommandOverrides(vehicleClass);
                if (!overrideFile.isEmpty()) {
                    _staticCommandTree[firmwareClass][vehicleClass] = new MissionCommandList(overrideFile, firmwareClass == QGCMAVLink::FirmwareClassGeneric && vehicleClass == QGCMAVLink::VehicleClassGeneric /* baseCommandList */, this);
                }
            }
        }
#ifdef UNITTEST_BUILD
    }
#endif
}

/// Add the next level of the hierarchy to a collapsed tree.
///     @param cmdList          List of mission commands to collapse into ui info
///     @param collapsedTree    Tree we are collapsing into
void MissionCommandTree::_collapseHierarchy(const MissionCommandList*               cmdList,
                                            QMap<MAV_CMD, MissionCommandUIInfo*>&   collapsedTree)
{
    if (!cmdList) {
        return;
    }

    for (MAV_CMD command: cmdList->commandIds()) {
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

void MissionCommandTree::_buildAllCommands(Vehicle* vehicle, QGCMAVLink::VehicleClass_t vtolMode)
{
    QGCMAVLink::FirmwareClass_t firmwareClass;
    QGCMAVLink::VehicleClass_t  vehicleClass;

    _firmwareAndVehicleClassInfo(vehicle, vtolMode, firmwareClass, vehicleClass);

    if (_allCommands.contains(firmwareClass) && _allCommands[firmwareClass].contains(vehicleClass)) {
        // Already built
        return;
    }

    QMap<MAV_CMD, MissionCommandUIInfo*>& collapsedTree = _allCommands[firmwareClass][vehicleClass];

    // Base of the tree is all commands
    _collapseHierarchy(_staticCommandTree[MAV_AUTOPILOT_GENERIC][QGCMAVLink::VehicleClassGeneric], collapsedTree);

    // Add the overrides for specific vehicle types
    if (vehicleClass != QGCMAVLink::VehicleClassGeneric) {
        _collapseHierarchy(_staticCommandTree[QGCMAVLink::FirmwareClassGeneric][vehicleClass], collapsedTree);
    }

    // Add the overrides for specific firmware class, all vehicles
    if (firmwareClass != QGCMAVLink::FirmwareClassGeneric) {
        _collapseHierarchy(_staticCommandTree[firmwareClass][QGCMAVLink::VehicleClassGeneric], collapsedTree);

        // Add overrides for specific vehicle class
        if (vehicleClass != QGCMAVLink::VehicleClassGeneric) {
            _collapseHierarchy(_staticCommandTree[firmwareClass][vehicleClass], collapsedTree);
        }
    }

    // Build category list from supported commands
    QList<MAV_CMD> supportedCommands = vehicle->firmwarePlugin()->supportedMissionCommands(vehicleClass);
    for (MAV_CMD cmd: collapsedTree.keys()) {
        if (supportedCommands.contains(cmd)) {
            QString newCategory = collapsedTree[cmd]->category();
            if (!_supportedCategories[firmwareClass][vehicleClass].contains(newCategory)) {
                _supportedCategories[firmwareClass][vehicleClass].append(newCategory);
            }
        }
    }
    _supportedCategories[firmwareClass][vehicleClass].append(_allCommandsCategory);
}

QStringList MissionCommandTree::_availableCategoriesForVehicle(Vehicle* vehicle)
{
    QGCMAVLink::FirmwareClass_t firmwareClass;
    QGCMAVLink::VehicleClass_t  vehicleClass;

    _firmwareAndVehicleClassInfo(vehicle, QGCMAVLink::VehicleClassGeneric, firmwareClass, vehicleClass);
    _buildAllCommands(vehicle, QGCMAVLink::VehicleClassGeneric);

    return _supportedCategories[firmwareClass][vehicleClass];
}

QString MissionCommandTree::friendlyName(MAV_CMD command)
{
    MissionCommandList *    commandList =   _staticCommandTree[QGCMAVLink::FirmwareClassGeneric][QGCMAVLink::VehicleClassGeneric];
    MissionCommandUIInfo*   uiInfo =        commandList->getUIInfo(command);

    if (uiInfo) {
        return uiInfo->friendlyName();
    } else {
        return QStringLiteral("MAV_CMD(%1)").arg((int)command);
    }
}

QString MissionCommandTree::rawName(MAV_CMD command)
{
    MissionCommandList *    commandList =   _staticCommandTree[QGCMAVLink::FirmwareClassGeneric][QGCMAVLink::VehicleClassGeneric];
    MissionCommandUIInfo*   uiInfo =        commandList->getUIInfo(command);

    if (uiInfo) {
        return uiInfo->rawName();
    } else {
        return QStringLiteral("MAV_CMD(%1)").arg((int)command);
    }
}

bool MissionCommandTree::isLandCommand(MAV_CMD command)
{
    MissionCommandList *    commandList =   _staticCommandTree[QGCMAVLink::FirmwareClassGeneric][QGCMAVLink::VehicleClassGeneric];
    MissionCommandUIInfo*   uiInfo =        commandList->getUIInfo(command);

    return uiInfo ? uiInfo->isLandCommand() : false;
}

bool MissionCommandTree::isTakeoffCommand(MAV_CMD command)
{
    MissionCommandList *    commandList =   _staticCommandTree[QGCMAVLink::FirmwareClassGeneric][QGCMAVLink::VehicleClassGeneric];
    MissionCommandUIInfo*   uiInfo =        commandList->getUIInfo(command);

    return uiInfo ? uiInfo->isTakeoffCommand() : false;
}

const QList<MAV_CMD>& MissionCommandTree::allCommandIds(void) const
{
    return _staticCommandTree[QGCMAVLink::FirmwareClassGeneric][QGCMAVLink::VehicleClassGeneric]->commandIds();
}

const MissionCommandUIInfo* MissionCommandTree::getUIInfo(Vehicle* vehicle, QGCMAVLink::VehicleClass_t vtolMode,  MAV_CMD command)
{
    QGCMAVLink::FirmwareClass_t firmwareClass;
    QGCMAVLink::VehicleClass_t  vehicleClass;

    _firmwareAndVehicleClassInfo(vehicle, vtolMode, firmwareClass, vehicleClass);
    _buildAllCommands(vehicle, vtolMode);

    const QMap<MAV_CMD, MissionCommandUIInfo*>& infoMap = _allCommands[firmwareClass][vehicleClass];
    if (infoMap.contains(command)) {
        return infoMap[command];
    } else {
        return nullptr;
    }
}

QVariantList MissionCommandTree::getCommandsForCategory(Vehicle* vehicle, const QString& category, bool showFlyThroughCommands)
{
    QGCMAVLink::FirmwareClass_t firmwareClass;
    QGCMAVLink::VehicleClass_t  vehicleClass;

    _firmwareAndVehicleClassInfo(vehicle, QGCMAVLink::VehicleClassGeneric, firmwareClass, vehicleClass);
    _buildAllCommands(vehicle, QGCMAVLink::VehicleClassGeneric);

    // vehicle can be null in which case _firmwareAndVehicleClassInfo will tell of the firmware/vehicle type for the offline editing vehicle.
    // We then use that to get a firmware plugin so we can get the list of supported commands.
    FirmwarePlugin* firmwarePlugin = qgcApp()->toolbox()->firmwarePluginManager()->firmwarePluginForAutopilot(QGCMAVLink::firmwareClassToAutopilot(firmwareClass), QGCMAVLink::vehicleClassToMavType(vehicleClass));
    QList<MAV_CMD>  supportedCommands = firmwarePlugin->supportedMissionCommands(vehicleClass);

    QVariantList list;
    QMap<MAV_CMD, MissionCommandUIInfo*> commandMap = _allCommands[firmwareClass][vehicleClass];
    for (MAV_CMD command: commandMap.keys()) {
        if (supportedCommands.isEmpty() || supportedCommands.contains(command)) {
            MissionCommandUIInfo* uiInfo = commandMap[command];
            if ((uiInfo->category() == category || category == _allCommandsCategory) && (showFlyThroughCommands || !uiInfo->specifiesCoordinate() || uiInfo->isStandaloneCoordinate())) {
                list.append(QVariant::fromValue(uiInfo));
            }
        }
    }

    return list;
}

void MissionCommandTree::_firmwareAndVehicleClassInfo(Vehicle* vehicle, QGCMAVLink::VehicleClass_t vtolMode, QGCMAVLink::FirmwareClass_t& firmwareClass, QGCMAVLink::VehicleClass_t& vehicleClass) const
{
    firmwareClass = QGCMAVLink::firmwareClass(vehicle->firmwareType());
    vehicleClass = QGCMAVLink::vehicleClass(vehicle->vehicleType());
    if (vehicleClass == QGCMAVLink::VehicleClassVTOL && vtolMode != QGCMAVLink::VehicleClassGeneric) {
        vehicleClass = vtolMode;
    }
}
