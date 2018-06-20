/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QVariantList>
#include <QGeoCoordinate>

#include "FactGroup.h"
#include "LinkInterface.h"
#include "QGCMAVLink.h"
#include "QmlObjectListModel.h"
#include "MAVLinkProtocol.h"
#include "UASMessageHandler.h"
#include "SettingsFact.h"

class UAS;
class UASInterface;
class FirmwarePlugin;
class FirmwarePluginManager;
class AutoPilotPlugin;
class MissionManager;
class GeoFenceManager;
class RallyPointManager;
class ParameterManager;
class JoystickManager;
class UASMessage;
class SettingsManager;
class ADSBVehicle;
class QGCCameraManager;

Q_DECLARE_LOGGING_CATEGORY(VehicleLog)

class Vehicle;

class VehicleDistanceSensorFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleDistanceSensorFactGroup(QObject* parent = NULL);

    Q_PROPERTY(Fact* rotationNone       READ rotationNone       CONSTANT)
    Q_PROPERTY(Fact* rotationYaw45      READ rotationYaw45      CONSTANT)
    Q_PROPERTY(Fact* rotationYaw90      READ rotationYaw90      CONSTANT)
    Q_PROPERTY(Fact* rotationYaw135     READ rotationYaw135     CONSTANT)
    Q_PROPERTY(Fact* rotationYaw180     READ rotationYaw180     CONSTANT)
    Q_PROPERTY(Fact* rotationYaw225     READ rotationYaw225     CONSTANT)
    Q_PROPERTY(Fact* rotationYaw270     READ rotationYaw270     CONSTANT)
    Q_PROPERTY(Fact* rotationYaw315     READ rotationYaw315     CONSTANT)
    Q_PROPERTY(Fact* rotationPitch90    READ rotationPitch90    CONSTANT)
    Q_PROPERTY(Fact* rotationPitch270   READ rotationPitch270   CONSTANT)

    Fact* rotationNone      (void) { return &_rotationNoneFact; }
    Fact* rotationYaw45     (void) { return &_rotationYaw45Fact; }
    Fact* rotationYaw90     (void) { return &_rotationYaw90Fact; }
    Fact* rotationYaw135    (void) { return &_rotationYaw90Fact; }
    Fact* rotationYaw180    (void) { return &_rotationYaw180Fact; }
    Fact* rotationYaw225    (void) { return &_rotationYaw180Fact; }
    Fact* rotationYaw270    (void) { return &_rotationYaw270Fact; }
    Fact* rotationYaw315    (void) { return &_rotationYaw315Fact; }
    Fact* rotationPitch90   (void) { return &_rotationPitch90Fact; }
    Fact* rotationPitch270  (void) { return &_rotationPitch270Fact; }

    bool idSet(void) { return _idSet; }
    void setIdSet(bool idSet) { _idSet = idSet; }
    uint8_t id(void) { return _id; }
    void setId(uint8_t id) { _id = id; }

    static const char* _rotationNoneFactName;
    static const char* _rotationYaw45FactName;
    static const char* _rotationYaw90FactName;
    static const char* _rotationYaw135FactName;
    static const char* _rotationYaw180FactName;
    static const char* _rotationYaw225FactName;
    static const char* _rotationYaw270FactName;
    static const char* _rotationYaw315FactName;
    static const char* _rotationPitch90FactName;
    static const char* _rotationPitch270FactName;

private:
    Fact _rotationNoneFact;
    Fact _rotationYaw45Fact;
    Fact _rotationYaw90Fact;
    Fact _rotationYaw135Fact;
    Fact _rotationYaw180Fact;
    Fact _rotationYaw225Fact;
    Fact _rotationYaw270Fact;
    Fact _rotationYaw315Fact;
    Fact _rotationPitch90Fact;
    Fact _rotationPitch270Fact;

    bool    _idSet; // true: _id is set to seen sensor id
    uint8_t _id;    // The id for the sensor being tracked. Current support for only a single sensor.
};

class VehicleSetpointFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleSetpointFactGroup(QObject* parent = NULL);

    Q_PROPERTY(Fact* roll       READ roll       CONSTANT)
    Q_PROPERTY(Fact* pitch      READ pitch      CONSTANT)
    Q_PROPERTY(Fact* yaw        READ yaw        CONSTANT)
    Q_PROPERTY(Fact* rollRate   READ rollRate   CONSTANT)
    Q_PROPERTY(Fact* pitchRate  READ pitchRate  CONSTANT)
    Q_PROPERTY(Fact* yawRate    READ yawRate    CONSTANT)

    Fact* roll      (void) { return &_rollFact; }
    Fact* pitch     (void) { return &_pitchFact; }
    Fact* yaw       (void) { return &_yawFact; }
    Fact* rollRate  (void) { return &_rollRateFact; }
    Fact* pitchRate (void) { return &_pitchRateFact; }
    Fact* yawRate   (void) { return &_yawRateFact; }

    static const char* _rollFactName;
    static const char* _pitchFactName;
    static const char* _yawFactName;
    static const char* _rollRateFactName;
    static const char* _pitchRateFactName;
    static const char* _yawRateFactName;

private:
    Fact _rollFact;
    Fact _pitchFact;
    Fact _yawFact;
    Fact _rollRateFact;
    Fact _pitchRateFact;
    Fact _yawRateFact;
};

class VehicleVibrationFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleVibrationFactGroup(QObject* parent = NULL);

    Q_PROPERTY(Fact* xAxis      READ xAxis      CONSTANT)
    Q_PROPERTY(Fact* yAxis      READ yAxis      CONSTANT)
    Q_PROPERTY(Fact* zAxis      READ zAxis      CONSTANT)
    Q_PROPERTY(Fact* clipCount1 READ clipCount1 CONSTANT)
    Q_PROPERTY(Fact* clipCount2 READ clipCount2 CONSTANT)
    Q_PROPERTY(Fact* clipCount3 READ clipCount3 CONSTANT)

    Fact* xAxis         (void) { return &_xAxisFact; }
    Fact* yAxis         (void) { return &_yAxisFact; }
    Fact* zAxis         (void) { return &_zAxisFact; }
    Fact* clipCount1    (void) { return &_clipCount1Fact; }
    Fact* clipCount2    (void) { return &_clipCount2Fact; }
    Fact* clipCount3    (void) { return &_clipCount3Fact; }

    static const char* _xAxisFactName;
    static const char* _yAxisFactName;
    static const char* _zAxisFactName;
    static const char* _clipCount1FactName;
    static const char* _clipCount2FactName;
    static const char* _clipCount3FactName;

private:
    Fact        _xAxisFact;
    Fact        _yAxisFact;
    Fact        _zAxisFact;
    Fact        _clipCount1Fact;
    Fact        _clipCount2Fact;
    Fact        _clipCount3Fact;
};

class VehicleWindFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleWindFactGroup(QObject* parent = NULL);

    Q_PROPERTY(Fact* direction      READ direction      CONSTANT)
    Q_PROPERTY(Fact* speed          READ speed          CONSTANT)
    Q_PROPERTY(Fact* verticalSpeed  READ verticalSpeed  CONSTANT)

    Fact* direction     (void) { return &_directionFact; }
    Fact* speed         (void) { return &_speedFact; }
    Fact* verticalSpeed (void) { return &_verticalSpeedFact; }

    static const char* _directionFactName;
    static const char* _speedFactName;
    static const char* _verticalSpeedFactName;

private:
    Fact        _directionFact;
    Fact        _speedFact;
    Fact        _verticalSpeedFact;
};

class VehicleGPSFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleGPSFactGroup(QObject* parent = NULL);

    Q_PROPERTY(Fact* lat                READ lat                CONSTANT)
    Q_PROPERTY(Fact* lon                READ lon                CONSTANT)
    Q_PROPERTY(Fact* hdop               READ hdop               CONSTANT)
    Q_PROPERTY(Fact* vdop               READ vdop               CONSTANT)
    Q_PROPERTY(Fact* courseOverGround   READ courseOverGround   CONSTANT)
    Q_PROPERTY(Fact* count              READ count              CONSTANT)
    Q_PROPERTY(Fact* lock               READ lock               CONSTANT)

    Fact* lat               (void) { return &_latFact; }
    Fact* lon               (void) { return &_lonFact; }
    Fact* hdop              (void) { return &_hdopFact; }
    Fact* vdop              (void) { return &_vdopFact; }
    Fact* courseOverGround  (void) { return &_courseOverGroundFact; }
    Fact* count             (void) { return &_countFact; }
    Fact* lock              (void) { return &_lockFact; }

    static const char* _latFactName;
    static const char* _lonFactName;
    static const char* _hdopFactName;
    static const char* _vdopFactName;
    static const char* _courseOverGroundFactName;
    static const char* _countFactName;
    static const char* _lockFactName;

private:
    Fact        _latFact;
    Fact        _lonFact;
    Fact        _hdopFact;
    Fact        _vdopFact;
    Fact        _courseOverGroundFact;
    Fact        _countFact;
    Fact        _lockFact;
};

class VehicleBatteryFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleBatteryFactGroup(QObject* parent = NULL);

    Q_PROPERTY(Fact* voltage            READ voltage            CONSTANT)
    Q_PROPERTY(Fact* percentRemaining   READ percentRemaining   CONSTANT)
    Q_PROPERTY(Fact* mahConsumed        READ mahConsumed        CONSTANT)
    Q_PROPERTY(Fact* current            READ current            CONSTANT)
    Q_PROPERTY(Fact* temperature        READ temperature        CONSTANT)
    Q_PROPERTY(Fact* cellCount          READ cellCount          CONSTANT)
    Q_PROPERTY(Fact* instantPower       READ instantPower       CONSTANT)
    Q_PROPERTY(Fact* timeRemaining      READ timeRemaining      CONSTANT)
    Q_PROPERTY(Fact* chargeState        READ chargeState        CONSTANT)

    Fact* voltage                   (void) { return &_voltageFact; }
    Fact* percentRemaining          (void) { return &_percentRemainingFact; }
    Fact* mahConsumed               (void) { return &_mahConsumedFact; }
    Fact* current                   (void) { return &_currentFact; }
    Fact* temperature               (void) { return &_temperatureFact; }
    Fact* cellCount                 (void) { return &_cellCountFact; }
    Fact* instantPower              (void) { return &_instantPowerFact; }
    Fact* timeRemaining             (void) { return &_timeRemainingFact; }
    Fact* chargeState               (void) { return &_chargeStateFact; }

    static const char* _voltageFactName;
    static const char* _percentRemainingFactName;
    static const char* _mahConsumedFactName;
    static const char* _currentFactName;
    static const char* _temperatureFactName;
    static const char* _cellCountFactName;
    static const char* _instantPowerFactName;
    static const char* _timeRemainingFactName;
    static const char* _chargeStateFactName;

    static const char* _settingsGroup;

    static const double _voltageUnavailable;
    static const int    _percentRemainingUnavailable;
    static const int    _mahConsumedUnavailable;
    static const int    _currentUnavailable;
    static const double _temperatureUnavailable;
    static const int    _cellCountUnavailable;
    static const double _instantPowerUnavailable;

private:
    Fact            _voltageFact;
    Fact            _percentRemainingFact;
    Fact            _mahConsumedFact;
    Fact            _currentFact;
    Fact            _temperatureFact;
    Fact            _cellCountFact;
    Fact            _instantPowerFact;
    Fact            _timeRemainingFact;
    Fact            _chargeStateFact;
};

class VehicleTemperatureFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleTemperatureFactGroup(QObject* parent = NULL);

    Q_PROPERTY(Fact* temperature1       READ temperature1       CONSTANT)
    Q_PROPERTY(Fact* temperature2       READ temperature2       CONSTANT)
    Q_PROPERTY(Fact* temperature3       READ temperature3       CONSTANT)

    Fact* temperature1 (void) { return &_temperature1Fact; }
    Fact* temperature2 (void) { return &_temperature2Fact; }
    Fact* temperature3 (void) { return &_temperature3Fact; }

    static const char* _temperature1FactName;
    static const char* _temperature2FactName;
    static const char* _temperature3FactName;

    static const char* _settingsGroup;

    static const double _temperatureUnavailable;

private:
    Fact            _temperature1Fact;
    Fact            _temperature2Fact;
    Fact            _temperature3Fact;
};

class VehicleClockFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleClockFactGroup(QObject* parent = NULL);

    Q_PROPERTY(Fact* currentTime        READ currentTime        CONSTANT)
    Q_PROPERTY(Fact* currentDate        READ currentDate        CONSTANT)

    Fact* currentTime (void) { return &_currentTimeFact; }
    Fact* currentDate (void) { return &_currentDateFact; }

    static const char* _currentTimeFactName;
    static const char* _currentDateFactName;

    static const char* _settingsGroup;

private slots:
    void _updateAllValues(void) override;

private:
    Fact            _currentTimeFact;
    Fact            _currentDateFact;
};

class Vehicle : public FactGroup
{
    Q_OBJECT

public:
    Vehicle(LinkInterface*          link,
            int                     vehicleId,
            int                     defaultComponentId,
            MAV_AUTOPILOT           firmwareType,
            MAV_TYPE                vehicleType,
            FirmwarePluginManager*  firmwarePluginManager,
            JoystickManager*        joystickManager);

    // The following is used to create a disconnected Vehicle for use while offline editing.
    Vehicle(MAV_AUTOPILOT           firmwareType,
            MAV_TYPE                vehicleType,
            FirmwarePluginManager*  firmwarePluginManager,
            QObject*                parent = NULL);

    ~Vehicle();

    /// Sensor bits from sensors*Bits properties
    enum MavlinkSysStatus {
        SysStatusSensor3dGyro =                 MAV_SYS_STATUS_SENSOR_3D_GYRO,
        SysStatusSensor3dAccel =                MAV_SYS_STATUS_SENSOR_3D_ACCEL,
        SysStatusSensor3dMag =                  MAV_SYS_STATUS_SENSOR_3D_MAG,
        SysStatusSensorAbsolutePressure =       MAV_SYS_STATUS_SENSOR_ABSOLUTE_PRESSURE,
        SysStatusSensorDifferentialPressure =   MAV_SYS_STATUS_SENSOR_DIFFERENTIAL_PRESSURE,
        SysStatusSensorGPS =                    MAV_SYS_STATUS_SENSOR_GPS,
        SysStatusSensorOpticalFlow =            MAV_SYS_STATUS_SENSOR_OPTICAL_FLOW,
        SysStatusSensorVisionPosition =         MAV_SYS_STATUS_SENSOR_VISION_POSITION,
        SysStatusSensorLaserPosition =          MAV_SYS_STATUS_SENSOR_LASER_POSITION,
        SysStatusSensorExternalGroundTruth =    MAV_SYS_STATUS_SENSOR_EXTERNAL_GROUND_TRUTH,
        SysStatusSensorAngularRateControl =     MAV_SYS_STATUS_SENSOR_ANGULAR_RATE_CONTROL,
        SysStatusSensorAttitudeStabilization =  MAV_SYS_STATUS_SENSOR_ATTITUDE_STABILIZATION,
        SysStatusSensorYawPosition =            MAV_SYS_STATUS_SENSOR_YAW_POSITION,
        SysStatusSensorZAltitudeControl =       MAV_SYS_STATUS_SENSOR_Z_ALTITUDE_CONTROL,
        SysStatusSensorXYPositionControl =      MAV_SYS_STATUS_SENSOR_XY_POSITION_CONTROL,
        SysStatusSensorMotorOutputs =           MAV_SYS_STATUS_SENSOR_MOTOR_OUTPUTS,
        SysStatusSensorRCReceiver =             MAV_SYS_STATUS_SENSOR_RC_RECEIVER,
        SysStatusSensor3dGyro2 =                MAV_SYS_STATUS_SENSOR_3D_GYRO2,
        SysStatusSensor3dAccel2 =               MAV_SYS_STATUS_SENSOR_3D_ACCEL2,
        SysStatusSensor3dMag2 =                 MAV_SYS_STATUS_SENSOR_3D_MAG2,
        SysStatusSensorGeoFence =               MAV_SYS_STATUS_GEOFENCE,
        SysStatusSensorAHRS =                   MAV_SYS_STATUS_AHRS,
        SysStatusSensorTerrain =                MAV_SYS_STATUS_TERRAIN,
        SysStatusSensorReverseMotor =           MAV_SYS_STATUS_REVERSE_MOTOR,
        SysStatusSensorLogging =                MAV_SYS_STATUS_LOGGING,
        SysStatusSensorBattery =                MAV_SYS_STATUS_SENSOR_BATTERY,
    };
    Q_ENUM(MavlinkSysStatus)

    Q_PROPERTY(int                  id                      READ id                                                     CONSTANT)
    Q_PROPERTY(AutoPilotPlugin*     autopilot               MEMBER _autopilotPlugin                                     CONSTANT)
    Q_PROPERTY(QGeoCoordinate       coordinate              READ coordinate                                             NOTIFY coordinateChanged)
    Q_PROPERTY(QGeoCoordinate       homePosition            READ homePosition                                           NOTIFY homePositionChanged)
    Q_PROPERTY(bool                 armed                   READ armed                  WRITE setArmed                  NOTIFY armedChanged)
    Q_PROPERTY(bool                 autoDisarm              READ autoDisarm                                             NOTIFY autoDisarmChanged)
    Q_PROPERTY(bool                 flightModeSetAvailable  READ flightModeSetAvailable                                 CONSTANT)
    Q_PROPERTY(QStringList          flightModes             READ flightModes                                            NOTIFY flightModesChanged)
    Q_PROPERTY(QString              flightMode              READ flightMode             WRITE setFlightMode             NOTIFY flightModeChanged)
    Q_PROPERTY(bool                 hilMode                 READ hilMode                WRITE setHilMode                NOTIFY hilModeChanged)
    Q_PROPERTY(QmlObjectListModel*  trajectoryPoints        READ trajectoryPoints                                       CONSTANT)
    Q_PROPERTY(QmlObjectListModel*  cameraTriggerPoints     READ cameraTriggerPoints                                    CONSTANT)
    Q_PROPERTY(float                latitude                READ latitude                                               NOTIFY coordinateChanged)
    Q_PROPERTY(float                longitude               READ longitude                                              NOTIFY coordinateChanged)
    Q_PROPERTY(bool                 messageTypeNone         READ messageTypeNone                                        NOTIFY messageTypeChanged)
    Q_PROPERTY(bool                 messageTypeNormal       READ messageTypeNormal                                      NOTIFY messageTypeChanged)
    Q_PROPERTY(bool                 messageTypeWarning      READ messageTypeWarning                                     NOTIFY messageTypeChanged)
    Q_PROPERTY(bool                 messageTypeError        READ messageTypeError                                       NOTIFY messageTypeChanged)
    Q_PROPERTY(int                  newMessageCount         READ newMessageCount                                        NOTIFY newMessageCountChanged)
    Q_PROPERTY(int                  messageCount            READ messageCount                                           NOTIFY messageCountChanged)
    Q_PROPERTY(QString              formatedMessages        READ formatedMessages                                       NOTIFY formatedMessagesChanged)
    Q_PROPERTY(QString              formatedMessage         READ formatedMessage                                        NOTIFY formatedMessageChanged)
    Q_PROPERTY(QString              latestError             READ latestError                                            NOTIFY latestErrorChanged)
    Q_PROPERTY(int                  joystickMode            READ joystickMode           WRITE setJoystickMode           NOTIFY joystickModeChanged)
    Q_PROPERTY(QStringList          joystickModes           READ joystickModes                                          CONSTANT)
    Q_PROPERTY(bool                 joystickEnabled         READ joystickEnabled        WRITE setJoystickEnabled        NOTIFY joystickEnabledChanged)
    Q_PROPERTY(bool                 active                  READ active                 WRITE setActive                 NOTIFY activeChanged)
    Q_PROPERTY(int                  flowImageIndex          READ flowImageIndex                                         NOTIFY flowImageIndexChanged)
    Q_PROPERTY(int                  rcRSSI                  READ rcRSSI                                                 NOTIFY rcRSSIChanged)
    Q_PROPERTY(bool                 px4Firmware             READ px4Firmware                                            NOTIFY firmwareTypeChanged)
    Q_PROPERTY(bool                 apmFirmware             READ apmFirmware                                            NOTIFY firmwareTypeChanged)
    Q_PROPERTY(bool                 soloFirmware            READ soloFirmware           WRITE setSoloFirmware           NOTIFY soloFirmwareChanged)
    Q_PROPERTY(bool                 genericFirmware         READ genericFirmware                                        CONSTANT)
    Q_PROPERTY(bool                 connectionLost          READ connectionLost                                         NOTIFY connectionLostChanged)
    Q_PROPERTY(bool                 connectionLostEnabled   READ connectionLostEnabled  WRITE setConnectionLostEnabled  NOTIFY connectionLostEnabledChanged)
    Q_PROPERTY(uint                 messagesReceived        READ messagesReceived                                       NOTIFY messagesReceivedChanged)
    Q_PROPERTY(uint                 messagesSent            READ messagesSent                                           NOTIFY messagesSentChanged)
    Q_PROPERTY(uint                 messagesLost            READ messagesLost                                           NOTIFY messagesLostChanged)
    Q_PROPERTY(bool                 fixedWing               READ fixedWing                                              NOTIFY vehicleTypeChanged)
    Q_PROPERTY(bool                 multiRotor              READ multiRotor                                             NOTIFY vehicleTypeChanged)
    Q_PROPERTY(bool                 vtol                    READ vtol                                                   NOTIFY vehicleTypeChanged)
    Q_PROPERTY(bool                 rover                   READ rover                                                  NOTIFY vehicleTypeChanged)
    Q_PROPERTY(bool                 sub                     READ sub                                                    NOTIFY vehicleTypeChanged)
    Q_PROPERTY(bool        supportsThrottleModeCenterZero   READ supportsThrottleModeCenterZero                         CONSTANT)
    Q_PROPERTY(bool                supportsNegativeThrust   READ supportsNegativeThrust                                 CONSTANT)
    Q_PROPERTY(bool                 supportsJSButton        READ supportsJSButton                                       CONSTANT)
    Q_PROPERTY(bool                 supportsRadio           READ supportsRadio                                          CONSTANT)
    Q_PROPERTY(bool               supportsMotorInterference READ supportsMotorInterference                              CONSTANT)
    Q_PROPERTY(bool                 autoDisconnect          MEMBER _autoDisconnect                                      NOTIFY autoDisconnectChanged)
    Q_PROPERTY(QString              prearmError             READ prearmError            WRITE setPrearmError            NOTIFY prearmErrorChanged)
    Q_PROPERTY(int                  motorCount              READ motorCount                                             CONSTANT)
    Q_PROPERTY(bool                 coaxialMotors           READ coaxialMotors                                          CONSTANT)
    Q_PROPERTY(bool                 xConfigMotors           READ xConfigMotors                                          CONSTANT)
    Q_PROPERTY(bool                 isOfflineEditingVehicle READ isOfflineEditingVehicle                                CONSTANT)
    Q_PROPERTY(QString              brandImageIndoor        READ brandImageIndoor                                       NOTIFY firmwareTypeChanged)
    Q_PROPERTY(QString              brandImageOutdoor       READ brandImageOutdoor                                      NOTIFY firmwareTypeChanged)
    Q_PROPERTY(QStringList          unhealthySensors        READ unhealthySensors                                       NOTIFY unhealthySensorsChanged)
    Q_PROPERTY(int                  sensorsPresentBits      READ sensorsPresentBits                                     NOTIFY sensorsPresentBitsChanged)
    Q_PROPERTY(int                  sensorsEnabledBits      READ sensorsEnabledBits                                     NOTIFY sensorsEnabledBitsChanged)
    Q_PROPERTY(int                  sensorsHealthBits       READ sensorsHealthBits                                      NOTIFY sensorsHealthBitsChanged)
    Q_PROPERTY(int                  sensorsUnhealthyBits    READ sensorsUnhealthyBits                                   NOTIFY sensorsUnhealthyBitsChanged) ///< Combination of enabled and health
    Q_PROPERTY(QString              missionFlightMode       READ missionFlightMode                                      CONSTANT)
    Q_PROPERTY(QString              pauseFlightMode         READ pauseFlightMode                                        CONSTANT)
    Q_PROPERTY(QString              rtlFlightMode           READ rtlFlightMode                                          CONSTANT)
    Q_PROPERTY(QString              landFlightMode          READ landFlightMode                                         CONSTANT)
    Q_PROPERTY(QString              takeControlFlightMode   READ takeControlFlightMode                                  CONSTANT)
    Q_PROPERTY(QString              firmwareTypeString      READ firmwareTypeString                                     NOTIFY firmwareTypeChanged)
    Q_PROPERTY(QString              vehicleTypeString       READ vehicleTypeString                                      NOTIFY vehicleTypeChanged)
    Q_PROPERTY(QString              vehicleImageOpaque      READ vehicleImageOpaque                                     CONSTANT)
    Q_PROPERTY(QString              vehicleImageOutline     READ vehicleImageOutline                                    CONSTANT)
    Q_PROPERTY(QString              vehicleImageCompass     READ vehicleImageCompass                                    CONSTANT)
    Q_PROPERTY(int                  telemetryRRSSI          READ telemetryRRSSI                                         NOTIFY telemetryRRSSIChanged)
    Q_PROPERTY(int                  telemetryLRSSI          READ telemetryLRSSI                                         NOTIFY telemetryLRSSIChanged)
    Q_PROPERTY(unsigned int         telemetryRXErrors       READ telemetryRXErrors                                      NOTIFY telemetryRXErrorsChanged)
    Q_PROPERTY(unsigned int         telemetryFixed          READ telemetryFixed                                         NOTIFY telemetryFixedChanged)
    Q_PROPERTY(unsigned int         telemetryTXBuffer       READ telemetryTXBuffer                                      NOTIFY telemetryTXBufferChanged)
    Q_PROPERTY(int                  telemetryLNoise         READ telemetryLNoise                                        NOTIFY telemetryLNoiseChanged)
    Q_PROPERTY(int                  telemetryRNoise         READ telemetryRNoise                                        NOTIFY telemetryRNoiseChanged)
    Q_PROPERTY(QVariantList         toolBarIndicators       READ toolBarIndicators                                      NOTIFY toolBarIndicatorsChanged)
    Q_PROPERTY(QmlObjectListModel*  adsbVehicles            READ adsbVehicles                                           CONSTANT)
    Q_PROPERTY(bool              initialPlanRequestComplete READ initialPlanRequestComplete                             NOTIFY initialPlanRequestCompleteChanged)
    Q_PROPERTY(QVariantList         staticCameraList        READ staticCameraList                                       CONSTANT)
    Q_PROPERTY(QGCCameraManager*    dynamicCameras          READ dynamicCameras                                         NOTIFY dynamicCamerasChanged)
    Q_PROPERTY(QString              hobbsMeter              READ hobbsMeter                                             NOTIFY hobbsMeterChanged)
    Q_PROPERTY(bool                 vtolInFwdFlight         READ vtolInFwdFlight        WRITE setVtolInFwdFlight        NOTIFY vtolInFwdFlightChanged)
    Q_PROPERTY(bool                 highLatencyLink         READ highLatencyLink                                        NOTIFY highLatencyLinkChanged)
    Q_PROPERTY(bool                 supportsTerrainFrame    READ supportsTerrainFrame                                   NOTIFY firmwareTypeChanged)
    Q_PROPERTY(QString              priorityLinkName        READ priorityLinkName       WRITE setPriorityLinkByName     NOTIFY priorityLinkNameChanged)
    Q_PROPERTY(QVariantList         links                   READ links                                                  NOTIFY linksChanged)
    Q_PROPERTY(LinkInterface*       priorityLink            READ priorityLink                                           NOTIFY priorityLinkNameChanged)

    // Vehicle state used for guided control
    Q_PROPERTY(bool flying                  READ flying NOTIFY flyingChanged)                               ///< Vehicle is flying
    Q_PROPERTY(bool landing                 READ landing NOTIFY landingChanged)                             ///< Vehicle is in landing pattern (DO_LAND_START)
    Q_PROPERTY(bool guidedMode              READ guidedMode WRITE setGuidedMode NOTIFY guidedModeChanged)   ///< Vehicle is in Guided mode and can respond to guided commands
    Q_PROPERTY(bool guidedModeSupported     READ guidedModeSupported CONSTANT)                              ///< Guided mode commands are supported by this vehicle
    Q_PROPERTY(bool pauseVehicleSupported   READ pauseVehicleSupported CONSTANT)                            ///< Pause vehicle command is supported
    Q_PROPERTY(bool orbitModeSupported      READ orbitModeSupported CONSTANT)                               ///< Orbit mode is supported by this vehicle
    Q_PROPERTY(bool takeoffVehicleSupported READ takeoffVehicleSupported CONSTANT)                          ///< Guided takeoff supported

    Q_PROPERTY(ParameterManager* parameterManager READ parameterManager CONSTANT)

    // FactGroup object model properties

    Q_PROPERTY(Fact* roll               READ roll               CONSTANT)
    Q_PROPERTY(Fact* pitch              READ pitch              CONSTANT)
    Q_PROPERTY(Fact* heading            READ heading            CONSTANT)
    Q_PROPERTY(Fact* rollRate           READ rollRate           CONSTANT)
    Q_PROPERTY(Fact* pitchRate          READ pitchRate          CONSTANT)
    Q_PROPERTY(Fact* yawRate            READ yawRate            CONSTANT)
    Q_PROPERTY(Fact* groundSpeed        READ groundSpeed        CONSTANT)
    Q_PROPERTY(Fact* airSpeed           READ airSpeed           CONSTANT)
    Q_PROPERTY(Fact* climbRate          READ climbRate          CONSTANT)
    Q_PROPERTY(Fact* altitudeRelative   READ altitudeRelative   CONSTANT)
    Q_PROPERTY(Fact* altitudeAMSL       READ altitudeAMSL       CONSTANT)
    Q_PROPERTY(Fact* flightDistance     READ flightDistance     CONSTANT)
    Q_PROPERTY(Fact* distanceToHome     READ distanceToHome     CONSTANT)
    Q_PROPERTY(Fact* hobbs              READ hobbs              CONSTANT)

    Q_PROPERTY(FactGroup* gps         READ gpsFactGroup         CONSTANT)
    Q_PROPERTY(FactGroup* battery     READ battery1FactGroup    CONSTANT)
    Q_PROPERTY(FactGroup* battery2    READ battery2FactGroup    CONSTANT)
    Q_PROPERTY(FactGroup* wind        READ windFactGroup        CONSTANT)
    Q_PROPERTY(FactGroup* vibration   READ vibrationFactGroup   CONSTANT)
    Q_PROPERTY(FactGroup* temperature READ temperatureFactGroup CONSTANT)
    Q_PROPERTY(FactGroup* clock       READ clockFactGroup       CONSTANT)
    Q_PROPERTY(FactGroup* setpoint    READ setpointFactGroup    CONSTANT)

    Q_PROPERTY(int      firmwareMajorVersion        READ firmwareMajorVersion       NOTIFY firmwareVersionChanged)
    Q_PROPERTY(int      firmwareMinorVersion        READ firmwareMinorVersion       NOTIFY firmwareVersionChanged)
    Q_PROPERTY(int      firmwarePatchVersion        READ firmwarePatchVersion       NOTIFY firmwareVersionChanged)
    Q_PROPERTY(int      firmwareVersionType         READ firmwareVersionType        NOTIFY firmwareVersionChanged)
    Q_PROPERTY(QString  firmwareVersionTypeString   READ firmwareVersionTypeString  NOTIFY firmwareVersionChanged)
    Q_PROPERTY(int      firmwareCustomMajorVersion  READ firmwareCustomMajorVersion NOTIFY firmwareCustomVersionChanged)
    Q_PROPERTY(int      firmwareCustomMinorVersion  READ firmwareCustomMinorVersion NOTIFY firmwareCustomVersionChanged)
    Q_PROPERTY(int      firmwareCustomPatchVersion  READ firmwareCustomPatchVersion NOTIFY firmwareCustomVersionChanged)
    Q_PROPERTY(QString  gitHash                     READ gitHash                    NOTIFY gitHashChanged)
    Q_PROPERTY(quint64  vehicleUID                  READ vehicleUID                 NOTIFY vehicleUIDChanged)
    Q_PROPERTY(QString  vehicleUIDStr               READ vehicleUIDStr              NOTIFY vehicleUIDChanged)

    /// Resets link status counters
    Q_INVOKABLE void resetCounters  ();

    /// Returns the number of buttons which are reserved for firmware use in the MANUAL_CONTROL mavlink
    /// message. For example PX4 Flight Stack reserves the first 8 buttons to simulate rc switches.
    /// The remainder can be assigned to Vehicle actions.
    /// @return -1: reserver all buttons, >0 number of buttons to reserve
    Q_PROPERTY(int manualControlReservedButtonCount READ manualControlReservedButtonCount CONSTANT)

    // Called when the message drop-down is invoked to clear current count
    Q_INVOKABLE void        resetMessages();

    Q_INVOKABLE void virtualTabletJoystickValue(double roll, double pitch, double yaw, double thrust);
    Q_INVOKABLE void disconnectInactiveVehicle(void);

    /// Command vehicle to return to launch
    Q_INVOKABLE void guidedModeRTL(void);

    /// Command vehicle to land at current location
    Q_INVOKABLE void guidedModeLand(void);

    /// Command vehicle to takeoff from current location
    Q_INVOKABLE void guidedModeTakeoff(double altitudeRelative);

    /// @return The minimum takeoff altitude (relative) for guided takeoff.
    Q_INVOKABLE double minimumTakeoffAltitude(void);

    /// Command vehicle to move to specified location (altitude is included and relative)
    Q_INVOKABLE void guidedModeGotoLocation(const QGeoCoordinate& gotoCoord);

    /// Command vehicle to change altitude
    ///     @param altitudeChange If > 0, go up by amount specified, if < 0, go down by amount specified
    Q_INVOKABLE void guidedModeChangeAltitude(double altitudeChange);

    /// Command vehicle to orbit given center point
    ///     @param centerCoord Orit around this point
    ///     @param radius Distance from vehicle to centerCoord
    ///     @param amslAltitude Desired vehicle altitude
    Q_INVOKABLE void guidedModeOrbit(const QGeoCoordinate& centerCoord, double radius, double amslAltitude);

    /// Command vehicle to pause at current location. If vehicle supports guide mode, vehicle will be left
    /// in guided mode after pause.
    Q_INVOKABLE void pauseVehicle(void);

    /// Command vehicle to kill all motors no matter what state
    Q_INVOKABLE void emergencyStop(void);

    /// Command vehicle to abort landing
    Q_INVOKABLE void abortLanding(double climbOutAltitude);

    Q_INVOKABLE void startMission(void);

    /// Alter the current mission item on the vehicle
    Q_INVOKABLE void setCurrentMissionSequence(int seq);

    /// Reboot vehicle
    Q_INVOKABLE void rebootVehicle();

    /// Clear Messages
    Q_INVOKABLE void clearMessages();

    Q_INVOKABLE void triggerCamera(void);
    Q_INVOKABLE void sendPlan(QString planFile);

#if 0
    // Temporarily removed, waiting for new command implementation
    /// Test motor
    ///     @param motor Motor number, 1-based
    ///     @param percent 0-no power, 100-full power
    ///     @param timeoutSecs Number of seconds for motor to run
    Q_INVOKABLE void motorTest(int motor, int percent, int timeoutSecs);
#endif

    bool guidedModeSupported    (void) const;
    bool pauseVehicleSupported  (void) const;
    bool orbitModeSupported     (void) const;
    bool takeoffVehicleSupported(void) const;

    // Property accessors

    QGeoCoordinate coordinate(void) { return _coordinate; }

    typedef enum {
        JoystickModeRC,         ///< Joystick emulates an RC Transmitter
        JoystickModeAttitude,
        JoystickModePosition,
        JoystickModeForce,
        JoystickModeVelocity,
        JoystickModeMax
    } JoystickMode_t;

    int joystickMode(void);
    void setJoystickMode(int mode);

    /// List of joystick mode names
    QStringList joystickModes(void);

    bool joystickEnabled(void);
    void setJoystickEnabled(bool enabled);

    // Is vehicle active with respect to current active vehicle in QGC
    bool active(void);
    void setActive(bool active);

    // Property accesors
    int id(void) { return _id; }
    MAV_AUTOPILOT firmwareType(void) const { return _firmwareType; }
    MAV_TYPE vehicleType(void) const { return _vehicleType; }
    Q_INVOKABLE QString vehicleTypeName(void) const;

    /// Returns the highest quality link available to the Vehicle. If you need to hold a reference to this link use
    /// LinkManager::sharedLinkInterfaceForGet to get QSharedPointer for link.
    LinkInterface* priorityLink(void) { return _priorityLink.data(); }

    /// Sends a message to the specified link
    /// @return true: message sent, false: Link no longer connected
    bool sendMessageOnLink(LinkInterface* link, mavlink_message_t message);

    /// Sends the specified messages multiple times to the vehicle in order to attempt to
    /// guarantee that it makes it to the vehicle.
    void sendMessageMultiple(mavlink_message_t message);

    /// Provides access to uas from vehicle. Temporary workaround until UAS is fully phased out.
    UAS* uas(void) { return _uas; }

    /// Provides access to uas from vehicle. Temporary workaround until AutoPilotPlugin is fully phased out.
    AutoPilotPlugin* autopilotPlugin(void) { return _autopilotPlugin; }

    /// Provides access to the Firmware Plugin for this Vehicle
    FirmwarePlugin* firmwarePlugin(void) { return _firmwarePlugin; }

    int manualControlReservedButtonCount(void);

    MissionManager*     missionManager(void)    { return _missionManager; }
    GeoFenceManager*    geoFenceManager(void)   { return _geoFenceManager; }
    RallyPointManager*  rallyPointManager(void) { return _rallyPointManager; }

    QGeoCoordinate homePosition(void);

    bool armed(void) { return _armed; }
    void setArmed(bool armed);

    bool flightModeSetAvailable(void);
    QStringList flightModes(void);
    QString flightMode(void) const;
    void setFlightMode(const QString& flightMode);

    QString priorityLinkName(void) const;
    QVariantList links(void) const;
    void setPriorityLinkByName(const QString& priorityLinkName);

    bool hilMode(void);
    void setHilMode(bool hilMode);

    bool fixedWing(void) const;
    bool multiRotor(void) const;
    bool vtol(void) const;
    bool rover(void) const;
    bool sub(void) const;

    bool supportsThrottleModeCenterZero (void) const;
    bool supportsNegativeThrust         (void) const;
    bool supportsRadio                  (void) const;
    bool supportsJSButton               (void) const;
    bool supportsMotorInterference      (void) const;
    bool supportsTerrainFrame           (void) const;

    void setGuidedMode(bool guidedMode);

    QString prearmError(void) const { return _prearmError; }
    void setPrearmError(const QString& prearmError);

    QmlObjectListModel* trajectoryPoints(void) { return &_mapTrajectoryList; }
    QmlObjectListModel* cameraTriggerPoints(void) { return &_cameraTriggerPoints; }
    QmlObjectListModel* adsbVehicles(void) { return &_adsbVehicles; }

    int  flowImageIndex() { return _flowImageIndex; }

    //-- Mavlink Logging
    void startMavlinkLog();
    void stopMavlinkLog();

    /// Requests the specified data stream from the vehicle
    ///     @param stream Stream which is being requested
    ///     @param rate Rate at which to send stream in Hz
    ///     @param sendMultiple Send multiple time to guarantee Vehicle reception
    void requestDataStream(MAV_DATA_STREAM stream, uint16_t rate, bool sendMultiple = true);

    typedef enum {
        MessageNone,
        MessageNormal,
        MessageWarning,
        MessageError
    } MessageType_t;

    bool            messageTypeNone         () { return _currentMessageType == MessageNone; }
    bool            messageTypeNormal       () { return _currentMessageType == MessageNormal; }
    bool            messageTypeWarning      () { return _currentMessageType == MessageWarning; }
    bool            messageTypeError        () { return _currentMessageType == MessageError; }
    int             newMessageCount         () { return _currentMessageCount; }
    int             messageCount            () { return _messageCount; }
    QString         formatedMessages        ();
    QString         formatedMessage         () { return _formatedMessage; }
    QString         latestError             () { return _latestError; }
    float           latitude                () { return _coordinate.latitude(); }
    float           longitude               () { return _coordinate.longitude(); }
    bool            mavPresent              () { return _mav != NULL; }
    int             rcRSSI                  () { return _rcRSSI; }
    bool            px4Firmware             () const { return _firmwareType == MAV_AUTOPILOT_PX4; }
    bool            apmFirmware             () const { return _firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA; }
    bool            genericFirmware         () const { return !px4Firmware() && !apmFirmware(); }
    bool            connectionLost          () const { return _connectionLost; }
    bool            connectionLostEnabled   () const { return _connectionLostEnabled; }
    uint            messagesReceived        () { return _messagesReceived; }
    uint            messagesSent            () { return _messagesSent; }
    uint            messagesLost            () { return _messagesLost; }
    bool            flying                  () const { return _flying; }
    bool            landing                 () const { return _landing; }
    bool            guidedMode              () const;
    bool            vtolInFwdFlight         () const { return _vtolInFwdFlight; }
    uint8_t         baseMode                () const { return _base_mode; }
    uint32_t        customMode              () const { return _custom_mode; }
    bool            isOfflineEditingVehicle () const { return _offlineEditingVehicle; }
    QString         brandImageIndoor        () const;
    QString         brandImageOutdoor       () const;
    QStringList     unhealthySensors        () const;
    int             sensorsPresentBits      () const { return _onboardControlSensorsPresent; }
    int             sensorsEnabledBits      () const { return _onboardControlSensorsEnabled; }
    int             sensorsHealthBits       () const { return _onboardControlSensorsHealth; }
    int             sensorsUnhealthyBits    () const { return _onboardControlSensorsUnhealthy; }
    QString         missionFlightMode       () const;
    QString         pauseFlightMode         () const;
    QString         rtlFlightMode           () const;
    QString         landFlightMode          () const;
    QString         takeControlFlightMode   () const;
    double          defaultCruiseSpeed      () const { return _defaultCruiseSpeed; }
    double          defaultHoverSpeed       () const { return _defaultHoverSpeed; }
    QString         firmwareTypeString      () const;
    QString         vehicleTypeString       () const;
    int             telemetryRRSSI          () { return _telemetryRRSSI; }
    int             telemetryLRSSI          () { return _telemetryLRSSI; }
    unsigned int    telemetryRXErrors       () { return _telemetryRXErrors; }
    unsigned int    telemetryFixed          () { return _telemetryFixed; }
    unsigned int    telemetryTXBuffer       () { return _telemetryTXBuffer; }
    int             telemetryLNoise         () { return _telemetryLNoise; }
    int             telemetryRNoise         () { return _telemetryRNoise; }
    bool            autoDisarm              ();
    bool            highLatencyLink         () const { return _highLatencyLink; }
    /// Get the maximum MAVLink protocol version supported
    /// @return the maximum version
    unsigned        maxProtoVersion         () const { return _maxProtoVersion; }

    Fact* roll              (void) { return &_rollFact; }
    Fact* pitch             (void) { return &_pitchFact; }
    Fact* heading           (void) { return &_headingFact; }
    Fact* rollRate          (void) { return &_rollRateFact; }
    Fact* pitchRate         (void) { return &_pitchRateFact; }
    Fact* yawRate           (void) { return &_yawRateFact; }
    Fact* airSpeed          (void) { return &_airSpeedFact; }
    Fact* groundSpeed       (void) { return &_groundSpeedFact; }
    Fact* climbRate         (void) { return &_climbRateFact; }
    Fact* altitudeRelative  (void) { return &_altitudeRelativeFact; }
    Fact* altitudeAMSL      (void) { return &_altitudeAMSLFact; }
    Fact* flightDistance    (void) { return &_flightDistanceFact; }
    Fact* distanceToHome    (void) { return &_distanceToHomeFact; }
    Fact* hobbs             (void) { return &_hobbsFact; }

    FactGroup* gpsFactGroup             (void) { return &_gpsFactGroup; }
    FactGroup* battery1FactGroup        (void) { return &_battery1FactGroup; }
    FactGroup* battery2FactGroup        (void) { return &_battery2FactGroup; }
    FactGroup* windFactGroup            (void) { return &_windFactGroup; }
    FactGroup* vibrationFactGroup       (void) { return &_vibrationFactGroup; }
    FactGroup* temperatureFactGroup     (void) { return &_temperatureFactGroup; }
    FactGroup* clockFactGroup           (void) { return &_clockFactGroup; }
    FactGroup* setpointFactGroup        (void) { return &_setpointFactGroup; }
    FactGroup* distanceSensorFactGroup  (void) { return &_distanceSensorFactGroup; }

    void setConnectionLostEnabled(bool connectionLostEnabled);

    ParameterManager* parameterManager(void) { return _parameterManager; }
    ParameterManager* parameterManager(void) const { return _parameterManager; }

    static const int cMaxRcChannels = 18;

    bool containsLink(LinkInterface* link) { return _links.contains(link); }

    /// Sends the specified MAV_CMD to the vehicle. If no Ack is received command will be retried. If a sendMavCommand is already in progress
    /// the command will be queued and sent when the previous command completes.
    ///     @param component Component to send to
    ///     @param command MAV_CMD to send
    ///     @param showError true: Display error to user if command failed, false:  no error shown
    /// Signals: mavCommandResult on success or failure
    void sendMavCommand(int component, MAV_CMD command, bool showError, float param1 = 0.0f, float param2 = 0.0f, float param3 = 0.0f, float param4 = 0.0f, float param5 = 0.0f, float param6 = 0.0f, float param7 = 0.0f);
    void sendMavCommandInt(int component, MAV_CMD command, MAV_FRAME frame, bool showError, float param1, float param2, float param3, float param4, double param5, double param6, float param7);

    /// Same as sendMavCommand but available from Qml.
    Q_INVOKABLE void sendCommand(int component, int command, bool showError, double param1 = 0.0f, double param2 = 0.0f, double param3 = 0.0f, double param4 = 0.0f, double param5 = 0.0f, double param6 = 0.0f, double param7 = 0.0f)
        { sendMavCommand(component, (MAV_CMD)command, showError, param1, param2, param3, param4, param5, param6, param7); }

    int firmwareMajorVersion(void) const { return _firmwareMajorVersion; }
    int firmwareMinorVersion(void) const { return _firmwareMinorVersion; }
    int firmwarePatchVersion(void) const { return _firmwarePatchVersion; }
    int firmwareVersionType(void) const { return _firmwareVersionType; }
    int firmwareCustomMajorVersion(void) const { return _firmwareCustomMajorVersion; }
    int firmwareCustomMinorVersion(void) const { return _firmwareCustomMinorVersion; }
    int firmwareCustomPatchVersion(void) const { return _firmwareCustomPatchVersion; }
    QString firmwareVersionTypeString(void) const;
    void setFirmwareVersion(int majorVersion, int minorVersion, int patchVersion, FIRMWARE_VERSION_TYPE versionType = FIRMWARE_VERSION_TYPE_OFFICIAL);
    void setFirmwareCustomVersion(int majorVersion, int minorVersion, int patchVersion);
    static const int versionNotSetValue = -1;

    QString gitHash(void) const { return _gitHash; }
    quint64 vehicleUID(void) const { return _uid; }
    QString vehicleUIDStr();

    bool soloFirmware(void) const { return _soloFirmware; }
    void setSoloFirmware(bool soloFirmware);

    int defaultComponentId(void) { return _defaultComponentId; }

    /// Sets the default component id for an offline editing vehicle
    void setOfflineEditingDefaultComponentId(int defaultComponentId);

    /// @return -1 = Unknown, Number of motors on vehicle
    int motorCount(void);

    /// @return true: Motors are coaxial like an X8 config, false: Quadcopter for example
    bool coaxialMotors(void);

    /// @return true: X confiuration, false: Plus configuration
    bool xConfigMotors(void);

    /// @return Firmware plugin instance data associated with this Vehicle
    QObject* firmwarePluginInstanceData(void) { return _firmwarePluginInstanceData; }

    /// Sets the firmware plugin instance data associated with this Vehicle. This object will be parented to the Vehicle
    /// and destroyed when the vehicle goes away.
    void setFirmwarePluginInstanceData(QObject* firmwarePluginInstanceData);

    QString vehicleImageOpaque  () const;
    QString vehicleImageOutline () const;
    QString vehicleImageCompass () const;

    const QVariantList&         toolBarIndicators   ();
    const QVariantList&         staticCameraList    (void) const;

    bool capabilitiesKnown      (void) const { return _vehicleCapabilitiesKnown; }
    uint64_t capabilityBits     (void) const { return _capabilityBits; }    // Change signalled by capabilityBitsChanged

    QGCCameraManager*           dynamicCameras      () { return _cameras; }
    QString                     hobbsMeter          ();

    /// @true: When flying a mission the vehicle is always facing towards the next waypoint
    bool vehicleYawsToNextWaypointInMission(void) const;

    /// The vehicle is responsible for making the initial request for the Plan.
    /// @return: true: initial request is complete, false: initial request is still in progress;
    bool initialPlanRequestComplete(void) const { return _initialPlanRequestComplete; }

    void forceInitialPlanRequestComplete(void);

    void _setFlying(bool flying);
    void _setLanding(bool landing);
    void setVtolInFwdFlight(bool vtolInFwdFlight);
    void _setHomePosition(QGeoCoordinate& homeCoord);
    void _setMaxProtoVersion (unsigned version);

    /// Vehicle is about to be deleted
    void prepareDelete();

signals:
    void allLinksInactive(Vehicle* vehicle);
    void coordinateChanged(QGeoCoordinate coordinate);
    void joystickModeChanged(int mode);
    void joystickEnabledChanged(bool enabled);
    void activeChanged(bool active);
    void mavlinkMessageReceived(const mavlink_message_t& message);
    void homePositionChanged(const QGeoCoordinate& homePosition);
    void armedChanged(bool armed);
    void flightModeChanged(const QString& flightMode);
    void hilModeChanged(bool hilMode);
    /** @brief HIL actuator controls (replaces HIL controls) */
    void hilActuatorControlsChanged(quint64 time, quint64 flags, float ctl_0, float ctl_1, float ctl_2, float ctl_3, float ctl_4, float ctl_5, float ctl_6, float ctl_7, float ctl_8, float ctl_9, float ctl_10, float ctl_11, float ctl_12, float ctl_13, float ctl_14, float ctl_15, quint8 mode);
    void connectionLostChanged(bool connectionLost);
    void connectionLostEnabledChanged(bool connectionLostEnabled);
    void autoDisconnectChanged(bool autoDisconnectChanged);
    void flyingChanged(bool flying);
    void landingChanged(bool landing);
    void guidedModeChanged(bool guidedMode);
    void vtolInFwdFlightChanged(bool vtolInFwdFlight);
    void prearmErrorChanged(const QString& prearmError);
    void soloFirmwareChanged(bool soloFirmware);
    void unhealthySensorsChanged(void);
    void defaultCruiseSpeedChanged(double cruiseSpeed);
    void defaultHoverSpeedChanged(double hoverSpeed);
    void firmwareTypeChanged(void);
    void vehicleTypeChanged(void);
    void dynamicCamerasChanged();
    void hobbsMeterChanged();
    void capabilitiesKnownChanged(bool capabilitiesKnown);
    void initialPlanRequestCompleteChanged(bool initialPlanRequestComplete);
    void capabilityBitsChanged(uint64_t capabilityBits);
    void toolBarIndicatorsChanged(void);
    void highLatencyLinkChanged(bool highLatencyLink);
    void priorityLinkNameChanged(const QString& priorityLinkName);
    void linksChanged(void);
    void linksPropertiesChanged(void);

    void messagesReceivedChanged    ();
    void messagesSentChanged        ();
    void messagesLostChanged        ();

    /// Used internally to move sendMessage call to main thread
    void _sendMessageOnLinkOnThread(LinkInterface* link, mavlink_message_t message);

    void messageTypeChanged         ();
    void newMessageCountChanged     ();
    void messageCountChanged        ();
    void formatedMessagesChanged    ();
    void formatedMessageChanged     ();
    void latestErrorChanged         ();
    void longitudeChanged           ();
    void currentConfigChanged       ();
    void flowImageIndexChanged      ();
    void rcRSSIChanged              (int rcRSSI);
    void telemetryRRSSIChanged      (int value);
    void telemetryLRSSIChanged      (int value);
    void telemetryRXErrorsChanged   (unsigned int value);
    void telemetryFixedChanged      (unsigned int value);
    void telemetryTXBufferChanged   (unsigned int value);
    void telemetryLNoiseChanged     (int value);
    void telemetryRNoiseChanged     (int value);
    void autoDisarmChanged          (void);
    void flightModesChanged         (void);
    void sensorsPresentBitsChanged  (int sensorsPresentBits);
    void sensorsEnabledBitsChanged  (int sensorsEnabledBits);
    void sensorsHealthBitsChanged   (int sensorsHealthBits);
    void sensorsUnhealthyBitsChanged(int sensorsUnhealthyBits);

    void firmwareVersionChanged(void);
    void firmwareCustomVersionChanged(void);
    void gitHashChanged(QString hash);
    void vehicleUIDChanged();

    /// New RC channel values
    ///     @param channelCount Number of available channels, cMaxRcChannels max
    ///     @param pwmValues -1 signals channel not available
    void rcChannelsChanged(int channelCount, int pwmValues[cMaxRcChannels]);

    /// Remote control RSSI changed  (0% - 100%)
    void remoteControlRSSIChanged(uint8_t rssi);

    void mavlinkRawImu(mavlink_message_t message);
    void mavlinkScaledImu1(mavlink_message_t message);
    void mavlinkScaledImu2(mavlink_message_t message);
    void mavlinkScaledImu3(mavlink_message_t message);

    // Mavlink Log Download
    void mavlinkLogData (Vehicle* vehicle, uint8_t target_system, uint8_t target_component, uint16_t sequence, uint8_t first_message, QByteArray data, bool acked);

    /// Signalled in response to usage of sendMavCommand
    ///     @param vehicleId Vehicle which command was sent to
    ///     @param component Component which command was sent to
    ///     @param command MAV_CMD Command which was sent
    ///     @param result MAV_RESULT returned in ack
    ///     @param noResponseFromVehicle true: vehicle did not respond to command, false: vehicle responsed, MAV_RESULT in result
    void mavCommandResult(int vehicleId, int component, int command, int result, bool noReponseFromVehicle);

    // MAVlink Serial Data
    void mavlinkSerialControl(uint8_t device, uint8_t flags, uint16_t timeout, uint32_t baudrate, QByteArray data);

    // MAVLink protocol version
    void requestProtocolVersion(unsigned version);

private slots:
    void _mavlinkMessageReceived(LinkInterface* link, mavlink_message_t message);
    void _linkInactiveOrDeleted(LinkInterface* link);
    void _sendMessageOnLink(LinkInterface* link, mavlink_message_t message);
    void _sendMessageMultipleNext(void);
    void _addNewMapTrajectoryPoint(void);
    void _parametersReady(bool parametersReady);
    void _remoteControlRSSIChanged(uint8_t rssi);
    void _handleFlightModeChanged(const QString& flightMode);
    void _announceArmedChanged(bool armed);
    void _offlineFirmwareTypeSettingChanged(QVariant value);
    void _offlineVehicleTypeSettingChanged(QVariant value);
    void _offlineCruiseSpeedSettingChanged(QVariant value);
    void _offlineHoverSpeedSettingChanged(QVariant value);
    void _updateHighLatencyLink(bool sendCommand = true);

    void _handleTextMessage                 (int newCount);
    void _handletextMessageReceived         (UASMessage* message);
    /** @brief A new camera image has arrived */
    void _imageReady                        (UASInterface* uas);
    void _prearmErrorTimeout(void);
    void _missionLoadComplete(void);
    void _geoFenceLoadComplete(void);
    void _rallyPointLoadComplete(void);
    void _sendMavCommandAgain(void);
    void _clearTrajectoryPoints(void);
    void _clearCameraTriggerPoints(void);
    void _updateDistanceToHome(void);
    void _updateHobbsMeter(void);
    void _vehicleParamLoaded(bool ready);
    void _sendQGCTimeToVehicle(void);

private:
    bool _containsLink(LinkInterface* link);
    void _addLink(LinkInterface* link);
    void _loadSettings(void);
    void _saveSettings(void);
    void _startJoystick(bool start);
    void _handlePing(LinkInterface* link, mavlink_message_t& message);
    void _handleHomePosition(mavlink_message_t& message);
    void _handleHeartbeat(mavlink_message_t& message);
    void _handleRadioStatus(mavlink_message_t& message);
    void _handleRCChannels(mavlink_message_t& message);
    void _handleRCChannelsRaw(mavlink_message_t& message);
    void _handleBatteryStatus(mavlink_message_t& message);
    void _handleSysStatus(mavlink_message_t& message);
    void _handleWindCov(mavlink_message_t& message);
    void _handleVibration(mavlink_message_t& message);
    void _handleExtendedSysState(mavlink_message_t& message);
    void _handleCommandAck(mavlink_message_t& message);
    void _handleCommandLong(mavlink_message_t& message);
    void _handleAutopilotVersion(LinkInterface* link, mavlink_message_t& message);
    void _handleProtocolVersion(LinkInterface* link, mavlink_message_t& message);
    void _handleHilActuatorControls(mavlink_message_t& message);
    void _handleGpsRawInt(mavlink_message_t& message);
    void _handleGlobalPositionInt(mavlink_message_t& message);
    void _handleAltitude(mavlink_message_t& message);
    void _handleVfrHud(mavlink_message_t& message);
    void _handleScaledPressure(mavlink_message_t& message);
    void _handleScaledPressure2(mavlink_message_t& message);
    void _handleScaledPressure3(mavlink_message_t& message);
    void _handleHighLatency2(mavlink_message_t& message);
    void _handleAttitudeWorker(double rollRadians, double pitchRadians, double yawRadians);
    void _handleAttitude(mavlink_message_t& message);
    void _handleAttitudeQuaternion(mavlink_message_t& message);
    void _handleAttitudeTarget(mavlink_message_t& message);
    void _handleDistanceSensor(mavlink_message_t& message);
    // ArduPilot dialect messages
#if !defined(NO_ARDUPILOT_DIALECT)
    void _handleCameraFeedback(const mavlink_message_t& message);
    void _handleWind(mavlink_message_t& message);
#endif
    void _handleCameraImageCaptured(const mavlink_message_t& message);
    void _handleADSBVehicle(const mavlink_message_t& message);
    void _missionManagerError(int errorCode, const QString& errorMsg);
    void _geoFenceManagerError(int errorCode, const QString& errorMsg);
    void _rallyPointManagerError(int errorCode, const QString& errorMsg);
    void _mapTrajectoryStart(void);
    void _mapTrajectoryStop(void);
    void _linkActiveChanged(LinkInterface* link, bool active, int vehicleID);
    void _say(const QString& text);
    QString _vehicleIdSpeech(void);
    void _handleMavlinkLoggingData(mavlink_message_t& message);
    void _handleMavlinkLoggingDataAcked(mavlink_message_t& message);
    void _ackMavlinkLogData(uint16_t sequence);
    void _sendNextQueuedMavCommand(void);
    void _updatePriorityLink(bool updateActive, bool sendCommand);
    void _commonInit(void);
    void _startPlanRequest(void);
    void _setupAutoDisarmSignalling(void);
    void _setCapabilities(uint64_t capabilityBits);
    void _updateArmed(bool armed);
    bool _apmArmingNotRequired(void);

    int     _id;                    ///< Mavlink system id
    int     _defaultComponentId;
    bool    _active;
    bool    _offlineEditingVehicle; ///< This Vehicle is a "disconnected" vehicle for ui use while offline editing

    MAV_AUTOPILOT       _firmwareType;
    MAV_TYPE            _vehicleType;
    FirmwarePlugin*     _firmwarePlugin;
    QObject*            _firmwarePluginInstanceData;
    AutoPilotPlugin*    _autopilotPlugin;
    MAVLinkProtocol*    _mavlink;
    bool                _soloFirmware;
    QGCToolbox*         _toolbox;
    SettingsManager*    _settingsManager;

    QList<LinkInterface*> _links;

    JoystickMode_t  _joystickMode;
    bool            _joystickEnabled;

    UAS* _uas;

    QGeoCoordinate  _coordinate;
    QGeoCoordinate  _homePosition;

    UASInterface*   _mav;
    int             _currentMessageCount;
    int             _messageCount;
    int             _currentErrorCount;
    int             _currentWarningCount;
    int             _currentNormalCount;
    MessageType_t   _currentMessageType;
    QString         _latestError;
    int             _updateCount;
    QString         _formatedMessage;
    int             _rcRSSI;
    double          _rcRSSIstore;
    bool            _autoDisconnect;    ///< true: Automatically disconnect vehicle when last connection goes away or lost heartbeat
    bool            _flying;
    bool            _landing;
    bool            _vtolInFwdFlight;
    uint32_t        _onboardControlSensorsPresent;
    uint32_t        _onboardControlSensorsEnabled;
    uint32_t        _onboardControlSensorsHealth;
    uint32_t        _onboardControlSensorsUnhealthy;
    bool            _gpsRawIntMessageAvailable;
    bool            _globalPositionIntMessageAvailable;
    double          _defaultCruiseSpeed;
    double          _defaultHoverSpeed;
    int             _telemetryRRSSI;
    int             _telemetryLRSSI;
    uint32_t        _telemetryRXErrors;
    uint32_t        _telemetryFixed;
    uint32_t        _telemetryTXBuffer;
    int             _telemetryLNoise;
    int             _telemetryRNoise;
    unsigned        _maxProtoVersion;
    bool            _vehicleCapabilitiesKnown;
    uint64_t        _capabilityBits;
    bool            _highLatencyLink;
    bool            _receivingAttitudeQuaternion;

    QGCCameraManager* _cameras;

    typedef struct {
        int         component;
        bool        commandInt; // true: use COMMAND_INT, false: use COMMAND_LONG
        MAV_CMD     command;
        MAV_FRAME   frame;
        double      rgParam[7];
        bool        showError;
    } MavCommandQueueEntry_t;

    QList<MavCommandQueueEntry_t>   _mavCommandQueue;
    QTimer                          _mavCommandAckTimer;
    int                             _mavCommandRetryCount;
    static const int                _mavCommandMaxRetryCount = 3;
    static const int                _mavCommandAckTimeoutMSecs = 3000;
    static const int                _mavCommandAckTimeoutMSecsHighLatency = 120000;

    QString             _prearmError;
    QTimer              _prearmErrorTimer;
    static const int    _prearmErrorTimeoutMSecs = 35 * 1000;   ///< Take away prearm error after 35 seconds

    // Lost connection handling
    bool                _connectionLost;
    bool                _connectionLostEnabled;

    bool                _initialPlanRequestComplete;

    MissionManager*     _missionManager;
    bool                _missionManagerInitialRequestSent;

    GeoFenceManager*    _geoFenceManager;
    bool                _geoFenceManagerInitialRequestSent;

    RallyPointManager*  _rallyPointManager;
    bool                _rallyPointManagerInitialRequestSent;

    ParameterManager*    _parameterManager;

    bool    _armed;         ///< true: vehicle is armed
    uint8_t _base_mode;     ///< base_mode from HEARTBEAT
    uint32_t _custom_mode;  ///< custom_mode from HEARTBEAT

    /// Used to store a message being sent by sendMessageMultiple
    typedef struct {
        mavlink_message_t   message;    ///< Message to send multiple times
        int                 retryCount; ///< Number of retries left
    } SendMessageMultipleInfo_t;

    QList<SendMessageMultipleInfo_t> _sendMessageMultipleList;    ///< List of messages being sent multiple times

    static const int _sendMessageMultipleRetries = 5;
    static const int _sendMessageMultipleIntraMessageDelay = 500;

    QTimer  _sendMultipleTimer;
    int     _nextSendMessageMultipleIndex;

    QTime               _flightTimer;
    QTimer              _mapTrajectoryTimer;
    QmlObjectListModel  _mapTrajectoryList;
    QGeoCoordinate      _mapTrajectoryLastCoordinate;
    bool                _mapTrajectoryHaveFirstCoordinate;
    static const int    _mapTrajectoryMsecsBetweenPoints = 1000;

    QmlObjectListModel  _cameraTriggerPoints;

    QmlObjectListModel              _adsbVehicles;
    QMap<uint32_t, ADSBVehicle*>    _adsbICAOMap;

    // Toolbox references
    FirmwarePluginManager*      _firmwarePluginManager;
    JoystickManager*            _joystickManager;

    int                         _flowImageIndex;

    bool _allLinksInactiveSent; ///< true: allLinkInactive signal already sent one time

    uint                _messagesReceived;
    uint                _messagesSent;
    uint                _messagesLost;
    uint8_t             _messageSeq;
    uint8_t             _compID;
    bool                _heardFrom;

    int _firmwareMajorVersion;
    int _firmwareMinorVersion;
    int _firmwarePatchVersion;
    int _firmwareCustomMajorVersion;
    int _firmwareCustomMinorVersion;
    int _firmwareCustomPatchVersion;
    FIRMWARE_VERSION_TYPE _firmwareVersionType;

    QString _gitHash;
    quint64 _uid;

    int _lastAnnouncedLowBatteryPercent;

    SharedLinkInterfacePointer _priorityLink;  // We always keep a reference to the priority link to manage shutdown ordering
    bool _priorityLinkCommanded;

    // FactGroup facts

    Fact _rollFact;
    Fact _pitchFact;
    Fact _headingFact;
    Fact _rollRateFact;
    Fact _pitchRateFact;
    Fact _yawRateFact;
    Fact _groundSpeedFact;
    Fact _airSpeedFact;
    Fact _climbRateFact;
    Fact _altitudeRelativeFact;
    Fact _altitudeAMSLFact;
    Fact _flightDistanceFact;
    Fact _flightTimeFact;
    Fact _distanceToHomeFact;
    Fact _hobbsFact;

    VehicleGPSFactGroup             _gpsFactGroup;
    VehicleBatteryFactGroup         _battery1FactGroup;
    VehicleBatteryFactGroup         _battery2FactGroup;
    VehicleWindFactGroup            _windFactGroup;
    VehicleVibrationFactGroup       _vibrationFactGroup;
    VehicleTemperatureFactGroup     _temperatureFactGroup;
    VehicleClockFactGroup           _clockFactGroup;
    VehicleSetpointFactGroup        _setpointFactGroup;
    VehicleDistanceSensorFactGroup  _distanceSensorFactGroup;

    static const char* _rollFactName;
    static const char* _pitchFactName;
    static const char* _headingFactName;
    static const char* _rollRateFactName;
    static const char* _pitchRateFactName;
    static const char* _yawRateFactName;
    static const char* _groundSpeedFactName;
    static const char* _airSpeedFactName;
    static const char* _climbRateFactName;
    static const char* _altitudeRelativeFactName;
    static const char* _altitudeAMSLFactName;
    static const char* _flightDistanceFactName;
    static const char* _flightTimeFactName;
    static const char* _distanceToHomeFactName;
    static const char* _hobbsFactName;

    static const char* _gpsFactGroupName;
    static const char* _battery1FactGroupName;
    static const char* _battery2FactGroupName;
    static const char* _windFactGroupName;
    static const char* _vibrationFactGroupName;
    static const char* _temperatureFactGroupName;
    static const char* _clockFactGroupName;
    static const char* _distanceSensorFactGroupName;

    static const int _vehicleUIUpdateRateMSecs = 100;

    // Settings keys
    static const char* _settingsGroup;
    static const char* _joystickModeSettingsKey;
    static const char* _joystickEnabledSettingsKey;

};
