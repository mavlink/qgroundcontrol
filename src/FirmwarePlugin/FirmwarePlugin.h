/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
#include "FollowMe.h"

#include <QList>
#include <QString>
#include <QVariantList>

class Vehicle;
class QGCCameraControl;
class QGCCameraManager;

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
        PauseVehicleCapability =            1 << 1, ///< Vehicle supports pausing at current location
        GuidedModeCapability =              1 << 2, ///< Vehicle supports guided mode commands
        OrbitModeCapability =               1 << 3, ///< Vehicle supports orbit mode
        TakeoffVehicleCapability =          1 << 4, ///< Vehicle supports guided takeoff
        ROIModeCapability =                 1 << 5, ///< Vehicle supports ROI
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

    /// @return The AutoPilotPlugin associated with this firmware plugin. Must be overridden.
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

    /// Returns the list of available flight modes for the Fly View dropdown. This may or may not be the full
    /// list available from the firmware. Call will be made again if advanced mode changes.
    virtual QStringList flightModes(Vehicle* /*vehicle*/) { return QStringList(); }

    /// Returns the list of additional flight modes to add to the list for joystick button actions.
    /// Call will be made again if advanced mode changes.
    virtual QStringList extraJoystickFlightModes(Vehicle* /*vehicle*/) { return QStringList(); }

    /// Returns the name for this flight mode. Flight mode names must be human readable as well as audio speakable.
    ///     @param base_mode Base mode from mavlink HEARTBEAT message
    ///     @param custom_mode Custom mode from mavlink HEARTBEAT message
    virtual QString flightMode(uint8_t base_mode, uint32_t custom_mode) const;

    /// Sets base_mode and custom_mode to specified flight mode.
    ///     @param[out] base_mode Base mode for SET_MODE mavlink message
    ///     @param[out] custom_mode Custom mode for SET_MODE mavlink message
    virtual bool setFlightMode(const QString& flightMode, uint8_t* base_mode, uint32_t* custom_mode);

    /// Returns The flight mode which indicates the vehicle is paused
    virtual QString pauseFlightMode(void) const { return QString(); }

    /// Returns the flight mode for running missions
    virtual QString missionFlightMode(void) const { return QString(); }

    /// Returns the flight mode for RTL
    virtual QString rtlFlightMode(void) const { return QString(); }

    /// Returns the flight mode for Smart RTL
    virtual QString smartRTLFlightMode(void) const { return QString(); }

    virtual bool supportsSmartRTL(void) const { return false; }

    /// Returns the flight mode for Land
    virtual QString landFlightMode(void) const { return QString(); }

    /// Returns the flight mode to use when the operator wants to take back control from autonomouse flight.
    virtual QString takeControlFlightMode(void) const { return QString(); }

    /// Returns whether the vehicle is in guided mode or not.
    virtual bool isGuidedMode(const Vehicle* vehicle) const;

    /// Returns the flight mode which the vehicle will be in if it is performing a goto location
    virtual QString gotoFlightMode(void) const;

    /// Returns the flight mode which the vehicle will be for follow me
    virtual QString followFlightMode(void) const { return QString(); }

    /// Set guided flight mode
    virtual void setGuidedMode(Vehicle* vehicle, bool guidedMode);

    /// Causes the vehicle to stop at current position. If guide mode is supported, vehicle will be let in guide mode.
    /// If not, vehicle will be left in Loiter.
    virtual void pauseVehicle(Vehicle* vehicle);

    /// Command vehicle to return to launch
    virtual void guidedModeRTL(Vehicle* vehicle, bool smartRTL);

    /// Command vehicle to land at current location
    virtual void guidedModeLand(Vehicle* vehicle);

    /// Command vehicle to takeoff from current location to a firmware specific height.
    virtual void guidedModeTakeoff(Vehicle* vehicle, double takeoffAltRel);

    /// @return The minimum takeoff altitude (relative) for guided takeoff.
    virtual double minimumTakeoffAltitude(Vehicle* /*vehicle*/) { return 10; }

    /// Command the vehicle to start the mission
    virtual void startMission(Vehicle* vehicle);

    /// Command vehicle to move to specified location (altitude is included and relative)
    virtual void guidedModeGotoLocation(Vehicle* vehicle, const QGeoCoordinate& gotoCoord);

    /// Command vehicle to change altitude
    ///     @param altitudeChange If > 0, go up by amount specified, if < 0, go down by amount specified
    virtual void guidedModeChangeAltitude(Vehicle* vehicle, double altitudeChange);

    /// Default tx mode to apply to joystick axes
    /// TX modes are as outlined here: http://www.rc-airplane-world.com/rc-transmitter-modes.html
    virtual int defaultJoystickTXMode(void);

    /// Returns true if the vehicle and firmware supports the use of a throttle joystick that
    /// is zero when centered. Typically not supported on vehicles that have bidirectional
    /// throttle.
    virtual bool supportsThrottleModeCenterZero(void);

    /// Returns true if the vehicle and firmware supports the use of negative thrust
    /// Typically supported rover.
    virtual bool supportsNegativeThrust(Vehicle *);

    /// Returns true if the firmware supports the use of the RC radio and requires the RC radio
    /// setup page. Returns true by default.
    virtual bool supportsRadio(void);

    /// Returns true if the firmware supports the AP_JSButton library, which allows joystick buttons
    /// to be assigned via parameters in firmware. Default is false.
    virtual bool supportsJSButton(void);

    /// Returns true if the firmware supports calibrating motor interference offsets for the compass
    /// (CompassMot). Default is true.
    virtual bool supportsMotorInterference(void);

    /// Returns true if the firmware supports MAV_FRAME_GLOBAL_TERRAIN_ALT
    virtual bool supportsTerrainFrame(void) const;

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

    /// Returns the parameter which is used to identify the version number of parameter set
    virtual QString getVersionParam(void) { return QString(); }

    /// Returns the parameter set version info pulled from inside the meta data file. -1 if not found.
    /// Note: The implementation for this must not vary by vehicle type.
    virtual void getParameterMetaDataVersionInfo(const QString& metaDataFile, int& majorVersion, int& minorVersion);

    /// Returns the internal resource parameter meta date file.
    virtual QString internalParameterMetaDataFile(Vehicle* /*vehicle*/) { return QString(); }

    /// Loads the specified parameter meta data file.
    /// @return Opaque parameter meta data information which must be stored with Vehicle. Vehicle is responsible to
    ///         call deleteParameterMetaData when no longer needed.
    virtual QObject* loadParameterMetaData(const QString& /*metaDataFile*/) { return nullptr; }

    /// Returns the FactMetaData associated with the parameter name
    ///     @param opaqueParameterMetaData Opaque pointer returned from loadParameterMetaData
    ///     @param name Parameter name
    virtual FactMetaData* getMetaDataForFact(QObject* /*parameterMetaData*/, const QString& /*name*/, MAV_TYPE /*vehicleType*/) { return nullptr; }

    /// Adds the parameter meta data to the Fact
    ///     @param opaqueParameterMetaData Opaque pointer returned from loadParameterMetaData
    virtual void addMetaDataToFact(QObject* /*parameterMetaData*/, Fact* /*fact*/, MAV_TYPE /*vehicleType*/) { return; }

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
    virtual bool multiRotorCoaxialMotors(Vehicle* /*vehicle*/) { return false; }

    /// @return true: X confiuration, false: Plus configuration
    virtual bool multiRotorXConfig(Vehicle* /*vehicle*/) { return false; }

    /// Return the resource file which contains the set of params loaded for offline editing.
    virtual QString offlineEditingParamFile(Vehicle* /*vehicle*/) { return QString(); }

    /// Return the resource file which contains the brand image for the vehicle for Indoor theme.
    virtual QString brandImageIndoor(const Vehicle* /*vehicle*/) const { return QString(); }

    /// Return the resource file which contains the brand image for the vehicle for Outdoor theme.
    virtual QString brandImageOutdoor(const Vehicle* /*vehicle*/) const { return QString(); }

    /// Return the resource file which contains the vehicle icon used in the flight view when the view is dark (Satellite for instance)
    virtual QString vehicleImageOpaque(const Vehicle* vehicle) const;

    /// Return the resource file which contains the vehicle icon used in the flight view when the view is light (Map for instance)
    virtual QString vehicleImageOutline(const Vehicle* vehicle) const;

    /// Return the resource file which contains the vehicle icon used in the compass
    virtual QString vehicleImageCompass(const Vehicle* vehicle) const;

    /// Allows the core plugin to override the toolbar indicators
    ///     signals toolbarIndicatorsChanged
    /// @return A list of QUrl with the indicators (see MainToolBarIndicators.qml)
    virtual const QVariantList& toolBarIndicators(const Vehicle* vehicle);

    /// Returns a list of CameraMetaData objects for available cameras on the vehicle.
    /// TODO: This should go into QGCCameraManager
    virtual const QVariantList& cameraList(const Vehicle* vehicle);

    /// Creates vehicle camera manager.
    virtual QGCCameraManager* createCameraManager(Vehicle *vehicle);

    /// Camera control.
    virtual QGCCameraControl* createCameraControl(const mavlink_camera_information_t* info, Vehicle* vehicle, int compID, QObject* parent = nullptr);

    /// Returns a pointer to a dictionary of firmware-specific FactGroups
    virtual QMap<QString, FactGroup*>* factGroups(void);

    /// @true: When flying a mission the vehicle is always facing towards the next waypoint
    virtual bool vehicleYawsToNextWaypointInMission(const Vehicle* vehicle) const;

    /// Returns the data needed to do battery consumption calculations
    ///     @param[out] mAhBattery Battery milliamp-hours rating (0 for no battery data available)
    ///     @param[out] hoverAmps Current draw in amps during hover
    ///     @param[out] cruiseAmps Current draw in amps during cruise
    virtual void batteryConsumptionData(Vehicle* vehicle, int& mAhBattery, double& hoverAmps, double& cruiseAmps) const;

    // Returns the parameter which control auto-disarm. Assume == 0 means no auto disarm
    virtual QString autoDisarmParameter(Vehicle* vehicle);

    /// Used to determine whether a vehicle has a gimbal.
    ///     @param[out] rollSupported Gimbal supports roll
    ///     @param[out] pitchSupported Gimbal supports pitch
    ///     @param[out] yawSupported Gimbal supports yaw
    /// @return true: vehicle has gimbal, false: gimbal support unknown
    virtual bool hasGimbal(Vehicle* vehicle, bool& rollSupported, bool& pitchSupported, bool& yawSupported);

    /// Returns true if the vehicle is a VTOL
    virtual bool isVtol(const Vehicle* vehicle) const;

    /// Convert from HIGH_LATENCY2.custom_mode value to correct 32 bit value.
    virtual uint32_t highLatencyCustomModeTo32Bits(uint16_t hlCustomMode);

    /// Used to check if running firmware is latest stable version.
    virtual void checkIfIsLatestStable(Vehicle* vehicle);

    /// Used to check if running current version is equal or higher than the one being compared.
    /// returns 1 if current > compare, 0 if current == compare, -1 if current < compare
    int versionCompare(Vehicle* vehicle, QString& compare);
    int versionCompare(Vehicle* vehicle, int major, int minor, int patch);

    /// Allows the Firmware plugin to override the facts meta data.
    ///     @param vehicleType - Type of current vehicle
    ///     @param metaData - MetaData for fact
    virtual void adjustMetaData(MAV_TYPE /*vehicleType*/, FactMetaData* /*metaData*/) {}

    /// Sends the appropriate mavlink message for follow me support
    virtual void sendGCSMotionReport(Vehicle* vehicle, FollowMe::GCSMotionReport& motionReport, uint8_t estimatationCapabilities);

    // FIXME: Hack workaround for non pluginize FollowMe support
    static const QString px4FollowMeFlightMode;

signals:
    void toolbarIndicatorsChanged(void);

protected:
    // Arms the vehicle with validation and retries
    // @return: true - vehicle armed, false - vehicle failed to arm
    bool _armVehicleAndValidate(Vehicle* vehicle);

    // Sets the vehicle to the specified flight mode with validation and retries
    // @return: true - vehicle in specified flight mode, false - flight mode change failed
    bool _setFlightModeAndValidate(Vehicle* vehicle, const QString& flightMode);

    // returns url with latest firmware release information.
    virtual QString _getLatestVersionFileUrl(Vehicle* /*vehicle*/) { return QString(); }

    // Callback to process file with latest release information
    virtual void _versionFileDownloadFinished(QString& remoteFile, QString& localFile, Vehicle* vehicle);

    // Returns regex QString to extract version information from text
    virtual QString _versionRegex() { return QString(); }

private:
    QVariantList _toolBarIndicatorList;
    static QVariantList _cameraList;    ///< Standard QGC camera list
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

    /// @return List of firmware types this plugin supports.
    virtual QList<MAV_AUTOPILOT> supportedFirmwareTypes(void) const = 0;

    /// @return List of vehicle types this plugin supports.
    virtual QList<MAV_TYPE> supportedVehicleTypes(void) const;
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
