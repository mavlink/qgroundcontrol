/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef FirmwarePlugin_H
#define FirmwarePlugin_H

#include "QGCMAVLink.h"
#include "VehicleComponent.h"
#include "AutoPilotPlugin.h"
#include "GeoFenceManager.h"
#include "RallyPointManager.h"

#include <QList>
#include <QString>

class Vehicle;

/// This is the base class for Firmware specific plugins
///
/// The FirmwarePlugin class represents the methods and objects which are specific to a certain Firmware flight stack.
/// This is the only place where flight stack specific code should reside in QGroundControl. The remainder of the
/// QGroundControl source is generic to a common mavlink implementation. The implementation in the base class supports
/// mavlink generic firmware. Override the base clase virtuals to create your own firmware specific plugin.

class FirmwarePlugin : public QObject
{
    Q_OBJECT

public:
    /// Set of optional capabilites which firmware may support
    typedef enum {
        SetFlightModeCapability =           1 << 0, ///< FirmwarePlugin::setFlightMode method is supported
        MavCmdPreflightStorageCapability =  1 << 1, ///< MAV_CMD_PREFLIGHT_STORAGE is supported
        PauseVehicleCapability =            1 << 2, ///< Vehicle supports pausing at current location
        GuidedModeCapability =              1 << 3, ///< Vehicle supports guided mode commands
        OrbitModeCapability =               1 << 4, ///< Vehicle supports orbit mode
    } FirmwareCapabilities;

    /// Maps from on parameter name to another
    ///     key:    parameter name to translate from
    ///     value:  mapped parameter name
    typedef QMap<QString, QString> remapParamNameMap_t;

    /// Maps from firmware minor version to remapParamNameMap_t entry
    ///     key:    firmware minor version
    ///     value:  remapParamNameMap_t entry
    typedef QMap<int, remapParamNameMap_t> remapParamNameMinorVersionRemapMap_t;

    /// Maps from firmware major version number to remapParamNameMinorVersionRemapMap_t entry
    ///     key:    firmware major version
    ///     value:  remapParamNameMinorVersionRemapMap_t entry
    typedef QMap<int, remapParamNameMinorVersionRemapMap_t> remapParamNameMajorVersionMap_t;

    /// @return The AutoPilotPlugin associated with this firmware plugin. Must be overriden.
    virtual AutoPilotPlugin* autopilotPlugin(Vehicle* vehicle);

    /// Called when Vehicle is first created to perform any firmware specific setup.
    virtual void initializeVehicle(Vehicle* vehicle);

    /// @return true: Firmware supports all specified capabilites
    virtual bool isCapable(const Vehicle *vehicle, FirmwareCapabilities capabilities);

    /// Returns VehicleComponents for specified Vehicle
    ///     @param vehicle Vehicle  to associate with components
    /// @return List of VehicleComponents for the specified vehicle. Caller owns returned objects and must
    ///         free when no longer needed.
    virtual QList<VehicleComponent*> componentsForVehicle(AutoPilotPlugin* vehicle);

    /// Returns the list of available flight modes
    virtual QStringList flightModes(Vehicle* vehicle) {
        Q_UNUSED(vehicle);
        return QStringList();
    }

    /// Returns the name for this flight mode. Flight mode names must be human readable as well as audio speakable.
    ///     @param base_mode Base mode from mavlink HEARTBEAT message
    ///     @param custom_mode Custom mode from mavlink HEARTBEAT message
    virtual QString flightMode(uint8_t base_mode, uint32_t custom_mode) const;

    /// Sets base_mode and custom_mode to specified flight mode.
    ///     @param[out] base_mode Base mode for SET_MODE mavlink message
    ///     @param[out] custom_mode Custom mode for SET_MODE mavlink message
    virtual bool setFlightMode(const QString& flightMode, uint8_t* base_mode, uint32_t* custom_mode);

    /// Returns whether the vehicle is in guided mode or not.
    virtual bool isGuidedMode(const Vehicle* vehicle) const;

    /// Set guided flight mode
    virtual void setGuidedMode(Vehicle* vehicle, bool guidedMode);

    /// Returns whether the vehicle is paused or not.
    virtual bool isPaused(const Vehicle* vehicle) const;

    /// Causes the vehicle to stop at current position. If guide mode is supported, vehicle will be let in guide mode.
    /// If not, vehicle will be left in Loiter.
    virtual void pauseVehicle(Vehicle* vehicle);

    /// Command vehicle to return to launch
    virtual void guidedModeRTL(Vehicle* vehicle);

    /// Command vehicle to land at current location
    virtual void guidedModeLand(Vehicle* vehicle);

    /// Command vehicle to takeoff from current location
    ///     @param altitudeRel Relative altitude to takeoff to
    virtual void guidedModeTakeoff(Vehicle* vehicle, double altitudeRel);

    /// Command vehicle to orbit given center point
    ///     @param centerCoord Center Coordinates
    virtual void guidedModeOrbit(Vehicle* vehicle, const QGeoCoordinate& centerCoord, double radius, double velocity, double altitude);

    /// Command vehicle to move to specified location (altitude is included and relative)
    virtual void guidedModeGotoLocation(Vehicle* vehicle, const QGeoCoordinate& gotoCoord);

    /// Command vehicle to change to the specified relatice altitude
    virtual void guidedModeChangeAltitude(Vehicle* vehicle, double altitudeRel);

    /// Returns the flight mode for running missions
    virtual QString missionFlightMode(void);

    /// Returns the flight mode for RTL
    virtual QString rtlFlightMode(void);

    /// Returns the flight mode to use when the operator wants to take back control from autonomouse flight.
    virtual QString takeControlFlightMode(void);

    /// FIXME: This isn't quite correct being here. All code for Joystick suvehicleTypepport is currently firmware specific
    /// not just this. I'm going to try to change that. If not, this will need to be removed.
    /// Returns the number of buttons which are reserved for firmware use in the MANUAL_CONTROL mavlink
    /// message. For example PX4 Flight Stack reserves the first 8 buttons to simulate rc switches.
    /// The remainder can be assigned to Vehicle actions.
    /// @return -1: reserver all buttons, >0 number of buttons to reserve
    virtual int manualControlReservedButtonCount(void);

    /// Returns true if the vehicle and firmware supports the use of a throttle joystick that
    /// is zero when centered. Typically not supported on vehicles that have bidirectional
    /// throttle.
    virtual bool supportsThrottleModeCenterZero(void);

    /// Returns true if the firmware supports the use of the MAVlink "MANUAL_CONTROL" message.
    /// By default, this returns false unless overridden in the firmware plugin.
    virtual bool supportsManualControl(void);

    /// Returns true if the firmware supports the use of the RC radio and requires the RC radio
    /// setup page. Returns true by default.
    virtual bool supportsRadio(void);

    /// Returns true if the firmware supports the AP_JSButton library, which allows joystick buttons
    /// to be assigned via parameters in firmware. Default is false.
    virtual bool supportsJSButton(void);

    /// Called before any mavlink message is processed by Vehicle such that the firmwre plugin
    /// can adjust any message characteristics. This is handy to adjust or differences in mavlink
    /// spec implementations such that the base code can remain mavlink generic.
    ///     @param vehicle Vehicle message came from
    ///     @param message[in,out] Mavlink message to adjust if needed.
    /// @return false: skip message, true: process message
    virtual bool adjustIncomingMavlinkMessage(Vehicle* vehicle, mavlink_message_t* message);

    /// Called before any mavlink message is sent to the Vehicle so plugin can adjust any message characteristics.
    /// This is handy to adjust or differences in mavlink spec implementations such that the base code can remain
    /// mavlink generic.
    ///     @param vehicle Vehicle message came from
    ///     @param outgoingLink Link that messae is going out on
    ///     @param message[in,out] Mavlink message to adjust if needed.
    virtual void adjustOutgoingMavlinkMessage(Vehicle* vehicle, LinkInterface* outgoingLink, mavlink_message_t* message);

    /// Determines how to handle the first item of the mission item list. Internally to QGC the first item
    /// is always the home position.
    /// @return
    ///     true: Send first mission item as home position to vehicle. When vehicle has no mission items on
    ///             it, it may or may not return a home position back in position 0.
    ///     false: Do not send first item to vehicle, sequence numbers must be adjusted
    virtual bool sendHomePositionToVehicle(void);

    /// Returns the parameter that is used to identify the default component
    virtual QString getDefaultComponentIdParam(void) const { return QString(); }

    /// Returns the parameter which is used to identify the version number of parameter set
    virtual QString getVersionParam(void) { return QString(); }

    /// Returns the parameter set version info pulled from inside the meta data file. -1 if not found.
    virtual void getParameterMetaDataVersionInfo(const QString& metaDataFile, int& majorVersion, int& minorVersion);

    /// Returns the internal resource parameter meta date file.
    virtual QString internalParameterMetaDataFile(Vehicle* vehicle) { Q_UNUSED(vehicle); return QString(); }

    /// Loads the specified parameter meta data file.
    /// @return Opaque parameter meta data information which must be stored with Vehicle. Vehicle is responsible to
    ///         call deleteParameterMetaData when no longer needed.
    virtual QObject* loadParameterMetaData(const QString& metaDataFile) { Q_UNUSED(metaDataFile); return NULL; }

    /// Adds the parameter meta data to the Fact
    ///     @param opaqueParameterMetaData Opaque pointer returned from loadParameterMetaData
    virtual void addMetaDataToFact(QObject* parameterMetaData, Fact* fact, MAV_TYPE vehicleType) { Q_UNUSED(parameterMetaData); Q_UNUSED(fact); Q_UNUSED(vehicleType); return; }

    /// List of supported mission commands. Empty list for all commands supported.
    virtual QList<MAV_CMD> supportedMissionCommands(void);

    /// Returns the name of the mission command json override file for the specified vehicle type.
    ///     @param vehicleType Vehicle type to return file for, MAV_TYPE_GENERIC is a request for overrides for all vehicle types
    virtual QString missionCommandOverrides(MAV_TYPE vehicleType) const;

    /// Returns the mapping structure which is used to map from one parameter name to another based on firmware version.
    virtual const remapParamNameMajorVersionMap_t& paramNameRemapMajorVersionMap(void) const;

    /// Returns the highest major version number that is known to the remap for this specified major version.
    virtual int remapParamNameHigestMinorVersionNumber(int majorVersionNumber) const;

    /// @return true: Motors are coaxial like an X8 config, false: Quadcopter for example
    virtual bool multiRotorCoaxialMotors(Vehicle* vehicle) { Q_UNUSED(vehicle); return false; }

    /// @return true: X confiuration, false: Plus configuration
    virtual bool multiRotorXConfig(Vehicle* vehicle) { Q_UNUSED(vehicle); return false; }

    /// Returns a newly created geofence manager for this vehicle.
    virtual GeoFenceManager* newGeoFenceManager(Vehicle* vehicle) { return new GeoFenceManager(vehicle); }

    /// Returns the parameter which holds the fence circle radius if supported.
    virtual QString geoFenceRadiusParam(Vehicle* vehicle) { Q_UNUSED(vehicle); return QString(); }

    /// Returns a newly created rally point manager for this vehicle.
    virtual RallyPointManager* newRallyPointManager(Vehicle* vehicle) { return new RallyPointManager(vehicle); }

    /// Return the resource file which contains the set of params loaded for offline editing.
    virtual QString offlineEditingParamFile(Vehicle* vehicle) { Q_UNUSED(vehicle); return QString(); }

    /// Return the resource file which contains the brand image for the vehicle.
    virtual QString brandImage(const Vehicle* vehicle) const { Q_UNUSED(vehicle) return QString(); }

    // FIXME: Hack workaround for non pluginize FollowMe support
    static const char* px4FollowMeFlightMode;
};

class FirmwarePluginFactory : public QObject
{
    Q_OBJECT

public:
    FirmwarePluginFactory(void);

    /// Returns appropriate plugin for autopilot type.
    ///     @param autopilotType Type of autopilot to return plugin for.
    ///     @param vehicleType Vehicle type of autopilot to return plugin for.
    /// @return Singleton FirmwarePlugin instance for the specified MAV_AUTOPILOT.
    virtual FirmwarePlugin* firmwarePluginForAutopilot(MAV_AUTOPILOT autopilotType, MAV_TYPE vehicleType) = 0;

    /// @return List of autopilot types this plugin supports.
    virtual QList<MAV_AUTOPILOT> knownFirmwareTypes(void) const = 0;
};

class FirmwarePluginFactoryRegister : public QObject
{
    Q_OBJECT

public:
    static FirmwarePluginFactoryRegister* instance(void);

    /// Registers the specified logging category to the system.
    void registerPluginFactory(FirmwarePluginFactory* pluginFactory) { _factoryList.append(pluginFactory); }

    QList<FirmwarePluginFactory*> pluginFactories(void) const { return _factoryList; }

private:
    QList<FirmwarePluginFactory*> _factoryList;
};

#endif
