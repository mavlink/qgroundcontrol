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

#include "PX4FirmwarePlugin.h"
#include "PX4ParameterMetaData.h"
#include "QGCApplication.h"
#include "PX4AutoPilotPlugin.h"
#include "PX4AdvancedFlightModesController.h"
#include "PX4SimpleFlightModesController.h"
#include "AirframeComponentController.h"
#include "SensorsComponentController.h"
#include "PowerComponentController.h"
#include "RadioComponentController.h"

#include <QDebug>

#include "px4_custom_mode.h"

PX4FirmwarePluginInstanceData::PX4FirmwarePluginInstanceData(QObject* parent)
    : QObject(parent)
    , versionNotified(false)
{

}

PX4FirmwarePlugin::PX4FirmwarePlugin(void)
    : _manualFlightMode(tr("Manual"))
    , _acroFlightMode(tr("Acro"))
    , _stabilizedFlightMode(tr("Stabilized"))
    , _rattitudeFlightMode(tr("Rattitude"))
    , _altCtlFlightMode(tr("Altitude"))
    , _posCtlFlightMode(tr("Position"))
    , _offboardFlightMode(tr("Offboard"))
    , _readyFlightMode(tr("Ready"))
    , _takeoffFlightMode(tr("Takeoff"))
    , _holdFlightMode(tr("Hold"))
    , _missionFlightMode(tr("Mission"))
    , _rtlFlightMode(tr("Return"))
    , _landingFlightMode(tr("Land"))
    , _rtgsFlightMode(tr("Return to Groundstation"))
    , _followMeFlightMode(tr("Follow Me"))
    , _simpleFlightMode(tr("Simple"))
{
    qmlRegisterType<PX4AdvancedFlightModesController>   ("QGroundControl.Controllers", 1, 0, "PX4AdvancedFlightModesController");
    qmlRegisterType<PX4SimpleFlightModesController>     ("QGroundControl.Controllers", 1, 0, "PX4SimpleFlightModesController");
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
        { PX4_CUSTOM_MAIN_MODE_MANUAL,      0,                                      true,   true,   true },
        { PX4_CUSTOM_MAIN_MODE_STABILIZED,  0,                                      true,   true,   true },
        { PX4_CUSTOM_MAIN_MODE_ACRO,        0,                                      true,   true,   true },
        { PX4_CUSTOM_MAIN_MODE_RATTITUDE,   0,                                      true,   true,   true },
        { PX4_CUSTOM_MAIN_MODE_ALTCTL,      0,                                      true,   true,   true },
        { PX4_CUSTOM_MAIN_MODE_POSCTL,      0,                                      true,   true,   true },
        // simple can't be set by the user right now
        { PX4_CUSTOM_MAIN_MODE_SIMPLE,      0,                                      false,   false,  true },
        { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_LOITER,        true,   true,   true },
        { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_MISSION,       true,   true,   true },
        { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_RTL,           true,   true,   true },
        { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_FOLLOW_TARGET, true,   false,  true },
        { PX4_CUSTOM_MAIN_MODE_OFFBOARD,    0,                                      true,   false,  true },
        // modes that can't be directly set by the user
        { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_LAND,          false,  true,   true },
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
        &_posCtlFlightMode,
        &_simpleFlightMode,
        &_holdFlightMode,
        &_missionFlightMode,
        &_rtlFlightMode,
        &_followMeFlightMode,
        &_offboardFlightMode,
        &_landingFlightMode,
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
    } else {
        qWarning() << "PX4 Flight Stack flight mode without custom mode enabled?";
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

int PX4FirmwarePlugin::manualControlReservedButtonCount(void)
{
    return 0;   // 0 buttons reserved for rc switch simulation
}

bool PX4FirmwarePlugin::isCapable(const Vehicle *vehicle, FirmwareCapabilities capabilities)
{
    int available = MavCmdPreflightStorageCapability | SetFlightModeCapability | PauseVehicleCapability | GuidedModeCapability;
    if (vehicle->multiRotor() || vehicle->vtol()) {
        available |= TakeoffVehicleCapability;
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

void PX4FirmwarePlugin::addMetaDataToFact(QObject* parameterMetaData, Fact* fact, MAV_TYPE vehicleType)
{
    PX4ParameterMetaData* px4MetaData = qobject_cast<PX4ParameterMetaData*>(parameterMetaData);

    if (px4MetaData) {
        px4MetaData->addMetaDataToFact(fact, vehicleType);
    } else {
        qWarning() << "Internal error: pointer passed to PX4FirmwarePlugin::addMetaDataToFact not PX4ParameterMetaData";
    }
}

QList<MAV_CMD> PX4FirmwarePlugin::supportedMissionCommands(void)
{
    QList<MAV_CMD> list;

    list << MAV_CMD_NAV_WAYPOINT
         << MAV_CMD_NAV_LOITER_UNLIM << MAV_CMD_NAV_LOITER_TIME << MAV_CMD_NAV_LOITER_TO_ALT
         << MAV_CMD_NAV_LAND << MAV_CMD_NAV_TAKEOFF
         << MAV_CMD_DO_JUMP
         << MAV_CMD_DO_VTOL_TRANSITION << MAV_CMD_NAV_VTOL_TAKEOFF << MAV_CMD_NAV_VTOL_LAND
         << MAV_CMD_DO_DIGICAM_CONTROL
         << MAV_CMD_DO_SET_CAM_TRIGG_DIST
         << MAV_CMD_DO_SET_SERVO
         << MAV_CMD_DO_CHANGE_SPEED
         << MAV_CMD_DO_LAND_START
         << MAV_CMD_DO_SET_ROI
         << MAV_CMD_DO_MOUNT_CONFIGURE
         << MAV_CMD_DO_MOUNT_CONTROL
         << MAV_CMD_SET_CAMERA_MODE
         << MAV_CMD_IMAGE_START_CAPTURE << MAV_CMD_IMAGE_STOP_CAPTURE << MAV_CMD_VIDEO_START_CAPTURE << MAV_CMD_VIDEO_STOP_CAPTURE
         << MAV_CMD_NAV_DELAY;

    return list;
}

QString PX4FirmwarePlugin::missionCommandOverrides(MAV_TYPE vehicleType) const
{
    switch (vehicleType) {
    case MAV_TYPE_GENERIC:
        return QStringLiteral(":/json/PX4/MavCmdInfoCommon.json");
        break;
    case MAV_TYPE_FIXED_WING:
        return QStringLiteral(":/json/PX4/MavCmdInfoFixedWing.json");
        break;
    case MAV_TYPE_QUADROTOR:
        return QStringLiteral(":/json/PX4/MavCmdInfoMultiRotor.json");
        break;
    case MAV_TYPE_VTOL_QUADROTOR:
        return QStringLiteral(":/json/PX4/MavCmdInfoVTOL.json");
        break;
    case MAV_TYPE_SUBMARINE:
        return QStringLiteral(":/json/PX4/MavCmdInfoSub.json");
        break;
    case MAV_TYPE_GROUND_ROVER:
        return QStringLiteral(":/json/PX4/MavCmdInfoRover.json");
        break;
    default:
        qWarning() << "PX4FirmwarePlugin::missionCommandOverrides called with bad MAV_TYPE:" << vehicleType;
        return QString();
    }
}

QObject* PX4FirmwarePlugin::loadParameterMetaData(const QString& metaDataFile)
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

void PX4FirmwarePlugin::guidedModeRTL(Vehicle* vehicle)
{
    _setFlightModeAndValidate(vehicle, _rtlFlightMode);
}

void PX4FirmwarePlugin::guidedModeLand(Vehicle* vehicle)
{
    _setFlightModeAndValidate(vehicle, _landingFlightMode);
}

void PX4FirmwarePlugin::guidedModeOrbit(Vehicle* vehicle, const QGeoCoordinate& centerCoord, double radius, double velocity, double altitude)
{
    if (!isGuidedMode(vehicle)) {
        setGuidedMode(vehicle, true);
    }

    vehicle->sendMavCommand(vehicle->defaultComponentId(),
                            MAV_CMD_SET_GUIDED_SUBMODE_CIRCLE,
                            true,   // show error if fails
                            radius,
                            velocity,
                            altitude,
                            NAN,
                            centerCoord.isValid() ? centerCoord.latitude()  : NAN,
                            centerCoord.isValid() ? centerCoord.longitude() : NAN,
                            centerCoord.isValid() ? centerCoord.altitude()  : NAN);
}

void PX4FirmwarePlugin::_mavCommandResult(int vehicleId, int component, int command, int result, bool noReponseFromVehicle)
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
        disconnect(vehicle, &Vehicle::mavCommandResult, this, &PX4FirmwarePlugin::_mavCommandResult);
        if (!vehicle->armed()) {
            vehicle->setArmed(true);
        }
    }
}

void PX4FirmwarePlugin::guidedModeTakeoff(Vehicle* vehicle)
{
    QString takeoffAltParam("MIS_TAKEOFF_ALT");

    if (qIsNaN(vehicle->altitudeAMSL()->rawValue().toDouble())) {
        qgcApp()->showMessage(tr("Unable to takeoff, vehicle position not known."));
        return;
    }

    if (!vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, takeoffAltParam)) {
        qgcApp()->showMessage(tr("Unable to takeoff, MIS_TAKEOFF_ALT parameter missing."));
        return;
    }
    Fact* takeoffAlt = vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, takeoffAltParam);

    connect(vehicle, &Vehicle::mavCommandResult, this, &PX4FirmwarePlugin::_mavCommandResult);
    vehicle->sendMavCommand(vehicle->defaultComponentId(),
                            MAV_CMD_NAV_TAKEOFF,
                            true,                           // show error is fails
                            -1,                             // No pitch requested
                            0, 0,                           // param 2-4 unused
                            NAN, NAN, NAN,                  // No yaw, lat, lon
                            vehicle->altitudeAMSL()->rawValue().toDouble() + takeoffAlt->rawValue().toDouble());
}

void PX4FirmwarePlugin::guidedModeGotoLocation(Vehicle* vehicle, const QGeoCoordinate& gotoCoord)
{
    if (qIsNaN(vehicle->altitudeAMSL()->rawValue().toDouble())) {
        qgcApp()->showMessage(tr("Unable to go to location, vehicle position not known."));
        return;
    }

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

void PX4FirmwarePlugin::guidedModeChangeAltitude(Vehicle* vehicle, double altitudeChange)
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

void PX4FirmwarePlugin::startMission(Vehicle* vehicle)
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

    PX4FirmwarePluginInstanceData* instanceData = qobject_cast<PX4FirmwarePluginInstanceData*>(vehicle->firmwarePluginInstanceData());
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
            qgcApp()->showMessage(QString("QGroundControl supports PX4 Pro firmware Version %1.%2.%3 and above. You are using a version prior to that which will lead to unpredictable results. Please upgrade your firmware.").arg(supportedMajorVersion).arg(supportedMinorVersion).arg(supportedPatchVersion));
        }
    }
}

bool PX4FirmwarePlugin::vehicleYawsToNextWaypointInMission(const Vehicle* vehicle) const
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
