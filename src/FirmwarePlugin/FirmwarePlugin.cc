  /****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FirmwarePlugin.h"
#include "AutoPilotPlugin.h"
#include "Autotune.h"
#include "GenericAutoPilotPlugin.h"
#include "MAVLinkProtocol.h"
#include "QGCApplication.h"
#include "QGCCameraManager.h"
#include "QGCFileDownload.h"
#include "QGCLoggingCategory.h"
#include "RadioComponentController.h"
#include "VehicleCameraControl.h"
#include "VehicleComponent.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QThread>

QGC_LOGGING_CATEGORY(FirmwarePluginLog, "qgc.firmwareplugin.firmwareplugin")

static const QString guided_mode_not_supported_by_vehicle = QObject::tr("Guided mode not supported by Vehicle.");

FirmwarePlugin::FirmwarePlugin(QObject *parent)
    : QObject(parent)
{
    // qCDebug(FirmwarePluginLog) << Q_FUNC_INFO << this;

    (void) qmlRegisterType<RadioComponentController>("QGroundControl.Controllers", 1, 0, "RadioComponentController");
}

FirmwarePlugin::~FirmwarePlugin()
{
    // qCDebug(FirmwarePluginLog) << Q_FUNC_INFO << this;
}

AutoPilotPlugin *FirmwarePlugin::autopilotPlugin(Vehicle *vehicle) const
{
    return new GenericAutoPilotPlugin(vehicle, vehicle);
}

QString FirmwarePlugin::flightMode(uint8_t base_mode, uint32_t custom_mode) const
{
    Q_UNUSED(custom_mode);

    struct Bit2Name {
        const uint8_t baseModeBit;
        const char *name;
    };

    static constexpr Bit2Name rgBit2Name[] = {
        { MAV_MODE_FLAG_MANUAL_INPUT_ENABLED, "Manual" },
        { MAV_MODE_FLAG_STABILIZE_ENABLED, "Stabilize" },
        { MAV_MODE_FLAG_GUIDED_ENABLED, "Guided" },
        { MAV_MODE_FLAG_AUTO_ENABLED, "Auto" },
        { MAV_MODE_FLAG_TEST_ENABLED, "Test" },
    };

    QString flightMode;
    if (base_mode == 0) {
        flightMode = "PreFlight";
    } else if (base_mode & MAV_MODE_FLAG_CUSTOM_MODE_ENABLED) {
        flightMode = _modeEnumToString.value(custom_mode, QString("Custom:0x%1").arg(custom_mode, 0, 16));
    } else {
        for (size_t i = 0; std::size(rgBit2Name); i++) {
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

bool FirmwarePlugin::setFlightMode(const QString &flightMode, uint8_t *base_mode, uint32_t *custom_mode) const
{
    Q_UNUSED(flightMode);
    Q_UNUSED(base_mode);
    Q_UNUSED(custom_mode);

    qCWarning(FirmwarePluginLog) << "FirmwarePlugin::setFlightMode called on base class, not supported";

    return false;
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
        qCWarning(FirmwarePluginLog) << "FirmwarePlugin::missionCommandOverrides called with bad VehicleClass_t:" << vehicleClass;
        return QString();
    }
}

void FirmwarePlugin::_getParameterMetaDataVersionInfo(const QString &metaDataFile, int &majorVersion, int &minorVersion) const
{
    Q_UNUSED(metaDataFile);
    majorVersion = -1;
    minorVersion = -1;
}

void FirmwarePlugin::setGuidedMode(Vehicle *vehicle, bool guidedMode) const
{
    Q_UNUSED(vehicle);
    Q_UNUSED(guidedMode);
    qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::pauseVehicle(Vehicle *vehicle) const
{
    Q_UNUSED(vehicle);
    qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::guidedModeRTL(Vehicle *vehicle, bool smartRTL) const
{
    Q_UNUSED(vehicle);
    Q_UNUSED(smartRTL);
    qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::guidedModeLand(Vehicle *vehicle) const
{
    Q_UNUSED(vehicle);
    qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::guidedModeTakeoff(Vehicle *vehicle, double takeoffAltRel) const
{
    Q_UNUSED(vehicle);
    Q_UNUSED(takeoffAltRel);
    qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::guidedModeGotoLocation(Vehicle *vehicle, const QGeoCoordinate &gotoCoord) const
{
    Q_UNUSED(vehicle);
    Q_UNUSED(gotoCoord);
    qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::guidedModeChangeAltitude(Vehicle*, double, bool pauseVehicle)
{
    Q_UNUSED(pauseVehicle);
    qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::guidedModeChangeGroundSpeedMetersSecond(Vehicle*, double) const
{
    qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::guidedModeChangeEquivalentAirspeedMetersSecond(Vehicle*, double) const
{
    qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::guidedModeChangeHeading(Vehicle *vehicle, const QGeoCoordinate &headingCoord) const
{
    Q_UNUSED(vehicle);
    qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
}

void FirmwarePlugin::startMission(Vehicle*) const
{
    qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
}

const FirmwarePlugin::remapParamNameMajorVersionMap_t &FirmwarePlugin::paramNameRemapMajorVersionMap(void) const
{
    static const remapParamNameMajorVersionMap_t remap;

    return remap;
}

const QVariantList &FirmwarePlugin::toolIndicators(const Vehicle*)
{
    //-- Default list of indicators for all vehicles.
    if (_toolIndicatorList.isEmpty()) {
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

const QVariantList &FirmwarePlugin::modeIndicators(const Vehicle*)
{
    //-- Default list of indicators for all vehicles.
    if (_modeIndicatorList.isEmpty()) {
        _modeIndicatorList = QVariantList({
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/MultiVehicleSelector.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/LinkIndicator.qml")),
        });
    }

    return _modeIndicatorList;
}

bool FirmwarePlugin::_armVehicleAndValidate(Vehicle *vehicle) const
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

bool FirmwarePlugin::_setFlightModeAndValidate(Vehicle *vehicle, const QString &flightMode) const
{
    if (vehicle->flightMode() == flightMode) {
        return true;
    }

    bool flightModeChanged = false;

    // We try 3 times
    for (int retries = 0; retries < 3; retries++) {
        vehicle->setFlightMode(flightMode);

        // Wait for vehicle to return flight mode
        for (int i = 0; i < 13; i++) {
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


void FirmwarePlugin::batteryConsumptionData(Vehicle *vehicle, int &mAhBattery, double &hoverAmps, double &cruiseAmps) const
{
    Q_UNUSED(vehicle);
    mAhBattery = 0;
    hoverAmps = 0;
    cruiseAmps = 0;
}

bool FirmwarePlugin::hasGimbal(Vehicle *vehicle, bool &rollSupported, bool &pitchSupported, bool &yawSupported) const
{
    Q_UNUSED(vehicle);
    rollSupported = false;
    pitchSupported = false;
    yawSupported = false;
    return false;
}

QGCCameraManager *FirmwarePlugin::createCameraManager(Vehicle *vehicle) const
{
    return new QGCCameraManager(vehicle);
}

MavlinkCameraControl *FirmwarePlugin::createCameraControl(const mavlink_camera_information_t *info, Vehicle *vehicle, int compID, QObject *parent) const
{
    return new VehicleCameraControl(info, vehicle, compID, parent);
}

void FirmwarePlugin::checkIfIsLatestStable(Vehicle *vehicle) const
{
    // This is required as mocklink uses a hardcoded firmware version
    if (qgcApp()->runningUnitTests()) {
        qCDebug(FirmwarePluginLog) << "Skipping version check";
        return;
    }

    const QString versionFile = _getLatestVersionFileUrl(vehicle);
    qCDebug(FirmwarePluginLog) << "Downloading" << versionFile;
    QGCFileDownload *const downloader = new QGCFileDownload(nullptr);
    (void) connect(downloader, &QGCFileDownload::downloadComplete, this, [vehicle, this](const QString &remoteFile, const QString &localFile, const QString &errorMsg) {
        if (errorMsg.isEmpty()) {
            _versionFileDownloadFinished(remoteFile, localFile, vehicle);
        } else {
            qCDebug(FirmwarePluginLog) << "Failed to download the latest fw version file. Error:" << errorMsg;
        }
        sender()->deleteLater();
    });

    if (!downloader->download(versionFile)) {
        downloader->deleteLater();
    }
}

void FirmwarePlugin::_versionFileDownloadFinished(const QString &remoteFile, const QString &localFile, const Vehicle *vehicle) const
{
    qCDebug(FirmwarePluginLog) << "Download complete" << remoteFile << localFile;
    // Now read the version file and pull out the version string
    QFile versionFile(localFile);
    if (!versionFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(FirmwarePluginLog) << "Error opening downloaded version file.";
        return;
    }

    QTextStream stream(&versionFile);
    const QString versionFileContents = stream.readAll();
    QString version;
    const QRegularExpressionMatch match = QRegularExpression(_versionRegex()).match(versionFileContents);

    qCDebug(FirmwarePluginLog) << "Looking for version number...";

    if (match.hasMatch()) {
        version = match.captured(1);
    } else {
        qCWarning(FirmwarePluginLog) << "Unable to parse version info from file" << remoteFile;
        return;
    }

    qCDebug(FirmwarePluginLog) << "Latest stable version = "  << version;

    const int currType = vehicle->firmwareVersionType();

    // Check if lower version than stable or same version but different type
    if ((currType == FIRMWARE_VERSION_TYPE_OFFICIAL) && (vehicle->versionCompare(version) < 0)) {
        const QString currentVersionNumber = QStringLiteral("%1.%2.%3").arg(vehicle->firmwareMajorVersion())
                                                                       .arg(vehicle->firmwareMinorVersion())
                                                                       .arg(vehicle->firmwarePatchVersion());
        qgcApp()->showAppMessage(tr("Vehicle is not running latest stable firmware! Running %1, latest stable is %2.").arg(currentVersionNumber, version));
    }
}

int FirmwarePlugin::versionCompare(const Vehicle *vehicle, int major, int minor, int patch) const
{
    const int currMajor = vehicle->firmwareMajorVersion();
    const int currMinor = vehicle->firmwareMinorVersion();
    const int currPatch = vehicle->firmwarePatchVersion();

    if ((currMajor == major) && (currMinor == minor) && (currPatch == patch)) {
        return 0;
    }

    if ((currMajor > major)
       || ((currMajor == major) && (currMinor > minor))
       || ((currMajor == major) && (currMinor == minor) && (currPatch > patch)))
    {
        return 1;
    }

    return -1;
}

int FirmwarePlugin::versionCompare(const Vehicle *vehicle, const QString &compare) const
{
    const QStringList versionNumbers = compare.split(".");
    if (versionNumbers.size() != 3) {
        qCWarning(FirmwarePluginLog) << "Error parsing version number: wrong format";
        return -1;
    }

    const int major = versionNumbers[0].toInt();
    const int minor = versionNumbers[1].toInt();
    const int patch = versionNumbers[2].toInt();

    return versionCompare(vehicle, major, minor, patch);
}

void FirmwarePlugin::sendGCSMotionReport(Vehicle *vehicle, const FollowMe::GCSMotionReport &motionReport, uint8_t estimationCapabilities) const
{
    SharedLinkInterfacePtr sharedLink = vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        return;
    }

    mavlink_follow_target_t follow_target{};

    follow_target.timestamp = qgcApp()->msecsSinceBoot();
    follow_target.est_capabilities = estimationCapabilities;
    follow_target.position_cov[0] = static_cast<float>(motionReport.pos_std_dev[0]);
    follow_target.position_cov[2] = static_cast<float>(motionReport.pos_std_dev[2]);
    follow_target.alt = static_cast<float>(motionReport.altMetersAMSL);
    follow_target.lat = motionReport.lat_int;
    follow_target.lon = motionReport.lon_int;
    follow_target.vel[0] = static_cast<float>(motionReport.vxMetersPerSec);
    follow_target.vel[1] = static_cast<float>(motionReport.vyMetersPerSec);

    mavlink_message_t message{};
    mavlink_msg_follow_target_encode_chan(
        static_cast<uint8_t>(MAVLinkProtocol::instance()->getSystemId()),
        static_cast<uint8_t>(MAVLinkProtocol::getComponentId()),
        sharedLink->mavlinkChannel(),
        &message,
        &follow_target
    );

    (void) vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), message);
}

Autotune *FirmwarePlugin::createAutotune(Vehicle *vehicle) const
{
    return new Autotune(vehicle);
}

void FirmwarePlugin::_updateFlightModeList(FlightModeList &flightModeList)
{
    _flightModeList.clear();

    for (FirmwareFlightMode &flightMode : flightModeList){
        if (_modeEnumToString.contains(flightMode.custom_mode)) {
            // Flight mode already exists in initial mapping, use that name which provides for localizations
            flightMode.mode_name = _modeEnumToString[flightMode.custom_mode];
        } else{
            // This is a custom flight mode that is not already known. Best we can do is used the provided name
            _modeEnumToString[flightMode.custom_mode] = flightMode.mode_name;
        }
        _addNewFlightMode(flightMode);
    }

    for (const FirmwareFlightMode &flightMode : _flightModeList) {
        qCDebug(FirmwarePluginLog) << "Flight Mode:" << flightMode.mode_name << " Custom Mode:" << flightMode.custom_mode;
    }
}

void FirmwarePlugin::_addNewFlightMode(FirmwareFlightMode &newFlightMode)
{
    for (const FirmwareFlightMode &existingFlightMode : _flightModeList) {
        if (existingFlightMode.custom_mode == newFlightMode.custom_mode) {
            // Already exists
            return;
        }
    }
    _flightModeList += newFlightMode;
}

/*===========================================================================*/

FirmwarePluginInstanceData::CommandSupportedResult FirmwarePluginInstanceData::getCommandSupported(MAV_CMD cmd) const
{
    if (anyVersionSupportsCommand(cmd) == CommandSupportedResult::UNSUPPORTED) {
        return CommandSupportedResult::UNSUPPORTED;
    }

    return MAV_CMD_supported.value(cmd, CommandSupportedResult::UNKNOWN);
}
