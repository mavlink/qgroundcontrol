/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCToolbox.h"
#include "QGCMAVLink.h"
#include "Vehicle.h"

#include <QVariantList>
#include <QMap>

class MissionCommandUIInfo;
class MissionCommandList;
class SettingsManager;
#ifdef UNITTEST_BUILD
class MissionCommandTreeTest;
#endif

/// Manages a hierarchy of MissionCommandUIInfo.
///
/// The static hierarchy allows for overriding mission command ui info based on firmware and vehicle class. The hierarchy of the tree is:
///     FirmwareClassGeneric - VehicleClassGeneric - Base set of all command definitions for any firmware, any vehicle, ui defined by mavlink spec
///         FirmwareClassGeneric - VehicleClassFixedWing
///             Known Firmware, Fixed Wing
///         Any Firmware, Multi Rotor (all types)
///             Known Firmware, Multi Rotor (all types)
///         Any Firmware, VTOL (all types)
///             Known Firmware, VTOL (all types)
///         Any Firmware, Rover
///             Known Firmware, Rover
///         Any Firmware, Sub
///             Known Firmware, Sub
/// For known firmwares, the override files are requested from the FirmwarePlugin.
///
/// When ui info is requested for a specific vehicle the static hierarchy in _staticCommandTree is collapsed into the set of available commands in
/// _allCommands taking into account the appropriate set of overrides for the MAV_AUTOPILOT/MAV_TYPE combination associated with the vehicle.
///
class MissionCommandTree : public QGCTool
{
    Q_OBJECT
    
public:
    MissionCommandTree(QGCApplication* app, QGCToolbox* toolbox, bool unitTest = false);

    /// Returns the friendly name for the specified command
    QString friendlyName(MAV_CMD command);

    /// Returns the raw name for the specified command
    QString rawName(MAV_CMD command);

    bool isLandCommand(MAV_CMD command);
    bool isTakeoffCommand(MAV_CMD command);

    const QList<MAV_CMD>& allCommandIds(void) const;

    Q_INVOKABLE QStringList categoriesForVehicle(Vehicle* vehicle) { return _availableCategoriesForVehicle(vehicle); }

    const MissionCommandUIInfo* getUIInfo(Vehicle* vehicle, QGCMAVLink::VehicleClass_t vtolMode, MAV_CMD command);

    /// @param showFlyThroughCommands - true: all commands shows, false: filter out commands which the vehicle flies through (specifiedCoordinate=true, standaloneCoordinate=false)
    Q_INVOKABLE QVariantList getCommandsForCategory(Vehicle* vehicle, const QString& category, bool showFlyThroughCommands);

    // Overrides from QGCTool
    virtual void setToolbox(QGCToolbox* toolbox);

private:
    void                        _collapseHierarchy              (const MissionCommandList* cmdList, QMap<MAV_CMD, MissionCommandUIInfo*>& collapsedTree);
    void                        _buildAllCommands               (Vehicle* vehicle, QGCMAVLink::VehicleClass_t vtolMode);
    QStringList                 _availableCategoriesForVehicle  (Vehicle* vehicle);
    void                        _firmwareAndVehicleClassInfo    (Vehicle* vehicle, QGCMAVLink::VehicleClass_t vtolMode, QGCMAVLink::FirmwareClass_t& firmwareClass, QGCMAVLink::VehicleClass_t& vehicleClass) const;

private:
    QString             _allCommandsCategory;   ///< Category which contains all available commands
    QList<int>          _allCommandIds;         ///< List of all known command ids (not vehicle specific)
    SettingsManager*    _settingsManager;
    bool                _unitTest;              ///< true: running in unit test mode

    /// Full hierarchy
    QMap<QGCMAVLink::FirmwareClass_t, QMap<QGCMAVLink::VehicleClass_t, MissionCommandList*>>                    _staticCommandTree;

    /// Collapsed hierarchy for specific vehicle type
    QMap<QGCMAVLink::FirmwareClass_t, QMap<QGCMAVLink::VehicleClass_t, QMap<MAV_CMD, MissionCommandUIInfo*>>>   _allCommands;

    /// Collapsed hierarchy for specific vehicle type
    QMap<QGCMAVLink::FirmwareClass_t, QMap<QGCMAVLink::VehicleClass_t, QStringList /* category */>>             _supportedCategories;


#ifdef UNITTEST_BUILD
    friend class MissionCommandTreeTest;
#endif
};
