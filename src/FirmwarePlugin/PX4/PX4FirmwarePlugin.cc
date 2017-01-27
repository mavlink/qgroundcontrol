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

struct Modes2Name {
    uint8_t     main_mode;
    uint8_t     sub_mode;
    const char* name;       ///< Name for flight mode
    bool        canBeSet;   ///< true: Vehicle can be set to this flight mode
    bool        fixedWing;  /// fixed wing compatible
    bool        multiRotor;  /// multi rotor compatible
};

const char* PX4FirmwarePlugin::_manualFlightMode =      "Manual";
const char* PX4FirmwarePlugin::_altCtlFlightMode =      "Altitude";
const char* PX4FirmwarePlugin::_posCtlFlightMode =      "Position";
const char* PX4FirmwarePlugin::_missionFlightMode =     "Mission";
const char* PX4FirmwarePlugin::_holdFlightMode =        "Hold";
const char* PX4FirmwarePlugin::_takeoffFlightMode =     "Takeoff";
const char* PX4FirmwarePlugin::_landingFlightMode =     "Land";
const char* PX4FirmwarePlugin::_rtlFlightMode =         "Return";
const char* PX4FirmwarePlugin::_acroFlightMode =        "Acro";
const char* PX4FirmwarePlugin::_offboardFlightMode =    "Offboard";
const char* PX4FirmwarePlugin::_stabilizedFlightMode =  "Stabilized";
const char* PX4FirmwarePlugin::_rattitudeFlightMode =   "Rattitude";
const char* PX4FirmwarePlugin::_followMeFlightMode =    "Follow Me";
const char* PX4FirmwarePlugin::_rtgsFlightMode =        "Return to Groundstation";
const char* PX4FirmwarePlugin::_readyFlightMode =       "Ready";

/// Tranlates from PX4 custom modes to flight mode names

static const struct Modes2Name rgModes2Name[] = {
    //main_mode                         sub_mode                                name                                        canBeSet  FW      MC
    { PX4_CUSTOM_MAIN_MODE_MANUAL,      0,                                      PX4FirmwarePlugin::_manualFlightMode,       true,   true,   true },
    { PX4_CUSTOM_MAIN_MODE_STABILIZED,  0,                                      PX4FirmwarePlugin::_stabilizedFlightMode,   true,   true,   true },
    { PX4_CUSTOM_MAIN_MODE_ACRO,        0,                                      PX4FirmwarePlugin::_acroFlightMode,         true,   false,  true },
    { PX4_CUSTOM_MAIN_MODE_RATTITUDE,   0,                                      PX4FirmwarePlugin::_rattitudeFlightMode,    true,   false,  true },
    { PX4_CUSTOM_MAIN_MODE_ALTCTL,      0,                                      PX4FirmwarePlugin::_altCtlFlightMode,       true,   true,   true },
    { PX4_CUSTOM_MAIN_MODE_POSCTL,      0,                                      PX4FirmwarePlugin::_posCtlFlightMode,       true,   true,   true },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_LOITER,        PX4FirmwarePlugin::_holdFlightMode,         true,   true,   true },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_MISSION,       PX4FirmwarePlugin::_missionFlightMode,      true,   true,   true },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_RTL,           PX4FirmwarePlugin::_rtlFlightMode,          true,   true,   true },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_FOLLOW_TARGET, PX4FirmwarePlugin::_followMeFlightMode,     true,   true,   true },
    { PX4_CUSTOM_MAIN_MODE_OFFBOARD,    0,                                      PX4FirmwarePlugin::_offboardFlightMode,     true,   true,   true },
    // modes that can't be directly set by the user
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_LAND,          PX4FirmwarePlugin::_landingFlightMode,      false,  true,   true },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_READY,         PX4FirmwarePlugin::_readyFlightMode,        false,  true,   true },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_RTGS,          PX4FirmwarePlugin::_rtgsFlightMode,         false,  true,   true },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_TAKEOFF,       PX4FirmwarePlugin::_takeoffFlightMode,      false,  true,   true },
};

PX4FirmwarePlugin::PX4FirmwarePlugin(void)
    : _versionNotified(false)
{
    qmlRegisterType<PX4AdvancedFlightModesController>   ("QGroundControl.Controllers", 1, 0, "PX4AdvancedFlightModesController");
    qmlRegisterType<PX4SimpleFlightModesController>     ("QGroundControl.Controllers", 1, 0, "PX4SimpleFlightModesController");
    qmlRegisterType<AirframeComponentController>        ("QGroundControl.Controllers", 1, 0, "AirframeComponentController");
    qmlRegisterType<SensorsComponentController>         ("QGroundControl.Controllers", 1, 0, "SensorsComponentController");
    qmlRegisterType<PowerComponentController>           ("QGroundControl.Controllers", 1, 0, "PowerComponentController");
    qmlRegisterType<RadioComponentController>           ("QGroundControl.Controllers", 1, 0, "RadioComponentController");
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

    for (size_t i=0; i<sizeof(rgModes2Name)/sizeof(rgModes2Name[0]); i++) {
        const struct Modes2Name* pModes2Name = &rgModes2Name[i];

        if (pModes2Name->canBeSet) {
            bool fw = (vehicle->fixedWing() && pModes2Name->fixedWing);
            bool mc = (vehicle->multiRotor() && pModes2Name->multiRotor);

            // show all modes for generic, vtol, etc
            bool other = !vehicle->fixedWing() && !vehicle->multiRotor();

            if (fw || mc || other) {
                flightModes += pModes2Name->name;
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
        for (size_t i=0; i<sizeof(rgModes2Name)/sizeof(rgModes2Name[0]); i++) {
            const struct Modes2Name* pModes2Name = &rgModes2Name[i];

            if (pModes2Name->main_mode == px4_mode.main_mode && pModes2Name->sub_mode == px4_mode.sub_mode) {
                flightMode = pModes2Name->name;
                found = true;
                break;
            }
        }

        if (!found) {
            qWarning() << "Unknown flight mode" << custom_mode;
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
    for (size_t i=0; i<sizeof(rgModes2Name)/sizeof(rgModes2Name[0]); i++) {
        const struct Modes2Name* pModes2Name = &rgModes2Name[i];

        if (flightMode.compare(pModes2Name->name, Qt::CaseInsensitive) == 0) {
            union px4_custom_mode px4_mode;

            px4_mode.data = 0;
            px4_mode.main_mode = pModes2Name->main_mode;
            px4_mode.sub_mode = pModes2Name->sub_mode;

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

bool PX4FirmwarePlugin::supportsManualControl(void)
{
    return true;
}

bool PX4FirmwarePlugin::isCapable(const Vehicle *vehicle, FirmwareCapabilities capabilities)
{
    if (vehicle->multiRotor()) {
        return (capabilities & (MavCmdPreflightStorageCapability | GuidedModeCapability | SetFlightModeCapability | PauseVehicleCapability | OrbitModeCapability)) == capabilities;
    } else {
        return (capabilities & (MavCmdPreflightStorageCapability | GuidedModeCapability | SetFlightModeCapability | PauseVehicleCapability)) == capabilities;
    }
}

void PX4FirmwarePlugin::initializeVehicle(Vehicle* vehicle)
{
    Q_UNUSED(vehicle);

    // PX4 Flight Stack doesn't need to do any extra work
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
         << MAV_CMD_DO_MOUNT_CONFIGURE
         << MAV_CMD_DO_MOUNT_CONTROL;

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
        return QStringLiteral(":/json/APM/MavCmdInfoVTOL.json");
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
    vehicle->setFlightMode(_rtlFlightMode);
}

void PX4FirmwarePlugin::guidedModeLand(Vehicle* vehicle)
{
    vehicle->setFlightMode(_landingFlightMode);
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

void PX4FirmwarePlugin::guidedModeTakeoff(Vehicle* vehicle, double altitudeRel)
{
    Q_UNUSED(altitudeRel);
    if (qIsNaN(vehicle->altitudeAMSL()->rawValue().toDouble())) {
        qgcApp()->showMessage(tr("Unable to takeoff, vehicle position not known."));
        return;
    }

    vehicle->sendMavCommand(vehicle->defaultComponentId(),
                            MAV_CMD_NAV_TAKEOFF,
                            true,   // show error is fails
                            -1.0f,
                            0.0f,
                            0.0f,
                            NAN,
                            NAN,
                            NAN,
                            vehicle->altitudeAMSL()->rawValue().toDouble() + altitudeRel);
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

void PX4FirmwarePlugin::guidedModeChangeAltitude(Vehicle* vehicle, double altitudeRel)
{
    if (!vehicle->homePositionAvailable()) {
        qgcApp()->showMessage(tr("Unable to change altitude, home position unknown."));
        return;
    }
    if (qIsNaN(vehicle->homePosition().altitude())) {
        qgcApp()->showMessage(tr("Unable to change altitude, home position altitude unknown."));
        return;
    }

    vehicle->sendMavCommand(vehicle->defaultComponentId(),
                            MAV_CMD_DO_REPOSITION,
                            true,   // show error is fails
                            -1.0f,
                            MAV_DO_REPOSITION_FLAGS_CHANGE_MODE,
                            0.0f,
                            NAN,
                            NAN,
                            NAN,
                            vehicle->homePosition().altitude() + altitudeRel);
}

void PX4FirmwarePlugin::setGuidedMode(Vehicle* vehicle, bool guidedMode)
{
    if (guidedMode) {
        vehicle->setFlightMode(_holdFlightMode);
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

    if (!_versionNotified) {
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
            _versionNotified = true;
            qgcApp()->showMessage(QString("QGroundControl supports PX4 Pro firmware Version %1.%2.%3 and above. You are using a version prior to that which will lead to unpredictable results. Please upgrade your firmware.").arg(supportedMajorVersion).arg(supportedMinorVersion).arg(supportedPatchVersion));
        }
    }
}

QString PX4FirmwarePlugin::missionFlightMode(void)
{
    return QString(_missionFlightMode);
}

QString PX4FirmwarePlugin::rtlFlightMode(void)
{
    return QString(_rtlFlightMode);
}

QString PX4FirmwarePlugin::takeControlFlightMode(void)
{
    return QString(_manualFlightMode);
}
