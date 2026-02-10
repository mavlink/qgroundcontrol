#include "FirmwareUpgradeController.h"

#include <QtCore/QDir>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QStandardPaths>

#include "Bootloader.h"
#include "Fact.h"
#include "FirmwareImage.h"
#include "FirmwareUpgradeSettings.h"
#include "JsonHelper.h"
#include "LinkManager.h"
#include "MultiVehicleManager.h"
#include "PX4FirmwareUpgradeThread.h"
#include "QGCApplication.h"
#include "QGCCorePlugin.h"
#include "QGCFileDownload.h"
#include "QGCLoggingCategory.h"
#include "QGCOptions.h"
#include "SettingsManager.h"

struct FirmwareToUrlElement_t
{
    FirmwareUpgradeController::AutoPilotStackType_t stackType;
    FirmwareUpgradeController::FirmwareBuildType_t firmwareType;
    FirmwareUpgradeController::FirmwareVehicleType_t vehicleType;
    QString url;
};

// PX4 manifest URL
static constexpr const char* _px4ManifestUrl = "https://px4-travis.s3.amazonaws.com/Firmware/manifest.json";

uint qHash(const FirmwareUpgradeController::FirmwareIdentifier& firmwareId)
{
    return static_cast<uint>(
        (firmwareId.autopilotStackType | (firmwareId.firmwareType << 8) | (firmwareId.firmwareVehicleType << 16)));
}

/// @Brief Constructs a new FirmwareUpgradeController Widget. This widget is used within the PX4VehicleConfig set of
/// screens.
FirmwareUpgradeController::FirmwareUpgradeController(void)
    : _singleFirmwareURL(QGCCorePlugin::instance()->options()->firmwareUpgradeSingleURL()),
      _singleFirmwareMode(!_singleFirmwareURL.isEmpty()),
      _downloadingFirmwareList(false),
      _statusLog(nullptr),
      _selectedFirmwareBuildType(StableFirmware),
      _image(nullptr),
      _apmBoardDescriptionReplaceText("<APMBoardDescription>"),
      _apmChibiOSSetting(SettingsManager::instance()->firmwareUpgradeSettings()->apmChibiOS()),
      _apmVehicleTypeSetting(SettingsManager::instance()->firmwareUpgradeSettings()->apmVehicleType())
{
    _manifestMavFirmwareVersionTypeToFirmwareBuildTypeMap["OFFICIAL"] = StableFirmware;
    _manifestMavFirmwareVersionTypeToFirmwareBuildTypeMap["BETA"] = BetaFirmware;
    _manifestMavFirmwareVersionTypeToFirmwareBuildTypeMap["DEV"] = DeveloperFirmware;

    _manifestMavTypeToFirmwareVehicleTypeMap["Copter"] = CopterFirmware;
    _manifestMavTypeToFirmwareVehicleTypeMap["HELICOPTER"] = HeliFirmware;
    _manifestMavTypeToFirmwareVehicleTypeMap["FIXED_WING"] = PlaneFirmware;
    _manifestMavTypeToFirmwareVehicleTypeMap["GROUND_ROVER"] = RoverFirmware;
    _manifestMavTypeToFirmwareVehicleTypeMap["SUBMARINE"] = SubFirmware;

    _threadController = new PX4FirmwareUpgradeThreadController(this);
    Q_CHECK_PTR(_threadController);

    connect(_threadController, &PX4FirmwareUpgradeThreadController::foundBoard, this,
            &FirmwareUpgradeController::_foundBoard);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::noBoardFound, this,
            &FirmwareUpgradeController::_noBoardFound);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::boardGone, this,
            &FirmwareUpgradeController::_boardGone);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::foundBoardInfo, this,
            &FirmwareUpgradeController::_foundBoardInfo);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::error, this, &FirmwareUpgradeController::_error);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::updateProgress, this,
            &FirmwareUpgradeController::_updateProgress);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::status, this, &FirmwareUpgradeController::_status);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::eraseStarted, this,
            &FirmwareUpgradeController::_eraseStarted);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::eraseComplete, this,
            &FirmwareUpgradeController::_eraseComplete);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::flashComplete, this,
            &FirmwareUpgradeController::_flashComplete);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::updateProgress, this,
            &FirmwareUpgradeController::_updateProgress);

    connect(&_eraseTimer, &QTimer::timeout, this, &FirmwareUpgradeController::_eraseProgressTick);

#if !defined(QGC_NO_ARDUPILOT_DIALECT)
    connect(_apmChibiOSSetting, &Fact::rawValueChanged, this, &FirmwareUpgradeController::_buildAPMFirmwareNames);
    connect(_apmVehicleTypeSetting, &Fact::rawValueChanged, this, &FirmwareUpgradeController::_buildAPMFirmwareNames);
#endif

    _downloadPX4Manifest();

#if !defined(QGC_NO_ARDUPILOT_DIALECT)
    _downloadArduPilotManifest();
#endif
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

void FirmwareUpgradeController::flash(AutoPilotStackType_t stackType, FirmwareBuildType_t firmwareType,
                                      FirmwareVehicleType_t vehicleType)
{
    qCDebug(FirmwareUpgradeLog) << "FirmwareUpgradeController::flash stackType:firmwareType:vehicleType" << stackType
                                << firmwareType << vehicleType;
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
        if (info.canFlash()) {
            info.getBoardInfo(boardType, boardName);
            names.append(boardName);
        }
    }

    return names;
}

void FirmwareUpgradeController::_foundBoard(bool firstAttempt, const QSerialPortInfo& info, int boardType,
                                            QString boardName)
{
    _boardInfo = info;
    _boardType = static_cast<QGCSerialPortInfo::BoardType_t>(boardType);
    _boardTypeName = boardName;

    qDebug() << info.manufacturer() << info.description();

    _startFlashWhenBootloaderFound = false;

    if (_boardType == QGCSerialPortInfo::BoardTypeSiKRadio) {
        if (!firstAttempt) {
            // Radio always flashes latest firmware, so we can start right away without
            // any further user input.
            _startFlashWhenBootloaderFound = true;
            _startFlashWhenBootloaderFoundFirmwareIdentity =
                FirmwareIdentifier(SiKRadio, StableFirmware, DefaultVehicleFirmware);
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
    _errorCancel("Unable to sync with bootloader.");
}

QHash<FirmwareUpgradeController::FirmwareIdentifier, QString>* FirmwareUpgradeController::_firmwareHashForBoardId(
    int boardId)
{
    _rgFirmwareDynamic.clear();

    switch (boardId) {
        case Bootloader::boardIDSiKRadio1000: {
            FirmwareToUrlElement_t element = {SiKRadio, StableFirmware, DefaultVehicleFirmware,
                                              "http://px4-travis.s3.amazonaws.com/SiK/stable/radio~hm_trp.ihx"};
            _rgFirmwareDynamic.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType),
                                      element.url);
        } break;
        case Bootloader::boardIDSiKRadio1060: {
            FirmwareToUrlElement_t element = {SiKRadio, StableFirmware, DefaultVehicleFirmware,
                                              "https://px4-travis.s3.amazonaws.com/SiK/stable/radio~hb1060.ihx"};
            _rgFirmwareDynamic.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType),
                                      element.url);
        } break;
        default:
            if (_px4ManifestLoaded) {
                _buildPX4FirmwareHashFromManifest(boardId);
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
    connect(downloader, &QGCFileDownload::downloadComplete, this,
            &FirmwareUpgradeController::_firmwareDownloadComplete);
    connect(downloader, &QGCFileDownload::downloadProgress, this,
            &FirmwareUpgradeController::_firmwareDownloadProgress);

    // Set SHA-256 for verification if available from PX4 manifest
    if (_px4FirmwareSha256Map.contains(_firmwareFilename)) {
        const QString& sha256 = _px4FirmwareSha256Map[_firmwareFilename];
        if (!sha256.isEmpty()) {
            downloader->setExpectedHash(sha256);
            _appendStatusLog(tr(" SHA-256: %1").arg(sha256));
        }
    }

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
            _errorCancel(tr("Image size of %1 is too large for board flash size %2")
                             .arg(image->imageSize())
                             .arg(_bootloaderBoardFlashSize));
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
    LinkManager::instance()->setConnectionsAllowed();
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
    _progressBar->setProperty(
        "value", static_cast<float>(_eraseTickCount * _eraseTickMsec) / static_cast<float>(_eraseTotalMsec));
}

/// Appends the specified text to the status log area in the ui
void FirmwareUpgradeController::_appendStatusLog(const QString& text, bool critical)
{
    Q_ASSERT(_statusLog);

    QString varText;

    if (critical) {
        varText = QString("<font color=\"yellow\">%1</font>").arg(text);
    } else {
        varText = text;
    }

    QMetaObject::invokeMethod(_statusLog, "append", Q_ARG(QString, varText));
}

void FirmwareUpgradeController::_errorCancel(const QString& msg)
{
    _appendStatusLog(msg, false);
    _appendStatusLog(tr("Upgrade cancelled"), true);
    _appendStatusLog("------------------------------------------", false);
    emit error();
    cancel();
    LinkManager::instance()->setConnectionsAllowed();
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
#if !defined(QGC_NO_ARDUPILOT_DIALECT)
    bool chibios = _apmChibiOSSetting->rawValue().toInt() == 0;
    FirmwareVehicleType_t vehicleType = static_cast<FirmwareVehicleType_t>(_apmVehicleTypeSetting->rawValue().toInt());
    QString boardDescription = _boardInfo.description();
    quint16 boardVID = _boardInfo.vendorIdentifier();
    quint16 boardPID = _boardInfo.productIdentifier();
    uint32_t rawBoardId =
        _bootloaderBoardID == Bootloader::boardIDPX4FMUV3 ? Bootloader::boardIDPX4FMUV2 : _bootloaderBoardID;

    qCDebug(FirmwareUpgradeLog) << QStringLiteral("_buildAPMFirmwareNames description(%1) vid(%2/0x%3) pid(%4/0x%5)")
                                       .arg(boardDescription)
                                       .arg(boardVID)
                                       .arg(boardVID, 1, 16)
                                       .arg(boardPID)
                                       .arg(boardPID, 1, 16);

    _apmFirmwareNames.clear();
    _apmFirmwareNamesBestIndex = -1;
    _apmFirmwareUrls.clear();

    QString apmDescriptionSuffix("-BL");
    QString boardDescriptionPrefix;
    bool bootloaderMatch = boardDescription.endsWith(apmDescriptionSuffix);

    int currentIndex = 0;
    for (const ManifestFirmwareInfo_t& firmwareInfo : _rgManifestFirmwareInfo) {
        bool match = false;
        if (firmwareInfo.firmwareBuildType == _selectedFirmwareBuildType && firmwareInfo.chibios == chibios &&
            firmwareInfo.vehicleType == vehicleType && firmwareInfo.boardId == rawBoardId) {
            if (firmwareInfo.fmuv2 && _bootloaderBoardID == Bootloader::boardIDPX4FMUV3) {
                qCDebug(FirmwareUpgradeLog)
                    << "Skipping fmuv2 manifest entry for fmuv3 board:" << firmwareInfo.friendlyName << boardDescription
                    << firmwareInfo.rgBootloaderPortString << firmwareInfo.url << firmwareInfo.vehicleType;
            } else {
                qCDebug(FirmwareUpgradeLog)
                    << "Board id match:" << firmwareInfo.friendlyName << boardDescription
                    << firmwareInfo.rgBootloaderPortString << firmwareInfo.url << firmwareInfo.vehicleType;
                match = true;
                if (bootloaderMatch && _apmFirmwareNamesBestIndex == -1 &&
                    firmwareInfo.rgBootloaderPortString.contains(boardDescription)) {
                    _apmFirmwareNamesBestIndex = currentIndex;
                    qCDebug(FirmwareUpgradeLog)
                        << "Bootloader best match:" << firmwareInfo.friendlyName << boardDescription
                        << firmwareInfo.rgBootloaderPortString << firmwareInfo.url << firmwareInfo.vehicleType;
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
#endif
}

FirmwareUpgradeController::FirmwareVehicleType_t FirmwareUpgradeController::vehicleTypeFromFirmwareSelectionIndex(
    int index)
{
    if (index < 0 || index >= _apmVehicleTypeFromCurrentVersionList.length()) {
        qWarning() << "Invalid index, index:count" << index << _apmVehicleTypeFromCurrentVersionList.length();
        return CopterFirmware;
    }

    return _apmVehicleTypeFromCurrentVersionList[index];
}

void FirmwareUpgradeController::_downloadPX4Manifest(void)
{
    qCDebug(FirmwareUpgradeLog) << "Downloading PX4 manifest from" << _px4ManifestUrl;
    _px4ManifestDownloading = true;
    emit px4ManifestLoadedChanged();

    QGCFileDownload* downloader = new QGCFileDownload(this);
    connect(downloader, &QGCFileDownload::downloadComplete, this,
            &FirmwareUpgradeController::_px4ManifestDownloadComplete);
    if (!downloader->download(QString(_px4ManifestUrl))) {
        qCWarning(FirmwareUpgradeLog) << "PX4 manifest download failed to start:" << downloader->errorString();
        _px4ManifestDownloading = false;
        _px4ManifestLoaded = false;
        emit px4ManifestLoadedChanged();
        downloader->deleteLater();
    }
}

void FirmwareUpgradeController::_px4ManifestDownloadComplete(QString remoteFile, QString localFile, QString errorMsg)
{
    Q_UNUSED(remoteFile);
    sender()->deleteLater();

    _px4ManifestDownloading = false;

    if (!errorMsg.isEmpty()) {
        qCWarning(FirmwareUpgradeLog) << "PX4 manifest download failed:" << errorMsg;
        _px4ManifestLoaded = false;
        emit px4ManifestLoadedChanged();
        return;
    }

    qCDebug(FirmwareUpgradeLog) << "PX4 manifest downloaded to" << localFile;

    QString errorString;
    QJsonDocument doc;
    if (!JsonHelper::isJsonFile(localFile, doc, errorString)) {
        qCWarning(FirmwareUpgradeLog) << "PX4 manifest JSON parse failed:" << errorString;
        _px4ManifestLoaded = false;
        emit px4ManifestLoadedChanged();
        return;
    }

    if (!_parsePX4Manifest(doc)) {
        qCWarning(FirmwareUpgradeLog) << "PX4 manifest parse failed";
        _px4ManifestLoaded = false;
        emit px4ManifestLoadedChanged();
    } else {
        qCDebug(FirmwareUpgradeLog) << "PX4 manifest parsed:" << _px4ManifestReleases.count()
                                    << "releases, stable:" << _px4ManifestLatestStable;
        _px4ManifestLoaded = true;
        emit px4ManifestLoadedChanged();
    }
}

bool FirmwareUpgradeController::_parsePX4Manifest(const QJsonDocument& doc)
{
    if (!doc.isObject()) {
        qCWarning(FirmwareUpgradeLog) << "PX4 manifest is not a JSON object";
        return false;
    }

    QJsonObject json = doc.object();

    // Validate format version
    int formatVersion = json[_px4ManifestFormatVersionKey].toInt(0);
    if (formatVersion != 2) {
        qCWarning(FirmwareUpgradeLog) << "Unsupported PX4 manifest format_version:" << formatVersion;
        return false;
    }

    // Extract latest version pointers
    _px4ManifestLatestStable = json[_px4ManifestLatestStableKey].toString();
    _px4ManifestLatestBeta = json[_px4ManifestLatestBetaKey].toString();
    _px4ManifestLatestDev = json[_px4ManifestLatestDevKey].toString();

    // Update version display strings
    _px4StableVersion = _px4ManifestLatestStable;
    _px4BetaVersion = _px4ManifestLatestBeta;
    _px4DevVersion = _px4ManifestLatestDev;
    emit px4StableVersionChanged(_px4StableVersion);
    emit px4BetaVersionChanged(_px4BetaVersion);
    emit px4DevVersionChanged(_px4DevVersion);

    // Parse releases
    _px4ManifestReleases.clear();
    _px4AvailableVersions.clear();

    QJsonObject releasesObj = json[_px4ManifestReleasesKey].toObject();
    for (auto it = releasesObj.begin(); it != releasesObj.end(); ++it) {
        const QString& versionKey = it.key();
        QJsonObject releaseObj = it.value().toObject();

        PX4ManifestReleaseInfo_t releaseInfo;
        releaseInfo.gitTag = releaseObj[_px4ManifestGitTagKey].toString();
        releaseInfo.releaseDate = releaseObj[_px4ManifestReleaseDateKey].toString();
        releaseInfo.channel = releaseObj[_px4ManifestChannelKey].toString();

        QJsonArray buildsArray = releaseObj[_px4ManifestBuildsKey].toArray();
        for (int i = 0; i < buildsArray.count(); i++) {
            QJsonObject buildObj = buildsArray[i].toObject();

            PX4ManifestBuildInfo_t buildInfo;
            buildInfo.boardId = static_cast<uint32_t>(buildObj[_manifestBoardIdJsonKey].toInt());
            buildInfo.boardRevision = static_cast<uint32_t>(buildObj[_px4ManifestBoardRevisionKey].toInt());
            buildInfo.filename = buildObj[_px4ManifestFilenameKey].toString();
            buildInfo.url = buildObj[_manifestUrlJsonKey].toString();
            buildInfo.version = versionKey;
            buildInfo.gitHash = buildObj[_px4ManifestGitHashKey].toString();
            buildInfo.sha256sum = buildObj[_px4ManifestSha256Key].toString();
            buildInfo.gitIdentity = buildObj[_px4ManifestGitIdentityKey].toString();
            buildInfo.channel = releaseObj[_px4ManifestChannelKey].toString();
            buildInfo.buildTime = static_cast<uint64_t>(buildObj[_px4ManifestBuildTimeKey].toDouble());
            buildInfo.imageSize = static_cast<uint64_t>(buildObj[_px4ManifestImageSizeKey].toDouble());
            buildInfo.mavAutopilot = buildObj[_px4ManifestMavAutopilotKey].toInt();

            releaseInfo.builds.append(buildInfo);
        }

        _px4ManifestReleases[versionKey] = releaseInfo;
        _px4AvailableVersions.append(versionKey);
    }

    // Sort versions in reverse order (latest first)
    std::sort(_px4AvailableVersions.begin(), _px4AvailableVersions.end(), std::greater<QString>());

    emit px4AvailableVersionsChanged();

    qCDebug(FirmwareUpgradeLog) << "PX4 manifest parsed:" << _px4ManifestReleases.count() << "releases,"
                                << "latest stable:" << _px4ManifestLatestStable
                                << "latest beta:" << _px4ManifestLatestBeta << "latest dev:" << _px4ManifestLatestDev;

    return true;
}

void FirmwareUpgradeController::_buildPX4FirmwareHashFromManifest(int boardId)
{
    _px4FirmwareSha256Map.clear();

    auto insertBuildForChannel = [this, boardId](const QString& versionTag, FirmwareBuildType_t buildType) {
        if (versionTag.isEmpty() || !_px4ManifestReleases.contains(versionTag)) {
            return;
        }

        // If user selected a specific version, only use that for stable
        QString effectiveVersion = versionTag;
        if (buildType == StableFirmware && !_selectedPX4Version.isEmpty() &&
            _px4ManifestReleases.contains(_selectedPX4Version)) {
            effectiveVersion = _selectedPX4Version;
        }

        const PX4ManifestReleaseInfo_t& release = _px4ManifestReleases[effectiveVersion];
        for (const PX4ManifestBuildInfo_t& build : release.builds) {
            if (static_cast<int>(build.boardId) == boardId) {
                _rgFirmwareDynamic.insert(FirmwareIdentifier(AutoPilotStackPX4, buildType, DefaultVehicleFirmware),
                                          build.url);
                if (!build.sha256sum.isEmpty()) {
                    _px4FirmwareSha256Map[build.url] = build.sha256sum;
                }
                qCDebug(FirmwareUpgradeLog)
                    << "PX4 manifest firmware for board" << boardId << "channel:" << build.channel
                    << "version:" << effectiveVersion << "url:" << build.url;
                return;  // Found a match for this board + channel
            }
        }
    };

    insertBuildForChannel(_px4ManifestLatestStable, StableFirmware);
    insertBuildForChannel(_px4ManifestLatestBeta, BetaFirmware);
    insertBuildForChannel(_px4ManifestLatestDev, DeveloperFirmware);
}

void FirmwareUpgradeController::_downloadArduPilotManifest(void)
{
    _downloadingFirmwareList = true;
    emit downloadingFirmwareListChanged(true);

    QGCFileDownload* downloader = new QGCFileDownload(this);
    connect(downloader, &QGCFileDownload::downloadComplete, this,
            &FirmwareUpgradeController::_ardupilotManifestDownloadComplete);
    // Use autoDecompress to stream-decompress directly during download
    downloader->download(QStringLiteral("https://firmware.ardupilot.org/manifest.json.gz"), {}, true);
}

void FirmwareUpgradeController::_ardupilotManifestDownloadComplete(QString remoteFile, QString localFile,
                                                                   QString errorMsg)
{
    if (errorMsg.isEmpty()) {
        // Delete the QGCFileDownload object
        sender()->deleteLater();

        qCDebug(FirmwareUpgradeLog) << "_ardupilotManifestDownloadFinished" << remoteFile << localFile;

        // localFile is already decompressed (autoDecompress=true streams directly to .json)
        QString errorString;
        QJsonDocument doc;
        if (!JsonHelper::isJsonFile(localFile, doc, errorString)) {
            qCWarning(FirmwareUpgradeLog) << "Json file read failed" << errorString;
            return;
        }

        QJsonObject json = doc.object();
        QJsonArray rgFirmware = json[_manifestFirmwareJsonKey].toArray();

        for (int i = 0; i < rgFirmware.count(); i++) {
            const QJsonObject& firmwareJson = rgFirmware[i].toObject();

            FirmwareVehicleType_t firmwareVehicleType =
                _manifestMavTypeToFirmwareVehicleType(firmwareJson[_manifestMavTypeJsonKey].toString());
            FirmwareBuildType_t firmwareBuildType = _manifestMavFirmwareVersionTypeToFirmwareBuildType(
                firmwareJson[_manifestMavFirmwareVersionTypeJsonKey].toString());
            QString format = firmwareJson[_manifestFormatJsonKey].toString();
            QString platform = firmwareJson[_manifestPlatformKey].toString();

            if (firmwareVehicleType != DefaultVehicleFirmware && firmwareBuildType != CustomFirmware &&
                (format == QStringLiteral("apj") || format == QStringLiteral("px4"))) {
                if (platform.contains("-heli") && firmwareVehicleType != HeliFirmware) {
                    continue;
                }

                _rgManifestFirmwareInfo.append(ManifestFirmwareInfo_t());
                ManifestFirmwareInfo_t& firmwareInfo = _rgManifestFirmwareInfo.last();

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
                    QString vid = vidpid[0];
                    QString pid = vidpid[1];

                    bool ok;
                    firmwareInfo.rgVID.append(vid.right(vid.length() - 2).toInt(&ok, 16));
                    firmwareInfo.rgPID.append(pid.right(pid.length() - 2).toInt(&ok, 16));
                }

                QString brandName = firmwareJson[_manifestBrandNameKey].toString();
                firmwareInfo.friendlyName =
                    QStringLiteral("%1 - %2").arg(brandName.isEmpty() ? platform : brandName).arg(firmwareInfo.version);
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

FirmwareUpgradeController::FirmwareBuildType_t
FirmwareUpgradeController::_manifestMavFirmwareVersionTypeToFirmwareBuildType(
    const QString& manifestMavFirmwareVersionType)
{
    if (_manifestMavFirmwareVersionTypeToFirmwareBuildTypeMap.contains(manifestMavFirmwareVersionType)) {
        return _manifestMavFirmwareVersionTypeToFirmwareBuildTypeMap[manifestMavFirmwareVersionType];
    } else {
        return CustomFirmware;
    }
}

FirmwareUpgradeController::FirmwareVehicleType_t FirmwareUpgradeController::_manifestMavTypeToFirmwareVehicleType(
    const QString& manifestMavType)
{
    if (_manifestMavTypeToFirmwareVehicleTypeMap.contains(manifestMavType)) {
        return _manifestMavTypeToFirmwareVehicleTypeMap[manifestMavType];
    } else {
        return DefaultVehicleFirmware;
    }
}
