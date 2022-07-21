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

    _downloadPX4Manifest();
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
        _downloadFirmware(_firmwareFilename);
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
        // Prepare the list of APM Firmware names if we already have the manifest info list updated
        if (_rgArdupilotManifestFirmwareInfo.count()) {
            _buildAPMFirmwareNames();
        }
        // Update the Build Variants list for the detected board
        _updatePX4BuildVariantsList();

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

QHash<FirmwareUpgradeController::FirmwareIdentifier, QString>* FirmwareUpgradeController::_px4FirmwareHashForBoardId(int boardId)
{
    _rgPX4FirmwareDynamic.clear();

    switch (boardId) {
    case Bootloader::boardIDPX4Flow:
        _rgPX4FirmwareDynamic = _rgPX4FLowFirmware;
        break;
    case Bootloader::boardIDSiKRadio1000:
    {
        FirmwareToUrlElement_t element = { SiKRadio, StableFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/SiK/stable/radio~hm_trp.ihx" };
        _rgPX4FirmwareDynamic.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }
        break;
    case Bootloader::boardIDSiKRadio1060:
    {
        FirmwareToUrlElement_t element = { SiKRadio, StableFirmware, DefaultVehicleFirmware, "https://px4-travis.s3.amazonaws.com/SiK/stable/radio~hb1060.ihx" };
        _rgPX4FirmwareDynamic.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }
        break;
    default:
        if (_px4_board_id_2_target_name.contains(boardId)) {
            const QString px4Url{"http://px4-travis.s3.amazonaws.com/Firmware/%1/%2.px4"};

            _rgPX4FirmwareDynamic.insert(FirmwareIdentifier(AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware), px4Url.arg("stable").arg(_px4_board_id_2_target_name.value(boardId)));
            _rgPX4FirmwareDynamic.insert(FirmwareIdentifier(AutoPilotStackPX4, BetaFirmware,      DefaultVehicleFirmware), px4Url.arg("beta").arg(_px4_board_id_2_target_name.value(boardId)));
            _rgPX4FirmwareDynamic.insert(FirmwareIdentifier(AutoPilotStackPX4, DeveloperFirmware, DefaultVehicleFirmware), px4Url.arg("master").arg(_px4_board_id_2_target_name.value(boardId)));
        }
        break;
    }

    return &_rgPX4FirmwareDynamic;
}

void FirmwareUpgradeController::_getFirmwareFile(FirmwareIdentifier firmwareId)
{
    // Get Firmware download URL
    QHash<FirmwareIdentifier, QString>* prgFirmware = _px4FirmwareHashForBoardId(static_cast<int>(_bootloaderBoardID));
    
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
        _downloadFirmware(_firmwareFilename);
    }
}

/// @brief Begins the process of downloading the selected firmware file.
void FirmwareUpgradeController::_downloadFirmware(const QString firmwareFileName)
{
    Q_ASSERT(!firmwareFileName.isEmpty());
    
    _appendStatusLog(tr("Downloading firmware..."));
    _appendStatusLog(tr(" From: %1").arg(firmwareFileName));
    
    QGCFileDownload* downloader = new QGCFileDownload(this);
    connect(downloader, &QGCFileDownload::downloadComplete, this, &FirmwareUpgradeController::_firmwareDownloadComplete);
    connect(downloader, &QGCFileDownload::downloadProgress, this, &FirmwareUpgradeController::_firmwareDownloadProgress);
    
    downloader->download(firmwareFileName);
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
    for (const ArdupilotManifestFirmwareInfo_t& firmwareInfo: _rgArdupilotManifestFirmwareInfo) {
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
    downloader->download(QStringLiteral("https://api.github.com/repos/PX4/PX4-Autopilot/releases"));
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

        bool foundStable = false;
        bool foundBeta = false;
        for (int i=0; i<releases.count() && (!foundStable || !foundBeta); i++) {
            QJsonObject release = releases[i].toObject();
            // The first release marked prerelease=false is stable
            if (!foundStable && !release["prerelease"].toBool()) {
                _px4StableVersion = release["name"].toString();
                emit px4StableVersionChanged(_px4StableVersion);
                qCDebug(FirmwareUpgradeLog()) << "Found px4 stable version" << _px4StableVersion;
                foundStable = true;
            
            // The first release marked prerelease=true is beta
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

void FirmwareUpgradeController::_downloadPX4Manifest(void)
{
    _downloadingFirmwareList = true;
    emit downloadingFirmwareListChanged(true);

    QGCFileDownload* downloader = new QGCFileDownload(this);
    connect(downloader, &QGCFileDownload::downloadComplete, this, &FirmwareUpgradeController::_PX4ManifestDownloadComplete);
    downloader->download(QStringLiteral("https://raw.githubusercontent.com/junwoo091400/junwoo091400.github.io/main/files_dumpyard/px4_board_information.json"));
}

void FirmwareUpgradeController::_PX4ManifestDownloadComplete(QString remoteFile, QString localFile, QString errorMsg)
{
    if (!errorMsg.isEmpty()) {
        qCWarning(FirmwareUpgradeLog) << "PX4 Manifest download failed" << errorMsg;
    }

    sender()->deleteLater(); // Delete the QGCFileDownload object
    qCDebug(FirmwareUpgradeLog) << "PX4 Board information Manifest Download Finished" << remoteFile << localFile;
    
    QString         errorString;
    QJsonDocument   doc;
    
    if (!JsonHelper::isJsonFile(localFile, doc, errorString)) {
        qCWarning(FirmwareUpgradeLog) << "Json file read failed" << errorString;
        return;
    }

    // Parse the document and get the JSON object
    QJsonObject json = doc.object();

    // Read in board informations of each board target
    QJsonArray boardInfoArray = json[_px4ManifestBoardInfoJsonKey].toArray();

    for (int board_idx = 0; board_idx < boardInfoArray.count(); board_idx++) {
        PX4Manifest_SingleBoardInfo_t boardInfoUnit = PX4Manifest_SingleBoardInfo_t();
        const QJsonObject& boardInfoUnitJson = boardInfoArray[board_idx].toObject();

        // Add the basic board information
        boardInfoUnit.boardName = boardInfoUnitJson[_px4ManifestBoardNameJsonKey].toString();
        boardInfoUnit.targetName = boardInfoUnitJson[_px4ManifestTargetNameJsonKey].toString();
        boardInfoUnit.description = boardInfoUnitJson[_px4ManifestDescriptionJsonKey].toString();
        boardInfoUnit.boardID = boardInfoUnitJson[_px4ManifestBoardIDJsonKey].toInt();

        // Add in the list of build variants
        QJsonArray buildVariantsArray = boardInfoUnitJson[_px4ManifestBuildVariantsJsonKey].toArray();
        for (int build_variant_idx = 0; build_variant_idx < buildVariantsArray.count(); build_variant_idx++) {
            QJsonObject build_variant = buildVariantsArray[build_variant_idx].toObject();
            // Build Variant's name is encapsulated under an extra layer of Json key "name"
            boardInfoUnit.buildVariantNames.append(build_variant[_px4ManifestBuildVariantNameJsonKey].toString());
        }

        // Add optional USB Autoconnect info
        boardInfoUnit.productID = boardInfoUnitJson[_px4ManifestProductIDJsonKey].toInt();
        qInfo() << "Adding Product ID " << boardInfoUnit.productID << " to board " << boardInfoUnit.boardName;
        boardInfoUnit.productName = boardInfoUnitJson[_px4ManifestProductNameJsonKey].toString();
        boardInfoUnit.vendorID = boardInfoUnitJson[_px4ManifestVendorIDJsonKey].toInt();
        boardInfoUnit.vendorName = boardInfoUnitJson[_px4ManifestVendorNameJsonKey].toString();

        // Add the board info into the list
        _px4BoardManifest.boards.append(boardInfoUnit);

        // Update the Board-ID <-> Target Name mapping
        _px4_board_id_2_target_name[boardInfoUnit.boardID] = boardInfoUnit.targetName;
    }

    // Read in binary URLs (that specifies where to download firmware files from)
    QJsonObject binaryUrls = json[_px4ManifestBinaryUrlsJsonKey].toObject();
    foreach(const QString& key, binaryUrls.keys()) {
        _px4BoardManifest.binary_urls[key] = binaryUrls[key].toString();
    }

    // If we already have the bootloader of the board found, update Build Variants List
    if (_bootloaderFound) {
        _updatePX4BuildVariantsList();
    }

    _downloadingFirmwareList = false;
    emit downloadingFirmwareListChanged(false);
}

void FirmwareUpgradeController::_updatePX4BuildVariantsList(void)
{
    qInfo() << "Updating PX4 Build Variants";

    if (_bootloaderFound) {
        const uint16_t board_vid = _boardInfo.vendorIdentifier();
        const uint16_t board_pid = _boardInfo.productIdentifier();

        foreach(const PX4Manifest_SingleBoardInfo_t &boardinfo, _px4BoardManifest.boards) {
            if (boardinfo.boardID == _bootloaderBoardID) {
                // Found the matching board in the manifest. Update the variants list
                qInfo() << "Found the board that has a matching bootloader ID";
                qInfo() << boardinfo.boardName;

                if (board_vid == boardinfo.vendorID && board_pid == boardinfo.productID) {
                    qInfo() << "Vendor ID and Product ID matches as well! Updating variants list...";
                    // Set the BuildVariants list
                    _px4FirmwareBuildVariants = QStringList(boardinfo.buildVariantNames);

                    // Set the selected index to the "default" build variant, but if not fallback to -1
                    _px4FirmwareBuildVariantSelectedIdx = _px4FirmwareBuildVariants.indexOf("default");

                    // Emit the signal so that the Build Variants display QML Combo box would get updated
                    emit px4FirmwareBuildVariantsChanged();
                    return;
                
                } else {
                    qInfo() << "but the Vendor ID or product ID are different, not this board!";
                    qInfo() << "Detected VID,PID : " << board_vid << ", " << board_pid;
                    qInfo() << "Board Manifest VID,PID : " << boardinfo.vendorID << ", " << boardinfo.productID;
                }
            }
        }
    }
}

// Ardupilot Manifest code

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
        QJsonArray  rgFirmware =    json[_ardupilotManifestFirmwareJsonKey].toArray();

        for (int i=0; i<rgFirmware.count(); i++) {
            const QJsonObject& firmwareJson = rgFirmware[i].toObject();

            FirmwareVehicleType_t   firmwareVehicleType =   _manifestMavTypeToFirmwareVehicleType(firmwareJson[_ardupilotManifestMavTypeJsonKey].toString());
            FirmwareBuildType_t     firmwareBuildType =     _manifestMavFirmwareVersionTypeToFirmwareBuildType(firmwareJson[_ardupilotManifestMavFirmwareVersionTypeJsonKey].toString());
            QString                 format =                firmwareJson[_ardupilotManifestFormatJsonKey].toString();
            QString                 platform =              firmwareJson[_ardupilotManifestPlatformKey].toString();

            if (firmwareVehicleType != DefaultVehicleFirmware && firmwareBuildType != CustomFirmware && (format == QStringLiteral("apj") || format == QStringLiteral("px4"))) {
                if (platform.contains("-heli") && firmwareVehicleType != HeliFirmware) {
                    continue;
                }

                _rgArdupilotManifestFirmwareInfo.append(ArdupilotManifestFirmwareInfo_t());
                ArdupilotManifestFirmwareInfo_t& firmwareInfo = _rgArdupilotManifestFirmwareInfo.last();

                firmwareInfo.boardId =              static_cast<uint32_t>(firmwareJson[_ardupilotManifestBoardIDJsonKey].toInt());
                firmwareInfo.firmwareBuildType =    firmwareBuildType;
                firmwareInfo.vehicleType =          firmwareVehicleType;
                firmwareInfo.url =                  firmwareJson[_ardupilotManifestUrlJsonKey].toString();
                firmwareInfo.version =              firmwareJson[_ardupilotManifestMavFirmwareVersionJsonKey].toString();
                firmwareInfo.chibios =              format == QStringLiteral("apj");                firmwareInfo.fmuv2 =                platform.contains(QStringLiteral("fmuv2"));

                QJsonArray bootloaderArray = firmwareJson[_ardupilotManifestBootloaderStrJsonKey].toArray();
                for (int j=0; j<bootloaderArray.count(); j++) {
                    firmwareInfo.rgBootloaderPortString.append(bootloaderArray[j].toString());
                }

                QJsonArray usbidArray = firmwareJson[_ardupilotManifestUSBIDJsonKey].toArray();
                for (int j=0; j<usbidArray.count(); j++) {
                    QStringList vidpid = usbidArray[j].toString().split('/');
                    QString vid = vidpid[0];
                    QString pid = vidpid[1];

                    bool ok;
                    firmwareInfo.rgVID.append(vid.right(vid.count() - 2).toInt(&ok, 16));
                    firmwareInfo.rgPID.append(pid.right(pid.count() - 2).toInt(&ok, 16));
                }

                QString brandName = firmwareJson[_ardupilotManifestBrandNameKey].toString();
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
