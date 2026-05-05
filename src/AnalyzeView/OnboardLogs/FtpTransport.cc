#include "FtpTransport.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QPointer>
#include <QtCore/QSignalBlocker>
#include <QtCore/QTimer>
#include <utility>

#include "AppSettings.h"
#include "FTPManager.h"
#include "FTPManagerJob.h"
#include "FirmwarePlugin.h"
#include "OnboardLogEntry.h"
#include "OnboardLogFileName.h"
#include "OnboardLogModel.h"
#include "QGCFormat.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

QGC_LOGGING_CATEGORY(FtpTransportLog, "AnalyzeView.OnboardLogs.FtpTransport")

// MAVLink FTP defines "@MAV_LOG" as the virtual log directory.
// Older firmware that doesn't implement the alias requires the physical path
// instead — which is firmware-specific.
static constexpr const char* kMavlinkLogRoot = "@MAV_LOG";

FtpTransport::FtpTransport(QObject* parent) : OnboardLogTransport(parent)
{
    qCDebug(FtpTransportLog) << this;
}

FtpTransport::~FtpTransport()
{
    qCDebug(FtpTransportLog) << this;
    setVehicle(nullptr);
}

void FtpTransport::setVehicle(Vehicle* vehicle)
{
    if (vehicle && (vehicle == _vehicle)) {
        return;
    }

    _setBatchProgress(0.);
    _setErrorMessage(QString());

    FTPManager* const ftp = _vehicle ? _vehicle->ftpManager() : nullptr;
    _cancelListing(ftp);
    _cancelDownload(ftp);
    _cancelErase(ftp);
    _listingSession.clear();
    _downloadSession.clear();
    _setDownloading(false);
    _eraseSession.clear();

    _logEntriesModel->clearAndDeleteContents();

    _vehicle = vehicle;
}

void FtpTransport::refresh()
{
    cancel();

    if (_downloadSession.canceling()) {
        _downloadSession.requestRefresh();
        return;
    }

    _refreshAfterCancel();
}

void FtpTransport::_refreshAfterCancel()
{
    const quint64 generation = _listingSession.generation();
    _setErrorMessage(QString());
    if (generation != _listingSession.generation()) {
        return;
    }
    _setBatchProgress(0.);
    if (generation != _listingSession.generation()) {
        return;
    }
    _logEntriesModel->clearAndDeleteContents();
    if (generation != _listingSession.generation()) {
        return;
    }

    if (!_vehicle) {
        const QString errorMessage = tr("No active vehicle is available for onboard log refresh");
        qCWarning(FtpTransportLog) << errorMessage;
        _setErrorMessage(errorMessage);
        if (generation == _listingSession.generation()) {
            _finishListing(ListingResult::Failed);
        }
        return;
    }

    _startListing();
}

void FtpTransport::_startListing()
{
    const quint64 generation = _listingSession.begin(QString::fromLatin1(kMavlinkLogRoot));

    _setListing(true);
    if (!_listingSession.isCurrent(generation) || !_vehicle) {
        return;
    }
    _listRoot();
}

void FtpTransport::_listRoot()
{
    const quint64 generation = _listingSession.generation();

    qCDebug(FtpTransportLog) << "listing root" << _listingSession.root();

    QPointer<Vehicle> vehicle(_vehicle);
    FTPListDirectoryJob* const listingJob =
        vehicle ? vehicle->ftpManager()->startListDirectory(MAV_COMP_ID_AUTOPILOT1, _listingSession.root(),
                                                            _listingSession.remainingEntryBudget())
                : nullptr;
    if (!_listingSession.isCurrent(generation) || (vehicle != _vehicle)) {
        if (listingJob) {
            listingJob->cancel();
        }
        return;
    }
    if (!listingJob) {
        const QString errorMessage = tr("Unable to start the onboard log directory listing");
        qCWarning(FtpTransportLog) << errorMessage << _listingSession.root();
        _setErrorMessage(errorMessage);
        if (!_listingSession.isCurrent(generation)) {
            return;
        }
        _finishListing(ListingResult::Failed);
        return;
    }

    _listingJob = listingJob;
    (void) connect(listingJob, &FTPListDirectoryJob::finished, this, &FtpTransport::_listDirComplete);
}

void FtpTransport::_listDirComplete(const QStringList& dirList, const QString& errorMsg, bool truncated)
{
    _listingJob = nullptr;
    if (_listingSession.canceling() || !_listingSession.active()) {
        return;
    }

    const quint64 generation = _listingSession.generation();
    const auto listingIsCurrent = [this, generation]() { return _listingSession.isCurrent(generation); };

    if (!errorMsg.isEmpty()) {
        if (_listingSession.state() == FtpListingSession::State::ListingRoot) {
            if (_vehicle) {
                const QString fallback = _vehicle->firmwarePlugin()->onboardLogPolicy(_vehicle).ftpFallbackDirectory;
                const QString failedRoot = _listingSession.root();
                if (!fallback.isEmpty() && _listingSession.useFallbackRoot(fallback)) {
                    qCDebug(FtpTransportLog) << "root listing of" << failedRoot << "failed (" << errorMsg
                                             << "), falling back to" << fallback;
                    _listRoot();
                    return;
                }
            }

            const QString errorMessage = tr("Unable to list onboard logs: %1").arg(errorMsg);
            qCWarning(FtpTransportLog) << errorMessage;
            _setErrorMessage(errorMessage);
            if (!listingIsCurrent()) {
                return;
            }
            _finishListing(ListingResult::Failed);
            return;
        }

        const QString currentDirectory = _listingSession.currentDirectory();
        const QString errorMessage = tr("Unable to list onboard log directory %1: %2").arg(currentDirectory, errorMsg);
        qCWarning(FtpTransportLog) << errorMessage;
        _listingSession.markPartial();
        _setErrorMessage(errorMessage);
        if (!listingIsCurrent()) {
            return;
        }
        _listingSession.completeCurrentDirectory();
        _listNextSubdir();
        return;
    }

    if (_listingSession.observeDirectoryResult(dirList.size(), truncated)) {
        _setErrorMessage(
            tr("The onboard log scan reached its %1-entry safety limit; some directories may not have been scanned")
                .arg(FtpListingSession::kMaxListingEntries));
        if (!listingIsCurrent()) {
            return;
        }
    }

    if (_listingSession.state() == FtpListingSession::State::ListingRoot) {
        // The root listing may contain log files directly (flat layout, e.g. @MAV_LOG)
        // and/or date subdirectories to descend into (physical fallback directories).
        const uint flatLogs = _processFileEntries(dirList, QString());
        if (!listingIsCurrent()) {
            return;
        }

        _listingSession.beginSubdirectories();
        qCDebug(FtpTransportLog) << "root listing of" << _listingSession.root() << "found" << flatLogs
                                 << "flat logs and" << _listingSession.pendingDirectoryCount() << "subdirectories";

        _listNextSubdir();
        return;
    }

    const QString currentDir = _listingSession.currentDirectory();
    const uint logsFoundInDir = _processFileEntries(dirList, currentDir);
    if (!listingIsCurrent()) {
        return;
    }

    qCDebug(FtpTransportLog) << currentDir << "->" << logsFoundInDir << "logs";

    _listingSession.completeCurrentDirectory();

    _listNextSubdir();
}

uint FtpTransport::_processFileEntries(const QStringList& dirList, const QString& subdir)
{
    const quint64 generation = _listingSession.generation();
    const FtpListingParser::ParseResult result = _listingSession.parse(dirList, subdir, subdir.isEmpty());
    QList<OnboardLogEntry*> newEntries;
    newEntries.reserve(result.logs.size());

    for (const QString& unsafePath : result.unsafePaths) {
        qCWarning(FtpTransportLog) << "ignoring unsafe FTP path component:" << unsafePath;
    }
    if (!result.unsafePaths.isEmpty()) {
        _setErrorMessage(tr("The vehicle reported %1 unsafe onboard log path or paths; those entries were omitted")
                             .arg(result.unsafePaths.size()));
        if (!_listingSession.isCurrent(generation)) {
            return 0;
        }
    }
    if (result.malformedSupportedLogs > 0) {
        const QString errorMessage =
            tr("The vehicle reported %1 malformed onboard log entry or entries; those logs were omitted")
                .arg(result.malformedSupportedLogs);
        qCWarning(FtpTransportLog) << errorMessage;
        _setErrorMessage(errorMessage);
        if (!_listingSession.isCurrent(generation)) {
            return 0;
        }
    }
    if (result.logLimitReached) {
        _setErrorMessage(tr("The vehicle reported more than %1 onboard logs; only the first %1 are shown")
                             .arg(FtpListingParser::kMaxLogEntries));
        if (!_listingSession.isCurrent(generation)) {
            return 0;
        }
    }
    if (result.directoryLimitReached) {
        _setErrorMessage(tr("The vehicle reported more than %1 log directories; only the first %1 were scanned")
                             .arg(FtpListingParser::kMaxLogDirectories));
        if (!_listingSession.isCurrent(generation)) {
            return 0;
        }
    }
    if (!_listingSession.isCurrent(generation)) {
        return 0;
    }

    if ((result.duplicateLogs > 0) || (result.duplicateDirectories > 0)) {
        qCDebug(FtpTransportLog) << "ignored" << result.duplicateLogs << "duplicate logs and"
                                 << result.duplicateDirectories << "duplicate directories";
    }

    for (const FtpListingParser::LogDescriptor& descriptor : result.logs) {
        OnboardLogEntry* const logEntry =
            new OnboardLogEntry(descriptor.id, descriptor.time, descriptor.size, true, this);
        (void) connect(logEntry, &OnboardLogEntry::selectedChanged, this, &FtpTransport::selectionChanged);
        logEntry->setFtpPath(descriptor.ftpPath);
        logEntry->setState(OnboardLogEntry::State::Available, tr("Available"));
        newEntries.append(logEntry);
    }

    if (!newEntries.isEmpty()) {
        _logEntriesModel->append(newEntries);
    }

    return static_cast<uint>(newEntries.size());
}

void FtpTransport::_listNextSubdir()
{
    const quint64 generation = _listingSession.generation();
    while (_listingSession.canListNextDirectory()) {
        const QString subdir = _listingSession.currentDirectory();
        const QString path = _listingSession.currentDirectoryPath();

        qCDebug(FtpTransportLog) << "listing subdir" << path;

        QPointer<Vehicle> vehicle(_vehicle);
        FTPListDirectoryJob* const listingJob =
            vehicle ? vehicle->ftpManager()->startListDirectory(MAV_COMP_ID_AUTOPILOT1, path,
                                                                _listingSession.remainingEntryBudget())
                    : nullptr;
        if (!_listingSession.isCurrent(generation) || (vehicle != _vehicle)) {
            if (listingJob) {
                listingJob->cancel();
            }
            return;
        }
        if (listingJob) {
            _listingJob = listingJob;
            (void) connect(listingJob, &FTPListDirectoryJob::finished, this, &FtpTransport::_listDirComplete);
            return;
        }

        const QString errorMessage = tr("Unable to list onboard log directory %1").arg(subdir);
        qCWarning(FtpTransportLog) << errorMessage;
        _listingSession.markPartial();
        _setErrorMessage(errorMessage);
        if (!_listingSession.isCurrent(generation)) {
            return;
        }
        _listingSession.completeCurrentDirectory();
    }

    if (_listingSession.hasPendingDirectories() && _listingSession.logLimitReached()) {
        _listingSession.markPartial();
        _setErrorMessage(tr("The vehicle reported more than %1 onboard logs; only the first %1 are shown")
                             .arg(FtpListingParser::kMaxLogEntries));
    } else if (_listingSession.hasPendingDirectories() && (_listingSession.remainingEntryBudget() == 0)) {
        _listingSession.markPartial();
        _setErrorMessage(
            tr("The onboard log scan reached its %1-entry safety limit; some directories may not have been scanned")
                .arg(FtpListingSession::kMaxListingEntries));
    }
    if (!_listingSession.isCurrent(generation)) {
        return;
    }
    qCDebug(FtpTransportLog) << "listing complete, found" << _logEntriesModel->count() << "logs";
    _finishListing(_listingSession.partial() ? ListingResult::Partial : ListingResult::Success);
}

void FtpTransport::_finishListing(ListingResult result)
{
    const quint64 generation = _listingSession.generation();
    _listingJob = nullptr;
    _listingSession.finish();
    _setListing(false);
    if ((generation == _listingSession.generation()) && !_listingSession.active()) {
        emit listingFinished(result);
    }
}

void FtpTransport::download(const QString& path)
{
    if (busy()) {
        qCWarning(FtpTransportLog) << "Ignoring download request: another onboard log operation is in progress";
        return;
    }

    const QString dir = path.isEmpty() ? SettingsManager::instance()->appSettings()->logSavePath() : path;
    _downloadToDirectory(dir);
}

void FtpTransport::_downloadToDirectory(const QString& dir)
{
    _setErrorMessage(QString());
    _setBatchProgress(0.);

    QString downloadPath = dir;
    QList<QPointer<OnboardLogEntry>> entries;
    _downloadSession.clear();
    _batchState.reset();
    const int numLogs = _logEntriesModel->count();
    entries.reserve(numLogs);
    for (int i = 0; i < numLogs; i++) {
        OnboardLogEntry* const entry = _logEntriesModel->value<OnboardLogEntry*>(i);
        if (entry && entry->selected() && !entry->ftpPath().isEmpty()) {
            entries.append(entry);
            _batchState.addFile(entry->size());
        }
    }

    const QString noSelectionError = tr("No selected onboard logs are available for FTP download");
    if (!_prepareDownload(_vehicle, downloadPath, !entries.isEmpty(), noSelectionError)) {
        qCWarning(FtpTransportLog) << errorMessage();
        _batchState.reset();
        return;
    }

    const quint64 generation = _downloadSession.begin(entries, downloadPath);
    qCDebug(FtpTransportLog) << "queued" << _downloadSession.pendingCount() << "logs for download to"
                             << _downloadSession.path();
    _setDownloading(true);
    if (!_downloadSession.isCurrent(generation) || !downloadingLogs()) {
        return;
    }

    _downloadNext();
}

void FtpTransport::_downloadNext()
{
    if (!downloadingLogs() || !_downloadSession.active() || _downloadSession.canceling() ||
        _downloadSession.currentEntry()) {
        return;
    }

    if (!_vehicle) {
        const QString errorMessage = tr("The active vehicle became unavailable during the onboard log download");
        qCWarning(FtpTransportLog) << errorMessage;
        _setErrorMessage(errorMessage);
        _downloadSession.finish();
        _resetSelection(false, tr("Skipped because the active vehicle became unavailable"));
        _setDownloading(false);
        return;
    }

    while (_downloadSession.pendingCount() > 0) {
        QPointer<OnboardLogEntry> entry = _downloadSession.takeNext();
        if (entry && !entry->ftpPath().isEmpty()) {
            _downloadEntry(entry);
            return;
        }
        _downloadSession.completeCurrent();
    }

    _downloadSession.finish();
    _setBatchProgress(1.);
    _resetSelection();
    _setDownloading(false);
}

void FtpTransport::_scheduleDownloadNext()
{
    const quint64 generation = _downloadSession.generation();
    QTimer::singleShot(0, this, [this, generation]() {
        if (generation == _downloadSession.generation()) {
            _downloadNext();
        }
    });
}

void FtpTransport::_downloadEntry(OnboardLogEntry* entry)
{
    if (!entry) {
        return;
    }
    if (!_vehicle) {
        const QString errorMessage = tr("The active vehicle became unavailable during the onboard log download");
        qCWarning(FtpTransportLog) << errorMessage;
        _setEntryError(entry, errorMessage);
        _downloadSession.finish();
        _resetSelection(false, tr("Skipped because the active vehicle became unavailable"));
        _setDownloading(false);
        return;
    }

    const quint64 generation = _downloadSession.generation();
    QPointer<OnboardLogEntry> guardedEntry(entry);
    const uint64_t entrySize = entry->size();

    {
        const QSignalBlocker blocker(this);
        entry->setSelected(false);
    }
    if (!guardedEntry || !_downloadSession.isCurrent(generation, guardedEntry) || !downloadingLogs()) {
        return;
    }

    entry->setState(OnboardLogEntry::State::Downloading, tr("Downloading"));
    if (!guardedEntry || !_downloadSession.isCurrent(generation, guardedEntry) || !downloadingLogs()) {
        return;
    }
    entry->setErrorMessage(QString());
    if (!guardedEntry || !_downloadSession.isCurrent(generation, guardedEntry) || !downloadingLogs()) {
        return;
    }

    const OnboardLogFileName::UniquePath localPath =
        OnboardLogFileName::uniquePath(_downloadSession.path(), _localFilenameForEntry(*entry));

    FTPManager* const ftp = _vehicle->ftpManager();

    qCDebug(FtpTransportLog) << "downloading" << entry->ftpPath() << "to" << localPath.filePath;

    _downloadSession.setInFlight(true);
    FTPDownloadJob* const downloadJob =
        ftp->startDownload(MAV_COMP_ID_AUTOPILOT1, entry->ftpPath(), _downloadSession.path(), localPath.fileName, true,
                           FTPManager::ExistingFilePolicy::FailIfExists);
    if (!downloadJob) {
        _downloadSession.setInFlight(false);
        const QString errorMessage =
            tr("Unable to start FTP download for %1").arg(QFileInfo(entry->ftpPath()).fileName());
        qCWarning(FtpTransportLog) << errorMessage;
        _setEntryError(guardedEntry, errorMessage);
        if (!guardedEntry || !_downloadSession.isCurrent(generation, guardedEntry) || !downloadingLogs()) {
            return;
        }
        _batchState.completeFile(entrySize);
        _updateBatchProgress();
        if (!_downloadSession.isCurrent(generation, guardedEntry) || !downloadingLogs()) {
            return;
        }
        _downloadSession.completeCurrent();
        _scheduleDownloadNext();
        return;
    }

    _downloadJob = downloadJob;
    (void) connect(downloadJob, &FTPDownloadJob::finished, this, &FtpTransport::_downloadComplete);
    (void) connect(downloadJob, &FTPJob::progress, this, &FtpTransport::_downloadProgress);
}

QString FtpTransport::_localFilenameForEntry(const OnboardLogEntry& entry)
{
    const QString sourceSuffix = QFileInfo(entry.ftpPath()).suffix().toLower();
    const QString suffix = (sourceSuffix == QStringLiteral("bin") || sourceSuffix == QStringLiteral("ulg"))
                               ? sourceSuffix
                               : QStringLiteral("ulg");

    QString baseName;
    if (entry.time().isValid() && entry.time().date().year() >= 2010) {
        baseName = entry.time().toString(QStringLiteral("yyyy-M-d-hh-mm-ss"));
    } else {
        baseName = QStringLiteral("log_") + QString::number(entry.id());
    }

    return baseName + QStringLiteral(".") + suffix;
}

void FtpTransport::_downloadComplete(const QString& file, const QString& errorMsg, const QString& warningMsg)
{
    _downloadJob = nullptr;
    const bool wasInFlight = _downloadSession.inFlight();
    _downloadSession.setInFlight(false);
    if (_downloadSession.canceling()) {
        qCDebug(FtpTransportLog) << "download canceled:" << file << errorMsg;
        _finishDownloadCancellation();
        return;
    }

    if (!_downloadSession.currentEntry()) {
        if (wasInFlight && _downloadSession.active()) {
            const quint64 generation = _downloadSession.generation();
            const uint64_t entrySize = _downloadSession.currentEntrySize();
            const QString completionMessage =
                errorMsg.isEmpty() ? warningMsg : tr("FTP download failed: %1").arg(errorMsg);
            if (!completionMessage.isEmpty()) {
                _setErrorMessage(completionMessage);
                if (!_downloadSession.isCurrent(generation) || !downloadingLogs()) {
                    return;
                }
            }
            _downloadSession.completeCurrent();
            _batchState.completeFile(entrySize);
            _updateBatchProgress();
            if (_downloadSession.isCurrent(generation) && downloadingLogs()) {
                _scheduleDownloadNext();
            }
        }
        return;
    }

    const quint64 generation = _downloadSession.generation();
    QPointer<OnboardLogEntry> entry(_downloadSession.currentEntry());
    const uint64_t entrySize = _downloadSession.currentEntrySize();
    _downloadSession.completeCurrent();
    _batchState.completeFile(entrySize);
    if (!entry) {
        return;
    }

    if (errorMsg.isEmpty()) {
        entry->setErrorMessage(warningMsg);
        if (!entry || !_downloadSession.isCurrent(generation) || !downloadingLogs()) {
            return;
        }
        entry->setState(OnboardLogEntry::State::Downloaded, tr("Downloaded"));
        qCDebug(FtpTransportLog) << "download complete" << file;
        if (!warningMsg.isEmpty()) {
            qCWarning(FtpTransportLog) << warningMsg;
            _setErrorMessage(warningMsg);
        }
    } else {
        const QString errorMessage = tr("FTP download failed: %1").arg(errorMsg);
        _setEntryError(entry, errorMessage);
        qCWarning(FtpTransportLog) << errorMessage;
    }

    if (!entry || !_downloadSession.isCurrent(generation) || !downloadingLogs()) {
        return;
    }
    _updateBatchProgress();
    if (!_downloadSession.isCurrent(generation) || !downloadingLogs()) {
        return;
    }
    _scheduleDownloadNext();
}

void FtpTransport::_downloadProgress(float value)
{
    const quint64 generation = _downloadSession.generation();
    QPointer<OnboardLogEntry> entry(_downloadSession.currentEntry());
    if (!entry) {
        return;
    }

    const std::optional<FtpDownloadSession::ProgressUpdate> progress =
        _downloadSession.updateProgress(value, kGuiUpdateInterval);
    if (!progress.has_value()) {
        return;
    }

    const QString status = QStringLiteral("%1 (%2/s)")
                               .arg(QGC::bigSizeToString(progress->bytes))
                               .arg(QGC::bigSizeToString(progress->rate));

    entry->setStatus(status);
    if (!entry || !_downloadSession.isCurrent(generation, entry)) {
        return;
    }
    _updateBatchProgress(progress->bytes, progress->progress);
}

void FtpTransport::cancel()
{
    FTPManager* const ftp = _vehicle ? _vehicle->ftpManager() : nullptr;
    _cancelListing(ftp);
    _cancelDownload(ftp);
    _cancelErase(ftp);

    _resetSelection(true);
}

void FtpTransport::_cancelListing(FTPManager* ftpManager)
{
    if (!_listingSession.cancel()) {
        return;
    }

    if (ftpManager) {
        if (_listingJob) {
            _listingJob->cancel();
        } else {
            ftpManager->cancelListDirectory();
        }
    }
    _finishListing(ListingResult::Canceled);
}

void FtpTransport::_cancelDownload(FTPManager* ftpManager)
{
    if (_downloadSession.canceling()) {
        return;
    }

    const FtpDownloadSession::Cancellation cancellation = _downloadSession.cancel();
    if (!cancellation.wasActive) {
        return;
    }

    if (cancellation.currentEntry && _logEntriesModel->contains(cancellation.currentEntry)) {
        cancellation.currentEntry->setState(OnboardLogEntry::State::Canceled, tr("Canceled"));
    }
    if (cancellation.cancelRemoteDownload && ftpManager) {
        if (_downloadJob) {
            _downloadJob->cancel();
        } else {
            ftpManager->cancelDownload();
        }
    } else {
        _finishDownloadCancellation();
    }
}

void FtpTransport::_finishDownloadCancellation()
{
    const bool refreshRequested = _downloadSession.finishCancellation();
    _setDownloading(false);
    _setBatchProgress(0.);

    if (refreshRequested) {
        _refreshAfterCancel();
    }
}

void FtpTransport::_cancelErase(FTPManager* ftpManager)
{
    if (_eraseSession.canceling()) {
        return;
    }

    const FtpEraseSession::Cancellation cancellation = _eraseSession.cancel();
    if (!cancellation.wasActive) {
        return;
    }

    for (const QPointer<OnboardLogEntry>& entry : cancellation.pendingEntries) {
        if (_eraseEntryIsCurrent(entry)) {
            entry->setState(OnboardLogEntry::State::Available, tr("Available"));
        }
    }
    if (_eraseEntryIsCurrent(cancellation.currentEntry)) {
        cancellation.currentEntry->setState(OnboardLogEntry::State::Canceled, tr("Canceled"));
    }
    if (ftpManager) {
        if (_deleteJob) {
            _deleteJob->cancel();
        } else {
            ftpManager->cancelDelete();
        }
    }
    _eraseSession.finishCancellation();
    _eraseEntryIndexes.clear();
    _setIdle(_vehicle);
}

void FtpTransport::eraseAll()
{
    if (busy()) {
        qCWarning(FtpTransportLog) << "Ignoring erase request: another onboard log operation is in progress";
        return;
    }

    if (!_vehicle) {
        const QString errorMessage = tr("No active vehicle is available for onboard log erase");
        qCWarning(FtpTransportLog) << errorMessage;
        _setErrorMessage(errorMessage);
        return;
    }
    if (_eraseSession.active()) {
        return;
    }

    QList<QPointer<OnboardLogEntry>> entries;
    QHash<const OnboardLogEntry*, QPersistentModelIndex> entryIndexes;
    const int numLogs = _logEntriesModel->count();
    entries.reserve(numLogs);
    entryIndexes.reserve(numLogs);
    for (int i = 0; i < numLogs; i++) {
        OnboardLogEntry* const entry = _logEntriesModel->value<OnboardLogEntry*>(i);
        if (entry && !entry->ftpPath().isEmpty()) {
            entries.append(entry);
            entryIndexes.insert(entry, QPersistentModelIndex(_logEntriesModel->index(i, 0)));
        }
    }

    if (entries.isEmpty()) {
        qCWarning(FtpTransportLog) << "eraseAll: nothing to delete";
        return;
    }

    _eraseEntryIndexes = std::move(entryIndexes);
    const quint64 generation = _eraseSession.begin(entries);
    QPointer<Vehicle> vehicle(_vehicle);
    const auto eraseIsCurrent = [this, generation, &vehicle]() {
        return (generation == _eraseSession.generation()) && _eraseSession.active() && vehicle && (vehicle == _vehicle);
    };

    _setOperation(Operation::Erasing, vehicle);
    if (!eraseIsCurrent()) {
        return;
    }
    _setErrorMessage(QString());
    if (!eraseIsCurrent()) {
        return;
    }

    for (const QPointer<OnboardLogEntry>& entry : std::as_const(entries)) {
        if (!_eraseEntryIsCurrent(entry)) {
            continue;
        }

        entry->setState(OnboardLogEntry::State::Queued, tr("Waiting"));
        if (!eraseIsCurrent()) {
            return;
        }
        if (!_eraseEntryIsCurrent(entry)) {
            continue;
        }
        entry->setErrorMessage(QString());
        if (!eraseIsCurrent()) {
            return;
        }
    }

    if (!_eraseSession.active()) {
        _finishErase();
        return;
    }

    qCDebug(FtpTransportLog) << "queued" << _eraseSession.pendingCount() << "files for delete";

    if (!eraseIsCurrent()) {
        return;
    }

    _eraseNext();
}

void FtpTransport::_eraseNext()
{
    if (!_eraseSession.active() || _eraseSession.canceling() || _eraseSession.currentEntry()) {
        return;
    }

    QPointer<OnboardLogEntry> entry = _eraseSession.takeNext();
    while (entry && !_eraseEntryIsCurrent(entry)) {
        _eraseEntryIndexes.remove(entry.data());
        _eraseSession.completeCurrent(false);
        entry = _eraseSession.takeNext();
    }

    if (!entry) {
        _finishErase();
        return;
    }

    const quint64 generation = _eraseSession.generation();
    QPointer<Vehicle> vehicle(_vehicle);
    const QString ftpPath = entry->ftpPath();
    const auto eraseIsCurrent = [this, generation, &vehicle, &entry]() {
        return (generation == _eraseSession.generation()) && _eraseSession.active() && vehicle &&
               (vehicle == _vehicle) && entry && (_eraseSession.currentEntry() == entry) && _eraseEntryIsCurrent(entry);
    };

    entry->setState(OnboardLogEntry::State::Erasing, tr("Deleting"));
    if (!eraseIsCurrent()) {
        return;
    }
    entry->setErrorMessage(QString());
    if (!eraseIsCurrent()) {
        return;
    }

    FTPDeleteJob* const deleteJob = vehicle->ftpManager()->startDeleteFile(MAV_COMP_ID_AUTOPILOT1, ftpPath);
    if (!deleteJob) {
        if (!eraseIsCurrent()) {
            return;
        }
        const QString errorMessage = tr("Unable to start deletion of %1").arg(QFileInfo(ftpPath).fileName());
        qCWarning(FtpTransportLog) << errorMessage;
        entry->setState(OnboardLogEntry::State::Error, tr("Error"));
        if (!eraseIsCurrent()) {
            return;
        }
        entry->setErrorMessage(errorMessage);
        if (!eraseIsCurrent()) {
            return;
        }
        _setErrorMessage(errorMessage);
        if (!eraseIsCurrent()) {
            return;
        }
        _eraseSession.completeCurrent(true);
        _scheduleEraseNext();
        return;
    }

    _deleteJob = deleteJob;
    (void) connect(deleteJob, &FTPDeleteJob::finished, this, &FtpTransport::_deleteComplete);
}

void FtpTransport::_scheduleEraseNext()
{
    const quint64 generation = _eraseSession.generation();
    QTimer::singleShot(0, this, [this, generation]() {
        if (generation == _eraseSession.generation()) {
            _eraseNext();
        }
    });
}

void FtpTransport::_deleteComplete(const QString& file, const QString& errorMsg)
{
    _deleteJob = nullptr;
    if (_eraseSession.canceling() || !_eraseSession.currentEntry()) {
        return;
    }

    const quint64 generation = _eraseSession.generation();
    QPointer<Vehicle> vehicle(_vehicle);
    QPointer<OnboardLogEntry> entry(_eraseSession.currentEntry());
    _eraseSession.completeCurrent(!errorMsg.isEmpty());

    const auto eraseIsCurrent = [this, generation, &vehicle, &entry]() {
        return (generation == _eraseSession.generation()) && _eraseSession.active() && vehicle &&
               (vehicle == _vehicle) && entry && _eraseEntryIsCurrent(entry);
    };

    if (!eraseIsCurrent()) {
        qCWarning(FtpTransportLog) << "delete completed for an entry outside the active model:" << file;
        if ((generation == _eraseSession.generation()) && _eraseSession.active()) {
            _scheduleEraseNext();
        }
        return;
    }

    if (errorMsg.isEmpty()) {
        qCDebug(FtpTransportLog) << "deleted" << file;
        // Remove the row from the model so the UI reflects the erase immediately.
        _logEntriesModel->removeOne(entry);
        _eraseEntryIndexes.remove(entry.data());
        if (entry) {
            entry->deleteLater();
        }
    } else {
        const QString errorMessage = tr("Unable to delete %1: %2").arg(QFileInfo(file).fileName(), errorMsg);
        qCWarning(FtpTransportLog) << errorMessage;
        entry->setState(OnboardLogEntry::State::Error, tr("Error"));
        if (!eraseIsCurrent()) {
            return;
        }
        entry->setErrorMessage(errorMessage);
        if (!eraseIsCurrent()) {
            return;
        }
        _setErrorMessage(errorMessage);
        if (!eraseIsCurrent()) {
            return;
        }
    }

    if (generation == _eraseSession.generation()) {
        _scheduleEraseNext();
    }
}

void FtpTransport::_finishErase()
{
    const quint64 generation = _eraseSession.generation();
    const uint failureCount = _eraseSession.finish();
    _eraseEntryIndexes.clear();
    _setIdle(_vehicle);
    if ((generation != _eraseSession.generation()) || _eraseSession.active()) {
        return;
    }

    if (failureCount > 0) {
        const QString errorMessage = tr("Failed to erase %1 onboard log(s)").arg(failureCount);
        qCWarning(FtpTransportLog) << errorMessage;
        _setErrorMessage(errorMessage);
    } else {
        qCDebug(FtpTransportLog) << "erase finished";
    }
}

bool FtpTransport::_eraseEntryIsCurrent(const QPointer<OnboardLogEntry>& entry) const
{
    if (!entry) {
        return false;
    }

    const auto index = _eraseEntryIndexes.constFind(entry.data());
    return (index != _eraseEntryIndexes.cend()) && index->isValid() && (index->model() == _logEntriesModel) &&
           (index->data(Qt::UserRole).value<QObject*>() == entry.data());
}

void FtpTransport::_setDownloading(bool active)
{
    OnboardLogTransport::_setDownloading(_vehicle, active);
}

void FtpTransport::_setListing(bool active)
{
    OnboardLogTransport::_setListing(_vehicle, active);
}
