#include "FtpTransport.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTimeZone>

#include "AppSettings.h"
#include "FTPManager.h"
#include "MultiVehicleManager.h"
#include "OnboardLogEntry.h"
#include "QGCFormat.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

QGC_LOGGING_CATEGORY(FtpTransportLog, "AnalyzeView.FtpTransport")

// MAVLink FTP defines "@MAV_LOG" as the virtual log directory.
// Older firmware that doesn't implement the alias requires the physical path
// instead — which is firmware-specific.
static constexpr const char* kMavlinkLogRoot = "@MAV_LOG";
static constexpr const char* kPx4LogRootFallback = "/fs/microsd/log";
static constexpr const char* kApmLogRootFallback = "/APM/LOGS";

FtpTransport::FtpTransport(QObject* parent) : OnboardLogTransport(parent)
{
    qCDebug(FtpTransportLog) << this;

    (void)connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this,
                  &FtpTransport::_setActiveVehicle);
    _setActiveVehicle(MultiVehicleManager::instance()->activeVehicle());
}

FtpTransport::~FtpTransport()
{
    qCDebug(FtpTransportLog) << this;
}

void FtpTransport::_setActiveVehicle(Vehicle* vehicle)
{
    if (vehicle == _vehicle) {
        return;
    }

    if (_vehicle) {
        _logEntriesModel->clearAndDeleteContents();
        FTPManager* const ftp = _vehicle->ftpManager();
        (void)disconnect(ftp, &FTPManager::listDirectoryComplete, this, &FtpTransport::_listDirComplete);
        (void)disconnect(ftp, &FTPManager::downloadComplete, this, &FtpTransport::_downloadComplete);
        (void)disconnect(ftp, &FTPManager::commandProgress, this, &FtpTransport::_downloadProgress);

        _listState = Idle;
        _dirsToList.clear();
        _logIdCounter = 0;
        _downloadQueue.clear();
        _currentDownloadEntry = nullptr;
    }

    _vehicle = vehicle;
}

void FtpTransport::refresh()
{
    _logEntriesModel->clearAndDeleteContents();

    if (!_vehicle) {
        qCWarning(FtpTransportLog) << "refresh: no active vehicle";
        return;
    }

    if (!_vehicle->capabilitiesKnown() || !(_vehicle->capabilityBits() & MAV_PROTOCOL_CAPABILITY_FTP)) {
        qCWarning(FtpTransportLog) << "refresh: vehicle does not advertise MAV_PROTOCOL_CAPABILITY_FTP"
                                   << "(capsKnown:" << _vehicle->capabilitiesKnown() << ")";
        return;
    }

    _startListing();
}

void FtpTransport::_startListing()
{
    _dirsToList.clear();
    _logIdCounter = 0;
    _logRoot = QString::fromLatin1(kMavlinkLogRoot);
    _triedFallbackRoot = false;

    FTPManager* const ftp = _vehicle->ftpManager();
    (void)disconnect(ftp, &FTPManager::listDirectoryComplete, this, &FtpTransport::_listDirComplete);
    (void)connect(ftp, &FTPManager::listDirectoryComplete, this, &FtpTransport::_listDirComplete);

    _setListing(true);
    _listRoot();
}

void FtpTransport::_listRoot()
{
    _listState = ListingRoot;

    qCDebug(FtpTransportLog) << "listing root" << _logRoot;

    if (!_vehicle->ftpManager()->listDirectory(MAV_COMP_ID_AUTOPILOT1, _logRoot)) {
        qCWarning(FtpTransportLog) << "failed to start root listing for" << _logRoot;
        _finishListing();
    }
}

void FtpTransport::_listDirComplete(const QStringList& dirList, const QString& errorMsg)
{
    if (!errorMsg.isEmpty()) {
        if (_listState == ListingRoot && !_triedFallbackRoot && _vehicle) {
            const char* fallback = nullptr;
            if (_vehicle->px4Firmware()) {
                fallback = kPx4LogRootFallback;
            } else if (_vehicle->apmFirmware()) {
                fallback = kApmLogRootFallback;
            }

            if (fallback) {
                qCDebug(FtpTransportLog) << "root listing of" << _logRoot << "failed (" << errorMsg
                                         << "), falling back to" << fallback;
                _triedFallbackRoot = true;
                _logRoot = QString::fromLatin1(fallback);
                _listRoot();
                return;
            }
        }

        qCWarning(FtpTransportLog) << "listing error:" << errorMsg;
        _finishListing();
        return;
    }

    if (_listState == ListingRoot) {
        // The root listing may contain log files directly (flat layout, e.g. @MAV_LOG)
        // and/or date subdirectories to descend into (PX4 fallback /fs/microsd/log).
        const uint flatLogs = _processFileEntries(dirList, QString());

        for (const QString& entry : dirList) {
            if (entry.startsWith(QLatin1Char('D'))) {
                const QString dirName = entry.mid(1);
                if (!dirName.isEmpty()) {
                    _dirsToList.append(dirName);
                }
            }
        }

        _dirsToList.sort();
        qCDebug(FtpTransportLog) << "root listing of" << _logRoot << "found" << flatLogs << "flat logs and"
                                 << _dirsToList.size() << "subdirectories";

        _listState = ListingSubdir;
        _listNextSubdir();
        return;
    }

    const QString currentDir = _dirsToList.isEmpty() ? QString() : _dirsToList.first();
    const uint logsFoundInDir = _processFileEntries(dirList, currentDir);

    qCDebug(FtpTransportLog) << currentDir << "->" << logsFoundInDir << "logs";

    if (!_dirsToList.isEmpty()) {
        _dirsToList.removeFirst();
    }

    _listNextSubdir();
}

uint FtpTransport::_processFileEntries(const QStringList& dirList, const QString& subdir)
{
    const QDate dirDate = subdir.isEmpty() ? QDate() : QDate::fromString(subdir, QStringLiteral("yyyy-MM-dd"));
    uint logsFound = 0;

    for (const QString& entry : dirList) {
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

        OnboardLogEntry* const logEntry = new OnboardLogEntry(_logIdCounter++, dateTime, fileSize, true, this);
        logEntry->setFtpPath(ftpPath);
        logEntry->setStatus(tr("Available"));
        _logEntriesModel->append(logEntry);
        logsFound++;
    }

    return logsFound;
}

void FtpTransport::_listNextSubdir()
{
    if (_dirsToList.isEmpty()) {
        qCDebug(FtpTransportLog) << "listing complete, found" << _logEntriesModel->count() << "logs";
        _finishListing();
        return;
    }

    const QString subdir = _dirsToList.first();
    const QString path = _logRoot + QStringLiteral("/") + subdir;

    qCDebug(FtpTransportLog) << "listing subdir" << path;

    if (!_vehicle->ftpManager()->listDirectory(MAV_COMP_ID_AUTOPILOT1, path)) {
        qCWarning(FtpTransportLog) << "failed to list subdir" << path;
        _dirsToList.removeFirst();
        _listNextSubdir();
    }
}

void FtpTransport::_finishListing()
{
    _listState = Idle;
    _setListing(false);
}

void FtpTransport::download(const QString& path)
{
    const QString dir = path.isEmpty() ? SettingsManager::instance()->appSettings()->logSavePath() : path;
    _downloadToDirectory(dir);
}

void FtpTransport::_downloadToDirectory(const QString& dir)
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
        OnboardLogEntry* const entry = _logEntriesModel->value<OnboardLogEntry*>(i);
        if (entry && entry->selected() && !entry->ftpPath().isEmpty()) {
            entry->setStatus(tr("Waiting"));
            _downloadQueue.enqueue(entry);
        }
    }

    if (_downloadQueue.isEmpty()) {
        qCWarning(FtpTransportLog) << "no selected logs have FTP paths for download";
        return;
    }

    qCDebug(FtpTransportLog) << "queued" << _downloadQueue.size() << "logs for download to" << _downloadPath;
    _setDownloading(true);

    _downloadEntry(_downloadQueue.dequeue());
}

void FtpTransport::_downloadEntry(OnboardLogEntry* entry)
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

    FTPManager* const ftp = _vehicle->ftpManager();
    (void)disconnect(ftp, &FTPManager::downloadComplete, this, &FtpTransport::_downloadComplete);
    (void)disconnect(ftp, &FTPManager::commandProgress, this, &FtpTransport::_downloadProgress);
    (void)connect(ftp, &FTPManager::downloadComplete, this, &FtpTransport::_downloadComplete);
    (void)connect(ftp, &FTPManager::commandProgress, this, &FtpTransport::_downloadProgress);

    qCDebug(FtpTransportLog) << "downloading" << entry->ftpPath() << "to" << _downloadPath + localFilename;

    if (!ftp->download(MAV_COMP_ID_AUTOPILOT1, entry->ftpPath(), _downloadPath, localFilename, true)) {
        qCWarning(FtpTransportLog) << "failed to start download for" << entry->ftpPath();
        entry->setStatus(tr("Error"));
        _currentDownloadEntry = nullptr;

        if (!_downloadQueue.isEmpty()) {
            _downloadEntry(_downloadQueue.dequeue());
        } else {
            _setDownloading(false);
        }
    }
}

void FtpTransport::_downloadComplete(const QString& file, const QString& errorMsg)
{
    if (!_currentDownloadEntry) {
        return;
    }

    if (errorMsg.isEmpty()) {
        _currentDownloadEntry->setStatus(tr("Downloaded"));
        qCDebug(FtpTransportLog) << "download complete" << file;
    } else {
        _currentDownloadEntry->setStatus(tr("Error"));
        qCWarning(FtpTransportLog) << "download error:" << errorMsg;
    }

    _currentDownloadEntry = nullptr;

    if (!_downloadQueue.isEmpty()) {
        _downloadEntry(_downloadQueue.dequeue());
    } else {
        _setDownloading(false);
    }
}

void FtpTransport::_downloadProgress(float value)
{
    if (!_currentDownloadEntry) {
        return;
    }

    if (_downloadElapsed.elapsed() < kGUIRateMs) {
        return;
    }

    const size_t totalBytes =
        static_cast<size_t>(static_cast<qreal>(_currentDownloadEntry->size()) * static_cast<qreal>(value));
    const size_t bytesSinceLastUpdate = totalBytes - _downloadBytesAtLastUpdate;
    const qreal elapsedSec = _downloadElapsed.elapsed() / 1000.0;
    const qreal rate = (elapsedSec > 0) ? (bytesSinceLastUpdate / elapsedSec) : 0;
    _downloadRateAvg = (_downloadRateAvg * 0.95) + (rate * 0.05);
    _downloadBytesAtLastUpdate = totalBytes;
    _downloadElapsed.start();

    const QString status =
        QStringLiteral("%1 (%2/s)").arg(QGC::bigSizeToString(totalBytes), QGC::bigSizeToString(_downloadRateAvg));

    _currentDownloadEntry->setStatus(status);
}

void FtpTransport::cancel()
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

    if (_erasing) {
        _vehicle->ftpManager()->cancelDelete();
        if (_currentEraseEntry) {
            _currentEraseEntry->setStatus(tr("Canceled"));
            _currentEraseEntry = nullptr;
        }
        _eraseQueue.clear();
        _finishErase();
    }

    _resetSelection(true);
    _setDownloading(false);
}

void FtpTransport::eraseAll()
{
    if (!_vehicle) {
        qCWarning(FtpTransportLog) << "eraseAll: no active vehicle";
        return;
    }
    if (_erasing) {
        return;
    }

    const int numLogs = _logEntriesModel->count();
    _eraseQueue.clear();
    _eraseFailureCount = 0;
    for (int i = 0; i < numLogs; i++) {
        OnboardLogEntry* const entry = _logEntriesModel->value<OnboardLogEntry*>(i);
        if (entry && !entry->ftpPath().isEmpty()) {
            entry->setStatus(tr("Waiting"));
            _eraseQueue.enqueue(entry);
        }
    }

    if (_eraseQueue.isEmpty()) {
        qCWarning(FtpTransportLog) << "eraseAll: nothing to delete";
        return;
    }

    qCDebug(FtpTransportLog) << "queued" << _eraseQueue.size() << "files for delete";

    FTPManager* const ftp = _vehicle->ftpManager();
    (void)disconnect(ftp, &FTPManager::deleteComplete, this, &FtpTransport::_deleteComplete);
    (void)connect(ftp, &FTPManager::deleteComplete, this, &FtpTransport::_deleteComplete);

    _erasing = true;
    // _setListing surfaces "busy" to QML so Refresh/Download stay disabled during erase.
    _setListing(true);

    _eraseNext();
}

void FtpTransport::_eraseNext()
{
    if (_eraseQueue.isEmpty()) {
        _finishErase();
        return;
    }

    _currentEraseEntry = _eraseQueue.dequeue();
    _currentEraseEntry->setStatus(tr("Deleting"));

    FTPManager* const ftp = _vehicle->ftpManager();
    if (!ftp->deleteFile(MAV_COMP_ID_AUTOPILOT1, _currentEraseEntry->ftpPath())) {
        qCWarning(FtpTransportLog) << "deleteFile failed to start for" << _currentEraseEntry->ftpPath();
        _currentEraseEntry->setStatus(tr("Error"));
        _currentEraseEntry = nullptr;
        ++_eraseFailureCount;
        _eraseNext();
    }
}

void FtpTransport::_deleteComplete(const QString& file, const QString& errorMsg)
{
    if (!_currentEraseEntry) {
        return;
    }

    if (errorMsg.isEmpty()) {
        qCDebug(FtpTransportLog) << "deleted" << file;
        // Remove the row from the model so the UI reflects the erase immediately.
        _logEntriesModel->removeOne(_currentEraseEntry);
        _currentEraseEntry->deleteLater();
    } else {
        qCWarning(FtpTransportLog) << "delete failed for" << file << ":" << errorMsg;
        _currentEraseEntry->setStatus(tr("Error"));
        ++_eraseFailureCount;
    }

    _currentEraseEntry = nullptr;
    _eraseNext();
}

void FtpTransport::_finishErase()
{
    _erasing = false;
    _setListing(false);

    if (_eraseFailureCount > 0) {
        qCWarning(FtpTransportLog) << "erase finished with" << _eraseFailureCount << "failure(s)";
    } else {
        qCDebug(FtpTransportLog) << "erase finished";
    }
}

void FtpTransport::_resetSelection(bool canceled)
{
    const int numLogs = _logEntriesModel->count();
    for (int i = 0; i < numLogs; i++) {
        OnboardLogEntry* const entry = _logEntriesModel->value<OnboardLogEntry*>(i);
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

void FtpTransport::_setDownloading(bool active)
{
    if (_downloadingLogs != active) {
        _downloadingLogs = active;
        if (_vehicle) {
            _vehicle->vehicleLinkManager()->setCommunicationLostEnabled(!active);
        }
        emit downloadingLogsChanged();
    }
}

void FtpTransport::_setListing(bool active)
{
    if (_requestingLogEntries != active) {
        _requestingLogEntries = active;
        if (_vehicle) {
            _vehicle->vehicleLinkManager()->setCommunicationLostEnabled(!active);
        }
        emit requestingListChanged();
    }
}
