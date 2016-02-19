/*===================================================================
QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

#include "MissionCommands.h"
#include "FactMetaData.h"
#include "Vehicle.h"
#include "FirmwarePluginManager.h"
#include "QGCApplication.h"
#include "QGroundControlQmlGlobal.h"

#include <QQmlEngine>

MissionCommands::MissionCommands(QGCApplication* app)
    : QGCTool(app)
    , _commonMissionCommands(QStringLiteral(":/json/MavCmdInfoCommon.json"))
{
}

void MissionCommands::setToolbox(QGCToolbox* toolbox)
{
    QGCTool::setToolbox(toolbox);

    // Setup overrides
    QString overrideCommonJsonFilename;
    QString overrideFixedWingJsonFilename;
    QString overrideMultiRotorJsonFilename;

    QList<MAV_AUTOPILOT> firmwareList;
    firmwareList << MAV_AUTOPILOT_GENERIC << MAV_AUTOPILOT_PX4 << MAV_AUTOPILOT_ARDUPILOTMEGA;

    foreach (MAV_AUTOPILOT firmwareType, firmwareList) {
        FirmwarePlugin* plugin = _toolbox->firmwarePluginManager()->firmwarePluginForAutopilot(firmwareType, MAV_TYPE_QUADROTOR);

        plugin->missionCommandOverrides(overrideCommonJsonFilename, overrideFixedWingJsonFilename, overrideMultiRotorJsonFilename);
        _autopilotToCommonMissionCommands[firmwareType] =       new MissionCommandList(overrideCommonJsonFilename);
        _autopilotToFixedWingMissionCommands[firmwareType] =    new MissionCommandList(overrideFixedWingJsonFilename);
        _autopilotToMultiRotorMissionCommands[firmwareType] =   new MissionCommandList(overrideMultiRotorJsonFilename);
    }

    _createCategories();
}

/// Create category hierarchy for support commands
void MissionCommands::_createCategories(void)
{
    // FIXME: This isn't quite right since it's hardcoding the firmware providers. But ok for now.
    QList<MAV_AUTOPILOT>    firmwareList;
    firmwareList << MAV_AUTOPILOT_GENERIC << MAV_AUTOPILOT_PX4 << MAV_AUTOPILOT_ARDUPILOTMEGA;

    foreach (MAV_AUTOPILOT firmwareType, firmwareList) {
        FirmwarePlugin* plugin = _toolbox->firmwarePluginManager()->firmwarePluginForAutopilot(firmwareType, MAV_TYPE_QUADROTOR);

        bool allCommandsSupported = false;
        QList<MAV_CMD> cmdList = plugin->supportedMissionCommands();
        if (cmdList.isEmpty()) {
            allCommandsSupported = true;
            cmdList = _commonMissionCommands.commandsIds();
        }

        foreach (MAV_CMD command, cmdList) {
            MavCmdInfo* mavCmdInfo = _commonMissionCommands.getMavCmdInfo(command);

            if (mavCmdInfo) {
                if (mavCmdInfo->friendlyEdit()) {
                    _categoryToMavCmdListMap[firmwareType][mavCmdInfo->category()].append(command);
                } else if (!allCommandsSupported) {
                    qWarning() << "Attempt to add non friendly edit supported command" << command;
                }
            } else {
                qCDebug(MissionCommandsLog) << "Command missing from json" << command;
            }
        }
    }

}

MAV_AUTOPILOT MissionCommands::_firmwareTypeFromVehicle(Vehicle* vehicle) const
{
    if (vehicle) {
        return vehicle->firmwareType();
    } else {
        QSettings settings;

        // FIXME: Hack duplicated code from QGroundControlQmlGlobal. Had to do this for now since
        // QGroundControlQmlGlobal is not available from C++ side.

        return (MAV_AUTOPILOT)settings.value("OfflineEditingFirmwareType", MAV_AUTOPILOT_ARDUPILOTMEGA).toInt();
    }
}

QString MissionCommands::categoryFromCommand(MavlinkQmlSingleton::Qml_MAV_CMD command) const
{
    return _commonMissionCommands.getMavCmdInfo((MAV_CMD)command)->category();
}

QVariant MissionCommands::getCommandsForCategory(Vehicle* vehicle, const QString& category) const
{
    QmlObjectListModel* list = new QmlObjectListModel();
    QQmlEngine::setObjectOwnership(list, QQmlEngine::JavaScriptOwnership);

    foreach (MAV_CMD command, _categoryToMavCmdListMap[_firmwareTypeFromVehicle(vehicle)][category]) {
        list->append(getMavCmdInfo(command, vehicle));
    }

    return QVariant::fromValue(list);
}

const QStringList MissionCommands::categories(Vehicle* vehicle) const
{
    QStringList list;

    foreach (const QString &category, _categoryToMavCmdListMap[_firmwareTypeFromVehicle(vehicle)].keys()) {
        list << category;
    }

    return list;
}

bool MissionCommands::contains(MAV_CMD command) const
{
    return _commonMissionCommands.contains(command);
}

MavCmdInfo* MissionCommands::getMavCmdInfo(MAV_CMD command, Vehicle* vehicle) const
{
    if (!contains(command)) {
        qWarning() << "Unknown command" << command;
        return NULL;
    }

    MavCmdInfo*     mavCmdInfo = NULL;
    MAV_AUTOPILOT   firmwareType = _firmwareTypeFromVehicle(vehicle);

    if (vehicle) {
        if (vehicle->fixedWing()) {
            if (_autopilotToFixedWingMissionCommands.contains(firmwareType) && _autopilotToFixedWingMissionCommands[firmwareType]->contains(command)) {
                mavCmdInfo = _autopilotToFixedWingMissionCommands[firmwareType]->getMavCmdInfo(command);
            }
        } else if (vehicle->multiRotor()) {
            if (_autopilotToMultiRotorMissionCommands.contains(firmwareType) && _autopilotToMultiRotorMissionCommands[firmwareType]->contains(command)) {
                mavCmdInfo = _autopilotToMultiRotorMissionCommands[firmwareType]->getMavCmdInfo(command);
            }
        }
    }

    if (!mavCmdInfo && _autopilotToCommonMissionCommands.contains(firmwareType) && _autopilotToCommonMissionCommands[firmwareType]->contains(command)) {
        mavCmdInfo = _autopilotToCommonMissionCommands[firmwareType]->getMavCmdInfo(command);
    }

    if (!mavCmdInfo) {
        mavCmdInfo = _commonMissionCommands.getMavCmdInfo(command);
    }

    return mavCmdInfo;
}

QList<MAV_CMD> MissionCommands::commandsIds(void) const
{
    return _commonMissionCommands.commandsIds();
}
