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

#ifndef MissionCommands_H
#define MissionCommands_H

#include "QGCToolbox.h"
#include "MissionCommandList.h"

/// Provides access to mission command information used for creating mission command ui editors. There is a base common set
/// of definitions. Individual commands can then be overriden depending on Vehicle information:
///     Common command definitions
///         MAV_AUTOPILOT common overrides
///         Fixed Wing
///             MAV_AUTOPILOT specific Fixed Wing overrides
///         Multi-Rotor
///             MAV_AUTOPILOT specific Multi Rotor overrides
/// The leaf nodes of the hierarchy take precedence over higher level branches
class MissionCommands : public QGCTool
{
    Q_OBJECT
    
public:
    MissionCommands(QGCApplication* app);

    Q_INVOKABLE const QStringList           categories              (Vehicle* vehicle) const;
    Q_INVOKABLE QString                     categoryFromCommand     (MavlinkQmlSingleton::Qml_MAV_CMD command) const;
    Q_INVOKABLE QVariant                    getCommandsForCategory  (Vehicle* vehicle, const QString& category) const;

    Q_INVOKABLE bool contains(MavlinkQmlSingleton::Qml_MAV_CMD command) const { return contains((MAV_CMD)command); }
    bool contains(MAV_CMD command) const;

    Q_INVOKABLE QVariant getMavCmdInfo(MavlinkQmlSingleton::Qml_MAV_CMD command, Vehicle* vehicle) const { return QVariant::fromValue(getMavCmdInfo((MAV_CMD)command, vehicle)); }
    MavCmdInfo* getMavCmdInfo(MAV_CMD command, Vehicle* vehicle) const;

    QList<MAV_CMD> commandsIds(void) const;

    // Overrides from QGCTool
    virtual void setToolbox(QGCToolbox* toolbox);

private:
    void _createCategories(void);
    MAV_AUTOPILOT _firmwareTypeFromVehicle(Vehicle* vehicle) const;

private:
    QMap<MAV_AUTOPILOT, QMap<QString, QList<MAV_CMD> > > _categoryToMavCmdListMap;

    MissionCommandList  _commonMissionCommands;                                     ///< Mission command definitions for common generic mavlink use case

    QMap<MAV_AUTOPILOT, MissionCommandList*> _autopilotToCommonMissionCommands;     ///< MAV_AUTOPILOT specific common overrides
    QMap<MAV_AUTOPILOT, MissionCommandList*> _autopilotToFixedWingMissionCommands;  ///< MAV_AUTOPILOT specific fixed wing overrides
    QMap<MAV_AUTOPILOT, MissionCommandList*> _autopilotToMultiRotorMissionCommands; ///< MAV_AUTOPILOT specific multi rotor overrides
};

#endif
