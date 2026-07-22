#include "LogProtocolTransport.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QPersistentModelIndex>
#include <QtCore/QPointer>
#include <QtCore/QSignalBlocker>
#include <QtCore/QTimer>
#include <utility>

#include "AppSettings.h"
#include "FirmwarePlugin.h"
#include "LogProtocolDownloadSession.h"
#include "MAVLinkLib.h"
#include "MAVLinkProtocol.h"
#include "OnboardLogEntry.h"
#include "OnboardLogFileName.h"
#include "OnboardLogModel.h"
#include "QGCFormat.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

QGC_LOGGING_CATEGORY(LogProtocolTransportLog, "AnalyzeView.OnboardLogs.LogProtocolTransport")

LogProtocolTransport::LogProtocolTransport(QObject* parent) : OnboardLogTransport(parent), _timer(new QTimer(this))
{
    qCDebug(LogProtocolTransportLog) << this;

    (void) connect(_timer, &QTimer::timeout, this, &LogProtocolTransport::_processDownload);

    _timer->setSingleShot(false);
}

LogProtocolTransport::~LogProtocolTransport()
{
    qCDebug(LogProtocolTransportLog) << this;
    _timer->stop();
    _removePartialDownload();
    _downloadBatch.clear();
    if (_vehicle) {
        (void) disconnect(_vehicle, &Vehicle::logEntry, this, &LogProtocolTransport::_logEntry);
        (void) disconnect(_vehicle, &Vehicle::logData, this, &LogProtocolTransport::_logData);
        _vehicle = nullptr;
    }
}

void LogProtocolTransport::download(const QString& path)
{
    if (busy()) {
        qCWarning(LogProtocolTransportLog) << "Ignoring download request: another onboard log operation is in progress";
        return;
    }

    const QString dir = path.isEmpty() ? SettingsManager::instance()->appSettings()->logSavePath() : path;
    _downloadToDirectory(dir);
}

void LogProtocolTransport::_downloadToDirectory(const QString& dir)
{
    _receivedAllEntries();

    _downloadBatch.clear();
    _batchState.reset();
    _setBatchProgress(0.);
    _setErrorMessage(QString());

    QString downloadPath = dir;
    QList<QPointer<OnboardLogEntry>> entries;
    const int numLogs = _logEntriesModel->count();
    entries.reserve(numLogs);
    for (int i = 0; i < numLogs; ++i) {
        OnboardLogEntry* const entry = _logEntriesModel->value<OnboardLogEntry*>(i);
        if (entry && entry->received() && entry->selected()) {
            entries.append(entry);
            _batchState.addFile(entry->size());
        }
    }

    const QString noSelectionError = tr("No selected onboard logs are available for download");
    if (!_prepareDownload(_vehicle, downloadPath, !entries.isEmpty(), noSelectionError)) {
        qCWarning(LogProtocolTransportLog) << errorMessage();
        _batchState.reset();
        return;
    }

    const QString fileExtension = _vehicle->firmwarePlugin()->onboardLogPolicy(_vehicle).logFileExtension;
    const quint64 generation = _downloadBatch.begin(entries, downloadPath, fileExtension);
    _setDownloading(true);
    if (!_downloadBatch.isCurrent(generation) || !downloadingLogs()) {
        return;
    }
    _receivedAllData();
}

void LogProtocolTransport::_processDownload()
{
    if (requestingList()) {
        _findMissingEntries();
    } else if (downloadingLogs()) {
        _findMissingData();
    }
}

void LogProtocolTransport::_findMissingEntries()
{
    const quint64 generation = _listingSession.generation();
    const int logCount = _logEntriesModel->count();
    if ((logCount == 0) && !_listingSession.receivedResponse()) {
        if (!_listingSession.tryConsumeRetry()) {
            const QString errorMessage = tr("The vehicle did not respond to the onboard log list request");
            qCWarning(LogProtocolTransportLog) << errorMessage;
            _setErrorMessage(errorMessage);
            if ((generation != _listingSession.generation()) || !requestingList()) {
                return;
            }
            _receivedAllEntries(ListingResult::Failed);
        } else {
            _requestLogList(0, 0xffff);
        }
        return;
    }

    const std::optional<LogProtocolListingSession::MissingRange> missingRange = _listingSession.firstMissingRange();
    if (!missingRange) {
        _receivedAllEntries(_listingSession.partial() ? ListingResult::Partial : ListingResult::Success);
        return;
    }

    if (!_listingSession.tryConsumeRetry()) {
        for (int i = 0; i < logCount; i++) {
            QPointer<OnboardLogEntry> entry = _logEntriesModel->value<OnboardLogEntry*>(i);
            if (entry && !entry->received()) {
                const QPersistentModelIndex entryIndex(_logEntriesModel->index(i, 0));
                const auto entryIsCurrent = [this, generation, &entry, &entryIndex]() {
                    return (generation == _listingSession.generation()) && requestingList() && entry &&
                           entryIndex.isValid() && (entryIndex.model() == _logEntriesModel) &&
                           (entryIndex.data(Qt::UserRole).value<QObject*>() == entry.data());
                };

                entry->setState(OnboardLogEntry::State::Error, tr("Error"));
                if (!entryIsCurrent()) {
                    return;
                }
                entry->setErrorMessage(tr("The vehicle did not provide this log entry"));
                if (!entryIsCurrent()) {
                    return;
                }
            }
        }

        const QString errorMessage = tr("The vehicle did not provide a complete onboard log list");
        _setErrorMessage(errorMessage);
        if ((generation != _listingSession.generation()) || !requestingList()) {
            return;
        }
        qCWarning(LogProtocolTransportLog) << errorMessage;
        _receivedAllEntries(ListingResult::Partial);
        return;
    }

    _requestLogList(missingRange->start, missingRange->end);
}

void LogProtocolTransport::setVehicle(Vehicle* vehicle)
{
    if (vehicle && (vehicle == _vehicle)) {
        return;
    }

    _setErrorMessage(QString());
    _setBatchProgress(0.);

    const bool hasState =
        _vehicle || busy() || _listingSession.active() || _downloadBatch.active() || (_logEntriesModel->count() > 0);
    if (hasState) {
        cancel();
        _logEntriesModel->clearAndDeleteContents();
        _listingSession.clear();
    }
    if (_vehicle) {
        (void) disconnect(_vehicle, &Vehicle::logEntry, this, &LogProtocolTransport::_logEntry);
        (void) disconnect(_vehicle, &Vehicle::logData, this, &LogProtocolTransport::_logData);
    }

    _vehicle = vehicle;

    if (_vehicle) {
        (void) connect(_vehicle, &Vehicle::logEntry, this, &LogProtocolTransport::_logEntry);
        (void) connect(_vehicle, &Vehicle::logData, this, &LogProtocolTransport::_logData);
    }
}

void LogProtocolTransport::_logEntry(uint32_t time_utc, uint32_t size, uint16_t id, uint16_t num_logs,
                                     uint16_t last_log_num)
{
    if (!requestingList()) {
        return;
    }

    const quint64 generation = _listingSession.generation();
    const LogProtocolListingSession::Response response = _listingSession.observeResponse(num_logs, last_log_num, id);

    if (response.firstResponse) {
        if (response.truncated) {
            _setErrorMessage(
                tr("The vehicle reported more than %1 onboard logs; only the first %1 are shown").arg(kMaxLogEntries));
            if ((generation != _listingSession.generation()) || !requestingList()) {
                return;
            }
        }

        QList<OnboardLogEntry*> entries;
        entries.reserve(response.entryCount);
        for (int i = 0; i < response.entryCount; i++) {
            const uint32_t logId = static_cast<uint32_t>(_listingSession.firstLogId()) + static_cast<uint32_t>(i);
            OnboardLogEntry* const entry = new OnboardLogEntry(logId, QDateTime(), 0, false, this);
            (void) connect(entry, &OnboardLogEntry::selectedChanged, this, &LogProtocolTransport::selectionChanged);
            entries.append(entry);
        }
        if (!entries.isEmpty()) {
            _logEntriesModel->append(entries);
            if ((generation != _listingSession.generation()) || !requestingList()) {
                return;
            }
        }
    }

    if (response.inconsistentCount) {
        const QString errorMessage =
            tr("The vehicle changed the onboard log count from %1 to %2; continuing with the original count")
                .arg(_listingSession.announcedLogCount())
                .arg(num_logs);
        qCWarning(LogProtocolTransportLog) << errorMessage;
        _setErrorMessage(errorMessage);
        if (num_logs == 0) {
            return;
        }
    }
    if (response.inconsistentRange) {
        const QString errorMessage =
            tr("The vehicle changed the onboard log ID range; continuing with the original range");
        qCWarning(LogProtocolTransportLog) << errorMessage;
        _setErrorMessage(errorMessage);
    }

    if (response.emptyList) {
        _receivedAllEntries(_listingSession.partial() ? ListingResult::Partial : ListingResult::Success);
        return;
    }

    const LogProtocolListingSession::EntryAcceptance acceptance = _listingSession.acceptEntry(id, size);
    if (acceptance.result == LogProtocolListingSession::EntryResult::NoLogs) {
        _receivedAllEntries(_listingSession.partial() ? ListingResult::Partial : ListingResult::Success);
        return;
    }
    if (acceptance.result == LogProtocolListingSession::EntryResult::OutOfRange) {
        qCWarning(LogProtocolTransportLog)
            << "Received onboard log entry for out-of-bound index:" << acceptance.modelIndex;
        return;
    }
    QPointer<OnboardLogEntry> entry = _logEntriesModel->value<OnboardLogEntry*>(acceptance.modelIndex);
    if (!entry) {
        return;
    }
    const QPersistentModelIndex entryIndex(_logEntriesModel->index(acceptance.modelIndex, 0));
    const auto listingIsCurrent = [this, generation, &entry, &entryIndex]() {
        return (generation == _listingSession.generation()) && requestingList() && entry && entryIndex.isValid() &&
               (entryIndex.model() == _logEntriesModel) &&
               (entryIndex.data(Qt::UserRole).value<QObject*>() == entry.data());
    };

    const QDateTime reportedTime = QDateTime::fromSecsSinceEpoch(time_utc);
    if (acceptance.result == LogProtocolListingSession::EntryResult::Duplicate) {
        if ((entry->size() != size) || (entry->time() != reportedTime)) {
            _listingSession.markPartial();
            const QString errorMessage =
                tr("The vehicle reported conflicting metadata for onboard log %1; the first entry was kept")
                    .arg(entry->id());
            qCWarning(LogProtocolTransportLog) << errorMessage;
            _setErrorMessage(errorMessage);
        } else {
            qCDebug(LogProtocolTransportLog) << "Ignored duplicate onboard log entry" << entry->id();
        }
        return;
    }

    if (acceptance.result == LogProtocolListingSession::EntryResult::IgnoredEmptyArduPilotLog) {
        entry->setTime(reportedTime);
        if (!listingIsCurrent()) {
            return;
        }
        entry->setState(OnboardLogEntry::State::Skipped, tr("Empty"));
        if (!listingIsCurrent()) {
            return;
        }
        entry->setErrorMessage(QString());
        if (!listingIsCurrent()) {
            return;
        }
        if (acceptance.complete) {
            _receivedAllEntries(_listingSession.partial() ? ListingResult::Partial : ListingResult::Success);
        } else {
            _listingSession.resetRetries();
            _timer->start(kTimeout);
        }
        return;
    }

    entry->setSize(size);
    if (!listingIsCurrent()) {
        return;
    }
    entry->setTime(reportedTime);
    if (!listingIsCurrent()) {
        return;
    }
    entry->setReceived(true);
    if (!listingIsCurrent()) {
        return;
    }
    entry->setState(OnboardLogEntry::State::Available, tr("Available"));
    if (!listingIsCurrent()) {
        return;
    }
    entry->setErrorMessage(QString());
    if (!listingIsCurrent()) {
        return;
    }

    if (acceptance.complete) {
        _receivedAllEntries(_listingSession.partial() ? ListingResult::Partial : ListingResult::Success);
    } else if (acceptance.result == LogProtocolListingSession::EntryResult::Accepted) {
        _listingSession.resetRetries();
        _timer->start(kTimeout);
    }
}

void LogProtocolTransport::_receivedAllEntries(ListingResult result)
{
    const quint64 generation = _listingSession.generation();
    const bool hadListingAttempt = _listingSession.active();
    _listingSession.finish();
    _timer->stop();
    _setListing(false);
    if (hadListingAttempt && (generation == _listingSession.generation()) && !_listingSession.active() &&
        !requestingList()) {
        emit selectionChanged();
        if ((generation != _listingSession.generation()) || _listingSession.active() || requestingList()) {
            return;
        }
        emit listingFinished(result);
    }
}

void LogProtocolTransport::_logData(uint32_t ofs, uint16_t id, uint8_t count, const uint8_t* data)
{
    LogProtocolDownloadSession* const download = _downloadBatch.current();
    if (!downloadingLogs() || !download) {
        return;
    }

    if (download->id() != id) {
        qCWarning(LogProtocolTransportLog) << "Received log data for wrong log";
        return;
    }

    const LogProtocolDownloadSession::PacketAcceptance acceptance = download->acceptPacket(ofs, count, data);
    switch (acceptance.result) {
        case LogProtocolDownloadSession::PacketResult::Accepted:
            break;
        case LogProtocolDownloadSession::PacketResult::Duplicate:
            qCDebug(LogProtocolTransportLog) << "Ignored duplicate onboard log packet @" << ofs;
            return;
        case LogProtocolDownloadSession::PacketResult::EntryUnavailable:
            _failDownload(tr("The active onboard log entry is no longer available"));
            return;
        case LogProtocolDownloadSession::PacketResult::InvalidPayload:
            _failDownload(tr("Received onboard log data without a valid payload"));
            return;
        case LogProtocolDownloadSession::PacketResult::MisalignedOffset:
            qCWarning(LogProtocolTransportLog) << "Ignored misaligned incoming packet @" << ofs;
            return;
        case LogProtocolDownloadSession::PacketResult::OutsideFile:
            _failDownload(tr("Received onboard log data beyond the reported end of the log"));
            return;
        case LogProtocolDownloadSession::PacketResult::SizeLimitExceeded:
            _failDownload(tr("The onboard log exceeded the allowed size limit"));
            return;
        case LogProtocolDownloadSession::PacketResult::UnexpectedSize:
            _failDownload(tr("Unexpected onboard log data packet size: received %1, maximum %2")
                              .arg(static_cast<uint>(count))
                              .arg(acceptance.expectedCount));
            return;
        case LogProtocolDownloadSession::PacketResult::WrongChunk:
            qCWarning(LogProtocolTransportLog) << "Ignored packet for an out-of-order chunk @" << ofs;
            return;
        case LogProtocolDownloadSession::PacketResult::BinOutOfRange:
            qCWarning(LogProtocolTransportLog) << "Out of range bin received";
            return;
        case LogProtocolDownloadSession::PacketResult::SeekFailed:
            _failDownload(tr("Error seeking the partial log file: %1").arg(download->fileErrorString()));
            return;
        case LogProtocolDownloadSession::PacketResult::WriteFailed:
            _failDownload(tr("Error writing the partial log file: %1").arg(download->fileErrorString()));
            return;
    }

    const quint64 generation = _downloadBatch.generation();
    if (!_updateDataRate() || !_downloadBatch.isCurrent(generation)) {
        return;
    }

    _downloadBatch.resetRetries();

    _timer->start(kTimeout);
    const LogProtocolDownloadSession::CompletionState completion = download->completionState();
    if (completion.logComplete) {
        QPointer<OnboardLogEntry> entry(download->entry());
        if (!entry) {
            _failDownload(tr("The active onboard log entry is no longer available"));
            return;
        }

        if (!download->commit()) {
            _failDownload(tr("Unable to finalize the onboard log file: %1").arg(download->fileErrorString()));
            return;
        }

        _batchState.completeFile(download->actualSize());
        entry->setErrorMessage(QString());
        if (!entry || !_downloadBatch.isCurrent(generation, entry)) {
            return;
        }
        entry->setState(OnboardLogEntry::State::Downloaded, tr("Downloaded"));
        if (!entry || !_downloadBatch.isCurrent(generation, entry)) {
            return;
        }
        _updateBatchProgress();
        if (!_downloadBatch.isCurrent(generation, entry)) {
            return;
        }
        _receivedAllData();
    } else if (completion.chunkComplete) {
        download->advanceChunk();
        if (!_requestLogData(download->id(), download->currentChunkOffset(), download->currentChunkRequestBytes())) {
            _failDownload(tr("Unable to request onboard log data"));
        }
    } else if (acceptance.followedByReceivedBin) {
        // Likely to be grabbing fragments and got to the end of a gap
        _findMissingData();
    }
}

void LogProtocolTransport::_findMissingData()
{
    LogProtocolDownloadSession* const download = _downloadBatch.current();
    if (!download) {
        return;
    }

    const LogProtocolDownloadSession::CompletionState completion = download->completionState();
    if (completion.logComplete) {
        _receivedAllData();
        return;
    }

    if (completion.chunkComplete) {
        download->advanceChunk();
    }

    if (!_downloadBatch.tryConsumeRetry(kMaxDownloadRetries)) {
        _failDownload(tr("Too many retries while downloading the onboard log"));
        return;
    }

    const quint64 generation = _downloadBatch.generation();
    if (!_updateDataRate() || !_downloadBatch.isCurrent(generation)) {
        return;
    }

    const std::optional<LogProtocolDownloadSession::MissingRange> missingRange = download->firstMissingRange();
    if (!missingRange || (missingRange->count == 0)) {
        _failDownload(tr("Onboard log contains no downloadable data"));
        return;
    }
    if (!_requestLogData(download->id(), missingRange->offset, missingRange->count, _downloadBatch.retryCount())) {
        _failDownload(tr("Unable to request onboard log data"));
    }
}

void LogProtocolTransport::_failDownload(const QString& reason)
{
    _timer->stop();
    qCWarning(LogProtocolTransportLog) << reason;
    _setErrorMessage(reason);

    if (_downloadBatch.current()) {
        _setEntryError(_downloadBatch.current()->entry(), reason);
        _removePartialDownload();
    }

    const QString skippedError = tr("Skipped after an earlier onboard log download failed");
    _downloadBatch.finish();
    _resetSelection(false, skippedError);
    _setDownloading(false);
}

void LogProtocolTransport::_removePartialDownload()
{
    if (!_downloadBatch.current()) {
        return;
    }

    _downloadBatch.current()->cancelWriting();
}

bool LogProtocolTransport::_updateDataRate(uint32_t byteThreshold)
{
    LogProtocolDownloadSession* const download = _downloadBatch.current();
    if (!download) {
        return false;
    }

    const std::optional<LogProtocolDownloadSession::ProgressUpdate> progress =
        download->takeProgressUpdate(kGuiUpdateInterval, byteThreshold);
    if (!progress) {
        return true;
    }

    const quint64 generation = _downloadBatch.generation();
    QPointer<OnboardLogEntry> entry(download->entry());
    if (!entry) {
        return false;
    }
    const QString status =
        QStringLiteral("%1 (%2/s)")
            .arg(QGC::bigSizeToString(progress->bytesWritten), QGC::bigSizeToString(progress->bytesPerSecond));
    entry->setStatus(status);
    if (!entry || !_downloadBatch.isCurrent(generation, entry)) {
        return false;
    }
    _updateBatchProgress(progress->bytesWritten);
    return _downloadBatch.isCurrent(generation, entry);
}

void LogProtocolTransport::_receivedAllData()
{
    _timer->stop();
    if (!downloadingLogs()) {
        return;
    }

    if (_prepareLogDownload()) {
        LogProtocolDownloadSession* const download = _downloadBatch.current();
        if (download &&
            _requestLogData(download->id(), download->currentChunkOffset(), download->currentChunkRequestBytes())) {
            _timer->start(kTimeout);
        } else {
            _failDownload(tr("Unable to request onboard log data"));
        }
    } else if (downloadingLogs()) {
        if (_batchState.hasFiles()) {
            _setBatchProgress(1.);
        }
        _downloadBatch.finish();
        _resetSelection();
        _setDownloading(false);
    }
}

bool LogProtocolTransport::_prepareLogDownload()
{
    _downloadBatch.completeCurrent();
    const quint64 generation = _downloadBatch.generation();

    if (!_vehicle) {
        _failDownload(tr("The active vehicle became unavailable during the onboard log download"));
        return false;
    }

    while (_downloadBatch.pendingCount() > 0) {
        LogProtocolDownloadSession* const download = _downloadBatch.startNext();
        QPointer<OnboardLogEntry> entry(download ? download->entry() : nullptr);
        if (!entry) {
            continue;
        }
        if (!_downloadBatch.isCurrent(generation, entry) || !downloadingLogs()) {
            return false;
        }

        const QString ftime = (entry->time().date().year() >= 2010)
                                  ? entry->time().toString(QStringLiteral("yyyy-M-d-hh-mm-ss"))
                                  : QStringLiteral("UnknownDate");

        QString filename = QStringLiteral("log_") + QString::number(entry->id()) + "_" + ftime;

        filename += QStringLiteral(".") + _downloadBatch.fileExtension();

        const OnboardLogFileName::UniquePath localPath =
            OnboardLogFileName::uniquePath(_downloadBatch.path(), filename);
        QString errorMessage;
        const LogProtocolDownloadSession::FileOpenResult openResult = download->openFile(localPath.filePath);
        if (openResult == LogProtocolDownloadSession::FileOpenResult::OpenFailed) {
            errorMessage = tr("Unable to create %1: %2").arg(localPath.fileName, download->fileErrorString());
        } else if (openResult == LogProtocolDownloadSession::FileOpenResult::EntryUnavailable) {
            errorMessage = tr("The active onboard log entry is no longer available");
        } else if (openResult == LogProtocolDownloadSession::FileOpenResult::InvalidLogId) {
            errorMessage = tr("Onboard log %1 has an invalid LOG protocol ID").arg(entry->id());
        } else if (openResult == LogProtocolDownloadSession::FileOpenResult::DestinationExists) {
            errorMessage = tr("Unable to create %1: the destination already exists").arg(localPath.fileName);
        } else {
            {
                const QSignalBlocker blocker(this);
                entry->setSelected(false);
            }
            if (!entry || !_downloadBatch.isCurrent(generation, entry) || !downloadingLogs()) {
                return false;
            }
            entry->setErrorMessage(QString());
            if (!entry || !_downloadBatch.isCurrent(generation, entry) || !downloadingLogs()) {
                return false;
            }
            entry->setState(OnboardLogEntry::State::Downloading, tr("Downloading"));
            if (!entry || !_downloadBatch.isCurrent(generation, entry) || !downloadingLogs()) {
                return false;
            }
            return true;
        }

        {
            const QSignalBlocker blocker(this);
            entry->setSelected(false);
        }
        if (!entry || !_downloadBatch.isCurrent(generation, entry) || !downloadingLogs()) {
            return false;
        }
        _failDownload(errorMessage);
        return false;
    }

    return false;
}

void LogProtocolTransport::refresh()
{
    const QPointer<LogProtocolTransport> self(this);
    cancel();
    if (!self) {
        return;
    }
    const bool oneBasedIds =
        _vehicle && _vehicle->firmwarePlugin()->onboardLogPolicy(_vehicle).logProtocolIdsAreOneBased;
    const quint64 generation = _listingSession.begin(oneBasedIds);
    _setErrorMessage(QString());
    if (!self || (generation != self->_listingSession.generation())) {
        return;
    }
    self->_setBatchProgress(0.);
    if (!self || (generation != self->_listingSession.generation())) {
        return;
    }
    self->_logEntriesModel->clearAndDeleteContents();
    if (!self || (generation != self->_listingSession.generation())) {
        return;
    }
    self->_requestLogList(0, 0xffff);
}

void LogProtocolTransport::cancel()
{
    const QPointer<LogProtocolTransport> self(this);
    _timer->stop();
    const bool listingWasActive = _listingSession.cancel();
    const LogProtocolDownloadBatchSession::Cancellation downloadCancellation = _downloadBatch.cancel();
    const quint64 listingGeneration = _listingSession.generation();
    const quint64 downloadGeneration = _downloadBatch.generation();
    const auto cancellationIsCurrent = [self, listingGeneration, downloadGeneration]() {
        return self && (self->_listingSession.generation() == listingGeneration) &&
               (self->_downloadBatch.generation() == downloadGeneration);
    };
    if (requestingList() || downloadingLogs() || downloadCancellation.wasActive) {
        _requestLogEnd();
        if (!cancellationIsCurrent()) {
            return;
        }
    }
    if (listingWasActive) {
        _setListing(false);
        if (!cancellationIsCurrent()) {
            return;
        }
        emit listingFinished(ListingResult::Canceled);
        if (!cancellationIsCurrent()) {
            return;
        }
    }

    if (downloadCancellation.currentEntry) {
        downloadCancellation.currentEntry->setState(OnboardLogEntry::State::Canceled, tr("Canceled"));
        if (!cancellationIsCurrent()) {
            return;
        }
    }

    _resetSelection(true);
    if (!cancellationIsCurrent()) {
        return;
    }
    _setDownloading(false);
    if (!cancellationIsCurrent()) {
        return;
    }
    _setBatchProgress(0.);
}

void LogProtocolTransport::eraseAll()
{
    if (busy()) {
        qCWarning(LogProtocolTransportLog) << "Ignoring erase request: another onboard log operation is in progress";
        return;
    }

    if (!_vehicle) {
        const QString errorMessage = tr("No active vehicle is available for onboard log erase");
        qCWarning(LogProtocolTransportLog) << errorMessage;
        _setErrorMessage(errorMessage);
        return;
    }

    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        const QString errorMessage = tr("No vehicle link is available for onboard log erase");
        qCWarning(LogProtocolTransportLog) << errorMessage;
        _setErrorMessage(errorMessage);
        return;
    }

    mavlink_message_t msg = {};
    (void) mavlink_msg_log_erase_pack_chan(MAVLinkProtocol::instance()->getSystemId(),
                                           MAVLinkProtocol::getComponentId(), sharedLink->mavlinkChannel(), &msg,
                                           _vehicle->id(), _vehicle->defaultComponentId());

    if (!_vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg)) {
        const QString errorMessage = tr("Unable to send the onboard log erase request");
        qCWarning(LogProtocolTransportLog) << errorMessage;
        _setErrorMessage(errorMessage);
        return;
    }

    refresh();
}

void LogProtocolTransport::_requestLogList(uint32_t start, uint32_t end)
{
    const quint64 generation = _listingSession.generation();
    if (!_vehicle) {
        const QString errorMessage = tr("No active vehicle is available for onboard log refresh");
        qCWarning(LogProtocolTransportLog) << errorMessage;
        _setErrorMessage(errorMessage);
        if ((generation == _listingSession.generation()) && _listingSession.active()) {
            _receivedAllEntries(ListingResult::Failed);
        }
        return;
    }

    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        const QString errorMessage = tr("No vehicle link is available for onboard log refresh");
        qCWarning(LogProtocolTransportLog) << errorMessage;
        _setErrorMessage(errorMessage);
        if ((generation == _listingSession.generation()) && _listingSession.active()) {
            _receivedAllEntries(_listingSession.receivedEntryCount() > 0 ? ListingResult::Partial
                                                                         : ListingResult::Failed);
        }
        return;
    }

    mavlink_message_t msg = {};
    (void) mavlink_msg_log_request_list_pack_chan(MAVLinkProtocol::instance()->getSystemId(),
                                                  MAVLinkProtocol::getComponentId(), sharedLink->mavlinkChannel(), &msg,
                                                  _vehicle->id(), _vehicle->defaultComponentId(), start, end);

    if (!_vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg)) {
        const QString errorMessage = tr("Unable to send the onboard log list request");
        qCWarning(LogProtocolTransportLog) << errorMessage;
        _setErrorMessage(errorMessage);
        if ((generation == _listingSession.generation()) && _listingSession.active()) {
            _receivedAllEntries(_listingSession.receivedEntryCount() > 0 ? ListingResult::Partial
                                                                         : ListingResult::Failed);
        }
        return;
    }

    qCDebug(LogProtocolTransportLog) << "Request onboard log entry list (" << start << "through" << end << ")";
    _setListing(true);
    if ((generation != _listingSession.generation()) || !requestingList()) {
        return;
    }
    _timer->start(kRequestLogListTimeout);
}

bool LogProtocolTransport::_requestLogData(uint16_t id, uint32_t offset, uint32_t count, int retryCount)
{
    if (!_vehicle) {
        qCWarning(LogProtocolTransportLog) << "Vehicle Unavailable";
        return false;
    }

    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCWarning(LogProtocolTransportLog) << "Link Unavailable";
        return false;
    }

    qCDebug(LogProtocolTransportLog) << "Request log data (id:" << id << "offset:" << offset << "size:" << count
                                     << "retryCount" << retryCount << ")";

    mavlink_message_t msg = {};
    (void) mavlink_msg_log_request_data_pack_chan(MAVLinkProtocol::instance()->getSystemId(),
                                                  MAVLinkProtocol::getComponentId(), sharedLink->mavlinkChannel(), &msg,
                                                  _vehicle->id(), _vehicle->defaultComponentId(), id, offset, count);

    if (!_vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg)) {
        qCWarning(LogProtocolTransportLog) << "Failed to send";
        return false;
    }

    return true;
}

void LogProtocolTransport::_requestLogEnd()
{
    if (!_vehicle) {
        qCWarning(LogProtocolTransportLog) << "Vehicle Unavailable";
        return;
    }

    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCWarning(LogProtocolTransportLog) << "Link Unavailable";
        return;
    }

    mavlink_message_t msg = {};
    (void) mavlink_msg_log_request_end_pack_chan(MAVLinkProtocol::instance()->getSystemId(),
                                                 MAVLinkProtocol::getComponentId(), sharedLink->mavlinkChannel(), &msg,
                                                 _vehicle->id(), _vehicle->defaultComponentId());

    if (!_vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg)) {
        qCWarning(LogProtocolTransportLog) << "Failed to send";
    }
}

void LogProtocolTransport::_setDownloading(bool active)
{
    OnboardLogTransport::_setDownloading(_vehicle, active);
}

void LogProtocolTransport::_setListing(bool active)
{
    OnboardLogTransport::_setListing(_vehicle, active);
}
