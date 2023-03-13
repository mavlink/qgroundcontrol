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
#include "QGCApplication.h"
#include "QGCFileDownload.h"
#include "QGCOptions.h"
#include "QGCCorePlugin.h"
#include "FirmwareUpgradeSettings.h"
#include "SettingsManager.h"
#include "QGCZlib.h"
#include "JsonHelper.h"
#include "LinkManager.h"

#include <QStandardPaths>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkProxy>

#include "zlib.h"

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

// See PX4 Bootloader board_types.txt - https://raw.githubusercontent.com/PX4/PX4-Bootloader/master/board_types.txt
static QMap<int, QString> px4_board_name_map {
    {9, "px4_fmu-v2_default"},
    {255, "px4_fmu-v3_default"}, // Simulated board id for V3 which is a V2 board which supports larger flash space
    {11, "px4_fmu-v4_default"},
    {13, "px4_fmu-v4pro_default"},
    {20, "uvify_core_default"},
    {50, "px4_fmu-v5_default"},
    {51, "px4_fmu-v5x_default"},
    {52, "px4_fmu-v6_default"},
    {53, "px4_fmu-v6x_default"},
    {54, "px4_fmu-v6u_default"},
    {56, "px4_fmu-v6c_default"},
    {57, "ark_fmu-v6x_default"},
    {55, "sky-drones_smartap-airlink_default"},
    {88, "airmind_mindpx-v2_default"},
    {12, "bitcraze_crazyflie_default"},
    {14, "bitcraze_crazyflie21_default"},
    {42, "omnibus_f4sd_default"},
    {33, "mro_x21_default"},
    {65, "intel_aerofc-v1_default"},
    {123, "holybro_kakutef7_default"},
    {41775, "modalai_fc-v1_default"},
    {41776, "modalai_fc-v2_default"},
    {78, "holybro_pix32v5_default"},
    {79, "holybro_can-gps-v1_default"},
    {28, "nxp_fmuk66-v3_default"},
    {30, "nxp_fmuk66-e_default"},
    {31, "nxp_fmurt1062-v1_default"},
    {85, "freefly_can-rtk-gps_default"},
    {120, "cubepilot_cubeyellow_default"},
    {136, "mro_x21-777_default"},
    {139, "holybro_durandal-v1_default"},
    {140, "cubepilot_cubeorange_default"},
    {1063, "cubepilot_cubeorangeplus_default"},
    {141, "mro_ctrl-zero-f7_default"},
    {142, "mro_ctrl-zero-f7-oem_default"},
    {212, "thepeach_k1_default"},
    {213, "thepeach_r1_default"},
    {1009, "cuav_nora_default"},
    {1010, "cuav_x7pro_default"},
    {1017, "mro_pixracerpro_default"},
    {1023, "mro_ctrl-zero-h7_default"},
    {1024, "mro_ctrl-zero-h7-oem_default"},
    {1048, "holybro_kakuteh7_default"},
    {1053, "holybro_kakuteh7v2_default"},
    {1054, "holybro_kakuteh7mini_default"},
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
    , _selectedFirmwareBuildType        (StableFirmware)
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
    connect(_threadController, &PX4FirmwareUpgradeThreadController::foundBoardInfo,         this, &FirmwareUpgradeController::_foundBoardInfo);
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
    qCDebug(FirmwareUpgradeLog) << "FirmwareUpgradeController::flash stackType:firmwareType:vehicleType" << stackType << firmwareType << vehicleType;
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
    for (const auto& info : ports) {
        if(info.canFlash()) {
            info.getBoardInfo(boardType, boardName);
            names.append(boardName);
        }
    }

    return names;
}

void FirmwareUpgradeController::_foundBoard(bool firstAttempt, const QSerialPortInfo& info, int boardType, QString boardName)
{
    _boardInfo      = info;
    _boardType      = static_cast<QGCSerialPortInfo::BoardType_t>(boardType);
    _boardTypeName  = boardName;

    qDebug() << info.manufacturer() << info.description();

    _startFlashWhenBootloaderFound = false;

    if (_boardType == QGCSerialPortInfo::BoardTypeSiKRadio) {
        if (!firstAttempt) {
            // Radio always flashes latest firmware, so we can start right away without
            // any further user input.
            _startFlashWhenBootloaderFound = true;
            _startFlashWhenBootloaderFoundFirmwareIdentity = FirmwareIdentifier(SiKRadio,
                                                                                StableFirmware,
                                                                                DefaultVehicleFirmware);
        }
    }

    qCDebug(FirmwareUpgradeLog) << _boardType << _boardTypeName;
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
void FirmwareUpgradeController::_foundBoardInfo(int bootloaderVersion, int boardID, int flashSize)
{
    _bootloaderFound            = true;
    _bootloaderVersion          = static_cast<uint32_t>(bootloaderVersion);
    _bootloaderBoardID          = static_cast<uint32_t>(boardID);
    _bootloaderBoardFlashSize   = static_cast<uint32_t>(flashSize);

    _appendStatusLog(tr("Connected to bootloader:"));
    _appendStatusLog(tr("  Version: %1").arg(_bootloaderVersion));
    _appendStatusLog(tr("  Board ID: %1").arg(_bootloaderBoardID));
    _appendStatusLog(tr("  Flash size: %1").arg(_bootloaderBoardFlashSize));

    if (_startFlashWhenBootloaderFound) {
        flash(_startFlashWhenBootloaderFoundFirmwareIdentity);
    } else {
        if (_rgManifestFirmwareInfo.count()) {
            _buildAPMFirmwareNames();
        }
        emit showFirmwareSelectDlg();
    }
}


/// @brief intializes the firmware hashes with proper urls.
/// This happens only once for a class instance first time when it is needed.
void FirmwareUpgradeController::_initFirmwareHash()
{
    // indirect check whether this function has been called before or not
    // may have to be modified if _rgPX4FMUV2Firmware disappears
    if (!_rgPX4FLowFirmware.isEmpty()) {
        return;
    }

    /////////////////////////////// px4flow firmwares ///////////////////////////////////////
    FirmwareToUrlElement_t rgPX4FLowFirmwareArray[] = {
        { PX4FlowPX4, StableFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Flow/master/px4flow.px4" },
    #if !defined(NO_ARDUPILOT_DIALECT)
        { PX4FlowAPM, StableFirmware, DefaultVehicleFirmware, "http://firmware.ardupilot.org/Tools/PX4Flow/px4flow-klt-latest.px4" },
    #endif
    };

    // We build the maps for PX4 firmwares dynamically using the data below
    for (auto& element : rgPX4FLowFirmwareArray) {
        _rgPX4FLowFirmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
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
    case Bootloader::boardIDSiKRadio1000:
    {
        FirmwareToUrlElement_t element = { SiKRadio, StableFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/SiK/stable/radio~hm_trp.ihx" };
        _rgFirmwareDynamic.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }
        break;
    case Bootloader::boardIDSiKRadio1060:
    {
        FirmwareToUrlElement_t element = { SiKRadio, StableFirmware, DefaultVehicleFirmware, "https://px4-travis.s3.amazonaws.com/SiK/stable/radio~hb1060.ihx" };
        _rgFirmwareDynamic.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }
        break;
    default:
        if (px4_board_name_map.contains(boardId)) {
            const QString px4Url{"http://px4-travis.s3.amazonaws.com/Firmware/%1/%2.px4"};

            _rgFirmwareDynamic.insert(FirmwareIdentifier(AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware), px4Url.arg("stable").arg(px4_board_name_map.value(boardId)));
            _rgFirmwareDynamic.insert(FirmwareIdentifier(AutoPilotStackPX4, BetaFirmware,      DefaultVehicleFirmware), px4Url.arg("beta").arg(px4_board_name_map.value(boardId)));
            _rgFirmwareDynamic.insert(FirmwareIdentifier(AutoPilotStackPX4, DeveloperFirmware, DefaultVehicleFirmware), px4Url.arg("master").arg(px4_board_name_map.value(boardId)));
        }
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
    connect(downloader, &QGCFileDownload::downloadComplete, this, &FirmwareUpgradeController::_firmwareDownloadComplete);
    connect(downloader, &QGCFileDownload::downloadProgress, this, &FirmwareUpgradeController::_firmwareDownloadProgress);
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
void FirmwareUpgradeController::_firmwareDownloadComplete(QString /*remoteFile*/, QString localFile, QString errorMsg)
{
    if (errorMsg.isEmpty()) {
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
    } else {
        _errorCancel(errorMsg);
    }
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
    bool                    chibios =           _apmChibiOSSetting->rawValue().toInt() == 0;
    FirmwareVehicleType_t   vehicleType =       static_cast<FirmwareVehicleType_t>(_apmVehicleTypeSetting->rawValue().toInt());
    QString                 boardDescription =  _boardInfo.description();
    quint16                 boardVID =          _boardInfo.vendorIdentifier();
    quint16                 boardPID =          _boardInfo.productIdentifier();
    uint32_t                rawBoardId =        _bootloaderBoardID == Bootloader::boardIDPX4FMUV3 ? Bootloader::boardIDPX4FMUV2 : _bootloaderBoardID;

    if (_boardType == QGCSerialPortInfo::BoardTypePX4Flow) {
        return;
    }

    qCDebug(FirmwareUpgradeLog) << QStringLiteral("_buildAPMFirmwareNames description(%1) vid(%2/0x%3) pid(%4/0x%5)").arg(boardDescription).arg(boardVID).arg(boardVID, 1, 16).arg(boardPID).arg(boardPID, 1, 16);

    _apmFirmwareNames.clear();
    _apmFirmwareNamesBestIndex = -1;
    _apmFirmwareUrls.clear();

    QString apmDescriptionSuffix("-BL");
    QString boardDescriptionPrefix;
    bool    bootloaderMatch = boardDescription.endsWith(apmDescriptionSuffix);

    int currentIndex = 0;
    for (const ManifestFirmwareInfo_t& firmwareInfo: _rgManifestFirmwareInfo) {
        bool match = false;
        if (firmwareInfo.firmwareBuildType == _selectedFirmwareBuildType && firmwareInfo.chibios == chibios && firmwareInfo.vehicleType == vehicleType && firmwareInfo.boardId == rawBoardId) {
            if (firmwareInfo.fmuv2 && _bootloaderBoardID == Bootloader::boardIDPX4FMUV3) {
                qCDebug(FirmwareUpgradeLog) << "Skipping fmuv2 manifest entry for fmuv3 board:" << firmwareInfo.friendlyName << boardDescription << firmwareInfo.rgBootloaderPortString << firmwareInfo.url << firmwareInfo.vehicleType;
            } else {
                qCDebug(FirmwareUpgradeLog) << "Board id match:" << firmwareInfo.friendlyName << boardDescription << firmwareInfo.rgBootloaderPortString << firmwareInfo.url << firmwareInfo.vehicleType;
                match = true;
                if (bootloaderMatch && _apmFirmwareNamesBestIndex == -1 && firmwareInfo.rgBootloaderPortString.contains(boardDescription)) {
                    _apmFirmwareNamesBestIndex = currentIndex;
                    qCDebug(FirmwareUpgradeLog) << "Bootloader best match:" << firmwareInfo.friendlyName << boardDescription << firmwareInfo.rgBootloaderPortString << firmwareInfo.url << firmwareInfo.vehicleType;
                }
            }
        }

        if (match) {
            _apmFirmwareNames.append(firmwareInfo.friendlyName);
            _apmFirmwareUrls.append(firmwareInfo.url);
            currentIndex++;
        }
    }

    if (_apmFirmwareNamesBestIndex == -1) {
        _apmFirmwareNamesBestIndex++;
        if (_apmFirmwareNames.count() > 1) {
            _apmFirmwareNames.prepend(tr("Choose board type"));
            _apmFirmwareUrls.prepend(QString());
        }
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
    connect(downloader, &QGCFileDownload::downloadComplete, this, &FirmwareUpgradeController::_px4ReleasesGithubDownloadComplete);
    downloader->download(QStringLiteral("https://api.github.com/repos/PX4/Firmware/releases"));
}

void FirmwareUpgradeController::_px4ReleasesGithubDownloadComplete(QString /*remoteFile*/, QString localFile, QString errorMsg)
{
    if (errorMsg.isEmpty()) {
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
    } else {
        qCWarning(FirmwareUpgradeLog) << "PX4 releases github download failed" << errorMsg;
    }
}

void FirmwareUpgradeController::_downloadArduPilotManifest(void)
{
    _downloadingFirmwareList = true;
    emit downloadingFirmwareListChanged(true);

    QGCFileDownload* downloader = new QGCFileDownload(this);
    connect(downloader, &QGCFileDownload::downloadComplete, this, &FirmwareUpgradeController::_ardupilotManifestDownloadComplete);
    downloader->download(QStringLiteral("http://firmware.ardupilot.org/manifest.json.gz"));
}

void FirmwareUpgradeController::_ardupilotManifestDownloadComplete(QString remoteFile, QString localFile, QString errorMsg)
{
    if (errorMsg.isEmpty()) {
        // Delete the QGCFileDownload object
        sender()->deleteLater();

        qCDebug(FirmwareUpgradeLog) << "_ardupilotManifestDownloadFinished" << remoteFile << localFile;

        QString jsonFileName(QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).absoluteFilePath("ArduPilot.Manifest.json"));
        if (!QGCZlib::inflateGzipFile(localFile, jsonFileName)) {
            qCWarning(FirmwareUpgradeLog) << "Inflate of compressed manifest failed" << localFile;
            return;
        }

        QString         errorString;
        QJsonDocument   doc;
        if (!JsonHelper::isJsonFile(jsonFileName, doc, errorString)) {
            qCWarning(FirmwareUpgradeLog) << "Json file read failed" << errorString;
            return;
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
                firmwareInfo.chibios =              format == QStringLiteral("apj");                firmwareInfo.fmuv2 =                platform.contains(QStringLiteral("fmuv2"));

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
    } else {
        qCWarning(FirmwareUpgradeLog) << "ArduPilot Manifest download failed" << errorMsg;
    }
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
