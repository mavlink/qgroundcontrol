/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FirmwareUpgradeController.h"
#include "Bootloader.h"
//-- TODO: #include "QGCQFileDialog.h"
#include "QGCApplication.h"
#include "QGCFileDownload.h"
#include "QGCOptions.h"
#include "QGCCorePlugin.h"
#include "FirmwareUpgradeSettings.h"
#include "SettingsManager.h"

#include <QStandardPaths>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkProxy>

const char* FirmwareUpgradeController::_manifestFirmwareJsonKey =               "firmware";
const char* FirmwareUpgradeController::_manifestBoardIdJsonKey =                "board_id";
const char* FirmwareUpgradeController::_manifestMavTypeJsonKey =                "mav-type";
const char* FirmwareUpgradeController::_manifestFormatJsonKey =                 "format";
const char* FirmwareUpgradeController::_manifestUrlJsonKey =                    "url";
const char* FirmwareUpgradeController::_manifestMavFirmwareVersionTypeJsonKey = "mav-firmware-version-type";
const char* FirmwareUpgradeController::_manifestUSBIDJsonKey =                  "USBID";
const char* FirmwareUpgradeController::_manifestMavFirmwareVersionJsonKey =     "mav-firmware-version";
const char* FirmwareUpgradeController::_manifestBootloaderStrJsonKey =          "bootloader_str";
const char* FirmwareUpgradeController::_manifestLatestKey =                     "latest";
const char* FirmwareUpgradeController::_manifestPlatformKey =                   "platform";
const char* FirmwareUpgradeController::_manifestBrandNameKey =                  "brand_name";

struct FirmwareToUrlElement_t {
    FirmwareUpgradeController::AutoPilotStackType_t     stackType;
    FirmwareUpgradeController::FirmwareBuildType_t      firmwareType;
    FirmwareUpgradeController::FirmwareVehicleType_t    vehicleType;
    QString                                             url;
};

uint qHash(const FirmwareUpgradeController::FirmwareIdentifier& firmwareId)
{
    return static_cast<uint>(( firmwareId.autopilotStackType |
                               (firmwareId.firmwareType << 8) |
                               (firmwareId.firmwareVehicleType << 16) ));
}

/// @Brief Constructs a new FirmwareUpgradeController Widget. This widget is used within the PX4VehicleConfig set of screens.
FirmwareUpgradeController::FirmwareUpgradeController(void)
    : _singleFirmwareURL                (qgcApp()->toolbox()->corePlugin()->options()->firmwareUpgradeSingleURL())
    , _singleFirmwareMode               (!_singleFirmwareURL.isEmpty())
    , _downloadingFirmwareList          (false)
    , _downloadManager                  (nullptr)
    , _downloadNetworkReply             (nullptr)
    , _statusLog                        (nullptr)
    , _selectedFirmwareBuildType             (StableFirmware)
    , _image                            (nullptr)
    , _apmBoardDescriptionReplaceText   ("<APMBoardDescription>")
    , _apmChibiOSSetting                (qgcApp()->toolbox()->settingsManager()->firmwareUpgradeSettings()->apmChibiOS())
    , _apmVehicleTypeSetting            (qgcApp()->toolbox()->settingsManager()->firmwareUpgradeSettings()->apmVehicleType())
{
    _manifestMavFirmwareVersionTypeToFirmwareBuildTypeMap["OFFICIAL"] =  StableFirmware;
    _manifestMavFirmwareVersionTypeToFirmwareBuildTypeMap["BETA"] =      BetaFirmware;
    _manifestMavFirmwareVersionTypeToFirmwareBuildTypeMap["DEV"] =       DeveloperFirmware;

    _manifestMavTypeToFirmwareVehicleTypeMap["Copter"] =        CopterFirmware;
    _manifestMavTypeToFirmwareVehicleTypeMap["HELICOPTER"] =    HeliFirmware;
    _manifestMavTypeToFirmwareVehicleTypeMap["FIXED_WING"] =    PlaneFirmware;
    _manifestMavTypeToFirmwareVehicleTypeMap["GROUND_ROVER"] =  RoverFirmware;
    _manifestMavTypeToFirmwareVehicleTypeMap["SUBMARINE"] =     SubFirmware;

    _threadController = new PX4FirmwareUpgradeThreadController(this);
    Q_CHECK_PTR(_threadController);

    connect(_threadController, &PX4FirmwareUpgradeThreadController::foundBoard,             this, &FirmwareUpgradeController::_foundBoard);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::noBoardFound,           this, &FirmwareUpgradeController::_noBoardFound);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::boardGone,              this, &FirmwareUpgradeController::_boardGone);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::foundBootloader,        this, &FirmwareUpgradeController::_foundBootloader);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::bootloaderSyncFailed,   this, &FirmwareUpgradeController::_bootloaderSyncFailed);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::error,                  this, &FirmwareUpgradeController::_error);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::updateProgress,         this, &FirmwareUpgradeController::_updateProgress);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::status,                 this, &FirmwareUpgradeController::_status);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::eraseStarted,           this, &FirmwareUpgradeController::_eraseStarted);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::eraseComplete,          this, &FirmwareUpgradeController::_eraseComplete);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::flashComplete,          this, &FirmwareUpgradeController::_flashComplete);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::updateProgress,         this, &FirmwareUpgradeController::_updateProgress);
    
    connect(&_eraseTimer, &QTimer::timeout, this, &FirmwareUpgradeController::_eraseProgressTick);

#if !defined(NO_ARDUPILOT_DIALECT)
    connect(_apmChibiOSSetting,     &Fact::rawValueChanged, this, &FirmwareUpgradeController::_buildAPMFirmwareNames);
    connect(_apmVehicleTypeSetting, &Fact::rawValueChanged, this, &FirmwareUpgradeController::_buildAPMFirmwareNames);
#endif

    _initFirmwareHash();
    _determinePX4StableVersion();

#if !defined(NO_ARDUPILOT_DIALECT)
    _downloadArduPilotManifest();
#endif
}

FirmwareUpgradeController::~FirmwareUpgradeController()
{
    qgcApp()->toolbox()->linkManager()->setConnectionsAllowed();
}

void FirmwareUpgradeController::startBoardSearch(void)
{
    LinkManager* linkMgr = qgcApp()->toolbox()->linkManager();

    linkMgr->setConnectionsSuspended(tr("Connect not allowed during Firmware Upgrade."));

    // FIXME: Why did we get here with active vehicle?
    if (!qgcApp()->toolbox()->multiVehicleManager()->activeVehicle()) {
        // We have to disconnect any inactive links
        linkMgr->disconnectAll();
    }

    _bootloaderFound = false;
    _startFlashWhenBootloaderFound = false;
    _threadController->startFindBoardLoop();
}

void FirmwareUpgradeController::flash(AutoPilotStackType_t stackType,
                                      FirmwareBuildType_t firmwareType,
                                      FirmwareVehicleType_t vehicleType)
{
    qCDebug(FirmwareUpgradeLog) << "_flash stackType:firmwareType:vehicleType" << stackType << firmwareType << vehicleType;
    FirmwareIdentifier firmwareId = FirmwareIdentifier(stackType, firmwareType, vehicleType);
    if (_bootloaderFound) {
        _getFirmwareFile(firmwareId);
    } else {
        // We haven't found the bootloader yet. Need to wait until then to flash
        _startFlashWhenBootloaderFound = true;
        _startFlashWhenBootloaderFoundFirmwareIdentity = firmwareId;
        _firmwareFilename.clear();
    }
}

void FirmwareUpgradeController::flashFirmwareUrl(QString firmwareFlashUrl)
{
    _firmwareFilename = firmwareFlashUrl;
    if (_bootloaderFound) {
        _downloadFirmware();
    } else {
        // We haven't found the bootloader yet. Need to wait until then to flash
        _startFlashWhenBootloaderFound = true;
    }
}

void FirmwareUpgradeController::flash(const FirmwareIdentifier& firmwareId)
{
    flash(firmwareId.autopilotStackType, firmwareId.firmwareType, firmwareId.firmwareVehicleType);
}

void FirmwareUpgradeController::flashSingleFirmwareMode(FirmwareBuildType_t firmwareType)
{
    flash(SingleFirmwareMode, firmwareType, DefaultVehicleFirmware);
}

void FirmwareUpgradeController::cancel(void)
{
    _eraseTimer.stop();
    _threadController->cancel();
}

QStringList FirmwareUpgradeController::availableBoardsName(void)
{
    QGCSerialPortInfo::BoardType_t boardType;
    QString boardName;
    QStringList names;

    auto ports = QGCSerialPortInfo::availablePorts();
    for(const auto info : ports) {
        if(info.canFlash()) {
            info.getBoardInfo(boardType, boardName);
            names.append(boardName);
        }
    }

    return names;
}

void FirmwareUpgradeController::_foundBoard(bool firstAttempt, const QSerialPortInfo& info, int boardType, QString boardName)
{
    _foundBoardInfo =       info;
    _foundBoardType =       static_cast<QGCSerialPortInfo::BoardType_t>(boardType);
    _foundBoardTypeName =   boardName;

    qDebug() << info.manufacturer() << info.description();

    _startFlashWhenBootloaderFound = false;

    if (_foundBoardType == QGCSerialPortInfo::BoardTypeSiKRadio) {
        if (!firstAttempt) {
            // Radio always flashes latest firmware, so we can start right away without
            // any further user input.
            _startFlashWhenBootloaderFound = true;
            _startFlashWhenBootloaderFoundFirmwareIdentity = FirmwareIdentifier(ThreeDRRadio,
                                                                                StableFirmware,
                                                                                DefaultVehicleFirmware);
        }
    }
    
    qCDebug(FirmwareUpgradeLog) << _foundBoardType << _foundBoardTypeName;
    emit boardFound();
}


void FirmwareUpgradeController::_noBoardFound(void)
{
    emit noBoardFound();
}

void FirmwareUpgradeController::_boardGone(void)
{
    emit boardGone();
}

/// @brief Called when the bootloader is connected to by the findBootloader process. Moves the state machine
///         to the next step.
void FirmwareUpgradeController::_foundBootloader(int bootloaderVersion, int boardID, int flashSize)
{
    _bootloaderFound = true;
    _bootloaderVersion = static_cast<uint32_t>(bootloaderVersion);
    _bootloaderBoardID = static_cast<uint32_t>(boardID);
    _bootloaderBoardFlashSize = static_cast<uint32_t>(flashSize);
    
    _appendStatusLog(tr("Connected to bootloader:"));
    _appendStatusLog(tr("  Version: %1").arg(_bootloaderVersion));
    _appendStatusLog(tr("  Board ID: %1").arg(_bootloaderBoardID));
    _appendStatusLog(tr("  Flash size: %1").arg(_bootloaderBoardFlashSize));
    
    if (_startFlashWhenBootloaderFound) {
        flash(_startFlashWhenBootloaderFoundFirmwareIdentity);
    }

    if (_rgManifestFirmwareInfo.count()) {
        _buildAPMFirmwareNames();
    }
}


/// @brief intializes the firmware hashes with proper urls.
/// This happens only once for a class instance first time when it is needed.
void FirmwareUpgradeController::_initFirmwareHash()
{
    // indirect check whether this function has been called before or not
    // may have to be modified if _rgPX4FMUV2Firmware disappears
    if (!_rgPX4FMUV2Firmware.isEmpty()) {
        return;
    }

    //////////////////////////////////// PX4FMU aerocore firmwares //////////////////////////////////////////////////
    FirmwareToUrlElement_t  rgAeroCoreFirmwareArray[] = {
        { AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware, "http://gumstix-aerocore.s3.amazonaws.com/PX4/stable/aerocore_default.px4"},
        { AutoPilotStackPX4, BetaFirmware,      DefaultVehicleFirmware, "http://gumstix-aerocore.s3.amazonaws.com/PX4/beta/aerocore_default.px4"},
        { AutoPilotStackPX4, DeveloperFirmware, DefaultVehicleFirmware, "http://gumstix-aerocore.s3.amazonaws.com/PX4/master/aerocore_default.px4"},
    };

    //////////////////////////////////// AUAVX2_1 firmwares //////////////////////////////////////////////////
    FirmwareToUrlElement_t rgAUAVX2_1FirmwareArray[] = {
        { AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/stable/auav-x21_default.px4"},
        { AutoPilotStackPX4, BetaFirmware,      DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/beta/auav-x21_default.px4"},
        { AutoPilotStackPX4, DeveloperFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/master/auav-x21_default.px4"},
    };
    //////////////////////////////////// MindPXFMUV2 firmwares //////////////////////////////////////////////////
    FirmwareToUrlElement_t rgMindPXFMUV2FirmwareArray[] = {
        { AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/stable/mindpx-v2_default.px4"},
        { AutoPilotStackPX4, BetaFirmware,      DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/beta/mindpx-v2_default.px4"},
        { AutoPilotStackPX4, DeveloperFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/master/mindpx-v2_default.px4"},
    };
    //////////////////////////////////// TAPV1 firmwares //////////////////////////////////////////////////
    FirmwareToUrlElement_t rgTAPV1FirmwareArray[] = {
        { AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/stable/tap-v1_default.px4"},
        { AutoPilotStackPX4, BetaFirmware,      DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/beta/tap-v1_default.px4"},
        { AutoPilotStackPX4, DeveloperFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/master/tap-v1_default.px4"},
        { SingleFirmwareMode,StableFirmware,    DefaultVehicleFirmware, _singleFirmwareURL},
    };
    //////////////////////////////////// ASCV1 firmwares //////////////////////////////////////////////////
    FirmwareToUrlElement_t rgASCV1FirmwareArray[] = {
        { AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/stable/asc-v1_default.px4"},
        { AutoPilotStackPX4, BetaFirmware,      DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/beta/asc-v1_default.px4"},
        { AutoPilotStackPX4, DeveloperFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/master/asc-v1_default.px4"},
    };

    //////////////////////////////////// Crazyflie 2.0 firmwares //////////////////////////////////////////////////
    FirmwareToUrlElement_t rgCrazyflie2FirmwareArray[] = {
        { AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/stable/crazyflie_default.px4"},
        { AutoPilotStackPX4, BetaFirmware,      DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/beta/crazyflie_default.px4"},
        { AutoPilotStackPX4, DeveloperFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/master/crazyflie_default.px4"},
    };

    //////////////////////////////////// Omnibus F4 SD firmwares //////////////////////////////////////////////////
    FirmwareToUrlElement_t rgOmnibusF4SDFirmwareArray[] = {
        { AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/stable/omnibus_f4sd_default.px4"},
        { AutoPilotStackPX4, BetaFirmware,      DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/beta/omnibus_f4sd_default.px4"},
        { AutoPilotStackPX4, DeveloperFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/master/omnibus_f4sd_default.px4"},
    };
    
    //////////////////////////////////// FMUK66V3 firmwares //////////////////////////////////////////////////
    FirmwareToUrlElement_t rgFMUK66V3FirmwareArray[] = {
        { AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/stable/nxp_fmuk66-v3_default.px4"},
        { AutoPilotStackPX4, BetaFirmware,      DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/beta/nxp_fmuk66-v3_default.px4"},
        { AutoPilotStackPX4, DeveloperFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/master/nxp_fmuk66-v3_default.px4"},
    };

    //////////////////////////////////// Kakute F7 firmwares //////////////////////////////////////////////////
    FirmwareToUrlElement_t rgKakuteF7FirmwareArray[] = {
        { AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/stable/holybro_kakutef7_default.px4"},
        { AutoPilotStackPX4, BetaFirmware,      DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/beta/holybro_kakutef7_default.px4"},
        { AutoPilotStackPX4, DeveloperFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/master/holybro_kakutef7_default.px4"},
    };
    
    //////////////////////////////////// Durandal firmwares //////////////////////////////////////////////////
    FirmwareToUrlElement_t rgDurandalV1FirmwareArray[] = {
        { AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/stable/holybro_durandal-v1_default.px4"},
        { AutoPilotStackPX4, BetaFirmware,      DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/beta/holybro_durandal-v1_default.px4"},
        { AutoPilotStackPX4, DeveloperFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/master/holybro_durandal-v1_default.px4"},
    };

    //////////////////////////////////// ModalAI FC v1 firmwares //////////////////////////////////////////////////
    FirmwareToUrlElement_t rgModalFCV1FirmwareArray[] = {
        { AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/stable/modalai_fc-v1_default.px4"},
        { AutoPilotStackPX4, BetaFirmware,      DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/beta/modalai_fc-v1_default.px4"},
        { AutoPilotStackPX4, DeveloperFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/master/modalai_fc-v1_default.px4"},
    };

    /////////////////////////////// px4flow firmwares ///////////////////////////////////////
    FirmwareToUrlElement_t rgPX4FLowFirmwareArray[] = {
        { PX4FlowPX4, StableFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Flow/master/px4flow.px4" },
    #if !defined(NO_ARDUPILOT_DIALECT)
        { PX4FlowAPM, StableFirmware, DefaultVehicleFirmware, "http://firmware.ardupilot.org/Tools/PX4Flow/px4flow-klt-latest.px4" },
    #endif
    };

    /////////////////////////////// 3dr radio firmwares ///////////////////////////////////////
    FirmwareToUrlElement_t rg3DRRadioFirmwareArray[] = {
        { ThreeDRRadio, StableFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/SiK/stable/radio~hm_trp.ihx"}
    };

    // We build the maps for PX4 firmwares dynamically using the data below

#if 0
    Example URLs for PX4 and ArduPilot
    { AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/stable/px4fmu-v4_default.px4"},
    { AutoPilotStackAPM, StableFirmware,    CopterFirmware,         "http://firmware.ardupilot.org/Copter/stable/PX4/ArduCopter-v4.px4"},
    { AutoPilotStackAPM, DeveloperFirmware, CopterChibiosFirmware,  "http://firmware.ardupilot.org/Copter/latest/fmuv4/arducopter.apj"},
#endif

    QString px4Url          ("http://px4-travis.s3.amazonaws.com/Firmware/%1/px4fmu-%2_default.px4");

    QMap<FirmwareBuildType_t, QString> px4MapFirmwareTypeToDir;
    px4MapFirmwareTypeToDir[StableFirmware] =       QStringLiteral("stable");
    px4MapFirmwareTypeToDir[BetaFirmware] =         QStringLiteral("beta");
    px4MapFirmwareTypeToDir[DeveloperFirmware] =    QStringLiteral("master");

    // PX4 Firmwares
    for (const FirmwareBuildType_t& firmwareType: px4MapFirmwareTypeToDir.keys()) {
        QString dir = px4MapFirmwareTypeToDir[firmwareType];
        _rgFMUV5Firmware.insert     (FirmwareIdentifier(AutoPilotStackPX4, firmwareType, DefaultVehicleFirmware), px4Url.arg(dir).arg("v5"));
        _rgFMUV4PROFirmware.insert  (FirmwareIdentifier(AutoPilotStackPX4, firmwareType, DefaultVehicleFirmware), px4Url.arg(dir).arg("v4pro"));
        _rgFMUV4Firmware.insert     (FirmwareIdentifier(AutoPilotStackPX4, firmwareType, DefaultVehicleFirmware), px4Url.arg(dir).arg("v4"));
        _rgFMUV3Firmware.insert     (FirmwareIdentifier(AutoPilotStackPX4, firmwareType, DefaultVehicleFirmware), px4Url.arg(dir).arg("v3"));
        _rgPX4FMUV2Firmware.insert  (FirmwareIdentifier(AutoPilotStackPX4, firmwareType, DefaultVehicleFirmware), px4Url.arg(dir).arg("v2"));
    }

    int size = sizeof(rgAeroCoreFirmwareArray)/sizeof(rgAeroCoreFirmwareArray[0]);
    for (int i = 0; i < size; i++) {
        const FirmwareToUrlElement_t& element = rgAeroCoreFirmwareArray[i];
        _rgAeroCoreFirmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }

    size = sizeof(rgAUAVX2_1FirmwareArray)/sizeof(rgAUAVX2_1FirmwareArray[0]);
    for (int i = 0; i < size; i++) {
        const FirmwareToUrlElement_t& element = rgAUAVX2_1FirmwareArray[i];
        _rgAUAVX2_1Firmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }

    size = sizeof(rgMindPXFMUV2FirmwareArray)/sizeof(rgMindPXFMUV2FirmwareArray[0]);
    for (int i = 0; i < size; i++) {
        const FirmwareToUrlElement_t& element = rgMindPXFMUV2FirmwareArray[i];
        _rgMindPXFMUV2Firmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }

    size = sizeof(rgTAPV1FirmwareArray)/sizeof(rgTAPV1FirmwareArray[0]);
    for (int i = 0; i < size; i++) {
        const FirmwareToUrlElement_t& element = rgTAPV1FirmwareArray[i];
        _rgTAPV1Firmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }

    size = sizeof(rgASCV1FirmwareArray)/sizeof(rgASCV1FirmwareArray[0]);
    for (int i = 0; i < size; i++) {
        const FirmwareToUrlElement_t& element = rgASCV1FirmwareArray[i];
        _rgASCV1Firmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }

    size = sizeof(rgCrazyflie2FirmwareArray)/sizeof(rgCrazyflie2FirmwareArray[0]);
    for (int i = 0; i < size; i++) {
        const FirmwareToUrlElement_t& element = rgCrazyflie2FirmwareArray[i];
        _rgCrazyflie2Firmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }

    size = sizeof(rgOmnibusF4SDFirmwareArray)/sizeof(rgOmnibusF4SDFirmwareArray[0]);
    for (int i = 0; i < size; i++) {
        const FirmwareToUrlElement_t& element = rgOmnibusF4SDFirmwareArray[i];
        _rgOmnibusF4SDFirmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }
    
    size = sizeof(rgKakuteF7FirmwareArray)/sizeof(rgKakuteF7FirmwareArray[0]);
    for (int i = 0; i < size; i++) {
        const FirmwareToUrlElement_t& element = rgKakuteF7FirmwareArray[i];
        _rgKakuteF7Firmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }

    size = sizeof(rgDurandalV1FirmwareArray)/sizeof(rgDurandalV1FirmwareArray[0]);
    for (int i = 0; i < size; i++) {
        const FirmwareToUrlElement_t& element = rgDurandalV1FirmwareArray[i];
        _rgDurandalV1Firmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }

    size = sizeof(rgFMUK66V3FirmwareArray)/sizeof(rgFMUK66V3FirmwareArray[0]);
    for (int i = 0; i < size; i++) {
        const FirmwareToUrlElement_t& element = rgFMUK66V3FirmwareArray[i];
        _rgFMUK66V3Firmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }

    size = sizeof(rgModalFCV1FirmwareArray)/sizeof(rgModalFCV1FirmwareArray[0]);
    for (int i = 0; i < size; i++) {
        const FirmwareToUrlElement_t& element = rgModalFCV1FirmwareArray[i];
        _rgModalFCV1Firmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }

    size = sizeof(rgPX4FLowFirmwareArray)/sizeof(rgPX4FLowFirmwareArray[0]);
    for (int i = 0; i < size; i++) {
        const FirmwareToUrlElement_t& element = rgPX4FLowFirmwareArray[i];
        _rgPX4FLowFirmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }

    size = sizeof(rg3DRRadioFirmwareArray)/sizeof(rg3DRRadioFirmwareArray[0]);
    for (int i = 0; i < size; i++) {
        const FirmwareToUrlElement_t& element = rg3DRRadioFirmwareArray[i];
        _rg3DRRadioFirmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }

    size = sizeof(rg3DRRadioFirmwareArray)/sizeof(rg3DRRadioFirmwareArray[0]);
    for (int i = 0; i < size; i++) {
        const FirmwareToUrlElement_t& element = rg3DRRadioFirmwareArray[i];
        _rg3DRRadioFirmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }
}

/// @brief Called when the findBootloader process is unable to sync to the bootloader. Moves the state
///         machine to the appropriate error state.
void FirmwareUpgradeController::_bootloaderSyncFailed(void)
{
    _errorCancel("Unable to sync with bootloader.");
}

QHash<FirmwareUpgradeController::FirmwareIdentifier, QString>* FirmwareUpgradeController::_firmwareHashForBoardId(int boardId)
{

    _rgFirmwareDynamic.clear();

    switch (boardId) {
    case Bootloader::boardIDPX4Flow:
        _rgFirmwareDynamic = _rgPX4FLowFirmware;
        break;
    case Bootloader::boardIDPX4FMUV2:
        _rgFirmwareDynamic = _rgPX4FMUV2Firmware;
        break;
    case Bootloader::boardIDPX4FMUV3:
        _rgFirmwareDynamic = _rgFMUV3Firmware;
        break;
    case Bootloader::boardIDPX4FMUV4:
        _rgFirmwareDynamic = _rgFMUV4Firmware;
        break;
    case Bootloader::boardIDPX4FMUV4PRO:
        _rgFirmwareDynamic = _rgFMUV4PROFirmware;
        break;
    case Bootloader::boardIDPX4FMUV5:
        _rgFirmwareDynamic = _rgFMUV5Firmware;
        break;
    case Bootloader::boardIDAeroCore:
        _rgFirmwareDynamic = _rgAeroCoreFirmware;
        break;
    case Bootloader::boardIDAUAVX2_1:
        _rgFirmwareDynamic = _rgAUAVX2_1Firmware;
        break;
    case Bootloader::boardIDMINDPXFMUV2:
        _rgFirmwareDynamic = _rgMindPXFMUV2Firmware;
        break;
    case Bootloader::boardIDTAPV1:
        _rgFirmwareDynamic = _rgTAPV1Firmware;
        break;
    case Bootloader::boardIDASCV1:
        _rgFirmwareDynamic = _rgASCV1Firmware;
        break;
    case Bootloader::boardIDCrazyflie2:
        _rgFirmwareDynamic = _rgCrazyflie2Firmware;
        break;
    case Bootloader::boardIDOmnibusF4SD:
        _rgFirmwareDynamic = _rgOmnibusF4SDFirmware;
        break;
    case Bootloader::boardIDKakuteF7:
        _rgFirmwareDynamic = _rgKakuteF7Firmware;
        break;
    case Bootloader::boardIDDurandalV1:
        _rgFirmwareDynamic = _rgDurandalV1Firmware;
        break;
    case Bootloader::boardIDFMUK66V3:
        _rgFirmwareDynamic = _rgFMUK66V3Firmware;
        break;
    case Bootloader::boardIDModalFCV1:
        _rgFirmwareDynamic = _rgModalFCV1Firmware;
        break;
    case Bootloader::boardID3DRRadio:
        _rgFirmwareDynamic = _rg3DRRadioFirmware;
        break;
    default:
        // Unknown board id
        break;
    }

    return &_rgFirmwareDynamic;
}

void FirmwareUpgradeController::_getFirmwareFile(FirmwareIdentifier firmwareId)
{
    QHash<FirmwareIdentifier, QString>* prgFirmware = _firmwareHashForBoardId(static_cast<int>(_bootloaderBoardID));
    if (firmwareId.firmwareType == CustomFirmware) {
        _firmwareFilename = QString();
        _errorCancel(tr("Custom firmware selected but no filename given."));
    } else {
        if (prgFirmware->contains(firmwareId)) {
            _firmwareFilename = prgFirmware->value(firmwareId);
        } else {
            _errorCancel(tr("Unable to find specified firmware for board type"));
            return;
        }
    }
    
    if (_firmwareFilename.isEmpty()) {
        _errorCancel(tr("No firmware file selected"));
    } else {
        _downloadFirmware();
    }
}

/// @brief Begins the process of downloading the selected firmware file.
void FirmwareUpgradeController::_downloadFirmware(void)
{
    Q_ASSERT(!_firmwareFilename.isEmpty());
    
    _appendStatusLog(tr("Downloading firmware..."));
    _appendStatusLog(tr(" From: %1").arg(_firmwareFilename));
    
    QGCFileDownload* downloader = new QGCFileDownload(this);
    connect(downloader, &QGCFileDownload::downloadFinished, this, &FirmwareUpgradeController::_firmwareDownloadFinished);
    connect(downloader, &QGCFileDownload::downloadProgress, this, &FirmwareUpgradeController::_firmwareDownloadProgress);
    connect(downloader, &QGCFileDownload::error,            this, &FirmwareUpgradeController::_firmwareDownloadError);
    downloader->download(_firmwareFilename);
}

/// @brief Updates the progress indicator while downloading
void FirmwareUpgradeController::_firmwareDownloadProgress(qint64 curr, qint64 total)
{
    // Take care of cases where 0 / 0 is emitted as error return value
    if (total > 0) {
        _progressBar->setProperty("value", static_cast<float>(curr) / static_cast<float>(total));
    }
}

/// @brief Called when the firmware download completes.
void FirmwareUpgradeController::_firmwareDownloadFinished(QString remoteFile, QString localFile)
{
    Q_UNUSED(remoteFile);

    _appendStatusLog(tr("Download complete"));
    
    FirmwareImage* image = new FirmwareImage(this);
    
    connect(image, &FirmwareImage::statusMessage, this, &FirmwareUpgradeController::_status);
    connect(image, &FirmwareImage::errorMessage, this, &FirmwareUpgradeController::_error);
    
    if (!image->load(localFile, _bootloaderBoardID)) {
        _errorCancel(tr("Image load failed"));
        return;
    }
    
    // We can't proceed unless we have the bootloader
    if (!_bootloaderFound) {
        _errorCancel(tr("Bootloader not found"));
        return;
    }
    
    if (_bootloaderBoardFlashSize != 0 && image->imageSize() > _bootloaderBoardFlashSize) {
        _errorCancel(tr("Image size of %1 is too large for board flash size %2").arg(image->imageSize()).arg(_bootloaderBoardFlashSize));
        return;
    }

    _threadController->flash(image);
}

/// @brief Called when an error occurs during download
void FirmwareUpgradeController::_firmwareDownloadError(QString errorMsg)
{
    _errorCancel(errorMsg);
}

/// @brief returns firmware type as a string
QString FirmwareUpgradeController::firmwareTypeAsString(FirmwareBuildType_t type) const
{
    switch (type) {
    case StableFirmware:
        return "stable";
    case DeveloperFirmware:
        return "developer";
    case BetaFirmware:
        return "beta";
    default:
        return "custom";
    }
}

/// @brief Signals completion of one of the specified bootloader commands. Moves the state machine to the
///         appropriate next step.
void FirmwareUpgradeController::_flashComplete(void)
{
    delete _image;
    _image = nullptr;
    
    _appendStatusLog(tr("Upgrade complete"), true);
    _appendStatusLog("------------------------------------------", false);
    emit flashComplete();
    qgcApp()->toolbox()->linkManager()->setConnectionsAllowed();
}

void FirmwareUpgradeController::_error(const QString& errorString)
{
    delete _image;
    _image = nullptr;
    
    _errorCancel(QString("Error: %1").arg(errorString));
}

void FirmwareUpgradeController::_status(const QString& statusString)
{
    _appendStatusLog(statusString);
}

/// @brief Updates the progress bar from long running bootloader commands
void FirmwareUpgradeController::_updateProgress(int curr, int total)
{
    // Take care of cases where 0 / 0 is emitted as error return value
    if (total > 0) {
        _progressBar->setProperty("value", static_cast<float>(curr) / static_cast<float>(total));
    }
}

/// @brief Moves the progress bar ahead on tick while erasing the board
void FirmwareUpgradeController::_eraseProgressTick(void)
{
    _eraseTickCount++;
    _progressBar->setProperty("value", static_cast<float>(_eraseTickCount*_eraseTickMsec) / static_cast<float>(_eraseTotalMsec));
}

/// Appends the specified text to the status log area in the ui
void FirmwareUpgradeController::_appendStatusLog(const QString& text, bool critical)
{
    Q_ASSERT(_statusLog);
    
    QVariant returnedValue;
    QVariant varText;
    
    if (critical) {
        varText = QString("<font color=\"yellow\">%1</font>").arg(text);
    } else {
        varText = text;
    }
    
    QMetaObject::invokeMethod(_statusLog,
                              "append",
                              Q_RETURN_ARG(QVariant, returnedValue),
                              Q_ARG(QVariant, varText));
}

void FirmwareUpgradeController::_errorCancel(const QString& msg)
{
    _appendStatusLog(msg, false);
    _appendStatusLog(tr("Upgrade cancelled"), true);
    _appendStatusLog("------------------------------------------", false);
    emit error();
    cancel();
    qgcApp()->toolbox()->linkManager()->setConnectionsAllowed();
}

void FirmwareUpgradeController::_eraseStarted(void)
{
    // We set up our own progress bar for erase since the erase command does not provide one
    _eraseTickCount = 0;
    _eraseTimer.start(_eraseTickMsec);
}

void FirmwareUpgradeController::_eraseComplete(void)
{
    _eraseTimer.stop();
}

void FirmwareUpgradeController::setSelectedFirmwareBuildType(FirmwareBuildType_t firmwareType)
{
    _selectedFirmwareBuildType = firmwareType;
    emit selectedFirmwareBuildTypeChanged(_selectedFirmwareBuildType);
    _buildAPMFirmwareNames();
}

void FirmwareUpgradeController::_buildAPMFirmwareNames(void)
{
#if !defined(NO_ARDUPILOT_DIALECT)
    qCDebug(FirmwareUpgradeLog) << "_buildAPMFirmwareNames";

    bool                    chibios =       _apmChibiOSSetting->rawValue().toInt() == 0;
    FirmwareVehicleType_t   vehicleType =   static_cast<FirmwareVehicleType_t>(_apmVehicleTypeSetting->rawValue().toInt());

    _apmFirmwareNames.clear();
    _apmFirmwareUrls.clear();

    QString apmDescriptionSuffix("-BL");
    bool    bootloaderMatch = _foundBoardInfo.description().endsWith(apmDescriptionSuffix);

    for (const ManifestFirmwareInfo_t& firmwareInfo: _rgManifestFirmwareInfo) {
        bool match = false;
        if (firmwareInfo.firmwareBuildType == _selectedFirmwareBuildType && firmwareInfo.chibios == chibios && firmwareInfo.vehicleType == vehicleType) {
            if (bootloaderMatch) {
                if (firmwareInfo.rgBootloaderPortString.contains(_foundBoardInfo.description())) {
                    qCDebug(FirmwareUpgradeLog) << "Bootloader match:" << firmwareInfo.friendlyName << _foundBoardInfo.description() << firmwareInfo.rgBootloaderPortString << firmwareInfo.url << firmwareInfo.vehicleType;
                    match = true;
                }
            } else {
                if (firmwareInfo.rgVID.contains(_foundBoardInfo.vendorIdentifier()) && firmwareInfo.rgPID.contains(_foundBoardInfo.productIdentifier())) {
                    qCDebug(FirmwareUpgradeLog) << "Fallback match:" << firmwareInfo.friendlyName << _foundBoardInfo.vendorIdentifier() << _foundBoardInfo.productIdentifier() << _bootloaderBoardID << firmwareInfo.url << firmwareInfo.vehicleType;
                    match = true;
                }
            }
        }

        // Do a final filter on fmuv2/fmuv3
        if (match && _bootloaderBoardID == Bootloader::boardIDPX4FMUV3) {
            match = !firmwareInfo.fmuv2;
        }

        if (match) {
            _apmFirmwareNames.append(firmwareInfo.friendlyName);
            _apmFirmwareUrls.append(firmwareInfo.url);
        }
    }

    if (_apmFirmwareNames.count() > 1) {
        _apmFirmwareNames.prepend(tr("Choose board type"));
        _apmFirmwareUrls.prepend(QString());
    }

    emit apmFirmwareNamesChanged();
#endif
}

FirmwareUpgradeController::FirmwareVehicleType_t FirmwareUpgradeController::vehicleTypeFromFirmwareSelectionIndex(int index)
{
    if (index < 0 || index >= _apmVehicleTypeFromCurrentVersionList.count()) {
        qWarning() << "Invalid index, index:count" << index << _apmVehicleTypeFromCurrentVersionList.count();
        return CopterFirmware;
    }

    return _apmVehicleTypeFromCurrentVersionList[index];
}

void FirmwareUpgradeController::_determinePX4StableVersion(void)
{
    QGCFileDownload* downloader = new QGCFileDownload(this);
    connect(downloader, &QGCFileDownload::downloadFinished, this, &FirmwareUpgradeController::_px4ReleasesGithubDownloadFinished);
    connect(downloader, &QGCFileDownload::error, this, &FirmwareUpgradeController::_px4ReleasesGithubDownloadError);
    downloader->download(QStringLiteral("https://api.github.com/repos/PX4/Firmware/releases"));
}

void FirmwareUpgradeController::_px4ReleasesGithubDownloadFinished(QString remoteFile, QString localFile)
{
    Q_UNUSED(remoteFile);

    QFile jsonFile(localFile);
    if (!jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(FirmwareUpgradeLog) << "Unable to open github px4 releases json file" << localFile << jsonFile.errorString();
        return;
    }
    QByteArray bytes = jsonFile.readAll();
    jsonFile.close();

    QJsonParseError jsonParseError;
    QJsonDocument doc = QJsonDocument::fromJson(bytes, &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError) {
        qCWarning(FirmwareUpgradeLog) <<  "Unable to open px4 releases json document" << localFile << jsonParseError.errorString();
        return;
    }

    // Json should be an array of release objects
    if (!doc.isArray()) {
        qCWarning(FirmwareUpgradeLog) <<  "px4 releases json document is not an array" << localFile;
        return;
    }
    QJsonArray releases = doc.array();

    // The first release marked prerelease=false is stable
    // The first release marked prerelease=true is beta
    bool foundStable = false;
    bool foundBeta = false;
    for (int i=0; i<releases.count() && (!foundStable || !foundBeta); i++) {
        QJsonObject release = releases[i].toObject();
        if (!foundStable && !release["prerelease"].toBool()) {
            _px4StableVersion = release["name"].toString();
            emit px4StableVersionChanged(_px4StableVersion);
            qCDebug(FirmwareUpgradeLog()) << "Found px4 stable version" << _px4StableVersion;
            foundStable = true;
        } else if (!foundBeta && release["prerelease"].toBool()) {
            _px4BetaVersion = release["name"].toString();
            emit px4StableVersionChanged(_px4BetaVersion);
            qCDebug(FirmwareUpgradeLog()) << "Found px4 beta version" << _px4BetaVersion;
            foundBeta = true;
        }
    }

    if (!foundStable) {
        qCDebug(FirmwareUpgradeLog()) << "Unable to find px4 stable version" << localFile;
    }
    if (!foundBeta) {
        qCDebug(FirmwareUpgradeLog()) << "Unable to find px4 beta version" << localFile;
    }
}

void FirmwareUpgradeController::_px4ReleasesGithubDownloadError(QString errorMsg)
{
    qCWarning(FirmwareUpgradeLog) << "PX4 releases github download failed" << errorMsg;
}

void FirmwareUpgradeController::_downloadArduPilotManifest(void)
{
    _downloadingFirmwareList = true;
    emit downloadingFirmwareListChanged(true);

    QGCFileDownload* downloader = new QGCFileDownload(this);
    connect(downloader, &QGCFileDownload::downloadFinished, this, &FirmwareUpgradeController::_ardupilotManifestDownloadFinished);
    connect(downloader, &QGCFileDownload::error,            this, &FirmwareUpgradeController::_ardupilotManifestDownloadError);
#if 0
    downloader->download(QStringLiteral("http://firmware.ardupilot.org/manifest.json.gz"));
#else
    downloader->download(QStringLiteral("http://firmware.ardupilot.org/manifest.json"));
#endif
}

void FirmwareUpgradeController::_ardupilotManifestDownloadFinished(QString remoteFile, QString localFile)
{
    Q_UNUSED(remoteFile);

    // Delete the QGCFileDownload object
    sender()->deleteLater();

    qDebug() << "_ardupilotManifestDownloadFinished" << remoteFile << localFile;

#if 0
    QFile gzipFile(localFile);
    if (!gzipFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(FirmwareUpgradeLog) << "Unable to open ArduPilot firmware manifest file" << localFile << gzipFile.errorString();
        QFile::remove(localFile);
        return;
    }

    // Store decompressed size as first four bytes. This is required by qUncompress routine.
    QByteArray raw;
    int decompressedSize = 3073444;
    raw.append((unsigned char)((decompressedSize >> 24) & 0xFF));
    raw.append((unsigned char)((decompressedSize >> 16) & 0xFF));
    raw.append((unsigned char)((decompressedSize >> 8) & 0xFF));
    raw.append((unsigned char)((decompressedSize >> 0) & 0xFF));

    raw.append(gzipFile.readAll());
    QByteArray bytes = qUncompress(raw);
#else


    QFile jsonFile(localFile);
    if (!jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(FirmwareUpgradeLog) << "Unable to open ArduPilot firmware manifest file" << localFile << jsonFile.errorString();
        QFile::remove(localFile);
        return;
    }
    QByteArray bytes = jsonFile.readAll();
    jsonFile.close();
#endif
    QFile::remove(localFile);

    QJsonParseError jsonParseError;
    QJsonDocument doc = QJsonDocument::fromJson(bytes, &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError) {
        qCWarning(FirmwareUpgradeLog) <<  "Unable to open ArduPilot manifest json document" << localFile << jsonParseError.errorString();
    }




    QJsonObject json =          doc.object();
    QJsonArray  rgFirmware =    json[_manifestFirmwareJsonKey].toArray();

    for (int i=0; i<rgFirmware.count(); i++) {
        const QJsonObject& firmwareJson = rgFirmware[i].toObject();

        FirmwareVehicleType_t   firmwareVehicleType =   _manifestMavTypeToFirmwareVehicleType(firmwareJson[_manifestMavTypeJsonKey].toString());
        FirmwareBuildType_t     firmwareBuildType =     _manifestMavFirmwareVersionTypeToFirmwareBuildType(firmwareJson[_manifestMavFirmwareVersionTypeJsonKey].toString());
        QString                 format =                firmwareJson[_manifestFormatJsonKey].toString();
        QString                 platform =              firmwareJson[_manifestPlatformKey].toString();

        if (firmwareVehicleType != DefaultVehicleFirmware && firmwareBuildType != CustomFirmware && (format == QStringLiteral("apj") || format == QStringLiteral("px4"))) {
            if (platform.contains("-heli") && firmwareVehicleType != HeliFirmware) {
                continue;
            }

            _rgManifestFirmwareInfo.append(ManifestFirmwareInfo_t());
            ManifestFirmwareInfo_t& firmwareInfo = _rgManifestFirmwareInfo.last();

            firmwareInfo.boardId =              static_cast<uint32_t>(firmwareJson[_manifestBoardIdJsonKey].toInt());
            firmwareInfo.firmwareBuildType =    firmwareBuildType;
            firmwareInfo.vehicleType =          firmwareVehicleType;
            firmwareInfo.url =                  firmwareJson[_manifestUrlJsonKey].toString();
            firmwareInfo.version =              firmwareJson[_manifestMavFirmwareVersionJsonKey].toString();
            firmwareInfo.chibios =              format == QStringLiteral("apj");
            firmwareInfo.fmuv2 =                platform.contains(QStringLiteral("fmuv2"));

            QJsonArray bootloaderArray = firmwareJson[_manifestBootloaderStrJsonKey].toArray();
            for (int j=0; j<bootloaderArray.count(); j++) {
                firmwareInfo.rgBootloaderPortString.append(bootloaderArray[j].toString());
            }

            QJsonArray usbidArray = firmwareJson[_manifestUSBIDJsonKey].toArray();
            for (int j=0; j<usbidArray.count(); j++) {
                QStringList vidpid = usbidArray[j].toString().split('/');
                QString vid = vidpid[0];
                QString pid = vidpid[1];

                bool ok;
                firmwareInfo.rgVID.append(vid.right(vid.count() - 2).toInt(&ok, 16));
                firmwareInfo.rgPID.append(pid.right(pid.count() - 2).toInt(&ok, 16));
            }

            QString brandName = firmwareJson[_manifestBrandNameKey].toString();
            firmwareInfo.friendlyName = QStringLiteral("%1 - %2").arg(brandName.isEmpty() ? platform : brandName).arg(firmwareInfo.version);
        }
    }

    if (_bootloaderFound) {
        _buildAPMFirmwareNames();
    }

    _downloadingFirmwareList = false;
    emit downloadingFirmwareListChanged(false);
}

void FirmwareUpgradeController::_ardupilotManifestDownloadError(QString errorMsg)
{
    qCWarning(FirmwareUpgradeLog) << "ArduPilot Manifest download failed" << errorMsg;
}

FirmwareUpgradeController::FirmwareBuildType_t FirmwareUpgradeController::_manifestMavFirmwareVersionTypeToFirmwareBuildType(const QString& manifestMavFirmwareVersionType)
{
    if (_manifestMavFirmwareVersionTypeToFirmwareBuildTypeMap.contains(manifestMavFirmwareVersionType)) {
        return _manifestMavFirmwareVersionTypeToFirmwareBuildTypeMap[manifestMavFirmwareVersionType];
    } else {
        return CustomFirmware;
    }
}

FirmwareUpgradeController::FirmwareVehicleType_t FirmwareUpgradeController::_manifestMavTypeToFirmwareVehicleType(const QString& manifestMavType)
{
    if (_manifestMavTypeToFirmwareVehicleTypeMap.contains(manifestMavType)) {
        return _manifestMavTypeToFirmwareVehicleTypeMap[manifestMavType];
    } else {
        return DefaultVehicleFirmware;
    }
}
