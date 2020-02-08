/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
#include "QGCMapCircle.h"

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
class QGCCameraManager;
class Joystick;
class VehicleObjectAvoidance;
class TrajectoryPoints;

#if defined(QGC_AIRMAP_ENABLED)
class AirspaceVehicleManager;
#endif

Q_DECLARE_LOGGING_CATEGORY(VehicleLog)

class Vehicle;

class VehicleDistanceSensorFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleDistanceSensorFactGroup(QObject* parent = nullptr);

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

    Fact* rotationNone      () { return &_rotationNoneFact; }
    Fact* rotationYaw45     () { return &_rotationYaw45Fact; }
    Fact* rotationYaw90     () { return &_rotationYaw90Fact; }
    Fact* rotationYaw135    () { return &_rotationYaw90Fact; }
    Fact* rotationYaw180    () { return &_rotationYaw180Fact; }
    Fact* rotationYaw225    () { return &_rotationYaw180Fact; }
    Fact* rotationYaw270    () { return &_rotationYaw270Fact; }
    Fact* rotationYaw315    () { return &_rotationYaw315Fact; }
    Fact* rotationPitch90   () { return &_rotationPitch90Fact; }
    Fact* rotationPitch270  () { return &_rotationPitch270Fact; }

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
};

class VehicleSetpointFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleSetpointFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* roll       READ roll       CONSTANT)
    Q_PROPERTY(Fact* pitch      READ pitch      CONSTANT)
    Q_PROPERTY(Fact* yaw        READ yaw        CONSTANT)
    Q_PROPERTY(Fact* rollRate   READ rollRate   CONSTANT)
    Q_PROPERTY(Fact* pitchRate  READ pitchRate  CONSTANT)
    Q_PROPERTY(Fact* yawRate    READ yawRate    CONSTANT)

    Fact* roll      () { return &_rollFact; }
    Fact* pitch     () { return &_pitchFact; }
    Fact* yaw       () { return &_yawFact; }
    Fact* rollRate  () { return &_rollRateFact; }
    Fact* pitchRate () { return &_pitchRateFact; }
    Fact* yawRate   () { return &_yawRateFact; }

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
    VehicleVibrationFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* xAxis      READ xAxis      CONSTANT)
    Q_PROPERTY(Fact* yAxis      READ yAxis      CONSTANT)
    Q_PROPERTY(Fact* zAxis      READ zAxis      CONSTANT)
    Q_PROPERTY(Fact* clipCount1 READ clipCount1 CONSTANT)
    Q_PROPERTY(Fact* clipCount2 READ clipCount2 CONSTANT)
    Q_PROPERTY(Fact* clipCount3 READ clipCount3 CONSTANT)

    Fact* xAxis         () { return &_xAxisFact; }
    Fact* yAxis         () { return &_yAxisFact; }
    Fact* zAxis         () { return &_zAxisFact; }
    Fact* clipCount1    () { return &_clipCount1Fact; }
    Fact* clipCount2    () { return &_clipCount2Fact; }
    Fact* clipCount3    () { return &_clipCount3Fact; }

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
    VehicleWindFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* direction      READ direction      CONSTANT)
    Q_PROPERTY(Fact* speed          READ speed          CONSTANT)
    Q_PROPERTY(Fact* verticalSpeed  READ verticalSpeed  CONSTANT)

    Fact* direction     () { return &_directionFact; }
    Fact* speed         () { return &_speedFact; }
    Fact* verticalSpeed () { return &_verticalSpeedFact; }

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
    VehicleGPSFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* lat                READ lat                CONSTANT)
    Q_PROPERTY(Fact* lon                READ lon                CONSTANT)
    Q_PROPERTY(Fact* mgrs               READ mgrs               CONSTANT)
    Q_PROPERTY(Fact* hdop               READ hdop               CONSTANT)
    Q_PROPERTY(Fact* vdop               READ vdop               CONSTANT)
    Q_PROPERTY(Fact* courseOverGround   READ courseOverGround   CONSTANT)
    Q_PROPERTY(Fact* count              READ count              CONSTANT)
    Q_PROPERTY(Fact* lock               READ lock               CONSTANT)

    Fact* lat               () { return &_latFact; }
    Fact* lon               () { return &_lonFact; }
    Fact* mgrs              () { return &_mgrsFact; }
    Fact* hdop              () { return &_hdopFact; }
    Fact* vdop              () { return &_vdopFact; }
    Fact* courseOverGround  () { return &_courseOverGroundFact; }
    Fact* count             () { return &_countFact; }
    Fact* lock              () { return &_lockFact; }

    static const char* _latFactName;
    static const char* _lonFactName;
    static const char* _mgrsFactName;
    static const char* _hdopFactName;
    static const char* _vdopFactName;
    static const char* _courseOverGroundFactName;
    static const char* _countFactName;
    static const char* _lockFactName;

private:
    Fact        _latFact;
    Fact        _lonFact;
    Fact        _mgrsFact;
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
    VehicleBatteryFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* voltage            READ voltage            CONSTANT)
    Q_PROPERTY(Fact* percentRemaining   READ percentRemaining   CONSTANT)
    Q_PROPERTY(Fact* mahConsumed        READ mahConsumed        CONSTANT)
    Q_PROPERTY(Fact* current            READ current            CONSTANT)
    Q_PROPERTY(Fact* temperature        READ temperature        CONSTANT)
    Q_PROPERTY(Fact* instantPower       READ instantPower       CONSTANT)
    Q_PROPERTY(Fact* timeRemaining      READ timeRemaining      CONSTANT)
    Q_PROPERTY(Fact* chargeState        READ chargeState        CONSTANT)

    Fact* voltage                   () { return &_voltageFact; }
    Fact* percentRemaining          () { return &_percentRemainingFact; }
    Fact* mahConsumed               () { return &_mahConsumedFact; }
    Fact* current                   () { return &_currentFact; }
    Fact* temperature               () { return &_temperatureFact; }
    Fact* instantPower              () { return &_instantPowerFact; }
    Fact* timeRemaining             () { return &_timeRemainingFact; }
    Fact* chargeState               () { return &_chargeStateFact; }

    static const char* _voltageFactName;
    static const char* _percentRemainingFactName;
    static const char* _mahConsumedFactName;
    static const char* _currentFactName;
    static const char* _temperatureFactName;
    static const char* _instantPowerFactName;
    static const char* _timeRemainingFactName;
    static const char* _chargeStateFactName;

    static const char* _settingsGroup;

private:
    Fact            _voltageFact;
    Fact            _percentRemainingFact;
    Fact            _mahConsumedFact;
    Fact            _currentFact;
    Fact            _temperatureFact;
    Fact            _instantPowerFact;
    Fact            _timeRemainingFact;
    Fact            _chargeStateFact;
};

class VehicleTemperatureFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleTemperatureFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* temperature1       READ temperature1       CONSTANT)
    Q_PROPERTY(Fact* temperature2       READ temperature2       CONSTANT)
    Q_PROPERTY(Fact* temperature3       READ temperature3       CONSTANT)

    Fact* temperature1 () { return &_temperature1Fact; }
    Fact* temperature2 () { return &_temperature2Fact; }
    Fact* temperature3 () { return &_temperature3Fact; }

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
    VehicleClockFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* currentTime        READ currentTime        CONSTANT)
    Q_PROPERTY(Fact* currentDate        READ currentDate        CONSTANT)

    Fact* currentTime () { return &_currentTimeFact; }
    Fact* currentDate () { return &_currentDateFact; }

    static const char* _currentTimeFactName;
    static const char* _currentDateFactName;

    static const char* _settingsGroup;

private slots:
    void _updateAllValues() override;

private:
    Fact            _currentTimeFact;
    Fact            _currentDateFact;
};

class VehicleEstimatorStatusFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleEstimatorStatusFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* goodAttitudeEstimate           READ goodAttitudeEstimate           CONSTANT)
    Q_PROPERTY(Fact* goodHorizVelEstimate           READ goodHorizVelEstimate           CONSTANT)
    Q_PROPERTY(Fact* goodVertVelEstimate            READ goodVertVelEstimate            CONSTANT)
    Q_PROPERTY(Fact* goodHorizPosRelEstimate        READ goodHorizPosRelEstimate        CONSTANT)
    Q_PROPERTY(Fact* goodHorizPosAbsEstimate        READ goodHorizPosAbsEstimate        CONSTANT)
    Q_PROPERTY(Fact* goodVertPosAbsEstimate         READ goodVertPosAbsEstimate         CONSTANT)
    Q_PROPERTY(Fact* goodVertPosAGLEstimate         READ goodVertPosAGLEstimate         CONSTANT)
    Q_PROPERTY(Fact* goodConstPosModeEstimate       READ goodConstPosModeEstimate       CONSTANT)
    Q_PROPERTY(Fact* goodPredHorizPosRelEstimate    READ goodPredHorizPosRelEstimate    CONSTANT)
    Q_PROPERTY(Fact* goodPredHorizPosAbsEstimate    READ goodPredHorizPosAbsEstimate    CONSTANT)
    Q_PROPERTY(Fact* gpsGlitch                      READ gpsGlitch                      CONSTANT)
    Q_PROPERTY(Fact* accelError                     READ accelError                     CONSTANT)
    Q_PROPERTY(Fact* velRatio                       READ velRatio                       CONSTANT)
    Q_PROPERTY(Fact* horizPosRatio                  READ horizPosRatio                  CONSTANT)
    Q_PROPERTY(Fact* vertPosRatio                   READ vertPosRatio                   CONSTANT)
    Q_PROPERTY(Fact* magRatio                       READ magRatio                       CONSTANT)
    Q_PROPERTY(Fact* haglRatio                      READ haglRatio                      CONSTANT)
    Q_PROPERTY(Fact* tasRatio                       READ tasRatio                       CONSTANT)
    Q_PROPERTY(Fact* horizPosAccuracy               READ horizPosAccuracy               CONSTANT)
    Q_PROPERTY(Fact* vertPosAccuracy                READ vertPosAccuracy                CONSTANT)

    Fact* goodAttitudeEstimate          () { return &_goodAttitudeEstimateFact; }
    Fact* goodHorizVelEstimate          () { return &_goodHorizVelEstimateFact; }
    Fact* goodVertVelEstimate           () { return &_goodVertVelEstimateFact; }
    Fact* goodHorizPosRelEstimate       () { return &_goodHorizPosRelEstimateFact; }
    Fact* goodHorizPosAbsEstimate       () { return &_goodHorizPosAbsEstimateFact; }
    Fact* goodVertPosAbsEstimate        () { return &_goodVertPosAbsEstimateFact; }
    Fact* goodVertPosAGLEstimate        () { return &_goodVertPosAGLEstimateFact; }
    Fact* goodConstPosModeEstimate      () { return &_goodConstPosModeEstimateFact; }
    Fact* goodPredHorizPosRelEstimate   () { return &_goodPredHorizPosRelEstimateFact; }
    Fact* goodPredHorizPosAbsEstimate   () { return &_goodPredHorizPosAbsEstimateFact; }
    Fact* gpsGlitch                     () { return &_gpsGlitchFact; }
    Fact* accelError                    () { return &_accelErrorFact; }
    Fact* velRatio                      () { return &_velRatioFact; }
    Fact* horizPosRatio                 () { return &_horizPosRatioFact; }
    Fact* vertPosRatio                  () { return &_vertPosRatioFact; }
    Fact* magRatio                      () { return &_magRatioFact; }
    Fact* haglRatio                     () { return &_haglRatioFact; }
    Fact* tasRatio                      () { return &_tasRatioFact; }
    Fact* horizPosAccuracy              () { return &_horizPosAccuracyFact; }
    Fact* vertPosAccuracy               () { return &_vertPosAccuracyFact; }

    static const char* _goodAttitudeEstimateFactName;
    static const char* _goodHorizVelEstimateFactName;
    static const char* _goodVertVelEstimateFactName;
    static const char* _goodHorizPosRelEstimateFactName;
    static const char* _goodHorizPosAbsEstimateFactName;
    static const char* _goodVertPosAbsEstimateFactName;
    static const char* _goodVertPosAGLEstimateFactName;
    static const char* _goodConstPosModeEstimateFactName;
    static const char* _goodPredHorizPosRelEstimateFactName;
    static const char* _goodPredHorizPosAbsEstimateFactName;
    static const char* _gpsGlitchFactName;
    static const char* _accelErrorFactName;
    static const char* _velRatioFactName;
    static const char* _horizPosRatioFactName;
    static const char* _vertPosRatioFactName;
    static const char* _magRatioFactName;
    static const char* _haglRatioFactName;
    static const char* _tasRatioFactName;
    static const char* _horizPosAccuracyFactName;
    static const char* _vertPosAccuracyFactName;

private:
    Fact _goodAttitudeEstimateFact;
    Fact _goodHorizVelEstimateFact;
    Fact _goodVertVelEstimateFact;
    Fact _goodHorizPosRelEstimateFact;
    Fact _goodHorizPosAbsEstimateFact;
    Fact _goodVertPosAbsEstimateFact;
    Fact _goodVertPosAGLEstimateFact;
    Fact _goodConstPosModeEstimateFact;
    Fact _goodPredHorizPosRelEstimateFact;
    Fact _goodPredHorizPosAbsEstimateFact;
    Fact _gpsGlitchFact;
    Fact _accelErrorFact;
    Fact _velRatioFact;
    Fact _horizPosRatioFact;
    Fact _vertPosRatioFact;
    Fact _magRatioFact;
    Fact _haglRatioFact;
    Fact _tasRatioFact;
    Fact _horizPosAccuracyFact;
    Fact _vertPosAccuracyFact;

#if 0
    typedef enum ESTIMATOR_STATUS_FLAGS
    {
       ESTIMATOR_ATTITUDE=1, /* True if the attitude estimate is good | */
       ESTIMATOR_VELOCITY_HORIZ=2, /* True if the horizontal velocity estimate is good | */
       ESTIMATOR_VELOCITY_VERT=4, /* True if the  vertical velocity estimate is good | */
       ESTIMATOR_POS_HORIZ_REL=8, /* True if the horizontal position (relative) estimate is good | */
       ESTIMATOR_POS_HORIZ_ABS=16, /* True if the horizontal position (absolute) estimate is good | */
       ESTIMATOR_POS_VERT_ABS=32, /* True if the vertical position (absolute) estimate is good | */
       ESTIMATOR_POS_VERT_AGL=64, /* True if the vertical position (above ground) estimate is good | */
       ESTIMATOR_CONST_POS_MODE=128, /* True if the EKF is in a constant position mode and is not using external measurements (eg GPS or optical flow) | */
       ESTIMATOR_PRED_POS_HORIZ_REL=256, /* True if the EKF has sufficient data to enter a mode that will provide a (relative) position estimate | */
       ESTIMATOR_PRED_POS_HORIZ_ABS=512, /* True if the EKF has sufficient data to enter a mode that will provide a (absolute) position estimate | */
       ESTIMATOR_GPS_GLITCH=1024, /* True if the EKF has detected a GPS glitch | */
       ESTIMATOR_ACCEL_ERROR=2048, /* True if the EKF has detected bad accelerometer data | */

        typedef struct __mavlink_estimator_status_t {
         uint64_t time_usec; /*< Timestamp (micros since boot or Unix epoch)*/
         float vel_ratio; /*< Velocity innovation test ratio*/
         float pos_horiz_ratio; /*< Horizontal position innovation test ratio*/
         float pos_vert_ratio; /*< Vertical position innovation test ratio*/
         float mag_ratio; /*< Magnetometer innovation test ratio*/
         float hagl_ratio; /*< Height above terrain innovation test ratio*/
         float tas_ratio; /*< True airspeed innovation test ratio*/
         float pos_horiz_accuracy; /*< Horizontal position 1-STD accuracy relative to the EKF local origin (m)*/
         float pos_vert_accuracy; /*< Vertical position 1-STD accuracy relative to the EKF local origin (m)*/
         uint16_t flags; /*< Integer bitmask indicating which EKF outputs are valid. See definition for ESTIMATOR_STATUS_FLAGS.*/
        } mavlink_estimator_status_t;
    };
#endif
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
            QObject*                parent = nullptr);

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

    enum CheckList {
        CheckListNotSetup = 0,
        CheckListPassed,
        CheckListFailed,
    };
    Q_ENUM(CheckList)

    Q_PROPERTY(int                  id                      READ id                                                     CONSTANT)
    Q_PROPERTY(AutoPilotPlugin*     autopilot               MEMBER _autopilotPlugin                                     CONSTANT)
    Q_PROPERTY(QGeoCoordinate       coordinate              READ coordinate                                             NOTIFY coordinateChanged)
    Q_PROPERTY(QGeoCoordinate       homePosition            READ homePosition                                           NOTIFY homePositionChanged)
    Q_PROPERTY(QGeoCoordinate       armedPosition           READ armedPosition                                          NOTIFY armedPositionChanged)
    Q_PROPERTY(bool                 armed                   READ armed                  WRITE setArmed                  NOTIFY armedChanged)
    Q_PROPERTY(bool                 autoDisarm              READ autoDisarm                                             NOTIFY autoDisarmChanged)
    Q_PROPERTY(bool                 flightModeSetAvailable  READ flightModeSetAvailable                                 CONSTANT)
    Q_PROPERTY(QStringList          flightModes             READ flightModes                                            NOTIFY flightModesChanged)
    Q_PROPERTY(QStringList          extraJoystickFlightModes READ extraJoystickFlightModes                              NOTIFY flightModesChanged)
    Q_PROPERTY(QString              flightMode              READ flightMode             WRITE setFlightMode             NOTIFY flightModeChanged)
    Q_PROPERTY(bool                 hilMode                 READ hilMode                WRITE setHilMode                NOTIFY hilModeChanged)
    Q_PROPERTY(TrajectoryPoints*    trajectoryPoints        MEMBER _trajectoryPoints                                    CONSTANT)
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
    Q_PROPERTY(QString              smartRTLFlightMode      READ smartRTLFlightMode                                     CONSTANT)
    Q_PROPERTY(bool                 supportsSmartRTL        READ supportsSmartRTL                                       CONSTANT)
    Q_PROPERTY(QString              landFlightMode          READ landFlightMode                                         CONSTANT)
    Q_PROPERTY(QString              takeControlFlightMode   READ takeControlFlightMode                                  CONSTANT)
    Q_PROPERTY(QString              followFlightMode        READ followFlightMode                                       CONSTANT)
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
    Q_PROPERTY(quint64              mavlinkSentCount        READ mavlinkSentCount                                       NOTIFY mavlinkStatusChanged)
    Q_PROPERTY(quint64              mavlinkReceivedCount    READ mavlinkReceivedCount                                   NOTIFY mavlinkStatusChanged)
    Q_PROPERTY(quint64              mavlinkLossCount        READ mavlinkLossCount                                       NOTIFY mavlinkStatusChanged)
    Q_PROPERTY(float                mavlinkLossPercent      READ mavlinkLossPercent                                     NOTIFY mavlinkStatusChanged)
    Q_PROPERTY(qreal                gimbalRoll              READ gimbalRoll                                             NOTIFY gimbalRollChanged)
    Q_PROPERTY(qreal                gimbalPitch             READ gimbalPitch                                            NOTIFY gimbalPitchChanged)
    Q_PROPERTY(qreal                gimbalYaw               READ gimbalYaw                                              NOTIFY gimbalYawChanged)
    Q_PROPERTY(bool                 gimbalData              READ gimbalData                                             NOTIFY gimbalDataChanged)
    Q_PROPERTY(bool                 isROIEnabled            READ isROIEnabled                                           NOTIFY isROIEnabledChanged)
    Q_PROPERTY(CheckList            checkListState          READ checkListState         WRITE setCheckListState         NOTIFY checkListStateChanged)

    // The following properties relate to Orbit status
    Q_PROPERTY(bool             orbitActive     READ orbitActive        NOTIFY orbitActiveChanged)
    Q_PROPERTY(QGCMapCircle*    orbitMapCircle  READ orbitMapCircle     CONSTANT)

    // Vehicle state used for guided control
    Q_PROPERTY(bool     flying                  READ flying                                         NOTIFY flyingChanged)       ///< Vehicle is flying
    Q_PROPERTY(bool     landing                 READ landing                                        NOTIFY landingChanged)      ///< Vehicle is in landing pattern (DO_LAND_START)
    Q_PROPERTY(bool     guidedMode              READ guidedMode                 WRITE setGuidedMode NOTIFY guidedModeChanged)   ///< Vehicle is in Guided mode and can respond to guided commands
    Q_PROPERTY(bool     guidedModeSupported     READ guidedModeSupported                            CONSTANT)                   ///< Guided mode commands are supported by this vehicle
    Q_PROPERTY(bool     pauseVehicleSupported   READ pauseVehicleSupported                          CONSTANT)                   ///< Pause vehicle command is supported
    Q_PROPERTY(bool     orbitModeSupported      READ orbitModeSupported                             CONSTANT)                   ///< Orbit mode is supported by this vehicle
    Q_PROPERTY(bool     roiModeSupported        READ roiModeSupported                               CONSTANT)                   ///< Orbit mode is supported by this vehicle
    Q_PROPERTY(bool     takeoffVehicleSupported READ takeoffVehicleSupported                        CONSTANT)                   ///< Guided takeoff supported
    Q_PROPERTY(QString  gotoFlightMode          READ gotoFlightMode                                 CONSTANT)                   ///< Flight mode vehicle is in while performing goto

    Q_PROPERTY(ParameterManager*        parameterManager    READ parameterManager   CONSTANT)
    Q_PROPERTY(VehicleObjectAvoidance*  objectAvoidance     READ objectAvoidance    CONSTANT)

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
    Q_PROPERTY(Fact* headingToNextWP    READ headingToNextWP    CONSTANT)
    Q_PROPERTY(Fact* headingToHome      READ headingToHome      CONSTANT)
    Q_PROPERTY(Fact* distanceToGCS      READ distanceToGCS      CONSTANT)
    Q_PROPERTY(Fact* hobbs              READ hobbs              CONSTANT)
    Q_PROPERTY(Fact* throttlePct        READ throttlePct        CONSTANT)

    Q_PROPERTY(FactGroup* gps               READ gpsFactGroup               CONSTANT)
    Q_PROPERTY(FactGroup* battery           READ battery1FactGroup          CONSTANT)
    Q_PROPERTY(FactGroup* battery2          READ battery2FactGroup          CONSTANT)
    Q_PROPERTY(FactGroup* wind              READ windFactGroup              CONSTANT)
    Q_PROPERTY(FactGroup* vibration         READ vibrationFactGroup         CONSTANT)
    Q_PROPERTY(FactGroup* temperature       READ temperatureFactGroup       CONSTANT)
    Q_PROPERTY(FactGroup* clock             READ clockFactGroup             CONSTANT)
    Q_PROPERTY(FactGroup* setpoint          READ setpointFactGroup          CONSTANT)
    Q_PROPERTY(FactGroup* estimatorStatus   READ estimatorStatusFactGroup   CONSTANT)

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

    // Called when the message drop-down is invoked to clear current count
    Q_INVOKABLE void        resetMessages();

    Q_INVOKABLE void virtualTabletJoystickValue(double roll, double pitch, double yaw, double thrust);
    Q_INVOKABLE void disconnectInactiveVehicle();

    /// Command vehicle to return to launch
    Q_INVOKABLE void guidedModeRTL(bool smartRTL);

    /// Command vehicle to land at current location
    Q_INVOKABLE void guidedModeLand();

    /// Command vehicle to takeoff from current location
    Q_INVOKABLE void guidedModeTakeoff(double altitudeRelative);

    /// @return The minimum takeoff altitude (relative) for guided takeoff.
    Q_INVOKABLE double minimumTakeoffAltitude();

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

    /// Command vehicle to keep given point as ROI
    ///     @param centerCoord ROI coordinates
    Q_INVOKABLE void guidedModeROI(const QGeoCoordinate& centerCoord);
    Q_INVOKABLE void stopGuidedModeROI();

    /// Command vehicle to pause at current location. If vehicle supports guide mode, vehicle will be left
    /// in guided mode after pause.
    Q_INVOKABLE void pauseVehicle();

    /// Command vehicle to kill all motors no matter what state
    Q_INVOKABLE void emergencyStop();

    /// Command vehicle to abort landing
    Q_INVOKABLE void abortLanding(double climbOutAltitude);

    Q_INVOKABLE void startMission();

    /// Alter the current mission item on the vehicle
    Q_INVOKABLE void setCurrentMissionSequence(int seq);

    /// Reboot vehicle
    Q_INVOKABLE void rebootVehicle();

    /// Clear Messages
    Q_INVOKABLE void clearMessages();

    Q_INVOKABLE void triggerCamera();
    Q_INVOKABLE void sendPlan(QString planFile);

    /// Used to check if running current version is equal or higher than the one being compared.
    //  returns 1 if current > compare, 0 if current == compare, -1 if current < compare
    Q_INVOKABLE int versionCompare(QString& compare);
    Q_INVOKABLE int versionCompare(int major, int minor, int patch);

    /// Test motor
    ///     @param motor Motor number, 1-based
    ///     @param percent 0-no power, 100-full power
    ///     @param timeoutSec Disabled motor after this amount of time
    Q_INVOKABLE void motorTest(int motor, int percent, int timeoutSecs);

    Q_INVOKABLE void setPIDTuningTelemetryMode(bool pidTuning);

    Q_INVOKABLE void gimbalControlValue (double pitch, double yaw);
    Q_INVOKABLE void gimbalPitchStep    (int direction);
    Q_INVOKABLE void gimbalYawStep      (int direction);
    Q_INVOKABLE void centerGimbal       ();

#if !defined(NO_ARDUPILOT_DIALECT)
    Q_INVOKABLE void flashBootloader();
#endif

    bool    guidedModeSupported     () const;
    bool    pauseVehicleSupported   () const;
    bool    orbitModeSupported      () const;
    bool    roiModeSupported        () const;
    bool    takeoffVehicleSupported () const;
    QString gotoFlightMode          () const;

    // Property accessors

    QGeoCoordinate coordinate() { return _coordinate; }
    QGeoCoordinate armedPosition    () { return _armedPosition; }

    typedef enum {
        JoystickModeRC,         ///< Joystick emulates an RC Transmitter
        JoystickModeAttitude,
        JoystickModePosition,
        JoystickModeForce,
        JoystickModeVelocity,
        JoystickModeMax
    } JoystickMode_t;

    void updateFlightDistance(double distance);

    int joystickMode();
    void setJoystickMode(int mode);

    /// List of joystick mode names
    QStringList joystickModes();

    bool joystickEnabled();
    void setJoystickEnabled(bool enabled);

    // Is vehicle active with respect to current active vehicle in QGC
    bool active();
    void setActive(bool active);

    // Property accesors
    int id() { return _id; }
    MAV_AUTOPILOT firmwareType() const { return _firmwareType; }
    MAV_TYPE vehicleType() const { return _vehicleType; }
    Q_INVOKABLE QString vehicleTypeName() const;

    /// Returns the highest quality link available to the Vehicle. If you need to hold a reference to this link use
    /// LinkManager::sharedLinkInterfaceForGet to get QSharedPointer for link.
    LinkInterface* priorityLink() { return _priorityLink.data(); }

    /// Sends a message to the specified link
    /// @return true: message sent, false: Link no longer connected
    bool sendMessageOnLink(LinkInterface* link, mavlink_message_t message);

    /// Sends the specified messages multiple times to the vehicle in order to attempt to
    /// guarantee that it makes it to the vehicle.
    void sendMessageMultiple(mavlink_message_t message);

    /// Provides access to uas from vehicle. Temporary workaround until UAS is fully phased out.
    UAS* uas() { return _uas; }

    /// Provides access to uas from vehicle. Temporary workaround until AutoPilotPlugin is fully phased out.
    AutoPilotPlugin* autopilotPlugin() { return _autopilotPlugin; }

    /// Provides access to the Firmware Plugin for this Vehicle
    FirmwarePlugin* firmwarePlugin() { return _firmwarePlugin; }

    MissionManager*     missionManager()    { return _missionManager; }
    GeoFenceManager*    geoFenceManager()   { return _geoFenceManager; }
    RallyPointManager*  rallyPointManager() { return _rallyPointManager; }

    QGeoCoordinate homePosition();

    bool armed      () { return _armed; }
    void setArmed   (bool armed);

    bool flightModeSetAvailable             ();
    QStringList flightModes                 ();
    QStringList extraJoystickFlightModes    ();
    QString flightMode                      () const;
    void setFlightMode                      (const QString& flightMode);

    QString priorityLinkName() const;
    QVariantList links() const;
    void setPriorityLinkByName(const QString& priorityLinkName);

    bool hilMode();
    void setHilMode(bool hilMode);

    bool fixedWing() const;
    bool multiRotor() const;
    bool vtol() const;
    bool rover() const;
    bool sub() const;

    bool supportsThrottleModeCenterZero () const;
    bool supportsNegativeThrust         ();
    bool supportsRadio                  () const;
    bool supportsJSButton               () const;
    bool supportsMotorInterference      () const;
    bool supportsTerrainFrame           () const;

    void setGuidedMode(bool guidedMode);

    QString prearmError() const { return _prearmError; }
    void setPrearmError(const QString& prearmError);

    QmlObjectListModel* cameraTriggerPoints () { return &_cameraTriggerPoints; }

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
    float           latitude                () { return static_cast<float>(_coordinate.latitude()); }
    float           longitude               () { return static_cast<float>(_coordinate.longitude()); }
    bool            mavPresent              () { return _mav != nullptr; }
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
    int             sensorsPresentBits      () const { return static_cast<int>(_onboardControlSensorsPresent); }
    int             sensorsEnabledBits      () const { return static_cast<int>(_onboardControlSensorsEnabled); }
    int             sensorsHealthBits       () const { return static_cast<int>(_onboardControlSensorsHealth); }
    int             sensorsUnhealthyBits    () const { return static_cast<int>(_onboardControlSensorsUnhealthy); }
    QString         missionFlightMode       () const;
    QString         pauseFlightMode         () const;
    QString         rtlFlightMode           () const;
    QString         smartRTLFlightMode      () const;
    bool            supportsSmartRTL        () const;
    QString         landFlightMode          () const;
    QString         takeControlFlightMode   () const;
    QString         followFlightMode        () const;
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
    bool            orbitActive             () const { return _orbitActive; }
    QGCMapCircle*   orbitMapCircle          () { return &_orbitMapCircle; }

    /// Get the maximum MAVLink protocol version supported
    /// @return the maximum version
    unsigned        maxProtoVersion         () const { return _maxProtoVersion; }

    Fact* roll                              () { return &_rollFact; }
    Fact* pitch                             () { return &_pitchFact; }
    Fact* heading                           () { return &_headingFact; }
    Fact* rollRate                          () { return &_rollRateFact; }
    Fact* pitchRate                         () { return &_pitchRateFact; }
    Fact* yawRate                           () { return &_yawRateFact; }
    Fact* airSpeed                          () { return &_airSpeedFact; }
    Fact* groundSpeed                       () { return &_groundSpeedFact; }
    Fact* climbRate                         () { return &_climbRateFact; }
    Fact* altitudeRelative                  () { return &_altitudeRelativeFact; }
    Fact* altitudeAMSL                      () { return &_altitudeAMSLFact; }
    Fact* flightDistance                    () { return &_flightDistanceFact; }
    Fact* distanceToHome                    () { return &_distanceToHomeFact; }
    Fact* headingToNextWP                   () { return &_headingToNextWPFact; }
    Fact* headingToHome                     () { return &_headingToHomeFact; }
    Fact* distanceToGCS                     () { return &_distanceToGCSFact; }
    Fact* hobbs                             () { return &_hobbsFact; }
    Fact* throttlePct                       () { return &_throttlePctFact; }

    FactGroup* gpsFactGroup                 () { return &_gpsFactGroup; }
    FactGroup* battery1FactGroup            () { return &_battery1FactGroup; }
    FactGroup* battery2FactGroup            () { return &_battery2FactGroup; }
    FactGroup* windFactGroup                () { return &_windFactGroup; }
    FactGroup* vibrationFactGroup           () { return &_vibrationFactGroup; }
    FactGroup* temperatureFactGroup         () { return &_temperatureFactGroup; }
    FactGroup* clockFactGroup               () { return &_clockFactGroup; }
    FactGroup* setpointFactGroup            () { return &_setpointFactGroup; }
    FactGroup* distanceSensorFactGroup      () { return &_distanceSensorFactGroup; }
    FactGroup* estimatorStatusFactGroup     () { return &_estimatorStatusFactGroup; }

    void setConnectionLostEnabled(bool connectionLostEnabled);

    ParameterManager*       parameterManager() { return _parameterManager; }
    ParameterManager*       parameterManager() const { return _parameterManager; }
    VehicleObjectAvoidance* objectAvoidance()  { return _objectAvoidance; }

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
    Q_INVOKABLE void sendCommand(int component, int command, bool showError, double param1 = 0.0, double param2 = 0.0, double param3 = 0.0, double param4 = 0.0, double param5 = 0.0, double param6 = 0.0, double param7 = 0.0)
    {
        sendMavCommand(
            component, static_cast<MAV_CMD>(command),
            showError,
            static_cast<float>(param1),
            static_cast<float>(param2),
            static_cast<float>(param3),
            static_cast<float>(param4),
            static_cast<float>(param5),
            static_cast<float>(param6),
            static_cast<float>(param7));
    }

    int firmwareMajorVersion() const { return _firmwareMajorVersion; }
    int firmwareMinorVersion() const { return _firmwareMinorVersion; }
    int firmwarePatchVersion() const { return _firmwarePatchVersion; }
    int firmwareVersionType() const { return _firmwareVersionType; }
    int firmwareCustomMajorVersion() const { return _firmwareCustomMajorVersion; }
    int firmwareCustomMinorVersion() const { return _firmwareCustomMinorVersion; }
    int firmwareCustomPatchVersion() const { return _firmwareCustomPatchVersion; }
    QString firmwareVersionTypeString() const;
    void setFirmwareVersion(int majorVersion, int minorVersion, int patchVersion, FIRMWARE_VERSION_TYPE versionType = FIRMWARE_VERSION_TYPE_OFFICIAL);
    void setFirmwareCustomVersion(int majorVersion, int minorVersion, int patchVersion);
    static const int versionNotSetValue = -1;

    QString gitHash() const { return _gitHash; }
    quint64 vehicleUID() const { return _uid; }
    QString vehicleUIDStr();

    bool soloFirmware() const { return _soloFirmware; }
    void setSoloFirmware(bool soloFirmware);

    int defaultComponentId() { return _defaultComponentId; }

    /// Sets the default component id for an offline editing vehicle
    void setOfflineEditingDefaultComponentId(int defaultComponentId);

    /// @return -1 = Unknown, Number of motors on vehicle
    int motorCount();

    /// @return true: Motors are coaxial like an X8 config, false: Quadcopter for example
    bool coaxialMotors();

    /// @return true: X confiuration, false: Plus configuration
    bool xConfigMotors();

    /// @return Firmware plugin instance data associated with this Vehicle
    QObject* firmwarePluginInstanceData() { return _firmwarePluginInstanceData; }

    /// Sets the firmware plugin instance data associated with this Vehicle. This object will be parented to the Vehicle
    /// and destroyed when the vehicle goes away.
    void setFirmwarePluginInstanceData(QObject* firmwarePluginInstanceData);

    QString vehicleImageOpaque  () const;
    QString vehicleImageOutline () const;
    QString vehicleImageCompass () const;

    const QVariantList&         toolBarIndicators   ();
    const QVariantList&         staticCameraList    () const;

    bool capabilitiesKnown      () const { return _vehicleCapabilitiesKnown; }
    uint64_t capabilityBits     () const { return _capabilityBits; }    // Change signalled by capabilityBitsChanged

    QGCCameraManager*           dynamicCameras      () { return _cameras; }
    QString                     hobbsMeter          ();

    /// @true: When flying a mission the vehicle is always facing towards the next waypoint
    bool vehicleYawsToNextWaypointInMission() const;

    /// The vehicle is responsible for making the initial request for the Plan.
    /// @return: true: initial request is complete, false: initial request is still in progress;
    bool initialPlanRequestComplete() const { return _initialPlanRequestComplete; }

    void forceInitialPlanRequestComplete();

    void _setFlying(bool flying);
    void _setLanding(bool landing);
    void _setHomePosition(QGeoCoordinate& homeCoord);
    void _setMaxProtoVersion (unsigned version);

    /// Vehicle is about to be deleted
    void prepareDelete();

    quint64     mavlinkSentCount        () { return _mavlinkSentCount; }        /// Calculated total number of messages sent to us
    quint64     mavlinkReceivedCount    () { return _mavlinkReceivedCount; }    /// Total number of sucessful messages received
    quint64     mavlinkLossCount        () { return _mavlinkLossCount; }        /// Total number of lost messages
    float       mavlinkLossPercent      () { return _mavlinkLossPercent; }      /// Running loss rate

    qreal       gimbalRoll              () { return static_cast<qreal>(_curGimbalRoll);}
    qreal       gimbalPitch             () { return static_cast<qreal>(_curGimbalPitch); }
    qreal       gimbalYaw               () { return static_cast<qreal>(_curGinmbalYaw); }
    bool        gimbalData              () { return _haveGimbalData; }
    bool        isROIEnabled            () { return _isROIEnabled; }

    CheckList   checkListState          () { return _checkListState; }
    void        setCheckListState       (CheckList cl)  { _checkListState = cl; emit checkListStateChanged(); }

public slots:
    void setVtolInFwdFlight             (bool vtolInFwdFlight);

signals:
    void allLinksInactive               (Vehicle* vehicle);
    void coordinateChanged              (QGeoCoordinate coordinate);
    void joystickModeChanged            (int mode);
    void joystickEnabledChanged         (bool enabled);
    void activeChanged                  (bool active);
    void mavlinkMessageReceived         (const mavlink_message_t& message);
    void homePositionChanged            (const QGeoCoordinate& homePosition);
    void armedPositionChanged();
    void armedChanged                   (bool armed);
    void flightModeChanged              (const QString& flightMode);
    void hilModeChanged                 (bool hilMode);
    /** @brief HIL actuator controls (replaces HIL controls) */
    void hilActuatorControlsChanged     (quint64 time, quint64 flags, float ctl_0, float ctl_1, float ctl_2, float ctl_3, float ctl_4, float ctl_5, float ctl_6, float ctl_7, float ctl_8, float ctl_9, float ctl_10, float ctl_11, float ctl_12, float ctl_13, float ctl_14, float ctl_15, quint8 mode);
    void connectionLostChanged          (bool connectionLost);
    void connectionLostEnabledChanged   (bool connectionLostEnabled);
    void autoDisconnectChanged          (bool autoDisconnectChanged);
    void flyingChanged                  (bool flying);
    void landingChanged                 (bool landing);
    void guidedModeChanged              (bool guidedMode);
    void vtolInFwdFlightChanged         (bool vtolInFwdFlight);
    void prearmErrorChanged             (const QString& prearmError);
    void soloFirmwareChanged            (bool soloFirmware);
    void unhealthySensorsChanged        ();
    void defaultCruiseSpeedChanged      (double cruiseSpeed);
    void defaultHoverSpeedChanged       (double hoverSpeed);
    void firmwareTypeChanged            ();
    void vehicleTypeChanged             ();
    void dynamicCamerasChanged          ();
    void hobbsMeterChanged              ();
    void capabilitiesKnownChanged       (bool capabilitiesKnown);
    void initialPlanRequestCompleteChanged(bool initialPlanRequestComplete);
    void capabilityBitsChanged          (uint64_t capabilityBits);
    void toolBarIndicatorsChanged       ();
    void highLatencyLinkChanged         (bool highLatencyLink);
    void priorityLinkNameChanged        (const QString& priorityLinkName);
    void linksChanged                   ();
    void linksPropertiesChanged         ();
    void textMessageReceived            (int uasid, int componentid, int severity, QString text);
    void checkListStateChanged          ();

    void messagesReceivedChanged        ();
    void messagesSentChanged            ();
    void messagesLostChanged            ();

    /// Used internally to move sendMessage call to main thread
    void _sendMessageOnLinkOnThread(LinkInterface* link, mavlink_message_t message);

    void messageTypeChanged             ();
    void newMessageCountChanged         ();
    void messageCountChanged            ();
    void formatedMessagesChanged        ();
    void formatedMessageChanged         ();
    void latestErrorChanged             ();
    void longitudeChanged               ();
    void currentConfigChanged           ();
    void flowImageIndexChanged          ();
    void rcRSSIChanged                  (int rcRSSI);
    void telemetryRRSSIChanged          (int value);
    void telemetryLRSSIChanged          (int value);
    void telemetryRXErrorsChanged       (unsigned int value);
    void telemetryFixedChanged          (unsigned int value);
    void telemetryTXBufferChanged       (unsigned int value);
    void telemetryLNoiseChanged         (int value);
    void telemetryRNoiseChanged         (int value);
    void autoDisarmChanged              ();
    void flightModesChanged             ();
    void sensorsPresentBitsChanged      (int sensorsPresentBits);
    void sensorsEnabledBitsChanged      (int sensorsEnabledBits);
    void sensorsHealthBitsChanged       (int sensorsHealthBits);
    void sensorsUnhealthyBitsChanged    (int sensorsUnhealthyBits);
    void orbitActiveChanged             (bool orbitActive);

    void firmwareVersionChanged         ();
    void firmwareCustomVersionChanged   ();
    void gitHashChanged                 (QString hash);
    void vehicleUIDChanged              ();

    /// New RC channel values
    ///     @param channelCount Number of available channels, cMaxRcChannels max
    ///     @param pwmValues -1 signals channel not available
    void rcChannelsChanged              (int channelCount, int pwmValues[cMaxRcChannels]);

    /// Remote control RSSI changed  (0% - 100%)
    void remoteControlRSSIChanged       (uint8_t rssi);

    void mavlinkRawImu                  (mavlink_message_t message);
    void mavlinkScaledImu1              (mavlink_message_t message);
    void mavlinkScaledImu2              (mavlink_message_t message);
    void mavlinkScaledImu3              (mavlink_message_t message);

    // Mavlink Log Download
    void mavlinkLogData                 (Vehicle* vehicle, uint8_t target_system, uint8_t target_component, uint16_t sequence, uint8_t first_message, QByteArray data, bool acked);

    /// Signalled in response to usage of sendMavCommand
    ///     @param vehicleId Vehicle which command was sent to
    ///     @param component Component which command was sent to
    ///     @param command MAV_CMD Command which was sent
    ///     @param result MAV_RESULT returned in ack
    ///     @param noResponseFromVehicle true: vehicle did not respond to command, false: vehicle responsed, MAV_RESULT in result
    void mavCommandResult               (int vehicleId, int component, int command, int result, bool noReponseFromVehicle);

    // MAVlink Serial Data
    void mavlinkSerialControl           (uint8_t device, uint8_t flags, uint16_t timeout, uint32_t baudrate, QByteArray data);

    // MAVLink protocol version
    void requestProtocolVersion         (unsigned version);
    void mavlinkStatusChanged           ();

    void gimbalRollChanged              ();
    void gimbalPitchChanged             ();
    void gimbalYawChanged               ();
    void gimbalDataChanged              ();
    void isROIEnabledChanged            ();

private slots:
    void _mavlinkMessageReceived        (LinkInterface* link, mavlink_message_t message);
    void _linkInactiveOrDeleted         (LinkInterface* link);
    void _sendMessageOnLink             (LinkInterface* link, mavlink_message_t message);
    void _sendMessageMultipleNext       ();
    void _parametersReady               (bool parametersReady);
    void _remoteControlRSSIChanged      (uint8_t rssi);
    void _handleFlightModeChanged       (const QString& flightMode);
    void _announceArmedChanged          (bool armed);
    void _offlineFirmwareTypeSettingChanged(QVariant value);
    void _offlineVehicleTypeSettingChanged(QVariant value);
    void _offlineCruiseSpeedSettingChanged(QVariant value);
    void _offlineHoverSpeedSettingChanged(QVariant value);
    void _updateHighLatencyLink         (bool sendCommand = true);

    void _handleTextMessage             (int newCount);
    void _handletextMessageReceived     (UASMessage* message);
    /** @brief A new camera image has arrived */
    void _imageReady                    (UASInterface* uas);
    void _prearmErrorTimeout            ();
    void _missionLoadComplete           ();
    void _geoFenceLoadComplete          ();
    void _rallyPointLoadComplete        ();
    void _sendMavCommandAgain           ();
    void _clearCameraTriggerPoints      ();
    void _updateDistanceHeadingToHome   ();
    void _updateHeadingToNextWP         ();
    void _updateDistanceToGCS           ();
    void _updateHobbsMeter              ();
    void _vehicleParamLoaded            (bool ready);
    void _sendQGCTimeToVehicle          ();
    void _mavlinkMessageStatus          (int uasId, uint64_t totalSent, uint64_t totalReceived, uint64_t totalLoss, float lossPercent);

    void _trafficUpdate                 (bool alert, QString traffic_id, QString vehicle_id, QGeoCoordinate location, float heading);
    void _orbitTelemetryTimeout         ();
    void _protocolVersionTimeOut        ();
    void _updateFlightTime              ();

private:
    bool _containsLink                  (LinkInterface* link);
    void _addLink                       (LinkInterface* link);
    void _joystickChanged               (Joystick* joystick);
    void _loadSettings                  ();
    void _saveSettings                  ();
    void _startJoystick                 (bool start);
    void _handlePing                    (LinkInterface* link, mavlink_message_t& message);
    void _handleHomePosition            (mavlink_message_t& message);
    void _handleHeartbeat               (mavlink_message_t& message);
    void _handleRadioStatus             (mavlink_message_t& message);
    void _handleRCChannels              (mavlink_message_t& message);
    void _handleRCChannelsRaw           (mavlink_message_t& message);
    void _handleBatteryStatus           (mavlink_message_t& message);
    void _handleSysStatus               (mavlink_message_t& message);
    void _handleWindCov                 (mavlink_message_t& message);
    void _handleVibration               (mavlink_message_t& message);
    void _handleExtendedSysState        (mavlink_message_t& message);
    void _handleCommandAck              (mavlink_message_t& message);
    void _handleCommandLong             (mavlink_message_t& message);
    void _handleAutopilotVersion        (LinkInterface* link, mavlink_message_t& message);
    void _handleProtocolVersion         (LinkInterface* link, mavlink_message_t& message);
    void _handleHilActuatorControls     (mavlink_message_t& message);
    void _handleGpsRawInt               (mavlink_message_t& message);
    void _handleGlobalPositionInt       (mavlink_message_t& message);
    void _handleAltitude                (mavlink_message_t& message);
    void _handleVfrHud                  (mavlink_message_t& message);
    void _handleScaledPressure          (mavlink_message_t& message);
    void _handleScaledPressure2         (mavlink_message_t& message);
    void _handleScaledPressure3         (mavlink_message_t& message);
    void _handleHighLatency2            (mavlink_message_t& message);
    void _handleAttitudeWorker          (double rollRadians, double pitchRadians, double yawRadians);
    void _handleAttitude                (mavlink_message_t& message);
    void _handleAttitudeQuaternion      (mavlink_message_t& message);
    void _handleAttitudeTarget          (mavlink_message_t& message);
    void _handleDistanceSensor          (mavlink_message_t& message);
    void _handleEstimatorStatus         (mavlink_message_t& message);
    void _handleStatusText              (mavlink_message_t& message, bool longVersion);
    void _handleOrbitExecutionStatus    (const mavlink_message_t& message);
    void _handleMessageInterval         (const mavlink_message_t& message);
    void _handleGimbalOrientation       (const mavlink_message_t& message);
    void _handleObstacleDistance        (const mavlink_message_t& message);
    // ArduPilot dialect messages
#if !defined(NO_ARDUPILOT_DIALECT)
    void _handleCameraFeedback          (const mavlink_message_t& message);
    void _handleWind                    (mavlink_message_t& message);
#endif
    void _handleCameraImageCaptured     (const mavlink_message_t& message);
    void _handleADSBVehicle             (const mavlink_message_t& message);
    void _missionManagerError           (int errorCode, const QString& errorMsg);
    void _geoFenceManagerError          (int errorCode, const QString& errorMsg);
    void _rallyPointManagerError        (int errorCode, const QString& errorMsg);
    void _linkActiveChanged             (LinkInterface* link, bool active, int vehicleID);
    void _say                           (const QString& text);
    QString _vehicleIdSpeech            ();
    void _handleMavlinkLoggingData      (mavlink_message_t& message);
    void _handleMavlinkLoggingDataAcked (mavlink_message_t& message);
    void _ackMavlinkLogData             (uint16_t sequence);
    void _sendNextQueuedMavCommand      ();
    void _updatePriorityLink            (bool updateActive, bool sendCommand);
    void _commonInit                    ();
    void _startPlanRequest              ();
    void _setupAutoDisarmSignalling     ();
    void _setCapabilities               (uint64_t capabilityBits);
    void _updateArmed                   (bool armed);
    bool _apmArmingNotRequired          ();
    void _pidTuningAdjustRates          (bool setRatesForTuning);
    void _handleUnsupportedRequestAutopilotCapabilities();
    void _handleUnsupportedRequestProtocolVersion();
    void _initializeCsv                 ();
    void _writeCsvLine                  ();
    void _flightTimerStart              ();
    void _flightTimerStop               ();
    void _batteryStatusWorker           (int batteryId, double voltage, double current, double batteryRemainingPct);

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

    QTimer              _csvLogTimer;
    QFile               _csvLogFile;

    QList<LinkInterface*> _links;

    JoystickMode_t  _joystickMode;
    bool            _joystickEnabled;

    UAS* _uas;

    QGeoCoordinate  _coordinate;
    QGeoCoordinate  _homePosition;
    QGeoCoordinate  _armedPosition;

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
    bool            _mavlinkProtocolRequestComplete;
    unsigned        _maxProtoVersion;
    bool            _vehicleCapabilitiesKnown;
    uint64_t        _capabilityBits;
    bool            _highLatencyLink;
    bool            _receivingAttitudeQuaternion;
    CheckList       _checkListState = CheckListNotSetup;

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

    ParameterManager*       _parameterManager   = nullptr;
    VehicleObjectAvoidance* _objectAvoidance    = nullptr;

#if defined(QGC_AIRMAP_ENABLED)
    AirspaceVehicleManager* _airspaceVehicleManager;
#endif

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

    QTime                           _flightTimer;
    QTimer                          _flightTimeUpdater;
    TrajectoryPoints*               _trajectoryPoints;
    QmlObjectListModel              _cameraTriggerPoints;
    //QMap<QString, ADSBVehicle*>     _trafficVehicleMap;

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

    float               _curGimbalRoll  = 0.0f;
    float               _curGimbalPitch = 0.0f;
    float               _curGinmbalYaw  = 0.0f;
    bool                _haveGimbalData = false;
    bool                _isROIEnabled   = false;
    Joystick*           _activeJoystick = nullptr;

    int _firmwareMajorVersion;
    int _firmwareMinorVersion;
    int _firmwarePatchVersion;
    int _firmwareCustomMajorVersion;
    int _firmwareCustomMinorVersion;
    int _firmwareCustomPatchVersion;
    FIRMWARE_VERSION_TYPE _firmwareVersionType;

    QString _gitHash;
    quint64 _uid;

    QTime   _lastBatteryAnnouncement;
    int     _lastAnnouncedLowBatteryPercent;

    SharedLinkInterfacePointer _priorityLink;  // We always keep a reference to the priority link to manage shutdown ordering
    bool _priorityLinkCommanded;

    uint64_t    _mavlinkSentCount       = 0;
    uint64_t    _mavlinkReceivedCount   = 0;
    uint64_t    _mavlinkLossCount       = 0;
    float       _mavlinkLossPercent     = 0.0f;

    QMap<QString, QTime> _noisySpokenPrearmMap; ///< Used to prevent PreArm messages from being spoken too often

    // Orbit status values
    bool            _orbitActive;
    QGCMapCircle    _orbitMapCircle;
    QTimer          _orbitTelemetryTimer;
    static const int _orbitTelemetryTimeoutMsecs = 3000; // No telemetry for this amount and orbit will go inactive

    // PID Tuning telemetry mode
    bool            _pidTuningTelemetryMode;
    bool            _pidTuningWaitingForRates;
    QList<int>      _pidTuningMessages;
    QMap<int, int>  _pidTuningMessageRatesUsecs;

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
    Fact _headingToNextWPFact;
    Fact _headingToHomeFact;
    Fact _distanceToGCSFact;
    Fact _hobbsFact;
    Fact _throttlePctFact;

    VehicleGPSFactGroup             _gpsFactGroup;
    VehicleBatteryFactGroup         _battery1FactGroup;
    VehicleBatteryFactGroup         _battery2FactGroup;
    VehicleWindFactGroup            _windFactGroup;
    VehicleVibrationFactGroup       _vibrationFactGroup;
    VehicleTemperatureFactGroup     _temperatureFactGroup;
    VehicleClockFactGroup           _clockFactGroup;
    VehicleSetpointFactGroup        _setpointFactGroup;
    VehicleDistanceSensorFactGroup  _distanceSensorFactGroup;
    VehicleEstimatorStatusFactGroup _estimatorStatusFactGroup;

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
    static const char* _headingToNextWPFactName;
    static const char* _headingToHomeFactName;
    static const char* _distanceToGCSFactName;
    static const char* _hobbsFactName;
    static const char* _throttlePctFactName;

    static const char* _gpsFactGroupName;
    static const char* _battery1FactGroupName;
    static const char* _battery2FactGroupName;
    static const char* _windFactGroupName;
    static const char* _vibrationFactGroupName;
    static const char* _temperatureFactGroupName;
    static const char* _clockFactGroupName;
    static const char* _distanceSensorFactGroupName;
    static const char* _estimatorStatusFactGroupName;

    static const int _vehicleUIUpdateRateMSecs = 100;

    // Settings keys
    static const char* _settingsGroup;
    static const char* _joystickModeSettingsKey;
    static const char* _joystickEnabledSettingsKey;

};
