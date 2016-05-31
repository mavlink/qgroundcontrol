/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
