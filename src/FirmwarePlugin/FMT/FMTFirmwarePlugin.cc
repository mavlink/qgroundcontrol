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

#include "FMTFirmwarePlugin.h"
#include "FMTParameterMetaData.h"
#include "QGCApplication.h"
#include "FMTAutoPilotPlugin.h"
#include "FMTAdvancedFlightModesController.h"
#include "FMTSimpleFlightModesController.h"
#include "AirframeComponentController.h"
#include "SensorsComponentController.h"
#include "PowerComponentController.h"
#include "RadioComponentController.h"
#include "QGCCameraManager.h"
#include "QGCFileDownload.h"

#include <QDebug>

#include "fmt_custom_mode.h"

FMTFirmwarePluginInstanceData::FMTFirmwarePluginInstanceData(QObject* parent)
    : QObject(parent)
    , versionNotified(false)
{

}

FMTFirmwarePlugin::FMTFirmwarePlugin(void)
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
    qmlRegisterType<FMTAdvancedFlightModesController>   ("QGroundControl.Controllers", 1, 0, "FMTAdvancedFlightModesController");
    qmlRegisterType<FMTSimpleFlightModesController>     ("QGroundControl.Controllers", 1, 0, "FMTSimpleFlightModesController");
    qmlRegisterType<AirframeComponentController>        ("QGroundControl.Controllers", 1, 0, "AirframeComponentController");
    qmlRegisterType<SensorsComponentController>         ("QGroundControl.Controllers", 1, 0, "SensorsComponentController");
    qmlRegisterType<PowerComponentController>           ("QGroundControl.Controllers", 1, 0, "PowerComponentController");
    qmlRegisterType<RadioComponentController>           ("QGroundControl.Controllers", 1, 0, "RadioComponentController");

    struct Modes2Name {
        uint8_t     main_mode;
        uint8_t     sub_mode;
        bool        canBeSet;   ///< true: Vehicle can be set to this flight mode
        bool        fixedWing;  /// fixed wing compatible
        bool        multiRotor;  /// multi rotor compatible
    };

    static const struct Modes2Name rgModes2Name[] = {
        //main_mode                         sub_mode                                canBeSet  FW      MC
        { FMT_CUSTOM_MAIN_MODE_MANUAL,      0,                                      true,   true,   true },
        { FMT_CUSTOM_MAIN_MODE_STABILIZED,  0,                                      true,   true,   true },
        { FMT_CUSTOM_MAIN_MODE_ACRO,        0,                                      true,   true,   true },
        { FMT_CUSTOM_MAIN_MODE_RATTITUDE,   0,                                      true,   true,   true },
        { FMT_CUSTOM_MAIN_MODE_ALTCTL,      0,                                      true,   true,   true },
        { FMT_CUSTOM_MAIN_MODE_OFFBOARD,    0,                                      true,   false,  true },
        { FMT_CUSTOM_MAIN_MODE_SIMPLE,      0,                                      false,  false,  true },
        { FMT_CUSTOM_MAIN_MODE_POSCTL,      FMT_CUSTOM_SUB_MODE_POSCTL_POSCTL,      true,   true,   true },
        { FMT_CUSTOM_MAIN_MODE_POSCTL,      FMT_CUSTOM_SUB_MODE_POSCTL_ORBIT,       false,  false,   false },
        { FMT_CUSTOM_MAIN_MODE_AUTO,        FMT_CUSTOM_SUB_MODE_AUTO_LOITER,        true,   true,   true },
        { FMT_CUSTOM_MAIN_MODE_AUTO,        FMT_CUSTOM_SUB_MODE_AUTO_MISSION,       true,   true,   true },
        { FMT_CUSTOM_MAIN_MODE_AUTO,        FMT_CUSTOM_SUB_MODE_AUTO_RTL,           true,   true,   true },
        { FMT_CUSTOM_MAIN_MODE_AUTO,        FMT_CUSTOM_SUB_MODE_AUTO_FOLLOW_TARGET, true,   false,  true },
        { FMT_CUSTOM_MAIN_MODE_AUTO,        FMT_CUSTOM_SUB_MODE_AUTO_LAND,          false,  true,   true },
        { FMT_CUSTOM_MAIN_MODE_AUTO,        FMT_CUSTOM_SUB_MODE_AUTO_PRECLAND,      false,  false,  true },
        { FMT_CUSTOM_MAIN_MODE_AUTO,        FMT_CUSTOM_SUB_MODE_AUTO_READY,         false,  true,   true },
        { FMT_CUSTOM_MAIN_MODE_AUTO,        FMT_CUSTOM_SUB_MODE_AUTO_RTGS,          false,  true,   true },
        { FMT_CUSTOM_MAIN_MODE_AUTO,        FMT_CUSTOM_SUB_MODE_AUTO_TAKEOFF,       false,  true,   true },
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

FMTFirmwarePlugin::~FMTFirmwarePlugin()
{
}

AutoPilotPlugin* FMTFirmwarePlugin::autopilotPlugin(Vehicle* vehicle)
{
    return new FMTAutoPilotPlugin(vehicle, vehicle);
}

QList<VehicleComponent*> FMTFirmwarePlugin::componentsForVehicle(AutoPilotPlugin* vehicle)
{
    Q_UNUSED(vehicle);

    return QList<VehicleComponent*>();
}

QStringList FMTFirmwarePlugin::flightModes(Vehicle* vehicle)
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

QString FMTFirmwarePlugin::flightMode(uint8_t base_mode, uint32_t custom_mode) const
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
    } else {
        qWarning() << "FMT Flight Stack flight mode without custom mode enabled?";
    }

    return flightMode;
}

bool FMTFirmwarePlugin::setFlightMode(const QString& flightMode, uint8_t* base_mode, uint32_t* custom_mode)
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

bool FMTFirmwarePlugin::isCapable(const Vehicle *vehicle, FirmwareCapabilities capabilities)
{
    int available = SetFlightModeCapability | PauseVehicleCapability | GuidedModeCapability;
    if (vehicle->multiRotor() || vehicle->vtol()) {
        available |= TakeoffVehicleCapability | OrbitModeCapability;
    }

    return (capabilities & available) == capabilities;
}

void FMTFirmwarePlugin::initializeVehicle(Vehicle* vehicle)
{
    vehicle->setFirmwarePluginInstanceData(new FMTFirmwarePluginInstanceData);
}

bool FMTFirmwarePlugin::sendHomePositionToVehicle(void)
{
    // FMT stack does not want home position sent in the first position.
    // Subsequent sequence numbers must be adjusted.
    return false;
}

FactMetaData* FMTFirmwarePlugin::getMetaDataForFact(QObject* parameterMetaData, const QString& name, MAV_TYPE vehicleType)
{
    FMTParameterMetaData* px4MetaData = qobject_cast<FMTParameterMetaData*>(parameterMetaData);

    if (px4MetaData) {
        return px4MetaData->getMetaDataForFact(name, vehicleType);
    } else {
        qWarning() << "Internal error: pointer passed to FMTFirmwarePlugin::getMetaDataForFact not FMTParameterMetaData";
    }

    return NULL;
}

void FMTFirmwarePlugin::addMetaDataToFact(QObject* parameterMetaData, Fact* fact, MAV_TYPE vehicleType)
{
    FMTParameterMetaData* px4MetaData = qobject_cast<FMTParameterMetaData*>(parameterMetaData);

    if (px4MetaData) {
        px4MetaData->addMetaDataToFact(fact, vehicleType);
    } else {
        qWarning() << "Internal error: pointer passed to FMTFirmwarePlugin::addMetaDataToFact not FMTParameterMetaData";
    }
}

void FMTFirmwarePlugin::getParameterMetaDataVersionInfo(const QString& metaDataFile, int& majorVersion, int& minorVersion)
{
    return FMTParameterMetaData::getParameterMetaDataVersionInfo(metaDataFile, majorVersion, minorVersion);
}

QList<MAV_CMD> FMTFirmwarePlugin::supportedMissionCommands(void)
{
    QList<MAV_CMD> list;

    list << MAV_CMD_NAV_WAYPOINT
         << MAV_CMD_NAV_LOITER_UNLIM << MAV_CMD_NAV_LOITER_TIME << MAV_CMD_NAV_LOITER_TO_ALT
         << MAV_CMD_NAV_LAND << MAV_CMD_NAV_TAKEOFF << MAV_CMD_NAV_RETURN_TO_LAUNCH
         << MAV_CMD_DO_JUMP
         << MAV_CMD_DO_VTOL_TRANSITION << MAV_CMD_NAV_VTOL_TAKEOFF << MAV_CMD_NAV_VTOL_LAND
         << MAV_CMD_DO_DIGICAM_CONTROL
         << MAV_CMD_DO_SET_CAM_TRIGG_DIST
         << MAV_CMD_DO_SET_SERVO
         << MAV_CMD_DO_CHANGE_SPEED
         << MAV_CMD_DO_LAND_START
         << MAV_CMD_DO_SET_ROI_LOCATION << MAV_CMD_DO_SET_ROI_WPNEXT_OFFSET << MAV_CMD_DO_SET_ROI_NONE
         << MAV_CMD_DO_MOUNT_CONFIGURE
         << MAV_CMD_DO_MOUNT_CONTROL
         << MAV_CMD_SET_CAMERA_MODE
         << MAV_CMD_IMAGE_START_CAPTURE << MAV_CMD_IMAGE_STOP_CAPTURE << MAV_CMD_VIDEO_START_CAPTURE << MAV_CMD_VIDEO_STOP_CAPTURE
         << MAV_CMD_NAV_DELAY
         << MAV_CMD_CONDITION_YAW;

    return list;
}

QString FMTFirmwarePlugin::missionCommandOverrides(MAV_TYPE vehicleType) const
{
    switch (vehicleType) {
    case MAV_TYPE_GENERIC:
        return QStringLiteral(":/json/FMT/MavCmdInfoCommon.json");
        break;
    case MAV_TYPE_FIXED_WING:
        return QStringLiteral(":/json/FMT/MavCmdInfoFixedWing.json");
        break;
    case MAV_TYPE_QUADROTOR:
        return QStringLiteral(":/json/FMT/MavCmdInfoMultiRotor.json");
        break;
    case MAV_TYPE_VTOL_QUADROTOR:
        return QStringLiteral(":/json/FMT/MavCmdInfoVTOL.json");
        break;
    case MAV_TYPE_SUBMARINE:
        return QStringLiteral(":/json/FMT/MavCmdInfoSub.json");
        break;
    case MAV_TYPE_GROUND_ROVER:
        return QStringLiteral(":/json/FMT/MavCmdInfoRover.json");
        break;
    default:
        qWarning() << "FMTFirmwarePlugin::missionCommandOverrides called with bad MAV_TYPE:" << vehicleType;
        return QString();
    }
}

QObject* FMTFirmwarePlugin::loadParameterMetaData(const QString& metaDataFile)
{
    FMTParameterMetaData* metaData = new FMTParameterMetaData;
    if (!metaDataFile.isEmpty()) {
        metaData->loadParameterFactMetaDataFile(metaDataFile);
    }
    return metaData;
}

void FMTFirmwarePlugin::pauseVehicle(Vehicle* vehicle)
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

void FMTFirmwarePlugin::guidedModeRTL(Vehicle* vehicle)
{
    _setFlightModeAndValidate(vehicle, _rtlFlightMode);
}

void FMTFirmwarePlugin::guidedModeLand(Vehicle* vehicle)
{
    _setFlightModeAndValidate(vehicle, _landingFlightMode);
}

void FMTFirmwarePlugin::_mavCommandResult(int vehicleId, int component, int command, int result, bool noReponseFromVehicle)
{
    Q_UNUSED(vehicleId);
    Q_UNUSED(component);
    Q_UNUSED(noReponseFromVehicle);

    Vehicle* vehicle = dynamic_cast<Vehicle*>(sender());
    if (!vehicle) {
        qWarning() << "Dynamic cast failed!";
        return;
    }

    if (command == MAV_CMD_NAV_TAKEOFF && result == MAV_RESULT_ACCEPTED) {
        // Now that we are in takeoff mode we can arm the vehicle which will cause it to takeoff.
        // We specifically don't retry arming if it fails. This way we don't fight with the user if
        // They are trying to disarm.
        disconnect(vehicle, &Vehicle::mavCommandResult, this, &FMTFirmwarePlugin::_mavCommandResult);
        if (!vehicle->armed()) {
            vehicle->setArmed(true);
        }
    }
}

double FMTFirmwarePlugin::minimumTakeoffAltitude(Vehicle* vehicle)
{
    QString takeoffAltParam("MIS_TAKEOFF_ALT");

    if (vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, takeoffAltParam)) {
        return vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, takeoffAltParam)->rawValue().toDouble();
    } else {
        return FirmwarePlugin::minimumTakeoffAltitude(vehicle);
    }
}

void FMTFirmwarePlugin::guidedModeTakeoff(Vehicle* vehicle, double takeoffAltRel)
{
    double vehicleAltitudeAMSL = vehicle->altitudeAMSL()->rawValue().toDouble();
    if (qIsNaN(vehicleAltitudeAMSL)) {
        qgcApp()->showMessage(tr("Unable to takeoff, vehicle position not known."));
        return;
    }

    double takeoffAltRelFromVehicle = minimumTakeoffAltitude(vehicle);
    double takeoffAltAMSL = qMax(takeoffAltRel, takeoffAltRelFromVehicle) + vehicleAltitudeAMSL;

    qDebug() << takeoffAltRel << takeoffAltRelFromVehicle << takeoffAltAMSL << vehicleAltitudeAMSL;

    connect(vehicle, &Vehicle::mavCommandResult, this, &FMTFirmwarePlugin::_mavCommandResult);
    vehicle->sendMavCommand(vehicle->defaultComponentId(),
                            MAV_CMD_NAV_TAKEOFF,
                            true,                           // show error is fails
                            -1,                             // No pitch requested
                            0, 0,                           // param 2-4 unused
                            NAN, NAN, NAN,                  // No yaw, lat, lon
                            takeoffAltAMSL);                // AMSL altitude
}

void FMTFirmwarePlugin::guidedModeGotoLocation(Vehicle* vehicle, const QGeoCoordinate& gotoCoord)
{
    if (qIsNaN(vehicle->altitudeAMSL()->rawValue().toDouble())) {
        qgcApp()->showMessage(tr("Unable to go to location, vehicle position not known."));
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
                                gotoCoord.latitude(),
                                gotoCoord.longitude(),
                                vehicle->altitudeAMSL()->rawValue().toFloat());
    }
}

void FMTFirmwarePlugin::guidedModeChangeAltitude(Vehicle* vehicle, double altitudeChange)
{
    if (!vehicle->homePosition().isValid()) {
        qgcApp()->showMessage(tr("Unable to change altitude, home position unknown."));
        return;
    }
    if (qIsNaN(vehicle->homePosition().altitude())) {
        qgcApp()->showMessage(tr("Unable to change altitude, home position altitude unknown."));
        return;
    }

    double currentAltRel = vehicle->altitudeRelative()->rawValue().toDouble();
    double newAltRel = currentAltRel + altitudeChange;

    vehicle->sendMavCommand(vehicle->defaultComponentId(),
                            MAV_CMD_DO_REPOSITION,
                            true,   // show error is fails
                            -1.0f,
                            MAV_DO_REPOSITION_FLAGS_CHANGE_MODE,
                            0.0f,
                            NAN,
                            NAN,
                            NAN,
                            vehicle->homePosition().altitude() + newAltRel);
}

void FMTFirmwarePlugin::startMission(Vehicle* vehicle)
{

    if (_setFlightModeAndValidate(vehicle, missionFlightMode())) {
        if (!_armVehicleAndValidate(vehicle)) {
            qgcApp()->showMessage(tr("Unable to start mission: Vehicle rejected arming."));
            return;
        }
    } else {
        qgcApp()->showMessage(tr("Unable to start mission: Vehicle not ready."));
    }
}

void FMTFirmwarePlugin::setGuidedMode(Vehicle* vehicle, bool guidedMode)
{
    if (guidedMode) {
        _setFlightModeAndValidate(vehicle, _holdFlightMode);
    } else {
        pauseVehicle(vehicle);
    }
}

bool FMTFirmwarePlugin::isGuidedMode(const Vehicle* vehicle) const
{
    // Not supported by generic vehicle
    return (vehicle->flightMode() == _holdFlightMode || vehicle->flightMode() == _takeoffFlightMode
            || vehicle->flightMode() == _landingFlightMode);
}

bool FMTFirmwarePlugin::adjustIncomingMavlinkMessage(Vehicle* vehicle, mavlink_message_t* message)
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

void FMTFirmwarePlugin::_handleAutopilotVersion(Vehicle* vehicle, mavlink_message_t* message)
{
    Q_UNUSED(vehicle);

    FMTFirmwarePluginInstanceData* instanceData = qobject_cast<FMTFirmwarePluginInstanceData*>(vehicle->firmwarePluginInstanceData());
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
            qgcApp()->showMessage(tr("QGroundControl supports FMT Pro firmware Version %1.%2.%3 and above. You are using a version prior to that which will lead to unpredictable results. Please upgrade your firmware.").arg(supportedMajorVersion).arg(supportedMinorVersion).arg(supportedPatchVersion));
        }
    }
}

bool FMTFirmwarePlugin::vehicleYawsToNextWaypointInMission(const Vehicle* vehicle) const
{
    if (vehicle->isOfflineEditingVehicle()) {
        return FirmwarePlugin::vehicleYawsToNextWaypointInMission(vehicle);
    } else {
        if (vehicle->multiRotor() && vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, QStringLiteral("MIS_YAWMODE"))) {
            Fact* yawMode = vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, QStringLiteral("MIS_YAWMODE"));
            return yawMode && yawMode->rawValue().toInt() == 1;
        }
    }
    return true;
}

QGCCameraManager* FMTFirmwarePlugin::createCameraManager(Vehicle* vehicle)
{
    return new QGCCameraManager(vehicle);
}

QGCCameraControl* FMTFirmwarePlugin::createCameraControl(const mavlink_camera_information_t* info, Vehicle *vehicle, int compID, QObject* parent)
{
    return new QGCCameraControl(info, vehicle, compID, parent);
}

uint32_t FMTFirmwarePlugin::highLatencyCustomModeTo32Bits(uint16_t hlCustomMode)
{
    union px4_custom_mode px4_cm;
    px4_cm.data = 0;
    px4_cm.custom_mode_hl = hlCustomMode;

    return px4_cm.data;
}

QString FMTFirmwarePlugin::_getLatestVersionFileUrl(Vehicle* vehicle){
    Q_UNUSED(vehicle);
    return QStringLiteral("https://api.github.com/repos/FMT/Firmware/releases");
}

QString FMTFirmwarePlugin::_versionRegex() {
    return QStringLiteral("v([0-9,\\.]*) Stable");
}
