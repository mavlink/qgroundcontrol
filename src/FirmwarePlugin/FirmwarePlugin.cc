/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
#include "QGCFileDownload.h"
#include "QGCCameraManager.h"

#include <QRegularExpression>
#include <QDebug>

QGC_LOGGING_CATEGORY(FirmwarePluginLog, "FirmwarePluginLog")

static FirmwarePluginFactoryRegister* _instance = nullptr;

const QString guided_mode_not_supported_by_vehicle = QObject::tr("Guided mode not supported by Vehicle.");

QVariantList FirmwarePlugin::_cameraList;

const QString FirmwarePlugin::px4FollowMeFlightMode(QObject::tr("Follow Me"));

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

int FirmwarePlugin::defaultJoystickTXMode(void)
{
    return 2;
}

bool FirmwarePlugin::supportsThrottleModeCenterZero(void)
{
    // By default, this is supported
    return true;
}

bool FirmwarePlugin::supportsNegativeThrust(Vehicle* /*vehicle*/)
{
    // By default, this is not supported
    return false;
}

bool FirmwarePlugin::supportsRadio(void)
{
    return true;
}

bool FirmwarePlugin::supportsMotorInterference(void)
{
    return true;
}

bool FirmwarePlugin::supportsJSButton(void)
{
    return false;
}

bool FirmwarePlugin::supportsTerrainFrame(void) const
{
    // Generic firmware supports this since we don't know
    return true;
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
    case MAV_TYPE_FIXED_WING:
        return QStringLiteral(":/json/MavCmdInfoFixedWing.json");
    case MAV_TYPE_QUADROTOR:
        return QStringLiteral(":/json/MavCmdInfoMultiRotor.json");
    case MAV_TYPE_VTOL_QUADROTOR:
        return QStringLiteral(":/json/MavCmdInfoVTOL.json");
    case MAV_TYPE_SUBMARINE:
        return QStringLiteral(":/json/MavCmdInfoSub.json");
    case MAV_TYPE_GROUND_ROVER:
        return QStringLiteral(":/json/MavCmdInfoRover.json");
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

void FirmwarePlugin::guidedModeRTL(Vehicle* vehicle, bool smartRTL)
{
    // Not supported by generic vehicle
    Q_UNUSED(vehicle);
    Q_UNUSED(smartRTL);
    qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::guidedModeLand(Vehicle* vehicle)
{
    // Not supported by generic vehicle
    Q_UNUSED(vehicle);
    qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::guidedModeTakeoff(Vehicle* vehicle, double takeoffAltRel)
{
    // Not supported by generic vehicle
    Q_UNUSED(vehicle);
    Q_UNUSED(takeoffAltRel);
    qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::guidedModeGotoLocation(Vehicle* vehicle, const QGeoCoordinate& gotoCoord)
{
    // Not supported by generic vehicle
    Q_UNUSED(vehicle);
    Q_UNUSED(gotoCoord);
    qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::guidedModeChangeAltitude(Vehicle*, double)
{
    // Not supported by generic vehicle
    qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::startMission(Vehicle*)
{
    // Not supported by generic vehicle
    qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
}

const FirmwarePlugin::remapParamNameMajorVersionMap_t& FirmwarePlugin::paramNameRemapMajorVersionMap(void) const
{
    static const remapParamNameMajorVersionMap_t remap;

    return remap;
}

int FirmwarePlugin::remapParamNameHigestMinorVersionNumber(int) const
{
    return 0;
}

QString FirmwarePlugin::vehicleImageOpaque(const Vehicle*) const
{
    return QStringLiteral("/qmlimages/vehicleArrowOpaque.svg");
}

QString FirmwarePlugin::vehicleImageOutline(const Vehicle*) const
{
    return QStringLiteral("/qmlimages/vehicleArrowOutline.svg");
}

QString FirmwarePlugin::vehicleImageCompass(const Vehicle*) const
{
    return QStringLiteral("/qmlimages/compassInstrumentArrow.svg");
}

const QVariantList &FirmwarePlugin::toolBarIndicators(const Vehicle*)
{
    //-- Default list of indicators for all vehicles.
    if(_toolBarIndicatorList.size() == 0) {
        _toolBarIndicatorList = QVariantList({
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/MessageIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/GPSIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/TelemetryRSSIIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/RCRSSIIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/BatteryIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/GPSRTKIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/ROIIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/ArmedIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/ModeIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/VTOLModeIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/MultiVehicleSelector.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/LinkIndicator.qml")),
        });
    }
    return _toolBarIndicatorList;
}

const QVariantList& FirmwarePlugin::cameraList(const Vehicle*)
{
    if (_cameraList.size() == 0) {
        CameraMetaData* metaData;

        metaData = new CameraMetaData(
            //tr("Canon S100 @ 5.2mm f/2"),
            tr("Canon S100 PowerShot"),
            7.6,                 // sensorWidth
            5.7,                 // sensorHeight
            4000,                // imageWidth
            3000,                // imageHeight
            5.2,                 // focalLength
            true,                // true: landscape orientation
            false,               // true: camera is fixed orientation
            0,                   // minimum trigger interval
            this);               // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
            //tr("Canon EOS-M 22mm f/2"),
            tr("Canon EOS-M 22mm"),
            22.3,                // sensorWidth
            14.9,                // sensorHeight
            5184,                // imageWidth
            3456,                // imageHeight
            22,                  // focalLength
            true,                // true: landscape orientation
            false,               // true: camera is fixed orientation
            0,                   // minimum trigger interval
            this);               // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
            //tr("Canon G9X @ 10.2mm f/2"),
            tr("Canon G9 X PowerShot"),
            13.2,                // sensorWidth
            8.8,                 // sensorHeight
            5488,                // imageWidth
            3680,                // imageHeight
            10.2,                // focalLength
            true,                // true: landscape orientation
            false,               // true: camera is fixed orientation
            0,                   // minimum trigger interval
            this);               // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
            //tr("Canon SX260 HS @ 4.5mm f/3.5"),
            tr("Canon SX260 HS PowerShot"),
            6.17,                // sensorWidth
            4.55,                // sensorHeight
            4000,                // imageWidth
            3000,                // imageHeight
            4.5,                 // focalLength
            true,                // true: landscape orientation
            false,               // true: camera is fixed orientation
            0,                   // minimum trigger interval
            this);               // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
            tr("GoPro Hero 4"),
            6.17,               // sensorWidth
            4.55,               // sendsorHeight
            4000,               // imageWidth
            3000,               // imageHeight
            2.98,               // focalLength
            true,               // landscape
            false,              // fixedOrientation
            0,                  // minTriggerInterval
            this);
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
            //tr("Parrot Sequoia RGB"),
            tr("Parrot Sequioa RGB"),
            6.17,               // sensorWidth
            4.63,               // sendsorHeight
            4608,               // imageWidth
            3456,               // imageHeight
            4.9,                // focalLength
            true,               // landscape
            false,              // fixedOrientation
            1,                  // minTriggerInterval
            this);
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
            //tr("Parrot Sequoia Monochrome"),
            tr("Parrot Sequioa Monochrome"),
            4.8,                // sensorWidth
            3.6,                // sendsorHeight
            1280,               // imageWidth
            960,                // imageHeight
            4.0,                // focalLength
            true,               // landscape
            false,              // fixedOrientation
            0.8,                // minTriggerInterval
            this);
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
            tr("RedEdge"),
            4.8,                // sensorWidth
            3.6,                // sendsorHeight
            1280,               // imageWidth
            960,                // imageHeight
            5.5,                // focalLength
            true,               // landscape
            false,              // fixedOrientation
            0,                  // minTriggerInterval
            this);
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
            //tr("Ricoh GR II 18.3mm f/2.8"),
            tr("Ricoh GR II"),
            23.7,               // sensorWidth
            15.7,               // sendsorHeight
            4928,               // imageWidth
            3264,               // imageHeight
            18.3,               // focalLength
            true,               // landscape
            false,              // fixedOrientation
            0,                  // minTriggerInterval
            this);
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
            tr("Sentera Double 4K Sensor"),
            6.2,                // sensorWidth
            4.65,               // sendsorHeight
            4000,               // imageWidth
            3000,               // imageHeight
            5.4,                // focalLength
            true,               // landscape
            false,              // fixedOrientation
            0,                  // minTriggerInterval
            this);
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
            tr("Sentera NDVI Single Sensor"),
            4.68,               // sensorWidth
            3.56,               // sendsorHeight
            1248,               // imageWidth
            952,                // imageHeight
            4.14,               // focalLength
            true,               // landscape
            false,              // fixedOrientation
            0,                  // minTriggerInterval
            this);
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
            //-- http://www.sony.co.uk/electronics/interchangeable-lens-cameras/ilce-6000-body-kit#product_details_default
            //tr("Sony a6000 Sony 16mm f/2.8"),
            tr("Sony a6000 16mm"),
            23.5,               // sensorWidth
            15.6,               // sensorHeight
            6000,               // imageWidth
            4000,               // imageHeight
            16,                 // focalLength
            true,               // true: landscape orientation
            false,              // true: camera is fixed orientation
            2.0,                // minimum trigger interval
            this);              // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
            tr("Sony a6300 Zeiss 21mm f/2.8"),
            23.5,               // sensorWidth
            15.6,               // sensorHeight
            6000,               // imageWidth
            4000,               // imageHeight
            21,                 // focalLength
            true,               // true: landscape orientation
            true,               // true: camera is fixed orientation
            2.0,                // minimum trigger interval
            this);              // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
            tr("Sony a6300 Sony 28mm f/2.0"),
            23.5,               // sensorWidth
            15.6,               // sensorHeight
            6000,               // imageWidth
            4000,               // imageHeight
            28,                 // focalLength
            true,               // true: landscape orientation
            true,               // true: camera is fixed orientation
            2.0,                // minimum trigger interval
            this);              // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
            tr("Sony a7R II Zeiss 21mm f/2.8"),
            35.814,             // sensorWidth
            23.876,             // sensorHeight
            7952,               // imageWidth
            5304,               // imageHeight
            21,                 // focalLength
            true,               // true: landscape orientation
            true,               // true: camera is fixed orientation
            2.0,                // minimum trigger interval
            this);              // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
            tr("Sony a7R II Sony 28mm f/2.0"),
            35.814,             // sensorWidth
            23.876,             // sensorHeight
            7952,               // imageWidth
            5304,               // imageHeight
            28,                 // focalLength
            true,               // true: landscape orientation
            true,               // true: camera is fixed orientation
            2.0,                // minimum trigger interval
            this);              // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
            tr("Sony DSC-QX30U @ 4.3mm f/3.5"),
            7.82,               // sensorWidth
            5.865,              // sensorHeight
            5184,               // imageWidth
            3888,               // imageHeight
            4.3,                // focalLength
            true,               // true: landscape orientation
            false,              // true: camera is fixed orientation
            2.0,                // minimum trigger interval
            this);              // parent
        _cameraList.append(QVariant::fromValue(metaData));
        
        metaData = new CameraMetaData(
            tr("Sony DSC-RX0"),
            13.2,               // sensorWidth
            8.8,                // sensorHeight
            4800,               // imageWidth
            3200,               // imageHeight
            7.7,                // focalLength
            true,               // true: landscape orientation
            false,              // true: camera is fixed orientation
            0,                  // minimum trigger interval
            this);              // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
            //-- http://www.sony.co.uk/electronics/interchangeable-lens-cameras/ilce-qx1-body-kit/specifications
            //-- http://www.sony.com/electronics/camera-lenses/sel16f28/specifications
            //tr("Sony ILCE-QX1 Sony 16mm f/2.8"),
            tr("Sony ILCE-QX1"),
            23.2,                // sensorWidth
            15.4,                // sensorHeight
            5456,                // imageWidth
            3632,                // imageHeight
            16,                  // focalLength
            true,                // true: landscape orientation
            false,               // true: camera is fixed orientation
            0,                   // minimum trigger interval
            this);               // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
            //-- http://www.sony.co.uk/electronics/interchangeable-lens-cameras/ilce-qx1-body-kit/specifications
            //tr("Sony NEX-5R Sony 20mm f/2.8"),
            tr("Sony NEX-5R 20mm"),
            23.2,                // sensorWidth
            15.4,                // sensorHeight
            4912,                // imageWidth
            3264,                // imageHeight
            20,                  // focalLength
            true,                // true: landscape orientation
            false,               // true: camera is fixed orientation
            1,                   // minimum trigger interval
            this);               // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
            //tr("Sony RX100 II @ 10.4mm f/1.8"),
            tr("Sony RX100 II 28mm"),
            13.2,                // sensorWidth
            8.8,                 // sensorHeight
            5472,                // imageWidth
            3648,                // imageHeight
            10.4,                // focalLength
            true,                // true: landscape orientation
            false,               // true: camera is fixed orientation
            0,                   // minimum trigger interval
            this);               // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
            tr("Yuneec CGOET"),
            5.6405,             // sensorWidth
            3.1813,             // sensorHeight
            1920,               // imageWidth
            1080,               // imageHeight
            3.5,                // focalLength
            true,               // true: landscape orientation
            true,               // true: camera is fixed orientation
            1.3,                // minimum trigger interval
            this);              // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
            tr("Yuneec E10T"),
            5.6405,             // sensorWidth
            3.1813,             // sensorHeight
            1920,               // imageWidth
            1080,               // imageHeight
            23,                 // focalLength
            true,               // true: landscape orientation
            true,               // true: camera is fixed orientation
            1.3,                // minimum trigger interval
            this);              // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
            tr("Yuneec E50"),
            6.2372,             // sensorWidth
            4.7058,             // sensorHeight
            4000,               // imageWidth
            3000,               // imageHeight
            7.2,                // focalLength
            true,               // true: landscape orientation
            true,               // true: camera is fixed orientation
            1.3,                // minimum trigger interval
            this);              // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
            tr("Yuneec E90"),
            13.3056,            // sensorWidth
            8.656,              // sensorHeight
            5472,               // imageWidth
            3648,               // imageHeight
            8.29,               // focalLength
            true,               // true: landscape orientation
            true,               // true: camera is fixed orientation
            1.3,                // minimum trigger interval
            this);              // parent
        _cameraList.append(QVariant::fromValue(metaData));

    }

    return _cameraList;
}

QMap<QString, FactGroup*>* FirmwarePlugin::factGroups(void) {
    // Generic plugin has no FactGroups
    return nullptr;
}

bool FirmwarePlugin::vehicleYawsToNextWaypointInMission(const Vehicle* vehicle) const
{
    return vehicle->multiRotor() ? false : true;
}

bool FirmwarePlugin::_armVehicleAndValidate(Vehicle* vehicle)
{
    if (vehicle->armed()) {
        return true;
    }

    bool armedChanged = false;

    // We try arming 3 times
    for (int retries=0; retries<3; retries++) {
        vehicle->setArmed(true);

        // Wait for vehicle to return armed state for 3 seconds
        for (int i=0; i<30; i++) {
            if (vehicle->armed()) {
                armedChanged = true;
                break;
            }
            QGC::SLEEP::msleep(100);
            qgcApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
        }
        if (armedChanged) {
            break;
        }
    }

    return armedChanged;
}

bool FirmwarePlugin::_setFlightModeAndValidate(Vehicle* vehicle, const QString& flightMode)
{
    if (vehicle->flightMode() == flightMode) {
        return true;
    }

    bool flightModeChanged = false;

    // We try 3 times
    for (int retries=0; retries<3; retries++) {
        vehicle->setFlightMode(flightMode);

        // Wait for vehicle to return flight mode
        for (int i=0; i<13; i++) {
            if (vehicle->flightMode() == flightMode) {
                flightModeChanged = true;
                break;
            }
            QGC::SLEEP::msleep(100);
            qgcApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
        }
        if (flightModeChanged) {
            break;
        }
    }

    return flightModeChanged;
}


void FirmwarePlugin::batteryConsumptionData(Vehicle* vehicle, int& mAhBattery, double& hoverAmps, double& cruiseAmps) const
{
    Q_UNUSED(vehicle);
    mAhBattery  = 0;
    hoverAmps   = 0;
    cruiseAmps  = 0;
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

bool FirmwarePlugin::isVtol(const Vehicle* vehicle) const
{
    switch (vehicle->vehicleType()) {
    case MAV_TYPE_VTOL_DUOROTOR:
    case MAV_TYPE_VTOL_QUADROTOR:
    case MAV_TYPE_VTOL_TILTROTOR:
    case MAV_TYPE_VTOL_RESERVED2:
    case MAV_TYPE_VTOL_RESERVED3:
    case MAV_TYPE_VTOL_RESERVED4:
    case MAV_TYPE_VTOL_RESERVED5:
        return true;
    default:
        return false;
    }
}

QGCCameraManager* FirmwarePlugin::createCameraManager(Vehicle* vehicle)
{
    return new QGCCameraManager(vehicle);
}

QGCCameraControl* FirmwarePlugin::createCameraControl(const mavlink_camera_information_t *info, Vehicle *vehicle, int compID, QObject* parent)
{
    return new QGCCameraControl(info, vehicle, compID, parent);
}

uint32_t FirmwarePlugin::highLatencyCustomModeTo32Bits(uint16_t hlCustomMode)
{
    // Standard implementation assumes no special handling. Upper part of 32 bit value is not used.
    return hlCustomMode;
}

void FirmwarePlugin::checkIfIsLatestStable(Vehicle* vehicle)
{
    // This is required as mocklink uses a hardcoded firmware version
    if (qgcApp()->runningUnitTests()) {
        qCDebug(FirmwarePluginLog) << "Skipping version check";
        return;
    }
    QString versionFile = _getLatestVersionFileUrl(vehicle);
    qCDebug(FirmwarePluginLog) << "Downloading" << versionFile;
    QGCFileDownload* downloader = new QGCFileDownload(this);
    connect(
        downloader,
        &QGCFileDownload::downloadFinished,
        this,
        [vehicle, this](QString remoteFile, QString localFile) {
            _versionFileDownloadFinished(remoteFile, localFile, vehicle);
            sender()->deleteLater();
        });
    downloader->download(versionFile);
}

void FirmwarePlugin::_versionFileDownloadFinished(QString& remoteFile, QString& localFile, Vehicle* vehicle)
{
    qCDebug(FirmwarePluginLog) << "Download complete" << remoteFile << localFile;
    // Now read the version file and pull out the version string
    QFile versionFile(localFile);
    if (!versionFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(FirmwarePluginLog) << "Error opening downloaded version file.";
        return;
    }

    QTextStream stream(&versionFile);
    QString versionFileContents = stream.readAll();
    QString version;
    QRegularExpressionMatch match = QRegularExpression(_versionRegex()).match(versionFileContents);

    qCDebug(FirmwarePluginLog) << "Looking for version number...";

    if (match.hasMatch()) {
        version = match.captured(1);
    } else {
        qCWarning(FirmwarePluginLog) << "Unable to parse version info from file" << remoteFile;
        return;
    }

    qCDebug(FirmwarePluginLog) << "Latest stable version = "  << version;

    int currType = vehicle->firmwareVersionType();

    // Check if lower version than stable or same version but different type
    if (currType == FIRMWARE_VERSION_TYPE_OFFICIAL && vehicle->versionCompare(version) < 0) {
        QString currentVersionNumber = QString("%1.%2.%3").arg(vehicle->firmwareMajorVersion())
                .arg(vehicle->firmwareMinorVersion())
                .arg(vehicle->firmwarePatchVersion());
        qgcApp()->showMessage(tr("Vehicle is not running latest stable firmware! Running %1, latest stable is %2.").arg(currentVersionNumber, version));
    }
}

int FirmwarePlugin::versionCompare(Vehicle* vehicle, int major, int minor, int patch)
{
    int currMajor = vehicle->firmwareMajorVersion();
    int currMinor = vehicle->firmwareMinorVersion();
    int currPatch = vehicle->firmwarePatchVersion();

    if (currMajor == major && currMinor == minor && currPatch == patch) {
        return 0;
    }

    if (currMajor > major
       || (currMajor == major && currMinor > minor)
       || (currMajor == major && currMinor == minor && currPatch > patch))
    {
        return 1;
    }
    return -1;
}

int FirmwarePlugin::versionCompare(Vehicle* vehicle, QString& compare)
{
    QStringList versionNumbers = compare.split(".");
    if(versionNumbers.size() != 3) {
        qCWarning(FirmwarePluginLog) << "Error parsing version number: wrong format";
        return -1;
    }
    int major = versionNumbers[0].toInt();
    int minor = versionNumbers[1].toInt();
    int patch = versionNumbers[2].toInt();
    return versionCompare(vehicle, major, minor, patch);
}

QString FirmwarePlugin::gotoFlightMode(void) const
{
    return QString();
}

void FirmwarePlugin::sendGCSMotionReport(Vehicle* vehicle, FollowMe::GCSMotionReport& motionReport, uint8_t estimationCapabilities)
{
    MAVLinkProtocol* mavlinkProtocol = qgcApp()->toolbox()->mavlinkProtocol();

    mavlink_follow_target_t follow_target = {};

    follow_target.timestamp =           qgcApp()->msecsSinceBoot();
    follow_target.est_capabilities =    estimationCapabilities;
    follow_target.position_cov[0] =     static_cast<float>(motionReport.pos_std_dev[0]);
    follow_target.position_cov[2] =     static_cast<float>(motionReport.pos_std_dev[2]);
    follow_target.alt =                 static_cast<float>(motionReport.altMetersAMSL);
    follow_target.lat =                 motionReport.lat_int;
    follow_target.lon =                 motionReport.lon_int;
    follow_target.vel[0] =              static_cast<float>(motionReport.vxMetersPerSec);
    follow_target.vel[1] =              static_cast<float>(motionReport.vyMetersPerSec);

    mavlink_message_t message;
    mavlink_msg_follow_target_encode_chan(static_cast<uint8_t>(mavlinkProtocol->getSystemId()),
                                          static_cast<uint8_t>(mavlinkProtocol->getComponentId()),
                                          vehicle->priorityLink()->mavlinkChannel(),
                                          &message,
                                          &follow_target);
    vehicle->sendMessageOnLink(vehicle->priorityLink(), message);
}
