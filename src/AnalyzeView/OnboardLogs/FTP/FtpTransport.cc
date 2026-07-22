#include "FtpTransport.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QPointer>
#include <QtCore/QScopeGuard>
#include <QtCore/QTimer>
#include <algorithm>
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
    _shutdown();
}

void FtpTransport::_shutdown()
{
    (void) disconnect(this, nullptr, nullptr, nullptr);
    if (_logEntriesModel) {
        (void) disconnect(_logEntriesModel, nullptr, this, nullptr);
    }

    const QPointer<FTPListDirectoryJob> listingJob = _listingJob;
    const QPointer<FTPDownloadJob> downloadJob = _downloadJob;
    const QPointer<FTPDeleteJob> deleteJob = _deleteJob;
    _listingJob = nullptr;
    _downloadJob = nullptr;
    _deleteJob = nullptr;
    for (FTPJob* const job : {static_cast<FTPJob*>(listingJob.data()), static_cast<FTPJob*>(downloadJob.data()),
                              static_cast<FTPJob*>(deleteJob.data())}) {
        if (job) {
            (void) disconnect(job, nullptr, this, nullptr);
        }
    }

    _pendingLogInsertion.reset();
    _listingSession.clear();
    _downloadSession.clear();
    _eraseSession.clear();
    _vehicle = nullptr;
    _communicationLostInhibitor.reset();
    _operation = Operation::Idle;
    _batchState.reset();

    if (listingJob) {
        listingJob->cancel();
    }
    if (downloadJob) {
        downloadJob->cancel();
    }
    if (deleteJob) {
        deleteJob->cancel();
    }
}

void FtpTransport::setVehicle(Vehicle* vehicle)
{
    if (vehicle && (vehicle == _vehicle)) {
        return;
    }

    const QPointer<FtpTransport> self(this);
    _setBatchProgress(0.);
    if (!self) {
        return;
    }
    _setErrorMessage(QString());
    if (!self) {
        return;
    }

    _cancelListing();
    if (!self) {
        return;
    }
    _cancelDownload();
    if (!self) {
        return;
    }
    _cancelErase();
    if (!self) {
        return;
    }
    if (_downloadJob) {
        (void) disconnect(_downloadJob, nullptr, this, nullptr);
        _downloadJob = nullptr;
    }
    _listingSession.clear();
    _downloadSession.clear();
    _setDownloading(false);
    if (!self) {
        return;
    }
    _eraseSession.clear();

    _logEntriesModel->clearAndDeleteContents();
    if (!self) {
        return;
    }

    _vehicle = vehicle;
}

void FtpTransport::refresh()
{
    _refreshPending = true;
    if (_cancelInProgress) {
        return;
    }
    cancel();
}

void FtpTransport::_runPendingRefresh()
{
    if (!_refreshPending) {
        return;
    }
    _refreshPending = false;
    if (_downloadSession.canceling()) {
        _downloadSession.requestRefresh();
    } else {
        _refreshAfterCancel();
    }
}

void FtpTransport::_refreshAfterCancel()
{
    const quint64 generation = _listingSession.generation();
    const QPointer<FtpTransport> self(this);
    _setErrorMessage(QString());
    if (!self || (generation != _listingSession.generation())) {
        return;
    }
    _setBatchProgress(0.);
    if (!self || (generation != _listingSession.generation())) {
        return;
    }
    _logEntriesModel->clearAndDeleteContents();
    if (!self || (generation != _listingSession.generation())) {
        return;
    }

    if (!_vehicle) {
        const QString errorMessage = tr("No active vehicle is available for onboard log refresh");
        qCWarning(FtpTransportLog) << errorMessage;
        _setErrorMessage(errorMessage);
        if (self && (generation == _listingSession.generation())) {
            _finishListing(ListingResult::Failed);
        }
        return;
    }

    _startListing();
}

void FtpTransport::_startListing()
{
    const quint64 generation = _listingSession.begin(QString::fromLatin1(kMavlinkLogRoot));
    _pendingLogInsertion = PendingLogInsertion{{}, 0, generation};

    const QPointer<FtpTransport> self(this);
    _setListing(true);
    if (!self || !_listingSession.isCurrent(generation) || !_vehicle) {
        return;
    }
    _listRoot();
}

void FtpTransport::_listRoot()
{
    const quint64 generation = _listingSession.generation();
    const QPointer<FtpTransport> self(this);

    qCDebug(FtpTransportLog) << "listing root" << _listingSession.root();

    QPointer<Vehicle> vehicle(_vehicle);
    const FTPManager::ListDirectoryStartResult startResult =
        vehicle ? vehicle->ftpManager()->startListDirectory(MAV_COMP_ID_AUTOPILOT1, _listingSession.root(),
                                                            _listingSession.remainingEntryBudget())
                : FTPManager::ListDirectoryStartResult::failure(FTPManager::StartError::InvalidArgument);
    FTPListDirectoryJob* const listingJob = startResult.job();
    if (!_listingSession.isCurrent(generation) || (vehicle != _vehicle)) {
        if (listingJob) {
            listingJob->cancel();
        }
        return;
    }
    if (!listingJob) {
        const QString errorMessage = startResult.error() == FTPManager::StartError::Busy
                                         ? tr("Vehicle FTP service is busy")
                                         : tr("Unable to start the onboard log directory listing");
        qCWarning(FtpTransportLog) << errorMessage << _listingSession.root();
        _setErrorMessage(errorMessage);
        if (!self || !_listingSession.isCurrent(generation)) {
            return;
        }
        _finishListing(ListingResult::Failed);
        return;
    }

    _listingJob = listingJob;
    const QPointer<FTPListDirectoryJob> guardedJob(listingJob);
    (void) connect(listingJob, &FTPListDirectoryJob::finished, this,
                   [this, guardedJob, generation](const QStringList& dirList, const QString& errorMsg, bool truncated) {
                       if (guardedJob && (guardedJob == _listingJob) && _listingSession.isCurrent(generation)) {
                           _listDirComplete(dirList, errorMsg, truncated);
                       }
                   });
}

void FtpTransport::_listDirComplete(const QStringList& dirList, const QString& errorMsg, bool truncated)
{
    _listingJob = nullptr;
    if (_listingSession.canceling() || !_listingSession.active()) {
        return;
    }

    const quint64 generation = _listingSession.generation();
    const QPointer<FtpTransport> self(this);
    const auto listingIsCurrent = [self, generation]() { return self && self->_listingSession.isCurrent(generation); };

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
        const std::optional<uint> flatLogs = _processFileEntries(dirList, QString());
        if (!flatLogs.has_value() || !_listingSession.isCurrent(generation)) {
            return;
        }

        _listingSession.beginSubdirectories();
        qCDebug(FtpTransportLog) << "root listing of" << _listingSession.root() << "found" << *flatLogs
                                 << "flat logs and" << _listingSession.pendingDirectoryCount() << "subdirectories";

        _listNextSubdir();
        return;
    }

    const QString currentDir = _listingSession.currentDirectory();
    const std::optional<uint> logsFoundInDir = _processFileEntries(dirList, currentDir);
    if (!logsFoundInDir.has_value() || !_listingSession.isCurrent(generation)) {
        return;
    }

    qCDebug(FtpTransportLog) << currentDir << "->" << *logsFoundInDir << "logs";
    _listingSession.completeCurrentDirectory();
    _listNextSubdir();
}

std::optional<uint> FtpTransport::_processFileEntries(const QStringList& dirList, const QString& subdir)
{
    const quint64 generation = _listingSession.generation();
    const QPointer<FtpTransport> self(this);
    FtpListingParser::ParseResult result = _listingSession.parse(dirList, subdir, subdir.isEmpty());

    for (const QString& unsafePath : result.unsafePaths) {
        qCWarning(FtpTransportLog) << "ignoring unsafe FTP path component:" << unsafePath;
    }
    if (!result.unsafePaths.isEmpty()) {
        _setErrorMessage(tr("The vehicle reported %1 unsafe onboard log path or paths; those entries were omitted")
                             .arg(result.unsafePaths.size()));
        if (!self || !_listingSession.isCurrent(generation)) {
            return std::nullopt;
        }
    }
    if (result.malformedSupportedLogs > 0) {
        const QString errorMessage =
            tr("The vehicle reported %1 malformed onboard log entry or entries; those logs were omitted")
                .arg(result.malformedSupportedLogs);
        qCWarning(FtpTransportLog) << errorMessage;
        _setErrorMessage(errorMessage);
        if (!self || !_listingSession.isCurrent(generation)) {
            return std::nullopt;
        }
    }
    if (result.logLimitReached) {
        _setErrorMessage(tr("The vehicle reported more than %1 onboard logs; only the first %1 are shown")
                             .arg(FtpListingParser::kMaxLogEntries));
        if (!self || !_listingSession.isCurrent(generation)) {
            return std::nullopt;
        }
    }
    if (result.directoryLimitReached) {
        _setErrorMessage(tr("The vehicle reported more than %1 log directories; only the first %1 were scanned")
                             .arg(FtpListingParser::kMaxLogDirectories));
        if (!self || !_listingSession.isCurrent(generation)) {
            return std::nullopt;
        }
    }
    if (!_listingSession.isCurrent(generation)) {
        return std::nullopt;
    }

    if ((result.duplicateLogs > 0) || (result.duplicateDirectories > 0)) {
        qCDebug(FtpTransportLog) << "ignored" << result.duplicateLogs << "duplicate logs and"
                                 << result.duplicateDirectories << "duplicate directories";
    }

    if (!_pendingLogInsertion.has_value()) {
        _pendingLogInsertion = PendingLogInsertion{{}, 0, generation};
    } else if (_pendingLogInsertion->generation != generation) {
        return std::nullopt;
    }

    const uint logCount = static_cast<uint>(result.logs.size());
    _pendingLogInsertion->descriptors.append(std::move(result.logs));
    return logCount;
}

void FtpTransport::_appendNextLogEntryBatch()
{
    if (!_pendingLogInsertion.has_value()) {
        return;
    }

    const quint64 generation = _pendingLogInsertion->generation;
    const QPointer<FtpTransport> self(this);
    if (!_listingSession.isCurrent(generation)) {
        _pendingLogInsertion.reset();
        return;
    }

    const qsizetype begin = _pendingLogInsertion->nextIndex;
    const qsizetype end = (std::min) (begin + kLogInsertionBatchSize, _pendingLogInsertion->descriptors.size());
    QList<OnboardLogEntry*> newEntries;
    newEntries.reserve(end - begin);
    for (qsizetype index = begin; index < end; ++index) {
        const FtpListingParser::LogDescriptor& descriptor = _pendingLogInsertion->descriptors.at(index);
        OnboardLogEntry* const logEntry =
            new OnboardLogEntry(descriptor.id, descriptor.time, descriptor.size, true, this);
        (void) connect(logEntry, &OnboardLogEntry::selectedChanged, this, &FtpTransport::selectionChanged);
        logEntry->setFtpPath(descriptor.ftpPath);
        const QPointer<OnboardLogEntry> guardedEntry(logEntry);
        logEntry->setState(OnboardLogEntry::State::Available, tr("Available"));
        if (!self) {
            return;
        }
        if (!guardedEntry) {
            continue;
        }
        newEntries.append(logEntry);
    }

    if (!_listingSession.isCurrent(generation)) {
        qDeleteAll(newEntries);
        _pendingLogInsertion.reset();
        return;
    }
    if (!newEntries.isEmpty()) {
        _logEntriesModel->append(newEntries);
        if (!self) {
            return;
        }
    }
    if (!_pendingLogInsertion.has_value() || (_pendingLogInsertion->generation != generation) ||
        !_listingSession.isCurrent(generation)) {
        return;
    }

    _pendingLogInsertion->nextIndex = end;
    if (end < _pendingLogInsertion->descriptors.size()) {
        QTimer::singleShot(0, this, &FtpTransport::_appendNextLogEntryBatch);
        return;
    }

    _finishListing(_listingSession.partial() ? ListingResult::Partial : ListingResult::Success);
}

void FtpTransport::_listNextSubdir()
{
    const quint64 generation = _listingSession.generation();
    const QPointer<FtpTransport> self(this);
    while (_listingSession.canListNextDirectory()) {
        const QString subdir = _listingSession.currentDirectory();
        const QString path = _listingSession.currentDirectoryPath();

        qCDebug(FtpTransportLog) << "listing subdir" << path;

        QPointer<Vehicle> vehicle(_vehicle);
        const FTPManager::ListDirectoryStartResult startResult =
            vehicle ? vehicle->ftpManager()->startListDirectory(MAV_COMP_ID_AUTOPILOT1, path,
                                                                _listingSession.remainingEntryBudget())
                    : FTPManager::ListDirectoryStartResult::failure(FTPManager::StartError::InvalidArgument);
        FTPListDirectoryJob* const listingJob = startResult.job();
        if (!_listingSession.isCurrent(generation) || (vehicle != _vehicle)) {
            if (listingJob) {
                listingJob->cancel();
            }
            return;
        }
        if (listingJob) {
            _listingJob = listingJob;
            const QPointer<FTPListDirectoryJob> guardedJob(listingJob);
            (void) connect(
                listingJob, &FTPListDirectoryJob::finished, this,
                [this, guardedJob, generation](const QStringList& dirList, const QString& errorMsg, bool truncated) {
                    if (guardedJob && (guardedJob == _listingJob) && _listingSession.isCurrent(generation)) {
                        _listDirComplete(dirList, errorMsg, truncated);
                    }
                });
            return;
        }

        const QString errorMessage = startResult.error() == FTPManager::StartError::Busy
                                         ? tr("Vehicle FTP service is busy")
                                         : tr("Unable to list onboard log directory %1").arg(subdir);
        qCWarning(FtpTransportLog) << errorMessage;
        _listingSession.markPartial();
        _setErrorMessage(errorMessage);
        if (!self || !_listingSession.isCurrent(generation)) {
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
    if (!self || !_listingSession.isCurrent(generation)) {
        return;
    }
    const qsizetype logCount = _pendingLogInsertion.has_value() ? _pendingLogInsertion->descriptors.size() : 0;
    qCDebug(FtpTransportLog) << "remote listing complete, found" << logCount << "logs";
    if (logCount == 0) {
        _finishListing(_listingSession.partial() ? ListingResult::Partial : ListingResult::Success);
    } else {
        _appendNextLogEntryBatch();
    }
}

void FtpTransport::_finishListing(ListingResult result)
{
    const quint64 generation = _listingSession.generation();
    _pendingLogInsertion.reset();
    _listingJob = nullptr;
    _listingSession.finish();
    const QPointer<FtpTransport> self(this);
    _setListing(false);
    if (self && (generation == _listingSession.generation()) && !_listingSession.active()) {
        emit listingFinished(result);
    }
}

void FtpTransport::download(const QString& path)
{
    if (_cancelInProgress) {
        qCWarning(FtpTransportLog) << "Ignoring download request while onboard log cancellation is in progress";
        return;
    }
    if (busy()) {
        qCWarning(FtpTransportLog) << "Ignoring download request: another onboard log operation is in progress";
        return;
    }

    const QString dir = path.isEmpty() ? SettingsManager::instance()->appSettings()->logSavePath() : path;
    _downloadToDirectory(dir);
}

void FtpTransport::_downloadToDirectory(const QString& dir)
{
    const QPointer<FtpTransport> self(this);
    _setErrorMessage(QString());
    if (!self) {
        return;
    }
    _setBatchProgress(0.);
    if (!self) {
        return;
    }

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
        if (!self) {
            return;
        }
        qCWarning(FtpTransportLog) << errorMessage();
        _batchState.reset();
        return;
    }

    const quint64 generation = _downloadSession.begin(entries, downloadPath);
    qCDebug(FtpTransportLog) << "queued" << _downloadSession.pendingCount() << "logs for download to"
                             << _downloadSession.path();
    _setDownloading(true);
    if (!self || !_downloadSession.isCurrent(generation) || !downloadingLogs()) {
        return;
    }

    _downloadNext();
}

void FtpTransport::_downloadNext()
{
    const QPointer<FtpTransport> self(this);
    if (!downloadingLogs() || !_downloadSession.active() || _downloadSession.canceling() ||
        _downloadSession.currentEntry()) {
        return;
    }

    if (!_vehicle) {
        const QString errorMessage = tr("The active vehicle became unavailable during the onboard log download");
        qCWarning(FtpTransportLog) << errorMessage;
        _setErrorMessage(errorMessage);
        if (!self) {
            return;
        }
        _downloadSession.finish();
        _resetSelection(false, tr("Skipped because the active vehicle became unavailable"));
        if (!self) {
            return;
        }
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
    if (!self) {
        return;
    }
    _resetSelection();
    if (!self) {
        return;
    }
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
    const QPointer<FtpTransport> self(this);
    if (!_vehicle) {
        const QString errorMessage = tr("The active vehicle became unavailable during the onboard log download");
        qCWarning(FtpTransportLog) << errorMessage;
        _setEntryError(entry, errorMessage);
        if (!self) {
            return;
        }
        _downloadSession.finish();
        _resetSelection(false, tr("Skipped because the active vehicle became unavailable"));
        if (!self) {
            return;
        }
        _setDownloading(false);
        return;
    }

    const quint64 generation = _downloadSession.generation();
    QPointer<OnboardLogEntry> guardedEntry(entry);
    const QPointer<Vehicle> vehicle(_vehicle);
    const uint64_t entrySize = entry->size();

    const bool signalsWereBlocked = blockSignals(true);
    entry->setSelected(false);
    if (!self) {
        return;
    }
    blockSignals(signalsWereBlocked);
    if (!_downloadEntryIsCurrent(generation, guardedEntry) || (_vehicle != vehicle)) {
        if (!guardedEntry || !_logEntriesModel->contains(guardedEntry)) {
            _completeRemovedDownloadEntry(generation, entrySize);
        }
        return;
    }

    entry->setState(OnboardLogEntry::State::Downloading, tr("Downloading"));
    if (!self) {
        return;
    }
    if (!_downloadEntryIsCurrent(generation, guardedEntry) || (_vehicle != vehicle)) {
        if (!guardedEntry || !_logEntriesModel->contains(guardedEntry)) {
            _completeRemovedDownloadEntry(generation, entrySize);
        }
        return;
    }
    entry->setErrorMessage(QString());
    if (!self) {
        return;
    }
    if (!_downloadEntryIsCurrent(generation, guardedEntry) || (_vehicle != vehicle)) {
        if (!guardedEntry || !_logEntriesModel->contains(guardedEntry)) {
            _completeRemovedDownloadEntry(generation, entrySize);
        }
        return;
    }

    const OnboardLogFileName::UniquePath localPath =
        OnboardLogFileName::uniquePath(_downloadSession.path(), _localFilenameForEntry(*entry));

    FTPManager* const ftp = vehicle->ftpManager();

    qCDebug(FtpTransportLog) << "downloading" << entry->ftpPath() << "to" << localPath.filePath;

    _downloadSession.setInFlight(true);
    const FTPManager::DownloadStartResult startResult =
        ftp->startDownload(MAV_COMP_ID_AUTOPILOT1, entry->ftpPath(), _downloadSession.path(), localPath.fileName, true,
                           FTPManager::ExistingFilePolicy::FailIfExists, entry->size());
    FTPDownloadJob* const downloadJob = startResult.job();
    if (!downloadJob) {
        _downloadSession.setInFlight(false);
        const QString errorMessage =
            startResult.error() == FTPManager::StartError::Busy
                ? tr("Vehicle FTP service is busy")
                : tr("Unable to start FTP download for %1").arg(QFileInfo(entry->ftpPath()).fileName());
        qCWarning(FtpTransportLog) << errorMessage;
        _setEntryError(guardedEntry, errorMessage);
        if (!self) {
            return;
        }
        if (!_downloadEntryIsCurrent(generation, guardedEntry)) {
            if (!guardedEntry || !_logEntriesModel->contains(guardedEntry)) {
                _completeRemovedDownloadEntry(generation, entrySize);
            }
            return;
        }
        _batchState.completeFile(entrySize);
        _updateBatchProgress();
        if (!self || !_downloadSession.isCurrent(generation, guardedEntry) || !downloadingLogs()) {
            return;
        }
        _downloadSession.completeCurrent();
        _scheduleDownloadNext();
        return;
    }

    _downloadJob = downloadJob;
    const QPointer<FTPDownloadJob> guardedJob(downloadJob);
    (void) connect(
        downloadJob, &FTPDownloadJob::finished, this,
        [this, guardedJob, generation](const QString& file, const QString& errorMsg, const QString& warningMsg) {
            if (guardedJob && (guardedJob == _downloadJob) &&
                (_downloadSession.isCurrent(generation) || _downloadSession.canceling())) {
                _downloadComplete(file, errorMsg, warningMsg);
            }
        });
    (void) connect(downloadJob, &FTPJob::progress, this, [this, guardedJob, generation](float value) {
        if (guardedJob && (guardedJob == _downloadJob) && _downloadSession.isCurrent(generation)) {
            _downloadProgress(value);
        }
    });
}

void FtpTransport::_completeRemovedDownloadEntry(quint64 generation, uint64_t entrySize)
{
    if (!_downloadSession.isCurrent(generation) || !downloadingLogs()) {
        return;
    }

    _downloadSession.completeCurrent();
    _batchState.completeFile(entrySize);
    _continueDownload(generation);
}

void FtpTransport::_continueDownload(quint64 generation)
{
    if (!_downloadSession.isCurrent(generation) || !downloadingLogs()) {
        return;
    }

    const QPointer<FtpTransport> self(this);
    _updateBatchProgress();
    if (self && _downloadSession.isCurrent(generation) && downloadingLogs()) {
        _scheduleDownloadNext();
    }
}

bool FtpTransport::_downloadEntryIsCurrent(quint64 generation, const QPointer<OnboardLogEntry>& entry) const
{
    return entry && _logEntriesModel->contains(entry) && _downloadSession.isCurrent(generation, entry) &&
           downloadingLogs();
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
    const QPointer<FtpTransport> self(this);
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
                if (!self || !_downloadSession.isCurrent(generation) || !downloadingLogs()) {
                    return;
                }
            }
            _downloadSession.completeCurrent();
            _batchState.completeFile(entrySize);
            _updateBatchProgress();
            if (self && _downloadSession.isCurrent(generation) && downloadingLogs()) {
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
    if (!entry || !_logEntriesModel->contains(entry)) {
        _continueDownload(generation);
        return;
    }

    if (errorMsg.isEmpty()) {
        entry->setErrorMessage(warningMsg);
        if (!self) {
            return;
        }
        if (!entry || !_logEntriesModel->contains(entry)) {
            _continueDownload(generation);
            return;
        }
        if (!_downloadSession.isCurrent(generation) || !downloadingLogs()) {
            return;
        }
        entry->setState(OnboardLogEntry::State::Downloaded, tr("Downloaded"));
        if (!self) {
            return;
        }
        if (!entry || !_logEntriesModel->contains(entry)) {
            _continueDownload(generation);
            return;
        }
        qCDebug(FtpTransportLog) << "download complete" << file;
        if (!warningMsg.isEmpty()) {
            qCWarning(FtpTransportLog) << warningMsg;
            _setErrorMessage(warningMsg);
            if (!self) {
                return;
            }
        }
    } else {
        const QString errorMessage = tr("FTP download failed: %1").arg(errorMsg);
        _setEntryError(entry, errorMessage);
        if (!self) {
            return;
        }
        qCWarning(FtpTransportLog) << errorMessage;
    }

    if (!entry || !_logEntriesModel->contains(entry)) {
        _continueDownload(generation);
        return;
    }
    if (!_downloadSession.isCurrent(generation) || !downloadingLogs()) {
        return;
    }
    _continueDownload(generation);
}

void FtpTransport::_downloadProgress(float value)
{
    const quint64 generation = _downloadSession.generation();
    const QPointer<FtpTransport> self(this);
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
    if (!self || !entry || !_downloadSession.isCurrent(generation, entry)) {
        return;
    }
    _updateBatchProgress(progress->bytes, progress->progress);
}

void FtpTransport::cancel()
{
    if (_cancelInProgress) {
        return;
    }

    const QPointer<FtpTransport> self(this);
    _cancelInProgress = true;
    const auto finishCancellation = qScopeGuard([self]() {
        if (!self) {
            return;
        }
        self->_cancelInProgress = false;
        self->_runPendingRefresh();
    });

    _cancelListing();
    if (!self) {
        return;
    }
    _cancelDownload();
    if (!self) {
        return;
    }
    _cancelErase();
    if (!self) {
        return;
    }

    _resetSelection(true);
}

void FtpTransport::_cancelListing()
{
    const QPointer<FtpTransport> self(this);
    _pendingLogInsertion.reset();
    if (!_listingSession.cancel()) {
        return;
    }

    if (_listingJob) {
        _listingJob->cancel();
        if (!self) {
            return;
        }
    }
    _finishListing(ListingResult::Canceled);
}

void FtpTransport::_cancelDownload()
{
    const QPointer<FtpTransport> self(this);
    if (_downloadSession.canceling()) {
        return;
    }

    const FtpDownloadSession::Cancellation cancellation = _downloadSession.cancel();
    if (!cancellation.wasActive) {
        return;
    }

    if (cancellation.currentEntry && _logEntriesModel->contains(cancellation.currentEntry)) {
        cancellation.currentEntry->setState(OnboardLogEntry::State::Canceled, tr("Canceled"));
        if (!self) {
            return;
        }
    }
    if (cancellation.cancelRemoteDownload && _downloadJob) {
        _downloadJob->cancel();
        if (!self) {
            return;
        }
    } else {
        _finishDownloadCancellation();
    }
}

void FtpTransport::_finishDownloadCancellation()
{
    const QPointer<FtpTransport> self(this);
    const bool refreshRequested = _downloadSession.finishCancellation();
    _setDownloading(false);
    if (!self) {
        return;
    }
    _setBatchProgress(0.);
    if (!self) {
        return;
    }

    if (refreshRequested) {
        _refreshAfterCancel();
    }
}

void FtpTransport::_cancelErase()
{
    const QPointer<FtpTransport> self(this);
    if (_eraseSession.canceling()) {
        return;
    }

    const FtpEraseSession::Cancellation cancellation = _eraseSession.cancel();
    if (!cancellation.wasActive) {
        return;
    }

    if (_eraseEntryIsCurrent(cancellation.currentEntry)) {
        cancellation.currentEntry->setState(OnboardLogEntry::State::Canceled, tr("Canceled"));
        if (!self) {
            return;
        }
    }
    if (_deleteJob) {
        _deleteJob->cancel();
        if (!self) {
            return;
        }
    }
    _eraseSession.finishCancellation();
    _setIdle(_vehicle);
}

void FtpTransport::eraseAll()
{
    if (_cancelInProgress) {
        qCWarning(FtpTransportLog) << "Ignoring erase request while onboard log cancellation is in progress";
        return;
    }
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
    const int numLogs = _logEntriesModel->count();
    entries.reserve(numLogs);
    for (int i = numLogs - 1; i >= 0; --i) {
        OnboardLogEntry* const entry = _logEntriesModel->value<OnboardLogEntry*>(i);
        if (entry && !entry->ftpPath().isEmpty()) {
            entries.append(entry);
        }
    }

    if (entries.isEmpty()) {
        qCWarning(FtpTransportLog) << "eraseAll: nothing to delete";
        return;
    }

    const quint64 generation = _eraseSession.begin(entries);
    const QPointer<FtpTransport> self(this);
    QPointer<Vehicle> vehicle(_vehicle);
    const auto eraseIsCurrent = [self, generation, &vehicle]() {
        return self && (generation == self->_eraseSession.generation()) && self->_eraseSession.active() && vehicle &&
               (vehicle == self->_vehicle);
    };

    _setOperation(Operation::Erasing, vehicle);
    if (!eraseIsCurrent()) {
        return;
    }
    _setErrorMessage(QString());
    if (!eraseIsCurrent()) {
        return;
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
    if (!_eraseSession.active() || _eraseSession.canceling() || _eraseSession.hasCurrent()) {
        return;
    }

    QPointer<OnboardLogEntry> entry = _eraseSession.takeNext();
    while (entry && !_eraseEntryIsCurrent(entry)) {
        _eraseSession.completeCurrent(false);
        entry = _eraseSession.takeNext();
    }

    if (!entry) {
        _finishErase();
        return;
    }

    const quint64 generation = _eraseSession.generation();
    const QPointer<FtpTransport> self(this);
    QPointer<Vehicle> vehicle(_vehicle);
    const QString ftpPath = entry->ftpPath();
    const auto eraseIsCurrent = [self, generation, &vehicle, &entry]() {
        return self && (generation == self->_eraseSession.generation()) && self->_eraseSession.active() && vehicle &&
               (vehicle == self->_vehicle) && entry && (self->_eraseSession.currentEntry() == entry) &&
               self->_eraseEntryIsCurrent(entry);
    };
    const auto continueAfterInvalidEntry = [self, generation, &entry]() {
        if (self && (generation == self->_eraseSession.generation()) && self->_eraseSession.active() &&
            !self->_eraseSession.canceling() && self->_eraseSession.hasCurrent() &&
            !self->_eraseEntryIsCurrent(entry)) {
            self->_eraseSession.completeCurrent(false);
            self->_scheduleEraseNext();
        }
    };

    entry->setState(OnboardLogEntry::State::Erasing, tr("Deleting"));
    if (!eraseIsCurrent()) {
        continueAfterInvalidEntry();
        return;
    }
    entry->setErrorMessage(QString());
    if (!eraseIsCurrent()) {
        continueAfterInvalidEntry();
        return;
    }

    const FTPManager::DeleteStartResult startResult =
        vehicle->ftpManager()->startDeleteFile(MAV_COMP_ID_AUTOPILOT1, ftpPath);
    FTPDeleteJob* const deleteJob = startResult.job();
    if (!deleteJob) {
        if (!eraseIsCurrent()) {
            continueAfterInvalidEntry();
            return;
        }
        const QString errorMessage = startResult.error() == FTPManager::StartError::Busy
                                         ? tr("Vehicle FTP service is busy")
                                         : tr("Unable to start deletion of %1").arg(QFileInfo(ftpPath).fileName());
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
    const QPointer<FTPDeleteJob> guardedJob(deleteJob);
    (void) connect(deleteJob, &FTPDeleteJob::finished, this,
                   [this, guardedJob, generation](const QString& file, const QString& errorMsg) {
                       if (guardedJob && (guardedJob == _deleteJob) && (generation == _eraseSession.generation())) {
                           _deleteComplete(file, errorMsg);
                       }
                   });
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
    if (_eraseSession.canceling() || !_eraseSession.hasCurrent()) {
        return;
    }

    const quint64 generation = _eraseSession.generation();
    const QPointer<FtpTransport> self(this);
    QPointer<Vehicle> vehicle(_vehicle);
    QPointer<OnboardLogEntry> entry(_eraseSession.currentEntry());
    _eraseSession.completeCurrent(!errorMsg.isEmpty());

    const auto eraseIsCurrent = [self, generation, &vehicle, &entry]() {
        return self && (generation == self->_eraseSession.generation()) && self->_eraseSession.active() && vehicle &&
               (vehicle == self->_vehicle) && entry && self->_eraseEntryIsCurrent(entry);
    };

    if (!eraseIsCurrent()) {
        qCWarning(FtpTransportLog) << "delete completed for an entry outside the active model:" << file;
        if (self && (generation == _eraseSession.generation()) && _eraseSession.active()) {
            _scheduleEraseNext();
        }
        return;
    }

    if (errorMsg.isEmpty()) {
        qCDebug(FtpTransportLog) << "deleted" << file;
        // Remove the row from the model so the UI reflects the erase immediately.
        _logEntriesModel->removeOne(entry);
        if (!self) {
            return;
        }
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

    if (self && (generation == _eraseSession.generation())) {
        _scheduleEraseNext();
    }
}

void FtpTransport::_finishErase()
{
    const quint64 generation = _eraseSession.generation();
    const uint failureCount = _eraseSession.finish();
    const QPointer<FtpTransport> self(this);
    _setIdle(_vehicle);
    if (!self || (generation != _eraseSession.generation()) || _eraseSession.active()) {
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

    return _logEntriesModel->contains(entry);
}

void FtpTransport::_setDownloading(bool active)
{
    OnboardLogTransport::_setDownloading(_vehicle, active);
}

void FtpTransport::_setListing(bool active)
{
    OnboardLogTransport::_setListing(_vehicle, active);
}
