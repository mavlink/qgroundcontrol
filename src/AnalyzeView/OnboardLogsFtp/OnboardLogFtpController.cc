#include "OnboardLogFtpController.h"
#include "AppSettings.h"
#include "FTPManager.h"
#include "MultiVehicleManager.h"
#include "OnboardLogFtpEntry.h"
#include "QGCFormat.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTimeZone>

QGC_LOGGING_CATEGORY(OnboardLogFtpControllerLog, "AnalyzeView.OnboardLogFtpController")

// MAVLink FTP defines "@MAV_LOG" as the virtual log directory.
// Older firmware that doesn't implement the alias requires the physical path
// instead — which is firmware-specific.
static constexpr const char *kMavlinkLogRoot = "@MAV_LOG";
static constexpr const char *kPx4LogRootFallback = "/fs/microsd/log";
static constexpr const char *kApmLogRootFallback = "/APM/LOGS";

OnboardLogFtpController::OnboardLogFtpController(QObject *parent)
    : QObject(parent)
    , _logEntriesModel(new QmlObjectListModel(this))
{
    qCDebug(OnboardLogFtpControllerLog) << this;

    (void) connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this, &OnboardLogFtpController::_setActiveVehicle);
    _setActiveVehicle(MultiVehicleManager::instance()->activeVehicle());
}

OnboardLogFtpController::~OnboardLogFtpController()
{
    qCDebug(OnboardLogFtpControllerLog) << this;
}

void OnboardLogFtpController::_setActiveVehicle(Vehicle *vehicle)
{
    if (vehicle == _vehicle) {
        return;
    }

    if (_vehicle) {
        _logEntriesModel->clearAndDeleteContents();
        FTPManager *const ftp = _vehicle->ftpManager();
        (void) disconnect(ftp, &FTPManager::listDirectoryComplete, this, &OnboardLogFtpController::_listDirComplete);
        (void) disconnect(ftp, &FTPManager::downloadComplete,      this, &OnboardLogFtpController::_downloadComplete);
        (void) disconnect(ftp, &FTPManager::commandProgress,       this, &OnboardLogFtpController::_downloadProgress);

        _listState = Idle;
        _dirsToList.clear();
        _logIdCounter = 0;
        _downloadQueue.clear();
        _currentDownloadEntry = nullptr;
    }

    _vehicle = vehicle;
}

void OnboardLogFtpController::refresh()
{
    _logEntriesModel->clearAndDeleteContents();

    if (!_vehicle) {
        qCWarning(OnboardLogFtpControllerLog) << "refresh: no active vehicle";
        return;
    }

    if (!_vehicle->capabilitiesKnown() || !(_vehicle->capabilityBits() & MAV_PROTOCOL_CAPABILITY_FTP)) {
        qCWarning(OnboardLogFtpControllerLog) << "refresh: vehicle does not advertise MAV_PROTOCOL_CAPABILITY_FTP"
            << "(capsKnown:" << _vehicle->capabilitiesKnown() << ")";
        return;
    }

    _startListing();
}

void OnboardLogFtpController::_startListing()
{
    _dirsToList.clear();
    _logIdCounter = 0;
    _logRoot = QString::fromLatin1(kMavlinkLogRoot);
    _triedFallbackRoot = false;

    FTPManager *const ftp = _vehicle->ftpManager();
    (void) disconnect(ftp, &FTPManager::listDirectoryComplete, this, &OnboardLogFtpController::_listDirComplete);
    (void) connect(ftp, &FTPManager::listDirectoryComplete, this, &OnboardLogFtpController::_listDirComplete);

    _setListing(true);
    _listRoot();
}

void OnboardLogFtpController::_listRoot()
{
    _listState = ListingRoot;

    qCDebug(OnboardLogFtpControllerLog) << "listing root" << _logRoot;

    if (!_vehicle->ftpManager()->listDirectory(MAV_COMP_ID_AUTOPILOT1, _logRoot)) {
        qCWarning(OnboardLogFtpControllerLog) << "failed to start root listing for" << _logRoot;
        _finishListing();
    }
}

void OnboardLogFtpController::_listDirComplete(const QStringList &dirList, const QString &errorMsg)
{
    if (!errorMsg.isEmpty()) {
        if (_listState == ListingRoot && !_triedFallbackRoot && _vehicle) {
            const char *fallback = nullptr;
            if (_vehicle->px4Firmware()) {
                fallback = kPx4LogRootFallback;
            } else if (_vehicle->apmFirmware()) {
                fallback = kApmLogRootFallback;
            }

            if (fallback) {
                qCDebug(OnboardLogFtpControllerLog) << "root listing of" << _logRoot << "failed (" << errorMsg
                    << "), falling back to" << fallback;
                _triedFallbackRoot = true;
                _logRoot = QString::fromLatin1(fallback);
                _listRoot();
                return;
            }
        }

        qCWarning(OnboardLogFtpControllerLog) << "listing error:" << errorMsg;
        _finishListing();
        return;
    }

    if (_listState == ListingRoot) {
        // The root listing may contain log files directly (flat layout, e.g. @MAV_LOG)
        // and/or date subdirectories to descend into (PX4 fallback /fs/microsd/log).
        const uint flatLogs = _processFileEntries(dirList, QString());

        for (const QString &entry : dirList) {
            if (entry.startsWith(QLatin1Char('D'))) {
                const QString dirName = entry.mid(1);
                if (!dirName.isEmpty()) {
                    _dirsToList.append(dirName);
                }
            }
        }

        _dirsToList.sort();
        qCDebug(OnboardLogFtpControllerLog) << "root listing of" << _logRoot
            << "found" << flatLogs << "flat logs and" << _dirsToList.size() << "subdirectories";

        _listState = ListingSubdir;
        _listNextSubdir();
        return;
    }

    const QString currentDir = _dirsToList.isEmpty() ? QString() : _dirsToList.first();
    const uint logsFoundInDir = _processFileEntries(dirList, currentDir);

    qCDebug(OnboardLogFtpControllerLog) << currentDir << "->" << logsFoundInDir << "logs";

    if (!_dirsToList.isEmpty()) {
        _dirsToList.removeFirst();
    }

    _listNextSubdir();
}

uint OnboardLogFtpController::_processFileEntries(const QStringList &dirList, const QString &subdir)
{
    const QDate dirDate = subdir.isEmpty() ? QDate() : QDate::fromString(subdir, QStringLiteral("yyyy-MM-dd"));
    uint logsFound = 0;

    for (const QString &entry : dirList) {
        if (!entry.startsWith(QLatin1Char('F'))) {
            continue;
        }

        const QString fileInfo = entry.mid(1);
        const int tabIdx = fileInfo.indexOf(QLatin1Char('\t'));
        if (tabIdx < 0) {
            continue;
        }

        const QString fileName = fileInfo.left(tabIdx);
        const QString sizeStr = fileInfo.mid(tabIdx + 1);

        if (!fileName.endsWith(QStringLiteral(".ulg"), Qt::CaseInsensitive) &&
            !fileName.endsWith(QStringLiteral(".bin"), Qt::CaseInsensitive)) {
            continue;
        }

        bool sizeOk = false;
        const uint fileSize = sizeStr.toUInt(&sizeOk);
        if (!sizeOk) {
            continue;
        }

        QDateTime dateTime;
        if (dirDate.isValid()) {
            const QString baseName = fileName.left(fileName.lastIndexOf(QLatin1Char('.')));
            const QTime fileTime = QTime::fromString(baseName, QStringLiteral("HH_mm_ss"));
            if (fileTime.isValid()) {
                dateTime = QDateTime(dirDate, fileTime, QTimeZone::UTC);
            } else {
                dateTime = QDateTime(dirDate, QTime(), QTimeZone::UTC);
            }
        }

        const QString ftpPath = subdir.isEmpty()
            ? (_logRoot + QStringLiteral("/") + fileName)
            : (_logRoot + QStringLiteral("/") + subdir + QStringLiteral("/") + fileName);

        QGCOnboardLogFtpEntry *const logEntry = new QGCOnboardLogFtpEntry(_logIdCounter++, dateTime, fileSize, true, this);
        logEntry->setFtpPath(ftpPath);
        logEntry->setStatus(tr("Available"));
        _logEntriesModel->append(logEntry);
        logsFound++;
    }

    return logsFound;
}

void OnboardLogFtpController::_listNextSubdir()
{
    if (_dirsToList.isEmpty()) {
        qCDebug(OnboardLogFtpControllerLog) << "listing complete, found" << _logEntriesModel->count() << "logs";
        _finishListing();
        return;
    }

    const QString subdir = _dirsToList.first();
    const QString path = _logRoot + QStringLiteral("/") + subdir;

    qCDebug(OnboardLogFtpControllerLog) << "listing subdir" << path;

    if (!_vehicle->ftpManager()->listDirectory(MAV_COMP_ID_AUTOPILOT1, path)) {
        qCWarning(OnboardLogFtpControllerLog) << "failed to list subdir" << path;
        _dirsToList.removeFirst();
        _listNextSubdir();
    }
}

void OnboardLogFtpController::_finishListing()
{
    _listState = Idle;
    _setListing(false);
}

void OnboardLogFtpController::download(const QString &path)
{
    const QString dir = path.isEmpty() ? SettingsManager::instance()->appSettings()->logSavePath() : path;
    _downloadToDirectory(dir);
}

void OnboardLogFtpController::_downloadToDirectory(const QString &dir)
{
    _downloadPath = dir;
    if (_downloadPath.isEmpty()) {
        return;
    }

    if (!_downloadPath.endsWith(QDir::separator())) {
        _downloadPath += QDir::separator();
    }

    _downloadQueue.clear();
    const int numLogs = _logEntriesModel->count();
    for (int i = 0; i < numLogs; i++) {
        QGCOnboardLogFtpEntry *const entry = _logEntriesModel->value<QGCOnboardLogFtpEntry*>(i);
        if (entry && entry->selected() && !entry->ftpPath().isEmpty()) {
            entry->setStatus(tr("Waiting"));
            _downloadQueue.enqueue(entry);
        }
    }

    if (_downloadQueue.isEmpty()) {
        qCWarning(OnboardLogFtpControllerLog) << "no selected logs have FTP paths for download";
        return;
    }

    qCDebug(OnboardLogFtpControllerLog) << "queued" << _downloadQueue.size() << "logs for download to" << _downloadPath;
    _setDownloading(true);

    _downloadEntry(_downloadQueue.dequeue());
}

void OnboardLogFtpController::_downloadEntry(QGCOnboardLogFtpEntry *entry)
{
    if (!entry || !_vehicle) {
        return;
    }

    entry->setSelected(false);
    emit selectionChanged();

    _currentDownloadEntry = entry;
    _downloadBytesAtLastUpdate = 0;
    _downloadRateAvg = 0.;
    _downloadElapsed.start();

    entry->setStatus(tr("Downloading"));

    QString localFilename;
    if (entry->time().isValid() && entry->time().date().year() >= 2010) {
        localFilename = entry->time().toString(QStringLiteral("yyyy-M-d-hh-mm-ss")) + QStringLiteral(".ulg");
    } else {
        localFilename = QStringLiteral("log_") + QString::number(entry->id()) + QStringLiteral(".ulg");
    }

    if (QFile::exists(_downloadPath + localFilename)) {
        const QStringList parts = localFilename.split(QLatin1Char('.'));
        uint numDups = 0;
        do {
            numDups++;
            localFilename = parts[0] + QStringLiteral("_") + QString::number(numDups) + QStringLiteral(".") + parts[1];
        } while (QFile::exists(_downloadPath + localFilename));
    }

    FTPManager *const ftp = _vehicle->ftpManager();
    (void) disconnect(ftp, &FTPManager::downloadComplete, this, &OnboardLogFtpController::_downloadComplete);
    (void) disconnect(ftp, &FTPManager::commandProgress,  this, &OnboardLogFtpController::_downloadProgress);
    (void) connect(ftp, &FTPManager::downloadComplete, this, &OnboardLogFtpController::_downloadComplete);
    (void) connect(ftp, &FTPManager::commandProgress,  this, &OnboardLogFtpController::_downloadProgress);

    qCDebug(OnboardLogFtpControllerLog) << "downloading" << entry->ftpPath() << "to" << _downloadPath + localFilename;

    if (!ftp->download(MAV_COMP_ID_AUTOPILOT1, entry->ftpPath(), _downloadPath, localFilename, true)) {
        qCWarning(OnboardLogFtpControllerLog) << "failed to start download for" << entry->ftpPath();
        entry->setStatus(tr("Error"));
        _currentDownloadEntry = nullptr;

        if (!_downloadQueue.isEmpty()) {
            _downloadEntry(_downloadQueue.dequeue());
        } else {
            _setDownloading(false);
        }
    }
}

void OnboardLogFtpController::_downloadComplete(const QString &file, const QString &errorMsg)
{
    if (!_currentDownloadEntry) {
        return;
    }

    if (errorMsg.isEmpty()) {
        _currentDownloadEntry->setStatus(tr("Downloaded"));
        qCDebug(OnboardLogFtpControllerLog) << "download complete" << file;
    } else {
        _currentDownloadEntry->setStatus(tr("Error"));
        qCWarning(OnboardLogFtpControllerLog) << "download error:" << errorMsg;
    }

    _currentDownloadEntry = nullptr;

    if (!_downloadQueue.isEmpty()) {
        _downloadEntry(_downloadQueue.dequeue());
    } else {
        _setDownloading(false);
    }
}

void OnboardLogFtpController::_downloadProgress(float value)
{
    if (!_currentDownloadEntry) {
        return;
    }

    if (_downloadElapsed.elapsed() < kGUIRateMs) {
        return;
    }

    const size_t totalBytes = static_cast<size_t>(static_cast<qreal>(_currentDownloadEntry->size()) * static_cast<qreal>(value));
    const size_t bytesSinceLastUpdate = totalBytes - _downloadBytesAtLastUpdate;
    const qreal elapsedSec = _downloadElapsed.elapsed() / 1000.0;
    const qreal rate = (elapsedSec > 0) ? (bytesSinceLastUpdate / elapsedSec) : 0;
    _downloadRateAvg = (_downloadRateAvg * 0.95) + (rate * 0.05);
    _downloadBytesAtLastUpdate = totalBytes;
    _downloadElapsed.start();

    const QString status = QStringLiteral("%1 (%2/s)").arg(
        QGC::bigSizeToString(totalBytes),
        QGC::bigSizeToString(_downloadRateAvg));

    _currentDownloadEntry->setStatus(status);
}

void OnboardLogFtpController::cancel()
{
    if (!_vehicle) {
        return;
    }

    if (_requestingLogEntries) {
        _vehicle->ftpManager()->cancelListDirectory();
        _dirsToList.clear();
        _finishListing();
    }

    if (_downloadingLogs) {
        _vehicle->ftpManager()->cancelDownload();
        if (_currentDownloadEntry) {
            _currentDownloadEntry->setStatus(tr("Canceled"));
            _currentDownloadEntry = nullptr;
        }
        _downloadQueue.clear();
    }

    _resetSelection(true);
    _setDownloading(false);
}

void OnboardLogFtpController::_resetSelection(bool canceled)
{
    const int numLogs = _logEntriesModel->count();
    for (int i = 0; i < numLogs; i++) {
        QGCOnboardLogFtpEntry *const entry = _logEntriesModel->value<QGCOnboardLogFtpEntry*>(i);
        if (!entry) {
            continue;
        }

        if (entry->selected()) {
            if (canceled) {
                entry->setStatus(tr("Canceled"));
            }
            entry->setSelected(false);
        }
    }

    emit selectionChanged();
}

void OnboardLogFtpController::_setDownloading(bool active)
{
    if (_downloadingLogs != active) {
        _downloadingLogs = active;
        if (_vehicle) {
            _vehicle->vehicleLinkManager()->setCommunicationLostEnabled(!active);
        }
        emit downloadingLogsChanged();
    }
}

void OnboardLogFtpController::_setListing(bool active)
{
    if (_requestingLogEntries != active) {
        _requestingLogEntries = active;
        if (_vehicle) {
            _vehicle->vehicleLinkManager()->setCommunicationLostEnabled(!active);
        }
        emit requestingListChanged();
    }
}
