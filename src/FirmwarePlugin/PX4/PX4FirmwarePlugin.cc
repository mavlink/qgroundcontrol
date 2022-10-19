/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
#include "RadioComponentController.h"
#include "QGCCameraManager.h"
#include "QGCFileDownload.h"
#include "SettingsManager.h"
#include "PlanViewSettings.h"

#include <QDebug>

#include "px4_custom_mode.h"

PX4FirmwarePluginInstanceData::PX4FirmwarePluginInstanceData(QObject* parent)
    : QObject(parent)
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

    struct Modes2Name {
        uint8_t     main_mode;
        uint8_t     sub_mode;
        bool        canBeSet;   ///< true: Vehicle can be set to this flight mode
        bool        fixedWing;  /// fixed wing compatible
        bool        multiRotor;  /// multi rotor compatible
    };

    static const struct Modes2Name rgModes2Name[] = {
        //main_mode                         sub_mode                                canBeSet  FW      MC
        { PX4_CUSTOM_MAIN_MODE_MANUAL,      0,                                      true,   true,   true },
        { PX4_CUSTOM_MAIN_MODE_STABILIZED,  0,                                      true,   true,   true },
        { PX4_CUSTOM_MAIN_MODE_ACRO,        0,                                      true,   true,   true },
        { PX4_CUSTOM_MAIN_MODE_RATTITUDE,   0,                                      true,   true,   true },
        { PX4_CUSTOM_MAIN_MODE_ALTCTL,      0,                                      true,   true,   true },
        { PX4_CUSTOM_MAIN_MODE_OFFBOARD,    0,                                      true,   false,  true },
        { PX4_CUSTOM_MAIN_MODE_SIMPLE,      0,                                      false,  false,  true },
        { PX4_CUSTOM_MAIN_MODE_POSCTL,      PX4_CUSTOM_SUB_MODE_POSCTL_POSCTL,      true,   true,   true },
        { PX4_CUSTOM_MAIN_MODE_POSCTL,      PX4_CUSTOM_SUB_MODE_POSCTL_ORBIT,       false,  false,   false },
        { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_LOITER,        true,   true,   true },
        { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_MISSION,       true,   true,   true },
        { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_RTL,           true,   true,   true },
        { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_FOLLOW_TARGET, true,   false,  true },
        { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_LAND,          false,  true,   true },
        { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_PRECLAND,      true,  false,  true },
        { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_READY,         false,  true,   true },
        { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_RTGS,          false,  true,   true },
        { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_TAKEOFF,       false,  true,   true },
    };

    // Must be in same order as above structure
    const QString* rgModeNames[] = {
        &_manualFlightMode,
        &_stabilizedFlightMode,
        &_acroFlightMode,
        &_rattitudeFlightMode,
        &_altCtlFlightMode,
        &_offboardFlightMode,
        &_simpleFlightMode,
        &_posCtlFlightMode,
        &_orbitFlightMode,
        &_holdFlightMode,
        &_missionFlightMode,
        &_rtlFlightMode,
        &_followMeFlightMode,
        &_landingFlightMode,
        &_preclandFlightMode,
        &_readyFlightMode,
        &_rtgsFlightMode,
        &_takeoffFlightMode,
    };

    // Convert static information to dynamic list. This allows for plugin override class to manipulate list.
    for (size_t i=0; i<sizeof(rgModes2Name)/sizeof(rgModes2Name[0]); i++) {
        const struct Modes2Name* pModes2Name = &rgModes2Name[i];

        FlightModeInfo_t info;

        info.main_mode =    pModes2Name->main_mode;
        info.sub_mode =     pModes2Name->sub_mode;
        info.name =         rgModeNames[i];
        info.canBeSet =     pModes2Name->canBeSet;
        info.fixedWing =    pModes2Name->fixedWing;
        info.multiRotor =   pModes2Name->multiRotor;

        _flightModeInfoList.append(info);
    }
}

PX4FirmwarePlugin::~PX4FirmwarePlugin()
{
}

AutoPilotPlugin* PX4FirmwarePlugin::autopilotPlugin(Vehicle* vehicle)
{
    return new PX4AutoPilotPlugin(vehicle, vehicle);
}

QList<VehicleComponent*> PX4FirmwarePlugin::componentsForVehicle(AutoPilotPlugin* vehicle)
{
    Q_UNUSED(vehicle);

    return QList<VehicleComponent*>();
}

QStringList PX4FirmwarePlugin::flightModes(Vehicle* vehicle)
{
    QStringList flightModes;

    foreach (const FlightModeInfo_t& info, _flightModeInfoList) {
        if (info.canBeSet) {
            bool fw = (vehicle->fixedWing() && info.fixedWing);
            bool mc = (vehicle->multiRotor() && info.multiRotor);

            // show all modes for generic, vtol, etc
            bool other = !vehicle->fixedWing() && !vehicle->multiRotor();

            if (fw || mc || other) {
                flightModes += *info.name;
            }
        }
    }

    return flightModes;
}

QString PX4FirmwarePlugin::flightMode(uint8_t base_mode, uint32_t custom_mode) const
{
    QString flightMode = "Unknown";

    if (base_mode & MAV_MODE_FLAG_CUSTOM_MODE_ENABLED) {
        union px4_custom_mode px4_mode;
        px4_mode.data = custom_mode;

        bool found = false;
        foreach (const FlightModeInfo_t& info, _flightModeInfoList) {
            if (info.main_mode == px4_mode.main_mode && info.sub_mode == px4_mode.sub_mode) {
                flightMode = *info.name;
                found = true;
                break;
            }
        }

        if (!found) {
            qWarning() << "Unknown flight mode" << custom_mode;
            return tr("Unknown %1:%2").arg(base_mode).arg(custom_mode);
        }
    }

    return flightMode;
}

bool PX4FirmwarePlugin::setFlightMode(const QString& flightMode, uint8_t* base_mode, uint32_t* custom_mode)
{
    *base_mode = 0;
    *custom_mode = 0;

    bool found = false;
    foreach (const FlightModeInfo_t& info, _flightModeInfoList) {
        if (flightMode.compare(info.name, Qt::CaseInsensitive) == 0) {
            union px4_custom_mode px4_mode;

            px4_mode.data = 0;
            px4_mode.main_mode = info.main_mode;
            px4_mode.sub_mode = info.sub_mode;

            *base_mode = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
            *custom_mode = px4_mode.data;

            found = true;
            break;
        }
    }

    if (!found) {
        qWarning() << "Unknown flight Mode" << flightMode;
    }

    return found;
}

bool PX4FirmwarePlugin::isCapable(const Vehicle *vehicle, FirmwareCapabilities capabilities)
{
    int available = SetFlightModeCapability | PauseVehicleCapability | GuidedModeCapability;
    //-- This is arbitrary until I find how to really tell if ROI is avaiable
    if (vehicle->multiRotor()) {
        available |= ROIModeCapability;
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

bool PX4FirmwarePlugin::sendHomePositionToVehicle(void)
{
    // PX4 stack does not want home position sent in the first position.
    // Subsequent sequence numbers must be adjusted.
    return false;
}

FactMetaData* PX4FirmwarePlugin::_getMetaDataForFact(QObject* parameterMetaData, const QString& name, FactMetaData::ValueType_t type, MAV_TYPE vehicleType)
{
    PX4ParameterMetaData* px4MetaData = qobject_cast<PX4ParameterMetaData*>(parameterMetaData);

    if (px4MetaData) {
        return px4MetaData->getMetaDataForFact(name, vehicleType, type);
    } else {
        qWarning() << "Internal error: pointer passed to PX4FirmwarePlugin::getMetaDataForFact not PX4ParameterMetaData";
    }

    return nullptr;
}

void PX4FirmwarePlugin::_getParameterMetaDataVersionInfo(const QString& metaDataFile, int& majorVersion, int& minorVersion)
{
    return PX4ParameterMetaData::getParameterMetaDataVersionInfo(metaDataFile, majorVersion, minorVersion);
}

QList<MAV_CMD> PX4FirmwarePlugin::supportedMissionCommands(QGCMAVLink::VehicleClass_t vehicleClass)
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

    if (qgcApp()->toolbox()->settingsManager()->planViewSettings()->useConditionGate()->rawValue().toBool()) {
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

void PX4FirmwarePlugin::pauseVehicle(Vehicle* vehicle)
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

void PX4FirmwarePlugin::guidedModeRTL(Vehicle* vehicle, bool smartRTL)
{
    Q_UNUSED(smartRTL);
    _setFlightModeAndValidate(vehicle, _rtlFlightMode);
}

void PX4FirmwarePlugin::guidedModeLand(Vehicle* vehicle)
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

void PX4FirmwarePlugin::guidedModeTakeoff(Vehicle* vehicle, double takeoffAltRel)
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

double PX4FirmwarePlugin::maximumHorizontalSpeedMultirotor(Vehicle* vehicle)
{
    QString speedParam("MPC_XY_VEL_MAX");

    if (vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, speedParam)) {
        return vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, speedParam)->rawValue().toDouble();
    }

    return FirmwarePlugin::maximumHorizontalSpeedMultirotor(vehicle);
}

double PX4FirmwarePlugin::maximumEquivalentAirspeed(Vehicle* vehicle)
{
    QString airspeedMax("FW_AIRSPD_MAX");

    if (vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, airspeedMax)) {
        return vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, airspeedMax)->rawValue().toDouble();
    }

    return FirmwarePlugin::maximumEquivalentAirspeed(vehicle);
}

double PX4FirmwarePlugin::minimumEquivalentAirspeed(Vehicle* vehicle)
{
    QString airspeedMin("FW_AIRSPD_MIN");

    if (vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, airspeedMin)) {
        return vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, airspeedMin)->rawValue().toDouble();
    }

    return FirmwarePlugin::minimumEquivalentAirspeed(vehicle);
}

bool PX4FirmwarePlugin::mulirotorSpeedLimitsAvailable(Vehicle* vehicle)
{
    return vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, "MPC_XY_VEL_MAX");
}

bool PX4FirmwarePlugin::fixedWingAirSpeedLimitsAvailable(Vehicle* vehicle)
{
    return vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, "FW_AIRSPD_MIN") &&
            vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, "FW_AIRSPD_MAX");
}

void PX4FirmwarePlugin::guidedModeGotoLocation(Vehicle* vehicle, const QGeoCoordinate& gotoCoord)
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

static void _pauseVehicleThenChangeAltResultHandler(void* resultHandlerData, int /*compId*/, MAV_RESULT commandResult, uint8_t progress, Vehicle::MavCmdResultFailureCode_t failureCode)
{
    Q_UNUSED(progress);

    if (commandResult != MAV_RESULT_ACCEPTED) {
        switch (failureCode) {
        case Vehicle::MavCmdResultCommandResultOnly:
            qDebug() << QStringLiteral("MAV_CMD_DO_REPOSITION error(%1)").arg(commandResult);
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
    pData->plugin->_changeAltAfterPause(resultHandlerData, commandResult == MAV_RESULT_ACCEPTED /* pauseSucceeded */);
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
        vehicle->sendMavCommandWithHandler(
                    _pauseVehicleThenChangeAltResultHandler,
                    resultData,
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

void PX4FirmwarePlugin::guidedModeChangeGroundSpeed(Vehicle* vehicle, double groundspeed)
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

void PX4FirmwarePlugin::guidedModeChangeEquivalentAirspeed(Vehicle* vehicle, double airspeed_equiv)
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

void PX4FirmwarePlugin::startMission(Vehicle* vehicle)
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

void PX4FirmwarePlugin::setGuidedMode(Vehicle* vehicle, bool guidedMode)
{
    if (guidedMode) {
        _setFlightModeAndValidate(vehicle, _holdFlightMode);
    } else {
        pauseVehicle(vehicle);
    }
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

uint32_t PX4FirmwarePlugin::highLatencyCustomModeTo32Bits(uint16_t hlCustomMode)
{
    union px4_custom_mode px4_cm;
    px4_cm.data = 0;
    px4_cm.custom_mode_hl = hlCustomMode;

    return px4_cm.data;
}

QString PX4FirmwarePlugin::_getLatestVersionFileUrl(Vehicle* vehicle){
    Q_UNUSED(vehicle);
    return QStringLiteral("https://api.github.com/repos/PX4/Firmware/releases");
}

QString PX4FirmwarePlugin::_versionRegex() {
    return QStringLiteral("v([0-9,\\.]*) Stable");
}

bool PX4FirmwarePlugin::supportsNegativeThrust(Vehicle* vehicle)
{
    return ((vehicle->vehicleType() == MAV_TYPE_GROUND_ROVER) || (vehicle->vehicleType() == MAV_TYPE_SUBMARINE));
}

QString PX4FirmwarePlugin::getHobbsMeter(Vehicle* vehicle)
{
    static const char* HOOBS_HI = "LND_FLIGHT_T_HI";
    static const char* HOOBS_LO = "LND_FLIGHT_T_LO";
    uint64_t hobbsTimeSeconds = 0;

    if (vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, HOOBS_HI) &&
            vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, HOOBS_LO)) {
        Fact* factHi = vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, HOOBS_HI);
        Fact* factLo = vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, HOOBS_LO);
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
