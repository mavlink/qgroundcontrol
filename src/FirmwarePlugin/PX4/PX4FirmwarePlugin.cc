/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "PX4FirmwarePlugin.h"
#include "PX4ParameterMetaData.h"
#include "QGCApplication.h"
#include "PX4AutoPilotPlugin.h"
#include "PX4SimpleFlightModesController.h"
#include "AirframeComponentController.h"
#include "SensorsComponentController.h"
#include "PowerComponentController.h"
#include "SettingsManager.h"
#include "PlanViewSettings.h"
#include "ParameterManager.h"
#include "Vehicle.h"

#include <QDebug>

#include "px4_custom_mode.h"

PX4FirmwarePluginInstanceData::PX4FirmwarePluginInstanceData(QObject* parent)
    : FirmwarePluginInstanceData(parent)
    , versionNotified(false)
{

}

PX4FirmwarePlugin::PX4FirmwarePlugin()
    : _manualFlightMode     (tr("Manual"))
    , _acroFlightMode       (tr("Acro"))
    , _stabilizedFlightMode (tr("Stabilized"))
    , _rattitudeFlightMode  (tr("Rattitude"))
    , _altCtlFlightMode     (tr("Altitude"))
    , _posCtlFlightMode     (tr("Position"))
    , _offboardFlightMode   (tr("Offboard"))
    , _readyFlightMode      (tr("Ready"))
    , _takeoffFlightMode    (tr("Takeoff"))
    , _holdFlightMode       (tr("Hold"))
    , _missionFlightMode    (tr("Mission"))
    , _rtlFlightMode        (tr("Return"))
    , _landingFlightMode    (tr("Land"))
    , _preclandFlightMode   (tr("Precision Land"))
    , _rtgsFlightMode       (tr("Return to Groundstation"))
    , _followMeFlightMode   (tr("Follow Me"))
    , _simpleFlightMode     (tr("Simple"))
    , _orbitFlightMode      (tr("Orbit"))
{
    qmlRegisterType<PX4SimpleFlightModesController>     ("QGroundControl.Controllers", 1, 0, "PX4SimpleFlightModesController");
    qmlRegisterType<AirframeComponentController>        ("QGroundControl.Controllers", 1, 0, "AirframeComponentController");
    qmlRegisterType<SensorsComponentController>         ("QGroundControl.Controllers", 1, 0, "SensorsComponentController");
    qmlRegisterType<PowerComponentController>           ("QGroundControl.Controllers", 1, 0, "PowerComponentController");

    _setModeEnumToModeStringMapping({
        { PX4CustomMode::MANUAL             ,    _manualFlightMode      },
        { PX4CustomMode::STABILIZED         ,    _stabilizedFlightMode  },
        { PX4CustomMode::ACRO               ,    _acroFlightMode        },
        { PX4CustomMode::RATTITUDE          ,    _rattitudeFlightMode   },
        { PX4CustomMode::ALTCTL             ,    _altCtlFlightMode      },
        { PX4CustomMode::OFFBOARD           ,    _offboardFlightMode    },
        { PX4CustomMode::SIMPLE             ,    _simpleFlightMode      },
        { PX4CustomMode::POSCTL_POSCTL      ,    _posCtlFlightMode      },
        { PX4CustomMode::POSCTL_ORBIT       ,    _orbitFlightMode       },
        { PX4CustomMode::AUTO_LOITER        ,    _holdFlightMode        },
        { PX4CustomMode::AUTO_MISSION       ,    _missionFlightMode     },
        { PX4CustomMode::AUTO_RTL           ,    _rtlFlightMode         },
        { PX4CustomMode::AUTO_LAND          ,    _landingFlightMode     },
        { PX4CustomMode::AUTO_PRECLAND      ,    _preclandFlightMode    },
        { PX4CustomMode::AUTO_READY         ,    _readyFlightMode       },
        { PX4CustomMode::AUTO_RTGS          ,    _rtgsFlightMode        },
        { PX4CustomMode::AUTO_TAKEOFF       ,    _takeoffFlightMode     },
    });

    static FlightModeList availableFlightModes = {
        // Mode Name            , Custom Mode                      CanBeSet  adv
        { _manualFlightMode     , PX4CustomMode::MANUAL            , true ,  true },
        { _stabilizedFlightMode , PX4CustomMode::STABILIZED        , true ,  true },
        { _acroFlightMode       , PX4CustomMode::ACRO              , true ,  true },
        { _rattitudeFlightMode  , PX4CustomMode::RATTITUDE         , true ,  false},
        { _altCtlFlightMode     , PX4CustomMode::ALTCTL            , true ,  false},
        { _offboardFlightMode   , PX4CustomMode::OFFBOARD          , true ,  true },
        { _simpleFlightMode     , PX4CustomMode::SIMPLE            , false,  false},
        { _posCtlFlightMode     , PX4CustomMode::POSCTL_POSCTL     , true ,  false},
        { _orbitFlightMode      , PX4CustomMode::POSCTL_ORBIT      , false,  true },
        { _holdFlightMode       , PX4CustomMode::AUTO_LOITER       , true ,  true },
        { _missionFlightMode    , PX4CustomMode::AUTO_MISSION      , true ,  true },
        { _rtlFlightMode        , PX4CustomMode::AUTO_RTL          , true ,  true },
        { _landingFlightMode    , PX4CustomMode::AUTO_LAND         , false,  true },
        { _preclandFlightMode   , PX4CustomMode::AUTO_PRECLAND     , true ,  true },
        { _readyFlightMode      , PX4CustomMode::AUTO_READY        , false,  false},
        { _rtgsFlightMode       , PX4CustomMode::AUTO_RTGS         , false,  false},
        { _takeoffFlightMode    , PX4CustomMode::AUTO_TAKEOFF      , false,  false},
    };
    updateAvailableFlightModes(availableFlightModes);
}

PX4FirmwarePlugin::~PX4FirmwarePlugin()
{
}

AutoPilotPlugin* PX4FirmwarePlugin::autopilotPlugin(Vehicle* vehicle) const
{
    return new PX4AutoPilotPlugin(vehicle, vehicle);
}

QStringList PX4FirmwarePlugin::flightModes(Vehicle* vehicle) const
{
    QStringList flightModesList;

    for (auto &mode : _flightModeList) {
        if (mode.canBeSet){
            bool fw = (vehicle->fixedWing() && mode.fixedWing);
            bool mc = (vehicle->multiRotor() && mode.multiRotor);

            // show all modes for generic, vtol, etc
            bool other = !vehicle->fixedWing() && !vehicle->multiRotor();
            if (fw || mc || other) {
                flightModesList += mode.mode_name;
            }
        }
    }

    return flightModesList;
}

QString PX4FirmwarePlugin::flightMode(uint8_t base_mode, uint32_t custom_mode) const
{
    QString flightMode = "Unknown";

    if (base_mode & MAV_MODE_FLAG_CUSTOM_MODE_ENABLED) {
        return _modeEnumToString.value(custom_mode, tr("Unknown %1:%2").arg(base_mode).arg(custom_mode));
    }

    return flightMode;
}

bool PX4FirmwarePlugin::setFlightMode(const QString& flightMode, uint8_t* base_mode, uint32_t* custom_mode) const
{
    *base_mode = 0;
    *custom_mode = 0;

    bool found = false;

    for (auto &mode: _flightModeList){
        if(flightMode.compare(mode.mode_name, Qt::CaseInsensitive) == 0){
            *base_mode = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
            *custom_mode = mode.custom_mode;
            found = true;
            break;
        }
    }

    if (!found) {
        qWarning() << "Unknown flight Mode" << flightMode;
    }

    return found;
}

bool PX4FirmwarePlugin::isCapable(const Vehicle *vehicle, FirmwareCapabilities capabilities) const
{
    int available = SetFlightModeCapability | PauseVehicleCapability | GuidedModeCapability;
    //-- This is arbitrary until I find how to really tell if ROI is avaiable
    if (vehicle->multiRotor()) {
        available |= ROIModeCapability | ChangeHeadingCapability;
    }
    if (vehicle->multiRotor() || vehicle->vtol()) {
        available |= TakeoffVehicleCapability | OrbitModeCapability;
    }
    return (capabilities & available) == capabilities;
}

void PX4FirmwarePlugin::initializeVehicle(Vehicle* vehicle)
{
    vehicle->setFirmwarePluginInstanceData(new PX4FirmwarePluginInstanceData);
}

bool PX4FirmwarePlugin::sendHomePositionToVehicle(void) const
{
    // PX4 stack does not want home position sent in the first position.
    // Subsequent sequence numbers must be adjusted.
    return false;
}

FactMetaData* PX4FirmwarePlugin::_getMetaDataForFact(QObject* parameterMetaData, const QString& name, FactMetaData::ValueType_t type, MAV_TYPE vehicleType) const
{
    PX4ParameterMetaData* px4MetaData = qobject_cast<PX4ParameterMetaData*>(parameterMetaData);

    if (px4MetaData) {
        return px4MetaData->getMetaDataForFact(name, vehicleType, type);
    } else {
        qWarning() << "Internal error: pointer passed to PX4FirmwarePlugin::getMetaDataForFact not PX4ParameterMetaData";
    }

    return nullptr;
}

void PX4FirmwarePlugin::_getParameterMetaDataVersionInfo(const QString& metaDataFile, int& majorVersion, int& minorVersion) const
{
    return PX4ParameterMetaData::getParameterMetaDataVersionInfo(metaDataFile, majorVersion, minorVersion);
}

QList<MAV_CMD> PX4FirmwarePlugin::supportedMissionCommands(QGCMAVLink::VehicleClass_t vehicleClass) const
{
    QList<MAV_CMD> supportedCommands = {
        MAV_CMD_NAV_WAYPOINT,
        MAV_CMD_NAV_LOITER_UNLIM, MAV_CMD_NAV_LOITER_TIME,
        MAV_CMD_NAV_RETURN_TO_LAUNCH,
        MAV_CMD_DO_JUMP,
        MAV_CMD_DO_DIGICAM_CONTROL,
        MAV_CMD_DO_SET_CAM_TRIGG_DIST,
        MAV_CMD_DO_SET_SERVO,
        MAV_CMD_DO_SET_ACTUATOR,
        MAV_CMD_DO_CHANGE_SPEED,
        MAV_CMD_DO_SET_HOME,
        MAV_CMD_DO_LAND_START,
        MAV_CMD_DO_SET_ROI_LOCATION, MAV_CMD_DO_SET_ROI_WPNEXT_OFFSET, MAV_CMD_DO_SET_ROI_NONE,
        MAV_CMD_DO_MOUNT_CONFIGURE,
        MAV_CMD_DO_MOUNT_CONTROL,
        MAV_CMD_SET_CAMERA_MODE,
        MAV_CMD_IMAGE_START_CAPTURE, MAV_CMD_IMAGE_STOP_CAPTURE, MAV_CMD_VIDEO_START_CAPTURE, MAV_CMD_VIDEO_STOP_CAPTURE,
        MAV_CMD_NAV_DELAY,
        MAV_CMD_CONDITION_YAW,
        MAV_CMD_NAV_LOITER_TO_ALT,
        MAV_CMD_DO_GRIPPER
    };

    QList<MAV_CMD> vtolCommands = {
        MAV_CMD_DO_VTOL_TRANSITION, MAV_CMD_NAV_VTOL_TAKEOFF, MAV_CMD_NAV_VTOL_LAND,
    };

    QList<MAV_CMD> flightCommands = {
        MAV_CMD_NAV_LAND, MAV_CMD_NAV_TAKEOFF,
    };

    if (vehicleClass == QGCMAVLink::VehicleClassGeneric) {
        supportedCommands   += vtolCommands;
        supportedCommands   += flightCommands;
    }
    if (vehicleClass == QGCMAVLink::VehicleClassVTOL) {
        supportedCommands += vtolCommands;
        supportedCommands += flightCommands;
    } else if (vehicleClass == QGCMAVLink::VehicleClassFixedWing || vehicleClass == QGCMAVLink::VehicleClassMultiRotor) {
        supportedCommands += flightCommands;
    }

    if (SettingsManager::instance()->planViewSettings()->useConditionGate()->rawValue().toBool()) {
        supportedCommands.append(MAV_CMD_CONDITION_GATE);
    }

    return supportedCommands;
}

QString PX4FirmwarePlugin::missionCommandOverrides(QGCMAVLink::VehicleClass_t vehicleClass) const
{
    switch (vehicleClass) {
    case QGCMAVLink::VehicleClassGeneric:
        return QStringLiteral(":/json/PX4-MavCmdInfoCommon.json");
    case QGCMAVLink::VehicleClassFixedWing:
        return QStringLiteral(":/json/PX4-MavCmdInfoFixedWing.json");
    case QGCMAVLink::VehicleClassMultiRotor:
        return QStringLiteral(":/json/PX4-MavCmdInfoMultiRotor.json");
    case QGCMAVLink::VehicleClassVTOL:
        return QStringLiteral(":/json/PX4-MavCmdInfoVTOL.json");
    case QGCMAVLink::VehicleClassSub:
        return QStringLiteral(":/json/PX4-MavCmdInfoSub.json");
    case QGCMAVLink::VehicleClassRoverBoat:
        return QStringLiteral(":/json/PX4-MavCmdInfoRover.json");
    default:
        qWarning() << "PX4FirmwarePlugin::missionCommandOverrides called with bad VehicleClass_t:" << vehicleClass;
        return QString();
    }
}

QObject* PX4FirmwarePlugin::_loadParameterMetaData(const QString& metaDataFile)
{
    PX4ParameterMetaData* metaData = new PX4ParameterMetaData;
    if (!metaDataFile.isEmpty()) {
        metaData->loadParameterFactMetaDataFile(metaDataFile);
    }
    return metaData;
}

void PX4FirmwarePlugin::pauseVehicle(Vehicle* vehicle) const
{
    vehicle->sendMavCommand(vehicle->defaultComponentId(),
                            MAV_CMD_DO_REPOSITION,
                            true,   // show error if failed
                            -1.0f,
                            MAV_DO_REPOSITION_FLAGS_CHANGE_MODE,
                            0.0f,
                            NAN,
                            NAN,
                            NAN,
                            NAN);
}

void PX4FirmwarePlugin::guidedModeRTL(Vehicle* vehicle, bool smartRTL) const
{
    Q_UNUSED(smartRTL);
    _setFlightModeAndValidate(vehicle, _rtlFlightMode);
}

void PX4FirmwarePlugin::guidedModeLand(Vehicle* vehicle) const
{
    _setFlightModeAndValidate(vehicle, _landingFlightMode);
}

void PX4FirmwarePlugin::_mavCommandResult(int vehicleId, int component, int command, int result, bool noReponseFromVehicle)
{
    Q_UNUSED(vehicleId);
    Q_UNUSED(component);
    Q_UNUSED(noReponseFromVehicle);

    auto* vehicle = qobject_cast<Vehicle*>(sender());
    if (!vehicle) {
        qWarning() << "Dynamic cast failed!";
        return;
    }

    if (command == MAV_CMD_NAV_TAKEOFF && result == MAV_RESULT_ACCEPTED) {
        // Now that we are in takeoff mode we can arm the vehicle which will cause it to takeoff.
        // We specifically don't retry arming if it fails. This way we don't fight with the user if
        // They are trying to disarm.
        disconnect(vehicle, &Vehicle::mavCommandResult, this, &PX4FirmwarePlugin::_mavCommandResult);
        if (!vehicle->armed()) {
            vehicle->setArmedShowError(true);
        }
    }
}

void PX4FirmwarePlugin::guidedModeTakeoff(Vehicle* vehicle, double takeoffAltRel) const
{
    double vehicleAltitudeAMSL = vehicle->altitudeAMSL()->rawValue().toDouble();
    if (qIsNaN(vehicleAltitudeAMSL)) {
        qgcApp()->showAppMessage(tr("Unable to takeoff, vehicle position not known."));
        return;
    }

    double takeoffAltAMSL = takeoffAltRel + vehicleAltitudeAMSL;

    connect(vehicle, &Vehicle::mavCommandResult, this, &PX4FirmwarePlugin::_mavCommandResult);
    vehicle->sendMavCommand(
        vehicle->defaultComponentId(),
        MAV_CMD_NAV_TAKEOFF,
        true,                                   // show error is fails
        -1,                                     // No pitch requested
        0, 0,                                   // param 2-4 unused
        NAN, NAN, NAN,                          // No yaw, lat, lon
        static_cast<float>(takeoffAltAMSL));    // AMSL altitude
}

double PX4FirmwarePlugin::maximumHorizontalSpeedMultirotor(Vehicle* vehicle) const
{
    QString speedParam("MPC_XY_VEL_MAX");

    if (vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, speedParam)) {
        return vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, speedParam)->rawValue().toDouble();
    }

    return FirmwarePlugin::maximumHorizontalSpeedMultirotor(vehicle);
}

double PX4FirmwarePlugin::maximumEquivalentAirspeed(Vehicle* vehicle) const
{
    QString airspeedMax("FW_AIRSPD_MAX");

    if (vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, airspeedMax)) {
        return vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, airspeedMax)->rawValue().toDouble();
    }

    return FirmwarePlugin::maximumEquivalentAirspeed(vehicle);
}

double PX4FirmwarePlugin::minimumEquivalentAirspeed(Vehicle* vehicle) const
{
    QString airspeedMin("FW_AIRSPD_MIN");

    if (vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, airspeedMin)) {
        return vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, airspeedMin)->rawValue().toDouble();
    }

    return FirmwarePlugin::minimumEquivalentAirspeed(vehicle);
}

bool PX4FirmwarePlugin::mulirotorSpeedLimitsAvailable(Vehicle* vehicle) const
{
    return vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, "MPC_XY_VEL_MAX");
}

bool PX4FirmwarePlugin::fixedWingAirSpeedLimitsAvailable(Vehicle* vehicle) const
{
    return vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, "FW_AIRSPD_MIN") &&
            vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, "FW_AIRSPD_MAX");
}

void PX4FirmwarePlugin::guidedModeGotoLocation(Vehicle* vehicle, const QGeoCoordinate& gotoCoord) const
{
    if (qIsNaN(vehicle->altitudeAMSL()->rawValue().toDouble())) {
        qgcApp()->showAppMessage(tr("Unable to go to location, vehicle position not known."));
        return;
    }

    if (vehicle->capabilityBits() & MAV_PROTOCOL_CAPABILITY_COMMAND_INT) {
        vehicle->sendMavCommandInt(vehicle->defaultComponentId(),
                                   MAV_CMD_DO_REPOSITION,
                                   MAV_FRAME_GLOBAL,
                                   true,   // show error is fails
                                   -1.0f,
                                   MAV_DO_REPOSITION_FLAGS_CHANGE_MODE,
                                   0.0f,
                                   NAN,
                                   gotoCoord.latitude(),
                                   gotoCoord.longitude(),
                                   vehicle->altitudeAMSL()->rawValue().toFloat());
    } else {
        vehicle->sendMavCommand(vehicle->defaultComponentId(),
                                MAV_CMD_DO_REPOSITION,
                                true,   // show error is fails
                                -1.0f,
                                MAV_DO_REPOSITION_FLAGS_CHANGE_MODE,
                                0.0f,
                                NAN,
                                static_cast<float>(gotoCoord.latitude()),
                                static_cast<float>(gotoCoord.longitude()),
                                vehicle->altitudeAMSL()->rawValue().toFloat());
    }
}

typedef struct {
    PX4FirmwarePlugin*  plugin;
    Vehicle*            vehicle;
    double              newAMSLAlt;
} PauseVehicleThenChangeAltData_t;

static void _pauseVehicleThenChangeAltResultHandler(void* resultHandlerData, int /*compId*/, const mavlink_command_ack_t& ack, Vehicle::MavCmdResultFailureCode_t failureCode)
{
    if (ack.result != MAV_RESULT_ACCEPTED) {
        switch (failureCode) {
        case Vehicle::MavCmdResultCommandResultOnly:
            qDebug() << QStringLiteral("MAV_CMD_DO_REPOSITION error(%1)").arg(ack.result);
            break;
        case Vehicle::MavCmdResultFailureNoResponseToCommand:
            qDebug() << "MAV_CMD_DO_REPOSITION no response from vehicle";
            break;
        case Vehicle::MavCmdResultFailureDuplicateCommand:
            qDebug() << "Internal Error: MAV_CMD_DO_REPOSITION could not be sent due to duplicate command";
            break;
        }
    }

    PauseVehicleThenChangeAltData_t* pData = static_cast<PauseVehicleThenChangeAltData_t*>(resultHandlerData);
    pData->plugin->_changeAltAfterPause(resultHandlerData, ack.result == MAV_RESULT_ACCEPTED /* pauseSucceeded */);
}

void PX4FirmwarePlugin::_changeAltAfterPause(void* resultHandlerData, bool pauseSucceeded)
{
    PauseVehicleThenChangeAltData_t* pData = static_cast<PauseVehicleThenChangeAltData_t*>(resultHandlerData);

    if (pauseSucceeded) {
        pData->vehicle->sendMavCommand(
                    pData->vehicle->defaultComponentId(),
                    MAV_CMD_DO_REPOSITION,
                    true,                                   // show error is fails
                    -1.0f,                                  // Don't change groundspeed
                    MAV_DO_REPOSITION_FLAGS_CHANGE_MODE,
                    0.0f,                                   // Reserved
                    qQNaN(), qQNaN(), qQNaN(),              // No change to yaw, lat, lon
                    static_cast<float>(pData->newAMSLAlt));
    } else {
        qgcApp()->showAppMessage(tr("Unable to pause vehicle."));
    }

    delete pData;
}

void PX4FirmwarePlugin::guidedModeChangeAltitude(Vehicle* vehicle, double altitudeChange, bool pauseVehicle)
{
    if (!vehicle->homePosition().isValid()) {
        qgcApp()->showAppMessage(tr("Unable to change altitude, home position unknown."));
        return;
    }
    if (qIsNaN(vehicle->homePosition().altitude())) {
        qgcApp()->showAppMessage(tr("Unable to change altitude, home position altitude unknown."));
        return;
    }

    double currentAltRel = vehicle->altitudeRelative()->rawValue().toDouble();
    double newAltRel = currentAltRel + altitudeChange;

    PauseVehicleThenChangeAltData_t* resultData = new PauseVehicleThenChangeAltData_t;
    resultData->plugin      = this;
    resultData->vehicle     = vehicle;
    resultData->newAMSLAlt  = vehicle->homePosition().altitude() + newAltRel;

    if (pauseVehicle) {
        Vehicle::MavCmdAckHandlerInfo_t handlerInfo = {};
        handlerInfo.resultHandler       = _pauseVehicleThenChangeAltResultHandler;
        handlerInfo.resultHandlerData   = resultData;

        vehicle->sendMavCommandWithHandler(
                    &handlerInfo,
                    vehicle->defaultComponentId(),
                    MAV_CMD_DO_REPOSITION,
                    -1.0f,                                  // Don't change groundspeed
                    MAV_DO_REPOSITION_FLAGS_CHANGE_MODE,
                    0.0f,                                   // Reserved
                    qQNaN(), qQNaN(), qQNaN(), qQNaN());    // No change to yaw, lat, lon, alt
    } else {
        _changeAltAfterPause(resultData, true /* pauseSucceeded */);
    }
}

void PX4FirmwarePlugin::guidedModeChangeGroundSpeedMetersSecond(Vehicle* vehicle, double groundspeed) const
{

    vehicle->sendMavCommand(
        vehicle->defaultComponentId(),
        MAV_CMD_DO_CHANGE_SPEED,
        true,                                   // show error is fails
        1,                                     // 0: airspeed, 1: groundspeed
        static_cast<float>(groundspeed),       // groundspeed setpoint
        -1,                                   // throttle
        0,                                    // 0: absolute speed, 1: relative to current
        NAN, NAN,NAN);                        // param 5-7 unused
}

void PX4FirmwarePlugin::guidedModeChangeEquivalentAirspeedMetersSecond(Vehicle* vehicle, double airspeed_equiv) const
{

    vehicle->sendMavCommand(
        vehicle->defaultComponentId(),
        MAV_CMD_DO_CHANGE_SPEED,
        true,                                   // show error is fails
        0,                                     // 0: airspeed, 1: groundspeed
        static_cast<float>(airspeed_equiv),       // groundspeed setpoint
        -1,                                   // throttle
        0,                                    // 0: absolute speed, 1: relative to current
        NAN, NAN,NAN);                        // param 5-7 unused
}

void PX4FirmwarePlugin::guidedModeChangeHeading(Vehicle* vehicle, const QGeoCoordinate &headingCoord) const
{
    if (!isCapable(vehicle, FirmwarePlugin::ChangeHeadingCapability)) {
        qgcApp()->showAppMessage(tr("Vehicle does not support guided rotate"));
        return;
    }

    const float degrees = vehicle->coordinate().azimuthTo(headingCoord);

    vehicle->sendMavCommand(
        vehicle->defaultComponentId(),
        MAV_CMD_DO_REPOSITION,
        true,
        -1.0f,                                  // no change in ground speed
        MAV_DO_REPOSITION_FLAGS_CHANGE_MODE,    // switch to guided mode
        0.0f,                                   // reserved
        degrees,                                // change heading
        NAN, NAN, NAN                           // no change lat, lon, alt
    );
}

void PX4FirmwarePlugin::startMission(Vehicle* vehicle) const
{
    if (_setFlightModeAndValidate(vehicle, missionFlightMode())) {
        if (!_armVehicleAndValidate(vehicle)) {
            qgcApp()->showAppMessage(tr("Unable to start mission: Vehicle rejected arming."));
            return;
        }
    } else {
        qgcApp()->showAppMessage(tr("Unable to start mission: Vehicle not changing to %1 flight mode.").arg(missionFlightMode()));
    }
}

void PX4FirmwarePlugin::setGuidedMode(Vehicle* vehicle, bool guidedMode) const
{
    if (guidedMode) {
        _setFlightModeAndValidate(vehicle, gotoFlightMode());
    } else {
        pauseVehicle(vehicle);
    }
}

QString PX4FirmwarePlugin::pauseFlightMode() const
{
    return _modeEnumToString.value(PX4CustomMode::AUTO_LOITER, _holdFlightMode);
}

QString PX4FirmwarePlugin::missionFlightMode() const
{
    return _modeEnumToString.value(PX4CustomMode::AUTO_MISSION, _missionFlightMode);
}

QString PX4FirmwarePlugin::rtlFlightMode() const
{
    return _modeEnumToString.value(PX4CustomMode::AUTO_RTL, _rtlFlightMode);
}

QString PX4FirmwarePlugin::landFlightMode() const
{
    return _modeEnumToString.value(PX4CustomMode::AUTO_LAND, _landingFlightMode);
}

QString PX4FirmwarePlugin::takeControlFlightMode() const
{
    return _modeEnumToString.value(PX4CustomMode::MANUAL, _manualFlightMode);
}

QString PX4FirmwarePlugin::gotoFlightMode() const
{
    return _modeEnumToString.value(PX4CustomMode::AUTO_LOITER, _holdFlightMode);
}

QString PX4FirmwarePlugin::followFlightMode() const
{
    return _modeEnumToString.value(PX4CustomMode::AUTO_FOLLOW_TARGET, _followMeFlightMode);
}

QString PX4FirmwarePlugin::takeOffFlightMode() const
{
    return _modeEnumToString.value(PX4CustomMode::AUTO_TAKEOFF, _takeoffFlightMode);
}

QString PX4FirmwarePlugin::stabilizedFlightMode() const
{
    return _modeEnumToString.value(PX4CustomMode::STABILIZED, _stabilizedFlightMode);
}

bool PX4FirmwarePlugin::isGuidedMode(const Vehicle* vehicle) const
{
    // Not supported by generic vehicle
    return (vehicle->flightMode() == _holdFlightMode || vehicle->flightMode() == _takeoffFlightMode
            || vehicle->flightMode() == _landingFlightMode);
}

bool PX4FirmwarePlugin::adjustIncomingMavlinkMessage(Vehicle* vehicle, mavlink_message_t* message)
{
    //-- Don't process messages to/from UDP Bridge. It doesn't suffer from these issues
    if (message->compid == MAV_COMP_ID_UDP_BRIDGE) {
        return true;
    }

    switch (message->msgid) {
    case MAVLINK_MSG_ID_AUTOPILOT_VERSION:
        _handleAutopilotVersion(vehicle, message);
        break;
    }

    return true;
}

void PX4FirmwarePlugin::_handleAutopilotVersion(Vehicle* vehicle, mavlink_message_t* message)
{
    Q_UNUSED(vehicle);

    auto* instanceData = qobject_cast<PX4FirmwarePluginInstanceData*>(vehicle->firmwarePluginInstanceData());
    if (!instanceData->versionNotified) {
        bool notifyUser = false;
        int supportedMajorVersion = 1;
        int supportedMinorVersion = 4;
        int supportedPatchVersion = 1;

        mavlink_autopilot_version_t version;
        mavlink_msg_autopilot_version_decode(message, &version);

        if (version.flight_sw_version != 0) {
            int majorVersion, minorVersion, patchVersion;

            majorVersion = (version.flight_sw_version >> (8*3)) & 0xFF;
            minorVersion = (version.flight_sw_version >> (8*2)) & 0xFF;
            patchVersion = (version.flight_sw_version >> (8*1)) & 0xFF;

            if (majorVersion < supportedMajorVersion) {
                notifyUser = true;
            } else if (majorVersion == supportedMajorVersion) {
                if (minorVersion < supportedMinorVersion) {
                    notifyUser = true;
                } else if (minorVersion == supportedMinorVersion) {
                    notifyUser = patchVersion < supportedPatchVersion;
                }
            }
        } else {
            notifyUser = true;
        }

        if (notifyUser) {
            instanceData->versionNotified = true;
            qgcApp()->showAppMessage(tr("QGroundControl supports PX4 Pro firmware Version %1.%2.%3 and above. You are using a version prior to that which will lead to unpredictable results. Please upgrade your firmware.").arg(supportedMajorVersion).arg(supportedMinorVersion).arg(supportedPatchVersion));
        }
    }
}

uint32_t PX4FirmwarePlugin::highLatencyCustomModeTo32Bits(uint16_t hlCustomMode) const
{
    union px4_custom_mode px4_cm;
    px4_cm.data = 0;
    px4_cm.custom_mode_hl = hlCustomMode;

    return px4_cm.data;
}

QString PX4FirmwarePlugin::_getLatestVersionFileUrl(Vehicle* vehicle) const
{
    Q_UNUSED(vehicle);
    return QStringLiteral("https://api.github.com/repos/PX4/Firmware/releases");
}

QString PX4FirmwarePlugin::_versionRegex() const
{
    return QStringLiteral("v([0-9,\\.]*) Stable");
}

bool PX4FirmwarePlugin::supportsNegativeThrust(Vehicle* vehicle) const
{
    return ((vehicle->vehicleType() == MAV_TYPE_GROUND_ROVER) || (vehicle->vehicleType() == MAV_TYPE_SUBMARINE));
}

QString PX4FirmwarePlugin::getHobbsMeter(Vehicle* vehicle) const
{
    static const char* HOOBS_HI = "LND_FLIGHT_T_HI";
    static const char* HOOBS_LO = "LND_FLIGHT_T_LO";
    uint64_t hobbsTimeSeconds = 0;

    if (vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, HOOBS_HI) &&
            vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, HOOBS_LO)) {
        Fact* factHi = vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, HOOBS_HI);
        Fact* factLo = vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, HOOBS_LO);
        hobbsTimeSeconds = ((uint64_t)factHi->rawValue().toUInt() << 32 | (uint64_t)factLo->rawValue().toUInt()) / 1000000;
        qCDebug(VehicleLog) << "Hobbs Meter raw PX4:" << "(" << factHi->rawValue().toUInt() << factLo->rawValue().toUInt() << ")";
    }

    int hours   = hobbsTimeSeconds / 3600;
    int minutes = (hobbsTimeSeconds % 3600) / 60;
    int seconds = hobbsTimeSeconds % 60;
    QString timeStr = QString::asprintf("%04d:%02d:%02d", hours, minutes, seconds);
    qCDebug(VehicleLog) << "Hobbs Meter string:" << timeStr;
    return timeStr;
}

bool PX4FirmwarePlugin::hasGripper(const Vehicle* vehicle) const
{
    if(vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, QStringLiteral("PD_GRIPPER_EN"))) {
        bool _hasGripper = (vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, QStringLiteral("PD_GRIPPER_EN"))->rawValue().toInt()) != 0 ? true : false;
        return _hasGripper;
    }
    return false;
}

QVariant PX4FirmwarePlugin::mainStatusIndicatorContentItem(const Vehicle*) const
{
    return QVariant::fromValue(QUrl::fromUserInput("qrc:/PX4/Indicators/PX4MainStatusIndicatorContentItem.qml"));
}

const QVariantList& PX4FirmwarePlugin::toolIndicators(const Vehicle* vehicle)
{
    if (_toolIndicatorList.size() == 0) {
        // First call the base class to get the standard QGC list
        _toolIndicatorList = FirmwarePlugin::toolIndicators(vehicle);

        // Find the generic flight mode indicator and replace with the custom one
        for (int i=0; i<_toolIndicatorList.size(); i++) {
            if (_toolIndicatorList.at(i).toUrl().toString().contains("FlightModeIndicator.qml")) {
                _toolIndicatorList[i] = QVariant::fromValue(QUrl::fromUserInput("qrc:/PX4/Indicators/PX4FlightModeIndicator.qml"));
                break;
            }
        }

        // Find the generic battery indicator and replace with the custom one
        for (int i=0; i<_toolIndicatorList.size(); i++) {
            if (_toolIndicatorList.at(i).toUrl().toString().contains("BatteryIndicator.qml")) {
                _toolIndicatorList[i] = QVariant::fromValue(QUrl::fromUserInput("qrc:/PX4/Indicators/PX4BatteryIndicator.qml"));
                break;
            }
        }
    }

    return _toolIndicatorList;
}

void PX4FirmwarePlugin::updateAvailableFlightModes(FlightModeList &modeList)
{
    for(auto &mode: modeList){
        PX4CustomMode::Mode cMode = static_cast<PX4CustomMode::Mode>(mode.custom_mode);

        // Update Multi Rotor
        switch (cMode) {
        case PX4CustomMode::MANUAL            :
        case PX4CustomMode::STABILIZED        :
        case PX4CustomMode::ACRO              :
        case PX4CustomMode::RATTITUDE         :
        case PX4CustomMode::ALTCTL            :
        case PX4CustomMode::OFFBOARD          :
        case PX4CustomMode::SIMPLE            :
        case PX4CustomMode::POSCTL_POSCTL     :
        case PX4CustomMode::AUTO_LOITER       :
        case PX4CustomMode::AUTO_MISSION      :
        case PX4CustomMode::AUTO_RTL          :
        case PX4CustomMode::AUTO_FOLLOW_TARGET:
        case PX4CustomMode::AUTO_LAND         :
        case PX4CustomMode::AUTO_PRECLAND     :
        case PX4CustomMode::AUTO_READY        :
        case PX4CustomMode::AUTO_RTGS         :
        case PX4CustomMode::AUTO_TAKEOFF      :
            mode.multiRotor = true;
            break;
        case PX4CustomMode::POSCTL_ORBIT      :
            mode.multiRotor = false;
            break;
        }

        // Update Fixed Wing
        switch (cMode){
        case PX4CustomMode::OFFBOARD          :
        case PX4CustomMode::SIMPLE            :
        case PX4CustomMode::POSCTL_ORBIT      :
        case PX4CustomMode::AUTO_FOLLOW_TARGET:
        case PX4CustomMode::AUTO_PRECLAND     :
            mode.fixedWing = false;
            break;
        case PX4CustomMode::MANUAL            :
        case PX4CustomMode::STABILIZED        :
        case PX4CustomMode::ACRO              :
        case PX4CustomMode::RATTITUDE         :
        case PX4CustomMode::ALTCTL            :
        case PX4CustomMode::POSCTL_POSCTL     :
        case PX4CustomMode::AUTO_LOITER       :
        case PX4CustomMode::AUTO_MISSION      :
        case PX4CustomMode::AUTO_RTL          :
        case PX4CustomMode::AUTO_LAND         :
        case PX4CustomMode::AUTO_READY        :
        case PX4CustomMode::AUTO_RTGS         :
        case PX4CustomMode::AUTO_TAKEOFF      :
            mode.fixedWing = true;
            break;
        }
    }
    _updateFlightModeList(modeList);
}
