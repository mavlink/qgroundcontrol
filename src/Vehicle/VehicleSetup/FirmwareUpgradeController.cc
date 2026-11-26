#include "FirmwareUpgradeController.h"

#include <algorithm>

#include <QtCore/QDir>

#include "Bootloader.h"
#include "Compression.h"
#include "Fact.h"
#include "FirmwareImage.h"
#include "FirmwarePluginManager.h"
#include "FirmwareUpgradeSettings.h"
#include "LinkManager.h"
#include "MultiVehicleManager.h"
#include "PX4FirmwareUpgradeThread.h"
#include "QGCCorePlugin.h"
#include "QGCFileDownload.h"
#include "QGCLoggingCategory.h"
#include "QGCOptions.h"
#include "SettingsManager.h"
#include "WorkerJob.h"

QGC_LOGGING_CATEGORY(FirmwareUpgradeControllerLog, "Vehicle.VehicleSetup.FirmwareUpgradeController")
#include <QtCore/QStandardPaths>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

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
    {59, "ark_fpv_default"},
    {35, "px4_fmu-v6xrt_default"},
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
    {37, "nxp_mr-tropic_default"},
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
    {1022, "mro_ctrl-zero-classic_default"},
    {1023, "mro_ctrl-zero-h7_default"},
    {1024, "mro_ctrl-zero-h7-oem_default"},
    {1048, "holybro_kakuteh7_default"},
    {1053, "holybro_kakuteh7v2_default"},
    {1054, "holybro_kakuteh7mini_default"},
    {1058, "holybro_kakuteh7mini_default"},
    {1105, "holybro_kakuteh7-wing_default"},
    {1110, "jfb_jfb110_default"},
    {1200, "jfb_jfb200_default"},
    {1209, "gearup_airbrainh743_default"},
    {1218, "amovlab_flycore_default"},
    {1198, "aedrox_aedroxh7_default"},
    {1123, "siyi_n7_default"},
    {1124, "3dr_ctrl-zero-h7-oem-revg_default"},
    {1949, "agam_fmu-v6xrt_default"},
    {5600, "zeroone_x6_default"},
    {6110, "svehicle_e2_default"},
    {7000, "cuav_7-nano_default"},
    {7001, "cuav_fmu-v6x_default"},
    {7002, "cuav_x25-evo_default"},
    {7003, "cuav_x25-super_default"},
    {7004, "cuav_x25-mega_default"},
    {7120, "accton-godwit_ga1_default"},
    {7122, "accton-godwit_gfh7_default"},
	{1183, "narinfc_h7_default"}
};

uint qHash(const FirmwareUpgradeController::FirmwareIdentifier& firmwareId)
{
    return static_cast<uint>(( firmwareId.autopilotStackType |
                               (firmwareId.firmwareType << 8) |
                               (firmwareId.firmwareVehicleType << 16) ));
}

/// @Brief Constructs a new FirmwareUpgradeController Widget. This widget is used within the PX4VehicleConfig set of screens.
FirmwareUpgradeController::FirmwareUpgradeController(void)
    : _singleFirmwareURL                (QGCCorePlugin::instance()->options()->firmwareUpgradeSingleURL())
    , _singleFirmwareMode               (!_singleFirmwareURL.isEmpty())
    , _downloadingFirmwareList          (false)
    , _statusLog                        (nullptr)
    , _selectedFirmwareBuildType        (StableFirmware)
    , _image                            (nullptr)
    , _apmBoardDescriptionReplaceText   ("<APMBoardDescription>")
    , _apmChibiOSSetting                (SettingsManager::instance()->firmwareUpgradeSettings()->apmChibiOS())
    , _apmVehicleTypeSetting            (SettingsManager::instance()->firmwareUpgradeSettings()->apmVehicleType())
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
    connect(_threadController, &PX4FirmwareUpgradeThreadController::portsAvailable,         this, &FirmwareUpgradeController::_portsAvailable);

    connect(&_eraseTimer, &QTimer::timeout, this, &FirmwareUpgradeController::_eraseProgressTick);

    // Only offer ArduPilot firmware when an ArduPilot plugin factory is registered
    if (FirmwarePluginManager::instance()->firmwareClassSupported(QGCMAVLink::FirmwareClassArduPilot)) {
        connect(_apmChibiOSSetting,     &Fact::rawValueChanged, this, &FirmwareUpgradeController::_buildAPMFirmwareNames);
        connect(_apmVehicleTypeSetting, &Fact::rawValueChanged, this, &FirmwareUpgradeController::_buildAPMFirmwareNames);
    }

    _determinePX4StableVersion();
}

FirmwareUpgradeController::~FirmwareUpgradeController()
{
    LinkManager::instance()->setConnectionsAllowed();
}

void FirmwareUpgradeController::startBoardSearch(void)
{
    LinkManager::instance()->setConnectionsSuspended(tr("Connect not allowed during Firmware Upgrade."));

    // FIXME: Why did we get here with active vehicle?
    if (!MultiVehicleManager::instance()->activeVehicle()) {
        // We have to disconnect any inactive links
        LinkManager::instance()->disconnectAll();
    }

    _bootloaderFound = false;
    _startFlashWhenBootloaderFound = false;
    _threadController->startFindBoardLoop();
}

void FirmwareUpgradeController::flash(AutoPilotStackType_t stackType,
                                      FirmwareBuildType_t firmwareType,
                                      FirmwareVehicleType_t vehicleType)
{
    qCDebug(FirmwareUpgradeControllerLog) << "FirmwareUpgradeController::flash stackType:firmwareType:vehicleType" << stackType << firmwareType << vehicleType;
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
    _flashCancelled = true;
    if (_activeDownloader) {
        _activeDownloader->cancel();
    }
    _threadController->cancel();
}

void FirmwareUpgradeController::flashPort(const QString& systemLocation)
{
    qCDebug(FirmwareUpgradeControllerLog) << "flashPort" << systemLocation;
    _bootloaderFound = false;
    _startFlashWhenBootloaderFound = false;
    _threadController->setTargetPort(systemLocation);
}

void FirmwareUpgradeController::_portsAvailable(const QVariantList& ports)
{
    _availablePorts = ports;
    emit availablePortsChanged();
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
    if (!boardName.isEmpty()) {
        _boardTypeName = boardName;
    } else if (!info.description().isEmpty()) {
        _boardTypeName = info.description();
    } else {
        _boardTypeName = info.portName();
    }

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

    qCDebug(FirmwareUpgradeControllerLog) << _boardType << _boardTypeName;
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
        if (_rgManifestFirmwareInfo.length()) {
            _buildAPMFirmwareNames();
        }
        emit showFirmwareSelectDlg();
    }
}

/// @brief Called when the findBootloader process is unable to sync to the bootloader. Moves the state
///         machine to the appropriate error state.
void FirmwareUpgradeController::_bootloaderSyncFailed(void)
{
    _errorCancel(tr("Unable to sync with bootloader."));
}

QHash<FirmwareUpgradeController::FirmwareIdentifier, QString>* FirmwareUpgradeController::_firmwareHashForBoardId(int boardId)
{
    _rgFirmwareDynamic.clear();

    switch (boardId) {
    case Bootloader::boardIDSiKRadio1000:
    {
        FirmwareToUrlElement_t element = { SiKRadio, StableFirmware, DefaultVehicleFirmware, "https://firmware.ardupilot.org/SiK/stable/radio~hm_trp.ihx" };
        _rgFirmwareDynamic.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }
        break;
    case Bootloader::boardIDSiKRadio1060:
    {
        FirmwareToUrlElement_t element = { SiKRadio, StableFirmware, DefaultVehicleFirmware, "https://firmware.ardupilot.org/SiK/stable/radio~hb1060.ihx" };
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
    if (_firmwareFilename.isEmpty()) {
        _errorCancel(tr("No firmware file selected"));
        return;
    }

    _flashCancelled = false;

    const bool isRemote = _firmwareFilename.startsWith(QStringLiteral("http"), Qt::CaseInsensitive);
    if (isRemote) {
        _appendStatusLog(tr("Downloading firmware from %1").arg(_firmwareFilename));
    } else {
        _appendStatusLog(tr("Using firmware file %1").arg(_firmwareFilename));
    }

    QGCFileDownload* downloader = new QGCFileDownload(this);
    _activeDownloader = downloader;
    connect(downloader, &QGCFileDownload::finished, downloader, &QObject::deleteLater);
    connect(downloader, &QGCFileDownload::finished, this, &FirmwareUpgradeController::_firmwareDownloadComplete);
    connect(downloader, &QGCFileDownload::downloadProgress, this, &FirmwareUpgradeController::_firmwareDownloadProgress);
    if (!downloader->start(_firmwareFilename)) {
        downloader->deleteLater();
        _activeDownloader = nullptr;
        _errorCancel(downloader->errorString());
        return;
    }
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
void FirmwareUpgradeController::_firmwareDownloadComplete(bool success, const QString &localFile, const QString &errorMsg)
{
    _activeDownloader = nullptr;

    if (_flashCancelled) {
        // User cancelled while the download was in flight; ignore late completion.
        return;
    }

    if (success) {
        const bool isRemote = _firmwareFilename.startsWith(QStringLiteral("http"), Qt::CaseInsensitive);
        if (isRemote) {
            _appendStatusLog(tr("Download complete"));
        }

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
    } else if (!errorMsg.isEmpty()) {
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
    emit flashComplete();
    LinkManager::instance()->setConnectionsAllowed();
}

void FirmwareUpgradeController::_error(const QString& errorString)
{
    delete _image;
    _image = nullptr;

    _errorCancel(tr("Error: %1").arg(errorString));
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
    if (!_statusLog) {
        return;
    }

    QString varText;

    if (critical) {
        varText = QString("<b>%1</b>").arg(text);
    } else {
        varText = text;
    }

    QMetaObject::invokeMethod(_statusLog,
                              "append",
                              Q_ARG(QString, varText));
}

void FirmwareUpgradeController::_errorCancel(const QString& msg)
{
    _appendStatusLog(msg, false);
    _appendStatusLog(tr("Upgrade cancelled"), true);
    emit error();
    cancel();
    LinkManager::instance()->setConnectionsAllowed();
}

void FirmwareUpgradeController::_eraseStarted(void)
{
    // We set up our own progress bar for erase since the erase command does not provide one
    _eraseTickCount = 0;
    _eraseTimer.start(_eraseTickMsec);
    emit eraseStarted();
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
    if (!FirmwarePluginManager::instance()->firmwareClassSupported(QGCMAVLink::FirmwareClassArduPilot)) {
        return;
    }

    bool                    chibios =           _apmChibiOSSetting->rawValue().toInt() == 0;
    FirmwareVehicleType_t   vehicleType =       static_cast<FirmwareVehicleType_t>(_apmVehicleTypeSetting->rawValue().toInt());
    QString                 boardDescription =  _boardInfo.description();
    quint16                 boardVID =          _boardInfo.vendorIdentifier();
    quint16                 boardPID =          _boardInfo.productIdentifier();
    uint32_t                rawBoardId =        _bootloaderBoardID == Bootloader::boardIDPX4FMUV3 ? Bootloader::boardIDPX4FMUV2 : _bootloaderBoardID;

    qCDebug(FirmwareUpgradeControllerLog) << QStringLiteral("_buildAPMFirmwareNames description(%1) vid(%2/0x%3) pid(%4/0x%5)").arg(boardDescription).arg(boardVID).arg(boardVID, 1, 16).arg(boardPID).arg(boardPID, 1, 16);

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
                qCDebug(FirmwareUpgradeControllerLog) << "Skipping fmuv2 manifest entry for fmuv3 board:" << firmwareInfo.friendlyName << boardDescription << firmwareInfo.rgBootloaderPortString << firmwareInfo.url << firmwareInfo.vehicleType;
            } else {
                qCDebug(FirmwareUpgradeControllerLog) << "Board id match:" << firmwareInfo.friendlyName << boardDescription << firmwareInfo.rgBootloaderPortString << firmwareInfo.url << firmwareInfo.vehicleType;
                match = true;
                if (bootloaderMatch && _apmFirmwareNamesBestIndex == -1 && firmwareInfo.rgBootloaderPortString.contains(boardDescription)) {
                    _apmFirmwareNamesBestIndex = currentIndex;
                    qCDebug(FirmwareUpgradeControllerLog) << "Bootloader best match:" << firmwareInfo.friendlyName << boardDescription << firmwareInfo.rgBootloaderPortString << firmwareInfo.url << firmwareInfo.vehicleType;
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
        if (_apmFirmwareNames.length() > 1) {
            _apmFirmwareNames.prepend(tr("Choose board type"));
            _apmFirmwareUrls.prepend(QString());
        }
    }

    emit apmFirmwareNamesChanged();
}

FirmwareUpgradeController::FirmwareVehicleType_t FirmwareUpgradeController::vehicleTypeFromFirmwareSelectionIndex(int index)
{
    if (index < 0 || index >= _apmVehicleTypeFromCurrentVersionList.length()) {
        qWarning() << "Invalid index, index:count" << index << _apmVehicleTypeFromCurrentVersionList.length();
        return CopterFirmware;
    }

    return _apmVehicleTypeFromCurrentVersionList[index];
}

void FirmwareUpgradeController::_determinePX4StableVersion(void)
{
    QGCFileDownload* downloader = new QGCFileDownload(this);
    connect(downloader, &QGCFileDownload::finished, downloader, &QObject::deleteLater);
    connect(downloader, &QGCFileDownload::finished, this, &FirmwareUpgradeController::_px4ReleasesGithubDownloadComplete);
    if (!downloader->start(QStringLiteral("https://api.github.com/repos/PX4/Firmware/releases"))) {
        qCWarning(FirmwareUpgradeControllerLog) << "PX4 releases github download did not start:" << downloader->errorString();
        downloader->deleteLater();
        return;
    }
}

void FirmwareUpgradeController::_px4ReleasesGithubDownloadComplete(bool success, const QString &localFile, const QString &errorMsg)
{
    if (success) {
        QFile jsonFile(localFile);
        if (!jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qCWarning(FirmwareUpgradeControllerLog) << "Unable to open github px4 releases json file" << localFile << jsonFile.errorString();
            return;
        }
        QByteArray bytes = jsonFile.readAll();
        jsonFile.close();

        QJsonParseError jsonParseError;
        QJsonDocument doc = QJsonDocument::fromJson(bytes, &jsonParseError);
        if (jsonParseError.error != QJsonParseError::NoError) {
            qCWarning(FirmwareUpgradeControllerLog) <<  "Unable to open px4 releases json document" << localFile << jsonParseError.errorString();
            return;
        }

        // Json should be an array of release objects
        if (!doc.isArray()) {
            qCWarning(FirmwareUpgradeControllerLog) <<  "px4 releases json document is not an array" << localFile;
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
                qCDebug(FirmwareUpgradeControllerLog()) << "Found px4 stable version" << _px4StableVersion;
                foundStable = true;
            } else if (!foundBeta && release["prerelease"].toBool()) {
                _px4BetaVersion = release["name"].toString();
                emit px4BetaVersionChanged(_px4BetaVersion);
                qCDebug(FirmwareUpgradeControllerLog()) << "Found px4 beta version" << _px4BetaVersion;
                foundBeta = true;
            }
        }

        if (!foundStable) {
            qCDebug(FirmwareUpgradeControllerLog()) << "Unable to find px4 stable version" << localFile;
        }
        if (!foundBeta) {
            qCDebug(FirmwareUpgradeControllerLog()) << "Unable to find px4 beta version" << localFile;
        }
    } else if (!errorMsg.isEmpty()) {
        qCWarning(FirmwareUpgradeControllerLog) << "PX4 releases github download failed" << errorMsg;
    }
}

void FirmwareUpgradeController::loadArduPilotFirmwareList()
{
    if (_downloadingFirmwareList || _arduPilotManifestRead ||
        !FirmwarePluginManager::instance()->firmwareClassSupported(QGCMAVLink::FirmwareClassArduPilot)) {
        return;
    }
    _downloadArduPilotManifest();
}

void FirmwareUpgradeController::_downloadArduPilotManifest(void)
{
    _downloadingFirmwareList = true;
    emit downloadingFirmwareListChanged(true);

    QGCFileDownload* downloader = new QGCFileDownload(this);
    connect(downloader, &QGCFileDownload::finished, downloader, &QObject::deleteLater);
    connect(downloader, &QGCFileDownload::finished, this, &FirmwareUpgradeController::_ardupilotManifestDownloadComplete);
    if (!downloader->start(QStringLiteral("https://firmware.ardupilot.org/manifest.json.gz"))) {
        qCWarning(FirmwareUpgradeControllerLog) << "ArduPilot Manifest download did not start:" << downloader->errorString();
        downloader->deleteLater();
        _downloadingFirmwareList = false;
        emit downloadingFirmwareListChanged(false);
    }
}

namespace {

/// Largest size of the ArduPilot firmware manifest, decompressed. It was 92 MiB in 2026, and grows with every board
/// and release: the limit leaves room for years of that, and keeps a damaged file from taking all memory
constexpr qint64 kMaxArduPilotManifestBytes = 512LL * 1024 * 1024;

}  // namespace

void FirmwareUpgradeController::_ardupilotManifestDownloadComplete(bool success, const QString &localFile, const QString &errorMsg)
{
    if (success) {
        qCDebug(FirmwareUpgradeControllerLog) << "_ardupilotManifestDownloadFinished" << localFile;

        // The manifest is far too large to read and parse on the GUI thread. A result that is no longer wanted, of an
        // earlier download, is dropped
        delete _manifestJob.data();
        const auto parse = [localFile, buildTypes = _manifestMavFirmwareVersionTypeToFirmwareBuildTypeMap,
                            vehicleTypes = _manifestMavTypeToFirmwareVehicleTypeMap]() {
            return _parseArduPilotManifest(localFile, buildTypes, vehicleTypes);
        };
        _manifestJob = QGC::runOnWorker(
            this, parse, [this](const ArduPilotManifest& manifest) { _ardupilotManifestParsed(manifest); });
    } else if (!errorMsg.isEmpty()) {
        qCWarning(FirmwareUpgradeControllerLog) << "ArduPilot Manifest download failed" << errorMsg;
        _clearDownloadingFirmwareList();
    }
}

void FirmwareUpgradeController::_ardupilotManifestParsed(const ArduPilotManifest& manifest)
{
    if (!manifest.errorString.isEmpty()) {
        qCWarning(FirmwareUpgradeControllerLog) << "Json file read failed" << manifest.errorString;
        _clearDownloadingFirmwareList();
        return;
    }

    _rgManifestFirmwareInfo = manifest.firmware;
    _arduPilotManifestRead = true;
    if (_bootloaderFound) {
        _buildAPMFirmwareNames();
    }
    _clearDownloadingFirmwareList();
}

void FirmwareUpgradeController::_clearDownloadingFirmwareList()
{
    if (_downloadingFirmwareList) {
        _downloadingFirmwareList = false;
        emit downloadingFirmwareListChanged(false);
    }
}

namespace {

// What the splitter of the ArduPilot manifest holds or follows at most: far more than the manifest needs
constexpr qsizetype kMaxManifestDepth = 64;
constexpr qsizetype kMaxManifestEntryBytes = 1024 * 1024;
constexpr qsizetype kMaxManifestKeyBytes = 256;
constexpr qsizetype kMaxManifestScalarBytes = 64;

bool isJsonSpace(char c)
{
    return (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r');
}

/// Whether @a c can be part of a number or of true, false and null
bool isJsonScalarChar(char c)
{
    return ((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) || (c == '-') ||
           (c == '+') || (c == '.');
}

bool isHexDigit(char c)
{
    return ((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'f')) || ((c >= 'A') && (c <= 'F'));
}

}  // namespace

FirmwareUpgradeController::ManifestSplitter::ManifestSplitter(ObjectHandler handler)
    : _handler(std::move(handler))
{}

bool FirmwareUpgradeController::ManifestSplitter::_fail(const QString& error)
{
    if (_errorString.isEmpty()) {
        _errorString = error;
    }
    return false;
}

bool FirmwareUpgradeController::ManifestSplitter::_failAt(const char* what, qsizetype index)
{
    return _fail(QStringLiteral("The firmware list is not valid JSON at byte %1: %2")
                     .arg(_offset + index)
                     .arg(QLatin1String(what)));
}

bool FirmwareUpgradeController::ManifestSplitter::feed(QByteArrayView chunk)
{
    if (!_errorString.isEmpty()) {
        return false;
    }
    // The manifest is 92 MiB, mostly strings and indentation: those are skipped in tight loops over the bytes
    const char* const data = chunk.data();
    const qsizetype size = chunk.size();
    qsizetype index = 0;
    while (index < size) {
        if (!_inString && !_inScalar) {
            while ((index < size) && isJsonSpace(data[index])) {
                ++index;
            }
            if (index == size) {
                break;
            }
        }
        const bool read =
            _inString ? _string(chunk, index) : (_inScalar ? _scalar(chunk, index) : _token(chunk, index));
        if (!read) {
            return false;
        }
    }
    if (_capturing) {
        _object.append(chunk.sliced(_captureFrom));
        if (_object.size() > kMaxManifestEntryBytes) {
            return _fail(
                QStringLiteral("An entry of the firmware list is larger than %1 bytes").arg(kMaxManifestEntryBytes));
        }
    }
    _captureFrom = 0;
    _offset += chunk.size();
    return true;
}

bool FirmwareUpgradeController::ManifestSplitter::finish()
{
    if (!_errorString.isEmpty()) {
        return false;
    }
    if (_inString || _inScalar || !_containers.isEmpty() || (_expect != Expect::Nothing)) {
        return _fail(QStringLiteral("The firmware list ends at byte %1, before its end").arg(_offset));
    }
    if (!_firmwareDone) {
        return _fail(QStringLiteral("The manifest has no firmware list"));
    }
    return true;
}

bool FirmwareUpgradeController::ManifestSplitter::_token(QByteArrayView chunk, qsizetype& index)
{
    const char c = chunk.data()[index];
    const bool valueExpected = (_expect == Expect::Value) || (_expect == Expect::ValueOrEnd);
    switch (c) {
        case '"':
            if ((_expect == Expect::Key) || (_expect == Expect::KeyOrEnd)) {
                _stringIsKey = true;
            } else if (valueExpected && _valueStarts(c, index)) {
                _stringIsKey = false;
            } else {
                return _failAt("a string where it cannot be", index);
            }
            _inString = true;
            ++index;
            return true;
        case '{':
        case '[':
            if (!valueExpected || !_valueStarts(c, index)) {
                return _failAt("a bracket where it cannot be", index);
            }
            return _open(c, index);
        case '}':
        case ']':
            return _close(c, chunk, index);
        case ':':
            if (_expect != Expect::Colon) {
                return _failAt("a colon where it cannot be", index);
            }
            _expect = Expect::Value;
            ++index;
            return true;
        case ',':
            if (_expect != Expect::CommaOrEnd) {
                return _failAt("a comma where it cannot be", index);
            }
            _expect = (_containers.back() == '{') ? Expect::Key : Expect::Value;
            ++index;
            return true;
        default:
            break;
    }
    if (!isJsonScalarChar(c) || !valueExpected || !_valueStarts(c, index)) {
        return _failAt("a character where it cannot be", index);
    }
    _inScalar = true;
    _scalarText.clear();
    return _scalar(chunk, index);
}

bool FirmwareUpgradeController::ManifestSplitter::_valueStarts(char first, qsizetype index)
{
    if (_containers.isEmpty() && (first != '{')) {
        return _failAt("the manifest is not an object", index);
    }
    if ((_containers.size() != 1) || !_firmwareKey) {
        return true;
    }
    _firmwareKey = false;
    if (first != '[') {
        return _failAt("the firmware list is not an array", index);
    }
    if (_firmwareFound) {
        return _failAt("a second firmware list", index);
    }
    _firmwareFound = true;
    _firmwareOpen = true;
    return true;
}

bool FirmwareUpgradeController::ManifestSplitter::_string(QByteArrayView chunk, qsizetype& index)
{
    const char* const data = chunk.data();
    const qsizetype size = chunk.size();
    while (index < size) {
        if ((_hexDigits == 0) && !_escape) {
            // Skip to what ends the string or needs a look
            const qsizetype start = index;
            while ((index < size) && (data[index] != '"') && (data[index] != '\\') &&
                   (static_cast<unsigned char>(data[index]) >= 0x20)) {
                ++index;
            }
            if (_stringIsKey && (_key.size() <= kMaxManifestKeyBytes)) {
                _key.append(data + start, std::min(index - start, kMaxManifestKeyBytes + 1 - _key.size()));
            }
            if (index == size) {
                return true;
            }
        }
        const char c = data[index];
        if (_hexDigits > 0) {
            if (!isHexDigit(c)) {
                return _failAt("an escape that is not four hex digits", index);
            }
            --_hexDigits;
        } else if (_escape) {
            _escape = false;
            if (c == 'u') {
                _hexDigits = 4;
            } else if (!QByteArrayView("\"\\/bfnrt").contains(c)) {
                return _failAt("an escape that JSON does not have", index);
            }
        } else if (c == '\\') {
            _escape = true;
        } else if (c == '"') {
            ++index;
            _inString = false;
            return _endString();
        } else {
            return _failAt("a control character in a string", index);
        }
        if (_stringIsKey && (_key.size() <= kMaxManifestKeyBytes)) {
            _key.append(c);
        }
        ++index;
    }
    return true;
}

bool FirmwareUpgradeController::ManifestSplitter::_endString()
{
    if (!_stringIsKey) {
        _valueDone();
        return true;
    }
    if (_containers.size() == 1) {
        QString key;
        if (_key.size() > kMaxManifestKeyBytes) {
            key.clear();
        } else if (_key.contains('\\')) {
            key = QJsonDocument::fromJson("[\"" + _key + "\"]").array().at(0).toString();
        } else {
            key = QString::fromUtf8(_key);
        }
        _firmwareKey = (key == QLatin1String(_manifestFirmwareJsonKey));
    }
    _key.clear();
    _expect = Expect::Colon;
    return true;
}

bool FirmwareUpgradeController::ManifestSplitter::_scalar(QByteArrayView chunk, qsizetype& index)
{
    const char* const data = chunk.data();
    const qsizetype start = index;
    while ((index < chunk.size()) && isJsonScalarChar(data[index])) {
        ++index;
    }
    if (_scalarText.size() <= kMaxManifestScalarBytes) {
        _scalarText.append(data + start, std::min(index - start, kMaxManifestScalarBytes + 1 - _scalarText.size()));
    }
    if (index == chunk.size()) {
        return true;
    }
    // What ends the number or the literal is read as a token next
    _inScalar = false;
    if (_scalarText.size() > kMaxManifestScalarBytes) {
        return _failAt("a number that is too long", index);
    }
    const char first = _scalarText.at(0);
    const bool literal = (_scalarText == "true") || (_scalarText == "false") || (_scalarText == "null");
    const bool number =
        ((first == '-') || ((first >= '0') && (first <= '9'))) &&
        std::all_of(_scalarText.cbegin(), _scalarText.cend(), [](char c) {
            return ((c >= '0') && (c <= '9')) || (c == '-') || (c == '+') || (c == '.') || (c == 'e') || (c == 'E');
        });
    if (!literal && !number) {
        return _failAt("a value that is not one", index);
    }
    _valueDone();
    return true;
}

bool FirmwareUpgradeController::ManifestSplitter::_open(char bracket, qsizetype& index)
{
    if (_containers.size() >= kMaxManifestDepth) {
        return _failAt("nested too deeply", index);
    }
    if ((bracket == '{') && _firmwareOpen && (_containers.size() == 2)) {
        _capturing = true;
        _captureFrom = index;
        _object.clear();
    }
    _containers.append(bracket);
    _expect = (bracket == '{') ? Expect::KeyOrEnd : Expect::ValueOrEnd;
    ++index;
    return true;
}

bool FirmwareUpgradeController::ManifestSplitter::_close(char bracket, QByteArrayView chunk, qsizetype& index)
{
    const bool object = (bracket == '}');
    const bool closes = object ? ((_expect == Expect::KeyOrEnd) || (_expect == Expect::CommaOrEnd))
                               : ((_expect == Expect::ValueOrEnd) || (_expect == Expect::CommaOrEnd));
    if (!closes || _containers.isEmpty() || (_containers.back() != (object ? '{' : '['))) {
        return _failAt("a bracket that closes nothing open", index);
    }
    _containers.chop(1);
    if (_capturing && (_containers.size() == 2)) {
        _capturing = false;
        const QByteArrayView end = chunk.sliced(_captureFrom, index + 1 - _captureFrom);
        bool taken = false;
        if (_object.isEmpty()) {
            taken = _handler(end);
        } else {
            _object.append(end);
            taken = (_object.size() <= kMaxManifestEntryBytes) && _handler(_object);
            _object.clear();
        }
        if (!taken) {
            return _failAt("an entry of the firmware list that cannot be read", index);
        }
    }
    if (_firmwareOpen && (_containers.size() == 1)) {
        _firmwareOpen = false;
        _firmwareDone = true;
    }
    ++index;
    _valueDone();
    return true;
}

void FirmwareUpgradeController::ManifestSplitter::_valueDone()
{
    _expect = _containers.isEmpty() ? Expect::Nothing : Expect::CommaOrEnd;
}

FirmwareUpgradeController::ArduPilotManifest FirmwareUpgradeController::_parseArduPilotManifest(
    const QString& localFile, const QMap<QString, FirmwareBuildType_t>& buildTypes,
    const QMap<QString, FirmwareVehicleType_t>& vehicleTypes)
{
    ArduPilotManifest manifest;
    QList<ManifestFirmwareInfo_t> firmware;
    QString entryError;
    ManifestSplitter splitter([&](QByteArrayView object) {
        // QGC keeps an entry only if its format is "apj" or "px4". As text, such an entry holds either string, or an
        // escape that may spell it: the others are not parsed, which is most of the manifest
        if (!object.contains("\"apj\"") && !object.contains("\"px4\"") && !object.contains("\\u")) {
            return true;
        }
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(object.toByteArray(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            entryError =
                QStringLiteral("An entry of the firmware list is not valid JSON: %1").arg(parseError.errorString());
            return false;
        }
        if (std::optional<ManifestFirmwareInfo_t> info =
                _manifestFirmwareInfo(document.object(), buildTypes, vehicleTypes)) {
            firmware.append(std::move(*info));
        }
        return true;
    });

    // Compressed with gzip. A manifest that is cut or damaged fails here, after entries were taken: they are dropped
    const Compression::Outcome read = Compression::readFileChunks(
        localFile, kMaxArduPilotManifestBytes, [&splitter](QByteArrayView chunk) { return splitter.feed(chunk); });
    if (!read || !splitter.finish()) {
        if (!entryError.isEmpty()) {
            manifest.errorString = entryError;
        } else if (!read && (read.error != Compression::Error::Cancelled)) {
            manifest.errorString = read.description();
        } else {
            manifest.errorString = splitter.errorString();
        }
        return manifest;
    }

    manifest.firmware = std::move(firmware);
    return manifest;
}

std::optional<FirmwareUpgradeController::ManifestFirmwareInfo_t> FirmwareUpgradeController::_manifestFirmwareInfo(
    const QJsonObject& firmwareJson, const QMap<QString, FirmwareBuildType_t>& buildTypes,
    const QMap<QString, FirmwareVehicleType_t>& vehicleTypes)
{
    FirmwareVehicleType_t firmwareVehicleType =
        vehicleTypes.value(firmwareJson[_manifestMavTypeJsonKey].toString(), DefaultVehicleFirmware);
    FirmwareBuildType_t firmwareBuildType =
        buildTypes.value(firmwareJson[_manifestMavFirmwareVersionTypeJsonKey].toString(), CustomFirmware);
    QString format = firmwareJson[_manifestFormatJsonKey].toString();
    QString platform = firmwareJson[_manifestPlatformKey].toString();

    if (firmwareVehicleType == DefaultVehicleFirmware || firmwareBuildType == CustomFirmware ||
        (format != QStringLiteral("apj") && format != QStringLiteral("px4"))) {
        return std::nullopt;
    }
    if (platform.contains("-heli") && firmwareVehicleType != HeliFirmware) {
        return std::nullopt;
    }

    ManifestFirmwareInfo_t firmwareInfo;
    firmwareInfo.boardId = static_cast<uint32_t>(firmwareJson[_manifestBoardIdJsonKey].toInt());
    firmwareInfo.firmwareBuildType = firmwareBuildType;
    firmwareInfo.vehicleType = firmwareVehicleType;
    firmwareInfo.url = firmwareJson[_manifestUrlJsonKey].toString();
    firmwareInfo.version = firmwareJson[_manifestMavFirmwareVersionJsonKey].toString();
    firmwareInfo.chibios = format == QStringLiteral("apj");
    firmwareInfo.fmuv2 = platform.contains(QStringLiteral("fmuv2"));

    QJsonArray bootloaderArray = firmwareJson[_manifestBootloaderStrJsonKey].toArray();
    for (int j = 0; j < bootloaderArray.count(); j++) {
        firmwareInfo.rgBootloaderPortString.append(bootloaderArray[j].toString());
    }

    QJsonArray usbidArray = firmwareJson[_manifestUSBIDJsonKey].toArray();
    for (int j = 0; j < usbidArray.count(); j++) {
        QStringList vidpid = usbidArray[j].toString().split('/');
        if (vidpid.size() < 2) {
            continue;
        }
        QString vid = vidpid[0];
        QString pid = vidpid[1];

        bool ok;
        firmwareInfo.rgVID.append(vid.right(vid.length() - 2).toInt(&ok, 16));
        firmwareInfo.rgPID.append(pid.right(pid.length() - 2).toInt(&ok, 16));
    }

    QString brandName = firmwareJson[_manifestBrandNameKey].toString();
    firmwareInfo.friendlyName =
        QStringLiteral("%1 - %2").arg(brandName.isEmpty() ? platform : brandName, firmwareInfo.version);
    return firmwareInfo;
}
