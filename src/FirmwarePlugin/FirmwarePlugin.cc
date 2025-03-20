/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FirmwarePlugin.h"
#include "QGCApplication.h"
#include "GenericAutoPilotPlugin.h"
#include "AutoPilotPlugin.h"
#include "QGCFileDownload.h"
#include "QGCCameraManager.h"
#include "RadioComponentController.h"
#include "Autotune.h"
#include "VehicleCameraControl.h"
#include "VehicleComponent.h"
#include "MAVLinkProtocol.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QThread>

QGC_LOGGING_CATEGORY(FirmwarePluginLog, "FirmwarePluginLog")

const QString guided_mode_not_supported_by_vehicle = QObject::tr("Guided mode not supported by Vehicle.");

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
        flightMode = _modeEnumToString.value(custom_mode, QString("Custom:0x%1").arg(custom_mode, 0, 16));
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

QList<MAV_CMD> FirmwarePlugin::supportedMissionCommands(QGCMAVLink::VehicleClass_t /* vehicleClass */) const
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

void
FirmwarePlugin::guidedModeChangeGroundSpeedMetersSecond(Vehicle*, double)
{
    // Not supported by generic vehicle
    qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
}

void
FirmwarePlugin::guidedModeChangeEquivalentAirspeedMetersSecond(Vehicle*, double)
{
    // Not supported by generic vehicle
    qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::guidedModeChangeHeading(Vehicle *vehicle, const QGeoCoordinate &headingCoord)
{
    Q_UNUSED(vehicle);
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

QVariant FirmwarePlugin::mainStatusIndicatorContentItem(const Vehicle*) const
{
    return QVariant();
}

const QVariantList& FirmwarePlugin::toolIndicators(const Vehicle*)
{
    //-- Default list of indicators for all vehicles.
    if(_toolIndicatorList.size() == 0) {
        _toolIndicatorList = QVariantList({
            QVariant::fromValue(QUrl::fromUserInput("qrc:/qml/QGroundControl/Controls/FlightModeIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/VehicleGPSIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/TelemetryRSSIIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/RCRSSIIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/qml/QGroundControl/Controls/BatteryIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/RemoteIDIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/GimbalIndicator.qml")),
        });
    }
    return _toolIndicatorList;
}

const QVariantList& FirmwarePlugin::modeIndicators(const Vehicle*)
{
    //-- Default list of indicators for all vehicles.
    if(_modeIndicatorList.size() == 0) {
        _modeIndicatorList = QVariantList({
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/MultiVehicleSelector.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/LinkIndicator.qml")),
        });
    }
    return _modeIndicatorList;
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

    // Wait 1500 msecs for vehicle to arm (waiting for the next heartbeat)
    for (int i = 0; i < 15; i++) {
        if (vehicle->armed()) {
            vehicleArmed = true;
            break;
        }
        QThread::msleep(100);
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
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
            QThread::msleep(100);
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
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

MavlinkCameraControl* FirmwarePlugin::createCameraControl(const mavlink_camera_information_t *info, Vehicle *vehicle, int compID, QObject* parent)
{
    return new VehicleCameraControl(info, vehicle, compID, parent);
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

int FirmwarePlugin::versionCompare(const Vehicle* vehicle, int major, int minor, int patch) const
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

int FirmwarePlugin::versionCompare(const Vehicle* vehicle, QString& compare) const
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

void FirmwarePlugin::sendGCSMotionReport(Vehicle *vehicle, FollowMe::GCSMotionReport &motionReport, uint8_t estimationCapabilities)
{
    Q_CHECK_PTR(vehicle);

    SharedLinkInterfacePtr sharedLink = vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        return;
    }

    mavlink_follow_target_t follow_target{0};

    follow_target.timestamp = qgcApp()->msecsSinceBoot();
    follow_target.est_capabilities = estimationCapabilities;
    follow_target.position_cov[0] = static_cast<float>(motionReport.pos_std_dev[0]);
    follow_target.position_cov[2] = static_cast<float>(motionReport.pos_std_dev[2]);
    follow_target.alt = static_cast<float>(motionReport.altMetersAMSL);
    follow_target.lat = motionReport.lat_int;
    follow_target.lon = motionReport.lon_int;
    follow_target.vel[0] = static_cast<float>(motionReport.vxMetersPerSec);
    follow_target.vel[1] = static_cast<float>(motionReport.vyMetersPerSec);

    mavlink_message_t message;
    mavlink_msg_follow_target_encode_chan(
        static_cast<uint8_t>(MAVLinkProtocol::instance()->getSystemId()),
        static_cast<uint8_t>(MAVLinkProtocol::getComponentId()),
        sharedLink->mavlinkChannel(),
        &message,
        &follow_target
    );
    vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), message);
}

Autotune* FirmwarePlugin::createAutotune(Vehicle *vehicle)
{
    return new Autotune(vehicle);
}

void FirmwarePlugin::updateAvailableFlightModes(FlightModeList flightModeList)
{
    _updateFlightModeList(flightModeList);
}

void FirmwarePlugin::_setModeEnumToModeStringMapping(FlightModeCustomModeMap enumToString)
{
    _modeEnumToString = enumToString;
}

void FirmwarePlugin::_updateFlightModeList(FlightModeList &flightModeList){

    _flightModeList.clear();

    for (FirmwareFlightMode &flightMode : flightModeList){
        if(_modeEnumToString.contains(flightMode.custom_mode)) {
            // Flight mode already exists in initial mapping, use that name which provides for localizations
            flightMode.mode_name = _modeEnumToString[flightMode.custom_mode];
        } else{
            // This is a custom flight mode that is not already known. Best we can do is used the provided name
            _modeEnumToString[flightMode.custom_mode] = flightMode.mode_name;
        }
        qDebug() << Q_FUNC_INFO << "Flight Mode: " << flightMode.mode_name << " Custom Mode: " << flightMode.custom_mode;
        _addNewFlightMode(flightMode);
    }

    for (auto &flightMode : _flightModeList){
        qDebug() << Q_FUNC_INFO << "Flight Mode: " << flightMode.mode_name << " Custom Mode: " << flightMode.custom_mode;
    }
}

void FirmwarePlugin::_addNewFlightMode(FirmwareFlightMode &newFlightMode)
{
    for (auto &existingFlightMode : _flightModeList){
        if (existingFlightMode.custom_mode == newFlightMode.custom_mode) {
            qDebug() << Q_FUNC_INFO << "Skipping duplicate flight mode: " << newFlightMode.mode_name << " Custom Mode: " << newFlightMode.custom_mode;
            // Already exists
            return;
        }
    }
    _flightModeList += newFlightMode;
}
