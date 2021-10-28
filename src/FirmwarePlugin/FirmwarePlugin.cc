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
#include "RadioComponentController.h"
#include "Autotune.h"

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

QList<QGCMAVLink::VehicleClass_t> FirmwarePluginFactory::supportedVehicleClasses(void) const
{
    return QGCMAVLink::allVehicleClasses();
}

FirmwarePluginFactoryRegister* FirmwarePluginFactoryRegister::instance(void)
{
    if (!_instance) {
        _instance = new FirmwarePluginFactoryRegister;
    }

    return _instance;
}


FirmwarePlugin::FirmwarePlugin(void)
{
    qmlRegisterType<RadioComponentController>       ("QGroundControl.Controllers",                       1, 0, "RadioComponentController");
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

bool FirmwarePlugin::adjustIncomingMavlinkMessage(Vehicle* vehicle, mavlink_message_t* message)
{
    Q_UNUSED(vehicle);
    Q_UNUSED(message);
    // Generic plugin does no message adjustment
    return true;
}

void FirmwarePlugin::adjustOutgoingMavlinkMessageThreadSafe(Vehicle* /*vehicle*/, LinkInterface* /*outgoingLink*/, mavlink_message_t* /*message*/)
{
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

QList<MAV_CMD> FirmwarePlugin::supportedMissionCommands(QGCMAVLink::VehicleClass_t /* vehicleClass */)
{
    // Generic supports all commands
    return QList<MAV_CMD>();
}

QString FirmwarePlugin::missionCommandOverrides(QGCMAVLink::VehicleClass_t vehicleClass) const
{
    switch (vehicleClass) {
    case QGCMAVLink::VehicleClassGeneric:
        return QStringLiteral(":/json/MavCmdInfoCommon.json");
    case QGCMAVLink::VehicleClassFixedWing:
        return QStringLiteral(":/json/MavCmdInfoFixedWing.json");
    case QGCMAVLink::VehicleClassMultiRotor:
        return QStringLiteral(":/json/MavCmdInfoMultiRotor.json");
    case QGCMAVLink::VehicleClassVTOL:
        return QStringLiteral(":/json/MavCmdInfoVTOL.json");
    case QGCMAVLink::VehicleClassSub:
        return QStringLiteral(":/json/MavCmdInfoSub.json");
    case QGCMAVLink::VehicleClassRoverBoat:
        return QStringLiteral(":/json/MavCmdInfoRover.json");
    default:
        qWarning() << "FirmwarePlugin::missionCommandOverrides called with bad VehicleClass_t:" << vehicleClass;
        return QString();
    }
}

void FirmwarePlugin::_getParameterMetaDataVersionInfo(const QString& metaDataFile, int& majorVersion, int& minorVersion)
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
    qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::pauseVehicle(Vehicle* vehicle)
{
    // Not supported by generic vehicle
    Q_UNUSED(vehicle);
    qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::guidedModeRTL(Vehicle* vehicle, bool smartRTL)
{
    // Not supported by generic vehicle
    Q_UNUSED(vehicle);
    Q_UNUSED(smartRTL);
    qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::guidedModeLand(Vehicle* vehicle)
{
    // Not supported by generic vehicle
    Q_UNUSED(vehicle);
    qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::guidedModeTakeoff(Vehicle* vehicle, double takeoffAltRel)
{
    // Not supported by generic vehicle
    Q_UNUSED(vehicle);
    Q_UNUSED(takeoffAltRel);
    qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::guidedModeGotoLocation(Vehicle* vehicle, const QGeoCoordinate& gotoCoord)
{
    // Not supported by generic vehicle
    Q_UNUSED(vehicle);
    Q_UNUSED(gotoCoord);
    qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::guidedModeChangeAltitude(Vehicle*, double, bool pauseVehicle)
{
    // Not supported by generic vehicle
    Q_UNUSED(pauseVehicle);
    qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::startMission(Vehicle*)
{
    // Not supported by generic vehicle
    qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
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

const QVariantList& FirmwarePlugin::toolIndicators(const Vehicle*)
{
    //-- Default list of indicators for all vehicles.
    if(_toolIndicatorList.size() == 0) {
        _toolIndicatorList = QVariantList({
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/MessageIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/GPSIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/TelemetryRSSIIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/RCRSSIIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/BatteryIndicator.qml")),
        });
    }
    return _toolIndicatorList;
}

const QVariantList& FirmwarePlugin::modeIndicators(const Vehicle*)
{
    //-- Default list of indicators for all vehicles.
    if(_modeIndicatorList.size() == 0) {
        _modeIndicatorList = QVariantList({
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/ROIIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/MultiVehicleSelector.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/LinkIndicator.qml")),
        });
    }
    return _modeIndicatorList;
}

const QVariantList& FirmwarePlugin::cameraList(const Vehicle*)
{
    if (_cameraList.size() == 0) {
        CameraMetaData* metaData;

        metaData = new CameraMetaData(
                    // Canon S100 @ 5.2mm f/2
                    "Canon S100 PowerShot",     // canonical name saved in plan file
                    tr("Canon"),                // brand
                    tr("S100 PowerShot"),       // model
                    7.6,                        // sensorWidth
                    5.7,                        // sensorHeight
                    4000,                       // imageWidth
                    3000,                       // imageHeight
                    5.2,                        // focalLength
                    true,                       // true: landscape orientation
                    false,                      // true: camera is fixed orientation
                    0,                          // minimum trigger interval
                    tr("Canon S100 PowerShot"), // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);                      // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    //tr("Canon EOS-M 22mm f/2"),
                    "Canon EOS-M 22mm",
                    tr("Canon"),
                    tr("EOS-M 22mm"),
                    22.3,                   // sensorWidth
                    14.9,                   // sensorHeight
                    5184,                   // imageWidth
                    3456,                   // imageHeight
                    22,                     // focalLength
                    true,                   // true: landscape orientation
                    false,                  // true: camera is fixed orientation
                    0,                      // minimum trigger interval
                    tr("Canon EOS-M 22mm"), // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);                  // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    // Canon G9X @ 10.2mm f/2
                    "Canon G9 X PowerShot",
                    tr("Canon"),
                    tr("G9 X PowerShot"),
                    13.2,                       // sensorWidth
                    8.8,                        // sensorHeight
                    5488,                       // imageWidth
                    3680,                       // imageHeight
                    10.2,                       // focalLength
                    true,                       // true: landscape orientation
                    false,                      // true: camera is fixed orientation
                    0,                          // minimum trigger interval
                    tr("Canon G9 X PowerShot"), // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);                      // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    // Canon SX260 HS @ 4.5mm f/3.5
                    "Canon SX260 HS PowerShot",
                    tr("Canon"),
                    tr("SX260 HS PowerShot"),
                    6.17,                           // sensorWidth
                    4.55,                           // sensorHeight
                    4000,                           // imageWidth
                    3000,                           // imageHeight
                    4.5,                            // focalLength
                    true,                           // true: landscape orientation
                    false,                          // true: camera is fixed orientation
                    0,                              // minimum trigger interval
                    tr("Canon SX260 HS PowerShot"), // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);                          // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "GoPro Hero 4",
                    tr("GoPro"),
                    tr("Hero 4"),
                    6.17,               // sensorWidth
                    4.55,               // sendsorHeight
                    4000,               // imageWidth
                    3000,               // imageHeight
                    2.98,               // focalLength
                    true,               // landscape
                    false,              // fixedOrientation
                    0,                  // minTriggerInterval
                    tr("GoPro Hero 4"), // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Parrot Sequioa RGB",
                    tr("Parrot"),
                    tr("Sequioa RGB"),
                    6.17,                       // sensorWidth
                    4.63,                       // sendsorHeight
                    4608,                       // imageWidth
                    3456,                       // imageHeight
                    4.9,                        // focalLength
                    true,                       // landscape
                    false,                      // fixedOrientation
                    1,                          // minTriggerInterval
                    tr("Parrot Sequioa RGB"),   // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Parrot Sequioa Monochrome",
                    tr("Parrot"),
                    tr("Sequioa Monochrome"),
                    4.8,                                // sensorWidth
                    3.6,                                // sendsorHeight
                    1280,                               // imageWidth
                    960,                                // imageHeight
                    4.0,                                // focalLength
                    true,                               // landscape
                    false,                              // fixedOrientation
                    0.8,                                // minTriggerInterval
                    tr("Parrot Sequioa Monochrome"),    // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "RedEdge",
                    tr("RedEdge"),
                    tr("RedEdge"),
                    4.8,            // sensorWidth
                    3.6,            // sendsorHeight
                    1280,           // imageWidth
                    960,            // imageHeight
                    5.5,            // focalLength
                    true,           // landscape
                    false,          // fixedOrientation
                    0,              // minTriggerInterval
                    tr("RedEdge"),  // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    // Ricoh GR II 18.3mm f/2.8
                    "Ricoh GR II",
                    tr("Ricoh"),
                    tr("GR II"),
                    23.7,               // sensorWidth
                    15.7,               // sendsorHeight
                    4928,               // imageWidth
                    3264,               // imageHeight
                    18.3,               // focalLength
                    true,               // landscape
                    false,              // fixedOrientation
                    0,                  // minTriggerInterval
                    tr("Ricoh GR II"),  // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Sentera Double 4K Sensor",
                    tr("Sentera"),
                    tr("Double 4K Sensor"),
                    6.2,                // sensorWidth
                    4.65,               // sendsorHeight
                    4000,               // imageWidth
                    3000,               // imageHeight
                    5.4,                // focalLength
                    true,               // landscape
                    false,              // fixedOrientation
                    0.8,                // minTriggerInterval
                    tr("Sentera Double 4K Sensor"),// SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Sentera NDVI Single Sensor",
                    tr("Sentera"),
                    tr("NDVI Single Sensor"),
                    4.68,               // sensorWidth
                    3.56,               // sendsorHeight
                    1248,               // imageWidth
                    952,                // imageHeight
                    4.14,               // focalLength
                    true,               // landscape
                    false,              // fixedOrientation
                    0.5,                // minTriggerInterval
                    tr("Sentera NDVI Single Sensor"),// SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Sentera 6X Sensor",
                    tr("Sentera"),
                    tr("6X Sensor"),
                    6.57,               // sensorWidth
                    4.93,               // sendsorHeight
                    1904,               // imageWidth
                    1428,               // imageHeight
                    8.0,                // focalLength
                    true,               // true: landscape orientation
                    false,              // true: camera is fixed orientation
                    0.2,                // minimum trigger interval
                    tr(""),             // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);              // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    //-- http://www.sony.co.uk/electronics/interchangeable-lens-cameras/ilce-6000-body-kit#product_details_default
                    // Sony a6000 Sony 16mm f/2.8"
                    "Sony a6000 16mm",
                    tr("Sony"),
                    tr("a6000 16mm"),
                    23.5,                   // sensorWidth
                    15.6,                   // sensorHeight
                    6000,                   // imageWidth
                    4000,                   // imageHeight
                    16,                     // focalLength
                    true,                   // true: landscape orientation
                    false,                  // true: camera is fixed orientation
                    1.0,                    // minimum trigger interval
                    tr("Sony a6000 16mm"),  // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);                  // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Sony a6000 35mm",
                    tr("Sony"),
                    tr("a6000 35mm"),
                    23.5,               // sensorWidth
                    15.6,               // sensorHeight
                    6000,               // imageWidth
                    4000,               // imageHeight
                    35,                 // focalLength
                    true,               // true: landscape orientation
                    false,              // true: camera is fixed orientation
                    1.0,                // minimum trigger interval
                    "",
                    this);              // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Sony a6300 Zeiss 21mm f/2.8",
                    tr("Sony"),
                    tr("a6300 Zeiss 21mm f/2.8"),
                    23.5,               // sensorWidth
                    15.6,               // sensorHeight
                    6000,               // imageWidth
                    4000,               // imageHeight
                    21,                 // focalLength
                    true,               // true: landscape orientation
                    false,              // true: camera is fixed orientation
                    1.0,                // minimum trigger interval
                    tr("Sony a6300 Zeiss 21mm f/2.8"),// SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);              // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Sony a6300 Sony 28mm f/2.0",
                    tr("Sony"),
                    tr("a6300 Sony 28mm f/2.0"),
                    23.5,                               // sensorWidth
                    15.6,                               // sensorHeight
                    6000,                               // imageWidth
                    4000,                               // imageHeight
                    28,                                 // focalLength
                    true,                               // true: landscape orientation
                    false,              // true: camera is fixed orientation
                    1.0,                                // minimum trigger interval
                    tr("Sony a6300 Sony 28mm f/2.0"),   // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);                              // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Sony a7R II Zeiss 21mm f/2.8",
                    tr("Sony"),
                    tr("a7R II Zeiss 21mm f/2.8"),
                    35.814,                             // sensorWidth
                    23.876,                             // sensorHeight
                    7952,                               // imageWidth
                    5304,                               // imageHeight
                    21,                                 // focalLength
                    true,                               // true: landscape orientation
                    true,                               // true: camera is fixed orientation
                    1.0,                                // minimum trigger interval
                    tr("Sony a7R II Zeiss 21mm f/2.8"), // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);                              // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Sony a7R II Sony 28mm f/2.0",
                    tr("Sony"),
                    tr("a7R II Sony 28mm f/2.0"),
                    35.814,             // sensorWidth
                    23.876,             // sensorHeight
                    7952,               // imageWidth
                    5304,               // imageHeight
                    28,                 // focalLength
                    true,               // true: landscape orientation
                    true,               // true: camera is fixed orientation
                    1.0,                // minimum trigger interval
                    tr("Sony a7R II Sony 28mm f/2.0"),// SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);              // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Sony a7r III 35mm",
                    tr("Sony"),
                    tr("a7r III 35mm"),
                    35.9,               // sensorWidth
                    24.0,               // sensorHeight
                    7952,               // imageWidth
                    5304,               // imageHeight
                    35,                 // focalLength
                    true,               // true: landscape orientation
                    false,              // true: camera is fixed orientation
                    1.0,                // minimum trigger interval
                    "",
                    this);              // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Sony a7r IV 35mm",
                    tr("Sony"),
                    tr("a7r IV 35mm"),
                    35.7,               // sensorWidth
                    23.8,               // sensorHeight
                    9504,               // imageWidth
                    6336,               // imageHeight
                    35,                 // focalLength
                    true,               // true: landscape orientation
                    false,               // true: camera is fixed orientation
                    1.0,                // minimum trigger interval
                    "",
                    this);              // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Sony DSC-QX30U @ 4.3mm f/3.5",
                    tr("Sony"),
                    tr("DSC-QX30U @ 4.3mm f/3.5"),
                    7.82,                               // sensorWidth
                    5.865,                              // sensorHeight
                    5184,                               // imageWidth
                    3888,                               // imageHeight
                    4.3,                                // focalLength
                    true,                               // true: landscape orientation
                    false,                              // true: camera is fixed orientation
                    2.0,                                // minimum trigger interval
                    tr("Sony DSC-QX30U @ 4.3mm f/3.5"), // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);                              // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Sony DSC-RX0",
                    tr("Sony"),
                    tr("DSC-RX0"),
                    13.2,               // sensorWidth
                    8.8,                // sensorHeight
                    4800,               // imageWidth
                    3200,               // imageHeight
                    7.7,                // focalLength
                    true,               // true: landscape orientation
                    false,              // true: camera is fixed orientation
                    0,                  // minimum trigger interval
                    tr("Sony DSC-RX0"),// SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);              // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Sony DSC-RX1R II 35mm",
                    tr("Sony"),
                    tr("DSC-RX1R II 35mm"),
                    35.9,             // sensorWidth
                    24.0,             // sensorHeight
                    7952,               // imageWidth
                    5304,               // imageHeight
                    35,                 // focalLength
                    true,               // true: landscape orientation
                    false,              // true: camera is fixed orientation
                    1.0,                // minimum trigger interval
                    "",
                    this);              // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    //-- http://www.sony.co.uk/electronics/interchangeable-lens-cameras/ilce-qx1-body-kit/specifications
                    //-- http://www.sony.com/electronics/camera-lenses/sel16f28/specifications
                    //tr("Sony ILCE-QX1 Sony 16mm f/2.8"),
                    "Sony ILCE-QX1",
                    tr("Sony"),
                    tr("ILCE-QX1"),
                    23.2,                   // sensorWidth
                    15.4,                   // sensorHeight
                    5456,                   // imageWidth
                    3632,                   // imageHeight
                    16,                     // focalLength
                    true,                   // true: landscape orientation
                    false,                  // true: camera is fixed orientation
                    0,                      // minimum trigger interval
                    tr("Sony ILCE-QX1"),    // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);                  // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    //-- http://www.sony.co.uk/electronics/interchangeable-lens-cameras/ilce-qx1-body-kit/specifications
                    // Sony NEX-5R Sony 20mm f/2.8"
                    "Sony NEX-5R 20mm",
                    tr("Sony"),
                    tr("NEX-5R 20mm"),
                    23.2,                   // sensorWidth
                    15.4,                   // sensorHeight
                    4912,                   // imageWidth
                    3264,                   // imageHeight
                    20,                     // focalLength
                    true,                   // true: landscape orientation
                    false,                  // true: camera is fixed orientation
                    1,                      // minimum trigger interval
                    tr("Sony NEX-5R 20mm"), // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);                  // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    // Sony RX100 II @ 10.4mm f/1.8
                    "Sony RX100 II 28mm",
                    tr("Sony"),
                    tr("RX100 II 28mm"),
                    13.2,                // sensorWidth
                    8.8,                 // sensorHeight
                    5472,                // imageWidth
                    3648,                // imageHeight
                    10.4,                // focalLength
                    true,                // true: landscape orientation
                    false,               // true: camera is fixed orientation
                    0,                   // minimum trigger interval
                    tr("Sony RX100 II 28mm"),// SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);               // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Yuneec CGOET",
                    tr("Yuneec"),
                    tr("CGOET"),
                    5.6405,             // sensorWidth
                    3.1813,             // sensorHeight
                    1920,               // imageWidth
                    1080,               // imageHeight
                    3.5,                // focalLength
                    true,               // true: landscape orientation
                    true,               // true: camera is fixed orientation
                    1.3,                // minimum trigger interval
                    tr("Yuneec CGOET"), // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);              // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Yuneec E10T",
                    tr("Yuneec"),
                    tr("E10T"),
                    5.6405,             // sensorWidth
                    3.1813,             // sensorHeight
                    1920,               // imageWidth
                    1080,               // imageHeight
                    23,                 // focalLength
                    true,               // true: landscape orientation
                    true,               // true: camera is fixed orientation
                    1.3,                // minimum trigger interval
                    tr("Yuneec E10T"),  // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);              // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Yuneec E50",
                    tr("Yuneec"),
                    tr("E50"),
                    6.2372,             // sensorWidth
                    4.7058,             // sensorHeight
                    4000,               // imageWidth
                    3000,               // imageHeight
                    7.2,                // focalLength
                    true,               // true: landscape orientation
                    true,               // true: camera is fixed orientation
                    1.3,                // minimum trigger interval
                    tr("Yuneec E50"),   // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);              // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Yuneec E90",
                    tr("Yuneec"),
                    tr("E90"),
                    13.3056,            // sensorWidth
                    8.656,              // sensorHeight
                    5472,               // imageWidth
                    3648,               // imageHeight
                    8.29,               // focalLength
                    true,               // true: landscape orientation
                    true,               // true: camera is fixed orientation
                    1.3,                // minimum trigger interval
                    tr("Yuneec E90"),   // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);              // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Flir Duo R",
                    tr("Flir"),
                    tr("Duo R"),
                    160,                // sensorWidth
                    120,                // sensorHeight
                    1920,               // imageWidth
                    1080,               // imageHeight
                    1.9,                // focalLength
                    true,               // true: landscape orientation
                    false,              // true: camera is fixed orientation
                    0,                  // minimum trigger interval
                    tr("Flir Duo R"),   // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);              // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Flir Duo Pro R",
                    tr("Flir"),
                    tr("Duo Pro R"),
                    10.88,                // sensorWidth
                    8.704,                // sensorHeight
                    640,               // imageWidth
                    512,               // imageHeight
                    19,                // focalLength
                    true,               // true: landscape orientation
                    false,              // true: camera is fixed orientation
                    1.0,                  // minimum trigger interval
                    "",   // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);              // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Workswell Wiris Security Thermal Camera",
                    tr("Workswell"),
                    tr("Wiris Security"),
                    13.6,                // sensorWidth
                    10.2,                // sensorHeight
                    800,               // imageWidth
                    600,               // imageHeight
                    35,                // focalLength
                    true,               // true: landscape orientation
                    false,              // true: camera is fixed orientation
                    1.8,                  // minimum trigger interval
                    "",   // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);              // parent
        _cameraList.append(QVariant::fromValue(metaData));

        metaData = new CameraMetaData(
                    "Workswell Wiris Security Visual Camera",
                    tr("Workswell"),
                    tr("Wiris Security"),
                    4.826,                // sensorWidth
                    3.556,                // sensorHeight
                    1920,               // imageWidth
                    1080,               // imageHeight
                    4.3,                // focalLength
                    true,               // true: landscape orientation
                    false,              // true: camera is fixed orientation
                    1.8,                  // minimum trigger interval
                    "",   // SHOULD BE BLANK FOR NEWLY ADDED CAMERAS. Deprecated translation from older builds.
                    this);              // parent
        _cameraList.append(QVariant::fromValue(metaData));
    }

    return _cameraList;
}

QMap<QString, FactGroup*>* FirmwarePlugin::factGroups(void) {
    // Generic plugin has no FactGroups
    return nullptr;
}

bool FirmwarePlugin::_armVehicleAndValidate(Vehicle* vehicle)
{
    if (vehicle->armed()) {
        return true;
    }

    bool vehicleArmed = false;

    // Only try arming the vehicle a single time. Doing retries on arming with a delay can lead to safety issues.
    vehicle->setArmed(true, false /* showError */);

    // Wait 1000 msecs for vehicle to arm
    for (int i=0; i<10; i++) {
        if (vehicle->armed()) {
            vehicleArmed = true;
            break;
        }
        QGC::SLEEP::msleep(100);
        qgcApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    return vehicleArmed;
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
        &QGCFileDownload::downloadComplete,
        this,
        [vehicle, this](QString remoteFile, QString localFile, QString errorMsg) {
            if (errorMsg.isEmpty()) {
                _versionFileDownloadFinished(remoteFile, localFile, vehicle);
            } else {
                qCDebug(FirmwarePluginLog) << "Failed to download the latest fw version file. Error: " << errorMsg;
            }
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
        qgcApp()->showAppMessage(tr("Vehicle is not running latest stable firmware! Running %1, latest stable is %2.").arg(currentVersionNumber, version));
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
    WeakLinkInterfacePtr weakLink = vehicle->vehicleLinkManager()->primaryLink();
    if (!weakLink.expired()) {
        MAVLinkProtocol*        mavlinkProtocol = qgcApp()->toolbox()->mavlinkProtocol();
        mavlink_follow_target_t follow_target   = {};
        SharedLinkInterfacePtr  sharedLink      = weakLink.lock();

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
                                              sharedLink->mavlinkChannel(),
                                              &message,
                                              &follow_target);
        vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), message);
    }
}

Autotune* FirmwarePlugin::createAutotune(Vehicle *vehicle)
{
    return new Autotune(vehicle);
}
