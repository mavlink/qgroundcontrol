/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCMAVLink.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QVariantList>
#include <QtQmlIntegration/QtQmlIntegration>

class MissionCommandList;
class MissionCommandTreeTest;
class MissionCommandUIInfo;
class Vehicle;

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
class MissionCommandTree : public QObject
{
    Q_OBJECT
    // QML_ELEMENT
    // QML_UNCREATABLE("")
    Q_MOC_INCLUDE("Vehicle.h")

    friend class MissionCommandTreeTest;

public:
    /// Constructs an MissionCommandTree object.
    ///     @param unitTest Unit Tests are Running.
    ///     @param parent The parent QObject.
    explicit MissionCommandTree(bool unitTest = false, QObject *parent = nullptr);

    /// Destructor for the MissionCommandTree class.
    ~MissionCommandTree();

    /// Gets the singleton instance of MissionCommandTree.
    ///     @return The singleton instance.
    static MissionCommandTree *instance();

    Q_INVOKABLE QStringList categoriesForVehicle(Vehicle *vehicle);

    /// @param showFlyThroughCommands - true: all commands shows, false: filter out commands which the vehicle flies through (specifiedCoordinate=true, standaloneCoordinate=false)
    Q_INVOKABLE QVariantList getCommandsForCategory(Vehicle *vehicle, const QString &category, bool showFlyThroughCommands);

    /// Returns the friendly name for the specified command
    QString friendlyName(MAV_CMD command) const;

    /// Returns the raw name for the specified command
    QString rawName(MAV_CMD command) const;

    bool isLandCommand(MAV_CMD command) const;
    bool isTakeoffCommand(MAV_CMD command) const;

    const QList<MAV_CMD> &allCommandIds() const;

    const MissionCommandUIInfo *getUIInfo(Vehicle *vehicle, QGCMAVLink::VehicleClass_t vtolMode, MAV_CMD command);

private:
    /// Add the next level of the hierarchy to a collapsed tree.
    ///     @param cmdList          List of mission commands to collapse into ui info
    ///     @param collapsedTree    Tree we are collapsing into
    void _collapseHierarchy(const MissionCommandList *cmdList, QMap<MAV_CMD, MissionCommandUIInfo*> &collapsedTree) const;
    void _buildAllCommands(Vehicle *vehicle, QGCMAVLink::VehicleClass_t vtolMode);
    QStringList _availableCategoriesForVehicle(Vehicle *vehicle);
    void _firmwareAndVehicleClassInfo(Vehicle *vehicle, QGCMAVLink::VehicleClass_t vtolMode, QGCMAVLink::FirmwareClass_t &firmwareClass, QGCMAVLink::VehicleClass_t &vehicleClass) const;

    const QString _allCommandsCategory = tr("All commands");    ///< Category which contains all available commands

    /// Full hierarchy
    QMap<QGCMAVLink::FirmwareClass_t, QMap<QGCMAVLink::VehicleClass_t, MissionCommandList*>> _staticCommandTree;

    /// Collapsed hierarchy for specific vehicle type
    QMap<QGCMAVLink::FirmwareClass_t, QMap<QGCMAVLink::VehicleClass_t, QMap<MAV_CMD, MissionCommandUIInfo*>>> _allCommands;

    /// Collapsed hierarchy for specific vehicle type
    QMap<QGCMAVLink::FirmwareClass_t, QMap<QGCMAVLink::VehicleClass_t, QStringList /* category */>> _supportedCategories;
};
