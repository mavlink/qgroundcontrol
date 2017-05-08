/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FirmwarePlugin.h"
#include "QGCApplication.h"
#include "Generic/GenericAutoPilotPlugin.h"
#include "CameraMetaData.h"
#include "SettingsManager.h"
#include "AppSettings.h"

#include <QDebug>

static FirmwarePluginFactoryRegister* _instance = NULL;

const char* guided_mode_not_supported_by_vehicle = "Guided mode not supported by Vehicle.";

QVariantList FirmwarePlugin::_cameraList;

const char* FirmwarePlugin::px4FollowMeFlightMode = "Follow Me";

FirmwarePluginFactory::FirmwarePluginFactory(void)
{
    FirmwarePluginFactoryRegister::instance()->registerPluginFactory(this);
}

QList<MAV_TYPE> FirmwarePluginFactory::supportedVehicleTypes(void) const
{
    QList<MAV_TYPE> vehicleTypes;
    vehicleTypes << MAV_TYPE_FIXED_WING << MAV_TYPE_QUADROTOR << MAV_TYPE_VTOL_QUADROTOR << MAV_TYPE_GROUND_ROVER << MAV_TYPE_SUBMARINE;
    return vehicleTypes;
}

FirmwarePluginFactoryRegister* FirmwarePluginFactoryRegister::instance(void)
{
    if (!_instance) {
        _instance = new FirmwarePluginFactoryRegister;
    }

    return _instance;
}

AutoPilotPlugin* FirmwarePlugin::autopilotPlugin(Vehicle* vehicle)
{
    return new GenericAutoPilotPlugin(vehicle, vehicle);
}

bool FirmwarePlugin::isCapable(const Vehicle *vehicle, FirmwareCapabilities capabilities)
{
    Q_UNUSED(vehicle);
    Q_UNUSED(capabilities);
    return false;
}

QList<VehicleComponent*> FirmwarePlugin::componentsForVehicle(AutoPilotPlugin* vehicle)
{
    Q_UNUSED(vehicle);

    return QList<VehicleComponent*>();
}

QString FirmwarePlugin::flightMode(uint8_t base_mode, uint32_t custom_mode) const
{
    QString flightMode;

    struct Bit2Name {
        uint8_t     baseModeBit;
        const char* name;
    };
    static const struct Bit2Name rgBit2Name[] = {
    { MAV_MODE_FLAG_MANUAL_INPUT_ENABLED,   "Manual" },
    { MAV_MODE_FLAG_STABILIZE_ENABLED,      "Stabilize" },
    { MAV_MODE_FLAG_GUIDED_ENABLED,         "Guided" },
    { MAV_MODE_FLAG_AUTO_ENABLED,           "Auto" },
    { MAV_MODE_FLAG_TEST_ENABLED,           "Test" },
};

    Q_UNUSED(custom_mode);

    if (base_mode == 0) {
        flightMode = "PreFlight";
    } else if (base_mode & MAV_MODE_FLAG_CUSTOM_MODE_ENABLED) {
        flightMode = QString("Custom:0x%1").arg(custom_mode, 0, 16);
    } else {
        for (size_t i=0; i<sizeof(rgBit2Name)/sizeof(rgBit2Name[0]); i++) {
            if (base_mode & rgBit2Name[i].baseModeBit) {
                if (i != 0) {
                    flightMode += " ";
                }
                flightMode += rgBit2Name[i].name;
            }
        }
    }

    return flightMode;
}

bool FirmwarePlugin::setFlightMode(const QString& flightMode, uint8_t* base_mode, uint32_t* custom_mode)
{
    Q_UNUSED(flightMode);
    Q_UNUSED(base_mode);
    Q_UNUSED(custom_mode);

    qWarning() << "FirmwarePlugin::setFlightMode called on base class, not supported";

    return false;
}

int FirmwarePlugin::manualControlReservedButtonCount(void)
{
    // We don't know whether the firmware is going to used any of these buttons.
    // So reserve them all.
    return -1;
}

int FirmwarePlugin::defaultJoystickTXMode(void)
{
    return 2;
}

bool FirmwarePlugin::supportsThrottleModeCenterZero(void)
{
    // By default, this is supported
    return true;
}

bool FirmwarePlugin::supportsManualControl(void)
{
    return false;
}

bool FirmwarePlugin::supportsRadio(void)
{
    return true;
}

bool FirmwarePlugin::supportsCalibratePressure(void)
{
    return false;
}

bool FirmwarePlugin::supportsMotorInterference(void)
{
    return true;
}

bool FirmwarePlugin::supportsJSButton(void)
{
    return false;
}

bool FirmwarePlugin::adjustIncomingMavlinkMessage(Vehicle* vehicle, mavlink_message_t* message)
{
    Q_UNUSED(vehicle);
    Q_UNUSED(message);
    // Generic plugin does no message adjustment
    return true;
}

void FirmwarePlugin::adjustOutgoingMavlinkMessage(Vehicle* vehicle, LinkInterface* outgoingLink, mavlink_message_t* message)
{
    Q_UNUSED(vehicle);
    Q_UNUSED(outgoingLink);
    Q_UNUSED(message);
    // Generic plugin does no message adjustment
}

void FirmwarePlugin::initializeVehicle(Vehicle* vehicle)
{
    Q_UNUSED(vehicle);

    // Generic Flight Stack is by definition "generic", so no extra work
}

bool FirmwarePlugin::sendHomePositionToVehicle(void)
{
    // Generic stack does not want home position sent in the first position.
    // Subsequent sequence numbers must be adjusted.
    // This is the mavlink spec default.
    return false;
}

QList<MAV_CMD> FirmwarePlugin::supportedMissionCommands(void)
{
    // Generic supports all commands
    return QList<MAV_CMD>();
}

QString FirmwarePlugin::missionCommandOverrides(MAV_TYPE vehicleType) const
{
    switch (vehicleType) {
    case MAV_TYPE_GENERIC:
        return QStringLiteral(":/json/MavCmdInfoCommon.json");
        break;
    case MAV_TYPE_FIXED_WING:
        return QStringLiteral(":/json/MavCmdInfoFixedWing.json");
        break;
    case MAV_TYPE_QUADROTOR:
        return QStringLiteral(":/json/MavCmdInfoMultiRotor.json");
        break;
    case MAV_TYPE_VTOL_QUADROTOR:
        return QStringLiteral(":/json/MavCmdInfoVTOL.json");
        break;
    case MAV_TYPE_SUBMARINE:
        return QStringLiteral(":/json/MavCmdInfoSub.json");
        break;
    case MAV_TYPE_GROUND_ROVER:
        return QStringLiteral(":/json/MavCmdInfoRover.json");
        break;
    default:
        qWarning() << "FirmwarePlugin::missionCommandOverrides called with bad MAV_TYPE:" << vehicleType;
        return QString();
    }
}

void FirmwarePlugin::getParameterMetaDataVersionInfo(const QString& metaDataFile, int& majorVersion, int& minorVersion)
{
    Q_UNUSED(metaDataFile);
    majorVersion = -1;
    minorVersion = -1;
}

bool FirmwarePlugin::isGuidedMode(const Vehicle* vehicle) const
{
    // Not supported by generic vehicle
    Q_UNUSED(vehicle);
    return false;
}

void FirmwarePlugin::setGuidedMode(Vehicle* vehicle, bool guidedMode)
{
    Q_UNUSED(vehicle);
    Q_UNUSED(guidedMode);
    qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::pauseVehicle(Vehicle* vehicle)
{
    // Not supported by generic vehicle
    Q_UNUSED(vehicle);
    qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::guidedModeRTL(Vehicle* vehicle)
{
    // Not supported by generic vehicle
    Q_UNUSED(vehicle);
    qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::guidedModeLand(Vehicle* vehicle)
{
    // Not supported by generic vehicle
    Q_UNUSED(vehicle);
    qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::guidedModeTakeoff(Vehicle* vehicle)
{
    // Not supported by generic vehicle
    Q_UNUSED(vehicle);
    qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::guidedModeOrbit(Vehicle* /*vehicle*/, const QGeoCoordinate& /*centerCoord*/, double /*radius*/, double /*velocity*/, double /*altitude*/)
{
    // Not supported by generic vehicle
    qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::guidedModeGotoLocation(Vehicle* vehicle, const QGeoCoordinate& gotoCoord)
{
    // Not supported by generic vehicle
    Q_UNUSED(vehicle);
    Q_UNUSED(gotoCoord);
    qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::guidedModeChangeAltitude(Vehicle* vehicle, double altitudeRel)
{
    // Not supported by generic vehicle
    Q_UNUSED(vehicle);
    Q_UNUSED(altitudeRel);
    qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::startMission(Vehicle* vehicle)
{
    // Not supported by generic vehicle
    Q_UNUSED(vehicle);
    qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
}

const FirmwarePlugin::remapParamNameMajorVersionMap_t& FirmwarePlugin::paramNameRemapMajorVersionMap(void) const
{
    static const remapParamNameMajorVersionMap_t remap;

    return remap;
}

int FirmwarePlugin::remapParamNameHigestMinorVersionNumber(int majorVersionNumber) const
{
    Q_UNUSED(majorVersionNumber);
    return 0;
}

QString FirmwarePlugin::vehicleImageOpaque(const Vehicle* vehicle) const
{
    Q_UNUSED(vehicle);
    return QStringLiteral("/qmlimages/vehicleArrowOpaque.svg");
}

QString FirmwarePlugin::vehicleImageOutline(const Vehicle* vehicle) const
{
    Q_UNUSED(vehicle);
    return QStringLiteral("/qmlimages/vehicleArrowOutline.svg");
}

QString FirmwarePlugin::vehicleImageCompass(const Vehicle* vehicle) const
{
    Q_UNUSED(vehicle);
    return QStringLiteral("/qmlimages/compassInstrumentArrow.svg");
}

const QVariantList &FirmwarePlugin::toolBarIndicators(const Vehicle* vehicle)
{
    Q_UNUSED(vehicle);
    //-- Default list of indicators for all vehicles.
    if(_toolBarIndicatorList.size() == 0) {
        _toolBarIndicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/MessageIndicator.qml")));
        _toolBarIndicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/GPSIndicator.qml")));
        _toolBarIndicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/TelemetryRSSIIndicator.qml")));
        _toolBarIndicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/RCRSSIIndicator.qml")));
        _toolBarIndicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/BatteryIndicator.qml")));
        _toolBarIndicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/ModeIndicator.qml")));
        _toolBarIndicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/ArmedIndicator.qml")));
    }
    return _toolBarIndicatorList;
}

const QVariantList& FirmwarePlugin::cameraList(const Vehicle* vehicle)
{
    Q_UNUSED(vehicle);

    if (_cameraList.size() == 0) {
        CameraMetaData* metaData;

        metaData = new CameraMetaData(tr("Typhoon H CGO3+"),    // Camera name
                                      6.264,                    // sensorWidth
                                      4.698,                    // sensorHeight
                                      4000,                     // imageWidth
                                      3000,                     // imageHeight
                                      14,                       // focalLength
                                      true,                     // landscape orientation
                                      true,                     // camera orientation is fixed
                                      this);                    // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(tr("Sony ILCE-QX1"),  //http://www.sony.co.uk/electronics/interchangeable-lens-cameras/ilce-qx1-body-kit/specifications
                                      23.2,                 //http://www.sony.com/electronics/camera-lenses/sel16f28/specifications
                                      15.4,
                                      5456,
                                      3632,
                                      16,
                                      true,
                                      false,
                                      this);
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(tr("Canon S100 PowerShot"),
                                      7.6,
                                      5.7,
                                      4000,
                                      3000,
                                      5.2,
                                      true,
                                      false,
                                      this);
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(tr("Canon G9 X PowerShot"),
                                      13.2,
                                      8.8,
                                      5488,
                                      3680,
                                      10.2,
                                      true,
                                      false,
                                      this);
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(tr("Canon SX260 HS PowerShot"),
                                      6.17,
                                      4.55,
                                      4000,
                                      3000,
                                      4.5,
                                      true,
                                      false,
                                      this);

        metaData = new CameraMetaData(tr("Canon EOS-M 22mm"),
                                      22.3,
                                      14.9,
                                      5184,
                                      3456,
                                      22,
                                      true,
                                      false,
                                      this);
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(tr("Sony a6000 16mm"),    //http://www.sony.co.uk/electronics/interchangeable-lens-cameras/ilce-6000-body-kit#product_details_default
                                      23.5,
                                      15.6,
                                      6000,
                                      4000,
                                      16,
                                      true,
                                      false,
                                      this);
        _cameraList.append(QVariant::fromValue(metaData));
    }

    return _cameraList;
}

bool FirmwarePlugin::vehicleYawsToNextWaypointInMission(const Vehicle* vehicle) const
{
    return vehicle->multiRotor() ? false : true;
}

bool FirmwarePlugin::_armVehicleAndValidate(Vehicle* vehicle)
{
    if (!vehicle->armed()) {
        vehicle->setArmed(true);
    }

    // Wait for vehicle to return armed state for 2 seconds
    for (int i=0; i<20; i++) {
        if (vehicle->armed()) {
            break;
        }
        QGC::SLEEP::msleep(100);
        qgcApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
    }
    return vehicle->armed();
}

void FirmwarePlugin::batteryConsumptionData(Vehicle* vehicle, int& mAhBattery, double& hoverAmps, double& cruiseAmps) const
{
    Q_UNUSED(vehicle);
    mAhBattery = 0;
    hoverAmps = 0;
    cruiseAmps = 0;
}

QString FirmwarePlugin::autoDisarmParameter(Vehicle* vehicle)
{
    Q_UNUSED(vehicle);
    return QString();
}

bool FirmwarePlugin::hasGimbal(Vehicle* vehicle, bool& rollSupported, bool& pitchSupported, bool& yawSupported)
{
    Q_UNUSED(vehicle);
    rollSupported = false;
    pitchSupported = false;
    yawSupported = false;
    return false;
}
