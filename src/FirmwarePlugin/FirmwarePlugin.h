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

#ifndef FirmwarePlugin_H
#define FirmwarePlugin_H

#include "QGCMAVLink.h"
#include "VehicleComponent.h"
#include "AutoPilotPlugin.h"

#include <QList>
#include <QString>

class Vehicle;

/// This is the base class for Firmware specific plugins
///
/// The FirmwarePlugin class is the abstract base class which represents the methods and objects
/// which are specific to a certain Firmware flight stack. This is the only place where
/// flight stack specific code should reside in QGroundControl. The remainder of the
/// QGroundControl source is generic to a common mavlink implementation. The implementation
/// in the base class supports mavlink generic firmware. Override the base clase virtuals
/// to create you firmware specific plugin.

class FirmwarePlugin : public QObject
{
    Q_OBJECT

public:
    /// Set of optional capabilites which firmware may support
    typedef enum {
        SetFlightModeCapability,            ///< FirmwarePlugin::setFlightMode method is supported
        MavCmdPreflightStorageCapability,   ///< MAV_CMD_PREFLIGHT_STORAGE is supported
        
    } FirmwareCapabilities;
    
    /// @return true: Firmware supports all specified capabilites
    virtual bool isCapable(FirmwareCapabilities capabilities) = 0;
    
    /// Returns VehicleComponents for specified Vehicle
    ///     @param vehicle Vehicle  to associate with components
    /// @return List of VehicleComponents for the specified vehicle. Caller owns returned objects and must
    ///         free when no longer needed.
    virtual QList<VehicleComponent*> componentsForVehicle(AutoPilotPlugin* vehicle) = 0;
    
    /// Returns the list of available flight modes
    virtual QStringList flightModes(void) = 0;
    
    /// Returns the name for this flight mode. Flight mode names must be human readable as well as audio speakable.
    ///     @param base_mode Base mode from mavlink HEARTBEAT message
    ///     @param custom_mode Custom mode from mavlink HEARTBEAT message
    virtual QString flightMode(uint8_t base_mode, uint32_t custom_mode) = 0;
    
    /// Sets base_mode and custom_mode to specified flight mode.
    ///     @param[out] base_mode Base mode for SET_MODE mavlink message
    ///     @param[out] custom_mode Custom mode for SET_MODE mavlink message
    virtual bool setFlightMode(const QString& flightMode, uint8_t* base_mode, uint32_t* custom_mode) = 0;
    
    /// FIXME: This isn't quite correct being here. All code for Joystick suvehicleTypepport is currently firmware specific
    /// not just this. I'm going to try to change that. If not, this will need to be removed.
    /// Returns the number of buttons which are reserved for firmware use in the MANUAL_CONTROL mavlink
    /// message. For example PX4 Flight Stack reserves the first 8 buttons to simulate rc switches.
    /// The remainder can be assigned to Vehicle actions.
    /// @return -1: reserver all buttons, >0 number of buttons to reserve
    virtual int manualControlReservedButtonCount(void) = 0;
    
    /// Called before any mavlink message is processed by Vehicle such that the firmwre plugin
    /// can adjust any message characteristics. This is handy to adjust or differences in mavlink
    /// spec implementations such that the base code can remain mavlink generic.
    ///     @param vehicle Vehicle message came from
    ///     @param message[in,out] Mavlink message to adjust if needed.
    virtual void adjustMavlinkMessage(Vehicle* vehicle, mavlink_message_t* message) = 0;
    
    /// Called when Vehicle is first created to send any necessary mavlink messages to the firmware.
    virtual void initializeVehicle(Vehicle* vehicle) = 0;

    /// Determines how to handle the first item of the mission item list. Internally to QGC the first item
    /// is always the home position.
    /// @return
    ///     true: Send first mission item as home position to vehicle. When vehicle has no mission items on
    ///             it, it may or may not return a home position back in position 0.
    ///     false: Do not send first item to vehicle, sequence numbers must be adjusted
    virtual bool sendHomePositionToVehicle(void) = 0;
    
    /// Returns the parameter that is used to identify the default component
    virtual QString getDefaultComponentIdParam(void) const = 0;

    /// Adds the parameter meta data to the Fact
    virtual void addMetaDataToFact(Fact* fact, MAV_TYPE vehicleType) = 0;

    /// List of supported mission commands. Empty list for all commands supported.
    virtual QList<MAV_CMD> supportedMissionCommands(void) = 0;
};

#endif
