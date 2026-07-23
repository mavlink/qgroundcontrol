#include "OnboardLogController.h"
#include "AppSettings.h"
#include "FTPManager.h"
#include "OnboardLogEntry.h"
#include "MAVLinkLib.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "ParameterManager.h"
#include "QGCFormat.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

#include <algorithm>

#include <QtCore/QApplicationStatic>
#include <QtCore/QDir>
#include <QtCore/QTimeZone>
#include <QtCore/QTimer>

QGC_LOGGING_CATEGORY(OnboardLogControllerLog, "AnalyzeView.OnboardLogController")

// MAVLink FTP defines "@MAV_LOG" as the virtual log directory.
// Older firmware that doesn't implement the alias requires the physical path
// instead — which is firmware-specific.
static constexpr const char *kMavlinkLogRoot = "@MAV_LOG";
static constexpr const char *kPx4LogRootFallback = "/fs/microsd/log";
static constexpr const char *kApmLogRootFallback = "/APM/LOGS";

OnboardLogController::OnboardLogController(QObject *parent)
    : QObject(parent)
    , _timer(new QTimer(this))
    , _logEntriesModel(new QmlObjectListModel(this))
{
    qCDebug(OnboardLogControllerLog) << this;

    (void) connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this, &OnboardLogController::_setActiveVehicle);
    (void) connect(_timer, &QTimer::timeout, this, &OnboardLogController::_processDownload);

    _timer->setSingleShot(false);

    _setActiveVehicle(MultiVehicleManager::instance()->activeVehicle());
}

OnboardLogController::~OnboardLogController()
{
    qCDebug(OnboardLogControllerLog) << this;
}

void OnboardLogController::download(const QString &path)
{
    const QString dir = path.isEmpty() ? SettingsManager::instance()->appSettings()->logSavePath() : path;
    if (_transport == Transport::Ftp) {
        _ftpDownloadToDirectory(dir);
    } else {
        _downloadToDirectory(dir);
    }
}

void OnboardLogController::_downloadToDirectory(const QString &dir)
{
    _receivedAllEntries();

    _downloadData.reset();

    _downloadPath = dir;
    if (_downloadPath.isEmpty()) {
        return;
    }

    if (!_downloadPath.endsWith(QDir::separator())) {
        _downloadPath += QDir::separator();
    }

    QGCOnboardLogEntry *const log = _getNextSelected();
    if (log) {
        log->setStatus(tr("Waiting"));
    }

    _setDownloading(true);
    _receivedAllData();
}

void OnboardLogController::_processDownload()
{
    if (_requestingLogEntries) {
        _findMissingEntries();
    } else if (_downloadingLogs) {
        _findMissingData();
    }
}

void OnboardLogController::_findMissingEntries()
{
    const int num_logs = _logEntriesModel->count();
    int start = -1;
    int end = -1;
    for (int i = 0; i < num_logs; i++) {
        const QGCOnboardLogEntry *const entry = _logEntriesModel->value<const QGCOnboardLogEntry*>(i);
        if (!entry) {
            continue;
        }

        if (!entry->received()) {
            if (start < 0) {
                start = i;
            } else {
                end = i;
            }
        } else if (start >= 0) {
            break;
        }
    }

    if (start < 0) {
        _receivedAllEntries();
        return;
    }

    if (_retries++ > 2) {
        for (int i = 0; i < num_logs; i++) {
            QGCOnboardLogEntry *const entry = _logEntriesModel->value<QGCOnboardLogEntry*>(i);
            if (entry && !entry->received()) {
                entry->setStatus(tr("Error"));
            }
        }

        _receivedAllEntries();
        qCWarning(OnboardLogControllerLog) << "Too many errors retreiving log list. Giving up.";
        return;
    }

    if (end < 0) {
        end = start;
    }

    start += _apmOffset;
    end += _apmOffset;

    _requestLogList(static_cast<uint32_t>(start), static_cast<uint32_t>(end));
}

void OnboardLogController::_setActiveVehicle(Vehicle *vehicle)
{
    if (vehicle == _vehicle) {
        return;
    }

    if (_vehicle) {
        // Tear down any in-progress messages-transport activity before the entries
        // it references are deleted with the model
        _timer->stop();
        if (_downloadData) {
            if (_downloadData->file.exists()) {
                (void) _downloadData->file.remove();
            }
            _downloadData.reset();
        }
        _setDownloading(false);
        _setListing(false);

        _logEntriesModel->clearAndDeleteContents();
        (void) disconnect(_vehicle, &Vehicle::logEntry, this, &OnboardLogController::_logEntry);
        (void) disconnect(_vehicle, &Vehicle::logData,  this, &OnboardLogController::_logData);

        FTPManager *const ftp = _vehicle->ftpManager();
        (void) disconnect(ftp, &FTPManager::listDirectoryComplete, this, &OnboardLogController::_ftpListDirComplete);
        (void) disconnect(ftp, &FTPManager::downloadComplete,      this, &OnboardLogController::_ftpDownloadComplete);
        (void) disconnect(ftp, &FTPManager::commandProgress,       this, &OnboardLogController::_ftpDownloadProgress);
        (void) disconnect(ftp, &FTPManager::deleteComplete,        this, &OnboardLogController::_ftpDeleteComplete);

        _ftpListState = FtpListState::Idle;
        _ftpDirsToList.clear();
        _ftpLogIdCounter = 0;
        _ftpDownloadQueue.clear();
        _ftpDeleteQueue.clear();
        _ftpDeleting = false;
        _ftpCurrentDownloadEntry = nullptr;
        _ftpDisabled = false;
        _ftpDownloadHadError = false;
        _setTransport(Transport::Messages);
    }

    _vehicle = vehicle;

    if (_vehicle) {
        (void) connect(_vehicle, &Vehicle::logEntry, this, &OnboardLogController::_logEntry);
        (void) connect(_vehicle, &Vehicle::logData,  this, &OnboardLogController::_logData);

        FTPManager *const ftp = _vehicle->ftpManager();
        (void) connect(ftp, &FTPManager::listDirectoryComplete, this, &OnboardLogController::_ftpListDirComplete);
        (void) connect(ftp, &FTPManager::downloadComplete,      this, &OnboardLogController::_ftpDownloadComplete);
        (void) connect(ftp, &FTPManager::commandProgress,       this, &OnboardLogController::_ftpDownloadProgress);
        (void) connect(ftp, &FTPManager::deleteComplete,        this, &OnboardLogController::_ftpDeleteComplete);
    }
}

void OnboardLogController::_logEntry(uint32_t time_utc, uint32_t size, uint16_t id, uint16_t num_logs, uint16_t last_log_num)
{
    Q_UNUSED(last_log_num);

    if (!_requestingLogEntries || (_transport != Transport::Messages)) {
        return;
    }

    if ((_logEntriesModel->count() == 0) && (num_logs > 0)) {
        if (_vehicle->firmwareType() == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            // APM ID starts at 1
            _apmOffset = 1;
        }

        for (int i = 0; i < num_logs; i++) {
            QGCOnboardLogEntry *const entry = new QGCOnboardLogEntry(i);
            (void) connect(entry, &QGCOnboardLogEntry::selectedChanged, this, &OnboardLogController::selectionChanged);
            _logEntriesModel->append(entry);
        }
    }

    if (num_logs > 0) {
        if ((size > 0) || (_vehicle->firmwareType() != MAV_AUTOPILOT_ARDUPILOTMEGA)) {
            id -= _apmOffset;
            if (id < _logEntriesModel->count()) {
                QGCOnboardLogEntry *const entry = _logEntriesModel->value<QGCOnboardLogEntry*>(id);
                entry->setSize(size);
                entry->setTime(QDateTime::fromSecsSinceEpoch(time_utc));
                entry->setReceived(true);
                entry->setStatus(tr("Available"));
            } else {
                qCWarning(OnboardLogControllerLog) << "Received onboard log entry for out-of-bound index:" << id;
            }
        }
    } else {
        _receivedAllEntries();
    }

    _retries = 0;

    if (_entriesComplete()) {
        _receivedAllEntries();
    } else {
        _timer->start(kTimeOutMs);
    }
}

void OnboardLogController::_receivedAllEntries()
{
    _timer->stop();
    _setListing(false);
}

bool OnboardLogController::_entriesComplete() const
{
    const int num_logs = _logEntriesModel->count();
    for (int i = 0; i < num_logs; i++) {
        const QGCOnboardLogEntry *const entry = _logEntriesModel->value<const QGCOnboardLogEntry*>(i);
        if (!entry) {
            continue;
        }

        if (!entry->received()) {
            return false;
        }
    }

    return true;
}

void OnboardLogController::_logData(uint32_t ofs, uint16_t id, uint8_t count, const uint8_t *data)
{
    if (!_downloadingLogs || !_downloadData || (_transport != Transport::Messages)) {
        return;
    }

    id -= _apmOffset;
    if (_downloadData->ID != id) {
        qCWarning(OnboardLogControllerLog) << "Received log data for wrong log";
        return;
    }

    if ((ofs % MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN) != 0) {
        qCWarning(OnboardLogControllerLog) << "Ignored misaligned incoming packet @" << ofs;
        return;
    }

    bool result = false;
    if (ofs <= _downloadData->entry->size()) {
        const uint32_t chunk = ofs / OnboardLogDownloadData::kChunkSize;
        // qCDebug(OnboardLogControllerLog) << "Received data - Offset:" << ofs << "Chunk:" << chunk;
        if (chunk != _downloadData->current_chunk) {
            qCWarning(OnboardLogControllerLog) << "Ignored packet for out of order chunk actual:expected" << chunk << _downloadData->current_chunk;
            return;
        }

        const uint16_t bin = (ofs - (chunk * OnboardLogDownloadData::kChunkSize)) / MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN;
        if (bin >= _downloadData->chunk_table.size()) {
            qCWarning(OnboardLogControllerLog) << "Out of range bin received";
        } else {
            _downloadData->chunk_table.setBit(bin);
        }

        if (_downloadData->file.pos() != ofs) {
            if (!_downloadData->file.seek(ofs)) {
                qCWarning(OnboardLogControllerLog) << "Error while seeking log file offset";
                return;
            }
        }

        if (_downloadData->file.write(reinterpret_cast<const char*>(data), count)) {
            _downloadData->written += count;
            _downloadData->rate_bytes += count;
            _updateDataRate();

            result = true;
            _retries = 0;

            _timer->start(kTimeOutMs);
            if (_logComplete()) {
                _downloadData->entry->setStatus(tr("Downloaded"));
                _receivedAllData();
            } else if (_chunkComplete()) {
                _downloadData->advanceChunk();
                _requestLogData(_downloadData->ID,
                                _downloadData->current_chunk * OnboardLogDownloadData::kChunkSize,
                                _downloadData->chunk_table.size() * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN);
            } else if ((bin < (_downloadData->chunk_table.size() - 1)) && _downloadData->chunk_table.at(bin + 1)) {
                // Likely to be grabbing fragments and got to the end of a gap
                _findMissingData();
            }
        } else {
            qCWarning(OnboardLogControllerLog) << "Error while writing log file chunk";
        }
    } else {
        qCWarning(OnboardLogControllerLog) << "Received log offset greater than expected";
    }

    if (!result) {
        _downloadData->entry->setStatus(tr("Error"));
    }
}

void OnboardLogController::_findMissingData()
{
    if (_logComplete()) {
        _receivedAllData();
        return;
    }

    if (_chunkComplete()) {
        _downloadData->advanceChunk();
    }

    _retries++;

    _updateDataRate();

    uint16_t start = 0, end = 0;
    const int size = _downloadData->chunk_table.size();
    for (; start < size; start++) {
        if (!_downloadData->chunk_table.testBit(start)) {
            break;
        }
    }

    for (end = start; end < size; end++) {
        if (_downloadData->chunk_table.testBit(end)) {
            break;
        }
    }

    const uint32_t pos = (_downloadData->current_chunk * OnboardLogDownloadData::kChunkSize) + (start * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN);
    const uint32_t len = (end - start) * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN;
    _requestLogData(_downloadData->ID, pos, len, _retries);
}

void OnboardLogController::_updateDataRate()
{
    constexpr uint kSizeUpdateThreshold = 102400; // 0.1 MB
    const bool timeThresholdMet = _downloadData->elapsed.elapsed() >= kGUIRateMs;
    const bool sizeThresholdMet = (_downloadData->written - _downloadData->last_status_written) >= kSizeUpdateThreshold;

    if (!timeThresholdMet && !sizeThresholdMet) {
        return;
    }

    QString status;
    if (timeThresholdMet) {
        // Update both rate and size
        const qreal rate = _downloadData->rate_bytes / (_downloadData->elapsed.elapsed() / 1000.0);
        _downloadData->rate_avg = (_downloadData->rate_avg * 0.95) + (rate * 0.05);
        _downloadData->rate_bytes = 0;

        status = QStringLiteral("%1 (%2/s)").arg(QGC::bigSizeToString(_downloadData->written),
                                                   QGC::bigSizeToString(_downloadData->rate_avg));
        _downloadData->elapsed.start();
    } else {
        // Update size only, keep previous rate
        status = QStringLiteral("%1 (%2/s)").arg(QGC::bigSizeToString(_downloadData->written),
                                                   QGC::bigSizeToString(_downloadData->rate_avg));
    }

    _downloadData->entry->setStatus(status);
    _downloadData->last_status_written = _downloadData->written;
}

bool OnboardLogController::_chunkComplete() const
{
    return _downloadData->chunkEquals(true);
}

bool OnboardLogController::_logComplete() const
{
    return (_chunkComplete() && ((_downloadData->current_chunk + 1) == _downloadData->numChunks()));
}

void OnboardLogController::_receivedAllData()
{
    _timer->stop();
    if (_prepareLogDownload()) {
        _requestLogData(_downloadData->ID, 0, _downloadData->chunk_table.size() * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN);
        _timer->start(kTimeOutMs);
    } else {
        _resetSelection();
        _setDownloading(false);
    }
}

bool OnboardLogController::_prepareLogDownload()
{
    _downloadData.reset();

    QGCOnboardLogEntry *const entry = _getNextSelected();
    if (!entry) {
        return false;
    }

    entry->setSelected(false);
    emit selectionChanged();

    const QString ftime = (entry->time().date().year() >= 2010) ? entry->time().toString(QStringLiteral("yyyy-M-d-hh-mm-ss")) : QStringLiteral("UnknownDate");

    _downloadData = std::make_unique<OnboardLogDownloadData>(entry);
    _downloadData->filename = QStringLiteral("log_") + QString::number(entry->id()) + "_" + ftime;

    if (_vehicle->firmwareType() == MAV_AUTOPILOT_PX4) {
        const QString loggerParam = QStringLiteral("SYS_LOGGER");
        ParameterManager *const parameterManager = _vehicle->parameterManager();
        Fact *const loggerFact = parameterManager->parameterExists(ParameterManager::defaultComponentId, loggerParam) ? parameterManager->getParameter(ParameterManager::defaultComponentId, loggerParam) : nullptr;
        if (loggerFact && (loggerFact->rawValue().toInt() == 0)) {
            _downloadData->filename += ".px4log";
        } else {
            _downloadData->filename += ".ulg";
        }
    } else {
        _downloadData->filename += ".bin";
    }

    _downloadData->file.setFileName(_downloadPath + _downloadData->filename);

    if (_downloadData->file.exists()) {
        uint32_t numDups = 0;
        const QStringList filename_spl = _downloadData->filename.split('.');
        do {
            numDups += 1;
            const QString filename = filename_spl[0] + '_' + QString::number(numDups) + '.' + filename_spl[1];
            _downloadData->file.setFileName(_downloadPath + filename);
        } while ( _downloadData->file.exists());
    }

    bool result = false;
    if (!_downloadData->file.open(QIODevice::WriteOnly)) {
        qCWarning(OnboardLogControllerLog) << "Failed to create log file:" <<  _downloadData->filename;
    } else if (!_downloadData->file.resize(entry->size())) {
        qCWarning(OnboardLogControllerLog) << "Failed to allocate space for log file:" <<  _downloadData->filename;
    } else {
        _downloadData->current_chunk = 0;
        _downloadData->chunk_table = QBitArray(_downloadData->chunkBins(), false);
        _downloadData->elapsed.start();
        result = true;
    }

    if (!result) {
        if (_downloadData->file.exists()) {
            (void) _downloadData->file.remove();
        }

        _downloadData->entry->setStatus(tr("Error"));
        _downloadData.reset();
    }

    return result;
}

void OnboardLogController::refresh()
{
    _logEntriesModel->clearAndDeleteContents();
    emit selectionChanged();

    if (_vehicle && !_ftpDisabled && _vehicle->capabilitiesKnown() && (_vehicle->capabilityBits() & MAV_PROTOCOL_CAPABILITY_FTP)) {
        _setTransport(Transport::Ftp);
        _ftpStartListing();
    } else {
        _setTransport(Transport::Messages);
        _requestLogList(0, 0xffff);
    }
}

QGCOnboardLogEntry *OnboardLogController::_getNextSelected() const
{
    const int numLogs = _logEntriesModel->count();
    for (int i = 0; i < numLogs; i++) {
        QGCOnboardLogEntry *const entry = _logEntriesModel->value<QGCOnboardLogEntry*>(i);
        if (!entry) {
            continue;
        }

        if (entry->selected()) {
           return entry;
        }
    }

    return nullptr;
}

void OnboardLogController::cancel()
{
    if (_transport == Transport::Ftp) {
        if (_vehicle) {
            if (_requestingLogEntries) {
                _vehicle->ftpManager()->cancelListDirectory();
                _ftpDirsToList.clear();
                _ftpFinishListing();
            }

            if (_ftpDeleting) {
                // The in-flight delete can't be aborted. Drop the queued deletes and let
                // _ftpDeleteComplete finish the cycle with an automatic refresh.
                _ftpDeleteQueue.clear();
                _resetSelection(true);
                return;
            }

            if (_downloadingLogs) {
                _vehicle->ftpManager()->cancelDownload();
                if (_ftpCurrentDownloadEntry) {
                    _ftpCurrentDownloadEntry->setStatus(tr("Canceled"));
                    _ftpCurrentDownloadEntry = nullptr;
                }
                _ftpDownloadQueue.clear();
            }
        }
    } else {
        _requestLogEnd();
        _receivedAllEntries();

        if (_downloadData) {
            _downloadData->entry->setStatus(tr("Canceled"));
            if (_downloadData->file.exists()) {
                (void) _downloadData->file.remove();
            }

            _downloadData.reset();
        }
    }

    _resetSelection(true);
    _setDownloading(false);
}

void OnboardLogController::selectAll(bool select)
{
    const int count = _logEntriesModel->count();
    for (int i = 0; i < count; i++) {
        QGCOnboardLogEntry *const entry = _logEntriesModel->value<QGCOnboardLogEntry*>(i);
        if (!entry || !entry->received()) {
            continue;
        }

        if (entry->selected() != select) {
            // Suppress the per-entry connection to avoid O(n²) allLogsSelected()
            // re-evaluations. The entry still notifies its own QML bindings.
            // A single selectionChanged() is emitted after the loop.
            (void) disconnect(entry, &QGCOnboardLogEntry::selectedChanged, this, &OnboardLogController::selectionChanged);
            entry->setSelected(select);
            (void) connect(entry, &QGCOnboardLogEntry::selectedChanged, this, &OnboardLogController::selectionChanged);
        }
    }
    emit selectionChanged();
}

int OnboardLogController::selectedCount() const
{
    int selected = 0;
    const int count = _logEntriesModel->count();
    for (int i = 0; i < count; i++) {
        const QGCOnboardLogEntry *const entry = _logEntriesModel->value<const QGCOnboardLogEntry*>(i);
        if (entry && entry->received() && entry->selected()) {
            selected++;
        }
    }

    return selected;
}

bool OnboardLogController::allLogsSelected() const
{
    int selectable = 0;
    int selected = 0;
    const int count = _logEntriesModel->count();
    for (int i = 0; i < count; i++) {
        const QGCOnboardLogEntry *const entry = _logEntriesModel->value<const QGCOnboardLogEntry*>(i);
        if (entry && entry->received()) {
            selectable++;
            if (entry->selected()) {
                selected++;
            }
        }
    }

    return (selectable > 0) && (selected == selectable);
}

void OnboardLogController::toggleSortByDate()
{
    setSortAscending(!_sortAscending);
}

void OnboardLogController::setSortAscending(bool ascending)
{
    if (_sortAscending == ascending) {
        return;
    }

    _sortAscending = ascending;
    _sortEntriesByTimestamp();
    emit sortAscendingChanged();
}

void OnboardLogController::_resetSelection(bool canceled)
{
    const int num_logs = _logEntriesModel->count();
    for (int i = 0; i < num_logs; i++) {
        QGCOnboardLogEntry *const entry = _logEntriesModel->value<QGCOnboardLogEntry*>(i);
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

void OnboardLogController::_sortEntriesByTimestamp()
{
    QObjectList sortedEntries = *_logEntriesModel->objectList();
    std::stable_sort(sortedEntries.begin(), sortedEntries.end(), [this](const QObject *lhsObj, const QObject *rhsObj) {
        const QGCOnboardLogEntry *const lhs = qobject_cast<const QGCOnboardLogEntry*>(lhsObj);
        const QGCOnboardLogEntry *const rhs = qobject_cast<const QGCOnboardLogEntry*>(rhsObj);
        if (lhs == rhs) {
            return false;
        }
        if (!lhs) {
            return false;
        }
        if (!rhs) {
            return true;
        }

        const bool lhsHasTime = lhs->received() && (lhs->time().toSecsSinceEpoch() > 0);
        const bool rhsHasTime = rhs->received() && (rhs->time().toSecsSinceEpoch() > 0);
        if (lhsHasTime != rhsHasTime) {
            // Keep entries with valid timestamps grouped first.
            return lhsHasTime;
        }

        if (lhsHasTime && rhsHasTime) {
            if (lhs->time() == rhs->time()) {
                return _sortAscending ? (lhs->id() < rhs->id()) : (lhs->id() > rhs->id());
            }

            return _sortAscending ? (lhs->time() < rhs->time()) : (lhs->time() > rhs->time());
        }

        // Fallback for entries with missing/invalid time.
        return _sortAscending ? (lhs->id() < rhs->id()) : (lhs->id() > rhs->id());
    });

    (void) _logEntriesModel->swapObjectList(sortedEntries);
}

void OnboardLogController::eraseAll()
{
    if (!_vehicle) {
        qCWarning(OnboardLogControllerLog) << "Vehicle Unavailable";
        return;
    }

    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCWarning(OnboardLogControllerLog) << "Link Unavailable";
        return;
    }

    mavlink_message_t msg{};
    (void) mavlink_msg_log_erase_pack_chan(
        MAVLinkProtocol::instance()->getSystemId(),
        MAVLinkProtocol::getComponentId(),
        sharedLink->mavlinkChannel(),
        &msg,
        _vehicle->id(),
        _vehicle->defaultComponentId()
    );

    if (!_vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg)) {
        qCWarning(OnboardLogControllerLog) << "Failed to send";
        return;
    }

    refresh();
}

void OnboardLogController::eraseSelected()
{
    if (!_vehicle) {
        qCWarning(OnboardLogControllerLog) << "Vehicle Unavailable";
        return;
    }

    if (_transport != Transport::Ftp) {
        qCWarning(OnboardLogControllerLog) << "ftp: selective erase requires the FTP transport";
        return;
    }

    _ftpDeleteQueue.clear();
    const int numLogs = _logEntriesModel->count();
    for (int i = 0; i < numLogs; i++) {
        QGCOnboardLogEntry *const entry = _logEntriesModel->value<QGCOnboardLogEntry*>(i);
        if (entry && entry->selected() && !entry->ftpPath().isEmpty()) {
            _ftpDeleteQueue.enqueue(entry);
        }
    }

    if (_ftpDeleteQueue.isEmpty()) {
        qCWarning(OnboardLogControllerLog) << "ftp: no selected logs have FTP paths for erase";
        return;
    }

    qCDebug(OnboardLogControllerLog) << "ftp: erasing" << _ftpDeleteQueue.size() << "selected logs";
    _ftpDeleting = true;
    _setDownloading(true);
    _ftpDeleteNext();
}

void OnboardLogController::_ftpDeleteNext()
{
    if (_ftpDeleteQueue.isEmpty()) {
        _ftpDeleting = false;
        _setDownloading(false);
        refresh();
        return;
    }

    QGCOnboardLogEntry *const entry = _ftpDeleteQueue.dequeue();
    entry->setSelected(false);
    entry->setStatus(tr("Erasing"));

    qCDebug(OnboardLogControllerLog) << "ftp: deleting" << entry->ftpPath();

    if (!_vehicle->ftpManager()->deleteFile(MAV_COMP_ID_AUTOPILOT1, entry->ftpPath())) {
        qCWarning(OnboardLogControllerLog) << "ftp: failed to start delete for" << entry->ftpPath();
        entry->setStatus(tr("Error"));
        _ftpDeleteNext();
    }
}

void OnboardLogController::_ftpDeleteComplete(const QString &file, const QString &errorMsg)
{
    if (!_ftpDeleting) {
        return;
    }

    if (!errorMsg.isEmpty()) {
        qCWarning(OnboardLogControllerLog) << "ftp: delete error:" << file << errorMsg;
    }

    _ftpDeleteNext();
}

void OnboardLogController::_requestLogList(uint32_t start, uint32_t end)
{
    if (!_vehicle) {
        qCWarning(OnboardLogControllerLog) << "Vehicle Unavailable";
        return;
    }

    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCWarning(OnboardLogControllerLog) << "Link Unavailable";
        return;
    }

    mavlink_message_t msg{};
    (void) mavlink_msg_log_request_list_pack_chan(
        MAVLinkProtocol::instance()->getSystemId(),
        MAVLinkProtocol::getComponentId(),
        sharedLink->mavlinkChannel(),
        &msg,
        _vehicle->id(),
        _vehicle->defaultComponentId(),
        start,
        end
    );

    if (!_vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg)) {
        qCWarning(OnboardLogControllerLog) << "Failed to send";
        return;
    }

    qCDebug(OnboardLogControllerLog) << "Request onboard log entry list (" << start << "through" << end << ")";
    _setListing(true);
    _timer->start(kRequestLogListTimeoutMs);
}

void OnboardLogController::_requestLogData(uint16_t id, uint32_t offset, uint32_t count, int retryCount)
{
    if (!_vehicle) {
        qCWarning(OnboardLogControllerLog) << "Vehicle Unavailable";
        return;
    }

    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCWarning(OnboardLogControllerLog) << "Link Unavailable";
        return;
    }

    id += _apmOffset;
    qCDebug(OnboardLogControllerLog) << "Request log data (id:" << id << "offset:" << offset << "size:" << count << "retryCount" << retryCount << ")";

    mavlink_message_t msg{};
    (void) mavlink_msg_log_request_data_pack_chan(
        MAVLinkProtocol::instance()->getSystemId(),
        MAVLinkProtocol::getComponentId(),
        sharedLink->mavlinkChannel(),
        &msg,
        _vehicle->id(),
        _vehicle->defaultComponentId(),
        id,
        offset,
        count
    );

    if (!_vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg)) {
        qCWarning(OnboardLogControllerLog) << "Failed to send";
    }
}

void OnboardLogController::_requestLogEnd()
{
    if (!_vehicle) {
        qCWarning(OnboardLogControllerLog) << "Vehicle Unavailable";
        return;
    }

    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCWarning(OnboardLogControllerLog) << "Link Unavailable";
        return;
    }

    mavlink_message_t msg{};
    (void) mavlink_msg_log_request_end_pack_chan(
        MAVLinkProtocol::instance()->getSystemId(),
        MAVLinkProtocol::getComponentId(),
        sharedLink->mavlinkChannel(),
        &msg,
        _vehicle->id(),
        _vehicle->defaultComponentId()
    );

    if (!_vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg)) {
        qCWarning(OnboardLogControllerLog) << "Failed to send";
    }
}

void OnboardLogController::_setDownloading(bool active)
{
    if (_downloadingLogs != active) {
        _downloadingLogs = active;
        if (_vehicle) {
            _vehicle->vehicleLinkManager()->setCommunicationLostEnabled(!active);
        }
        emit downloadingLogsChanged();
    }
}

void OnboardLogController::_setListing(bool active)
{
    if (_requestingLogEntries != active) {
        _requestingLogEntries = active;
        if (_vehicle) {
            _vehicle->vehicleLinkManager()->setCommunicationLostEnabled(!active);
        }
        if (!active) {
            _sortEntriesByTimestamp();
        }
        emit requestingListChanged();
    }
}

void OnboardLogController::_setTransport(Transport transport)
{
    if (_transport != transport) {
        _transport = transport;
        emit transportChanged();
    }
}

void OnboardLogController::_ftpStartListing()
{
    _ftpDirsToList.clear();
    _ftpLogIdCounter = 0;
    _ftpLogRoot = QString::fromLatin1(kMavlinkLogRoot);
    _ftpTriedFallbackRoot = false;

    _setListing(true);
    _ftpListRoot();
}

void OnboardLogController::_ftpListRoot()
{
    _ftpListState = FtpListState::ListingRoot;

    qCDebug(OnboardLogControllerLog) << "ftp: listing root" << _ftpLogRoot;

    if (!_vehicle->ftpManager()->listDirectory(MAV_COMP_ID_AUTOPILOT1, _ftpLogRoot)) {
        qCWarning(OnboardLogControllerLog) << "ftp: failed to start root listing for" << _ftpLogRoot;
        _ftpFallbackToMessages();
    }
}

void OnboardLogController::_ftpListDirComplete(const QStringList &dirList, const QString &errorMsg)
{
    if (_ftpListState == FtpListState::Idle) {
        return;
    }

    if (!errorMsg.isEmpty()) {
        if ((_ftpListState == FtpListState::ListingRoot) && !_ftpTriedFallbackRoot && _vehicle) {
            const char *fallback = nullptr;
            if (_vehicle->px4Firmware()) {
                fallback = kPx4LogRootFallback;
            } else if (_vehicle->apmFirmware()) {
                fallback = kApmLogRootFallback;
            }

            if (fallback) {
                qCDebug(OnboardLogControllerLog) << "ftp: root listing of" << _ftpLogRoot << "failed (" << errorMsg
                    << "), falling back to" << fallback;
                _ftpTriedFallbackRoot = true;
                _ftpLogRoot = QString::fromLatin1(fallback);
                _ftpListRoot();
                return;
            }
        }

        qCWarning(OnboardLogControllerLog) << "ftp: listing error:" << errorMsg;
        _ftpFallbackToMessages();
        return;
    }

    if (_ftpListState == FtpListState::ListingRoot) {
        // The root listing may contain log files directly (flat layout, e.g. @MAV_LOG)
        // and/or date subdirectories to descend into (PX4 fallback /fs/microsd/log).
        const uint flatLogs = _ftpProcessFileEntries(dirList, QString());

        for (const QString &entry : dirList) {
            if (entry.startsWith(QLatin1Char('D'))) {
                const QString dirName = entry.mid(1);
                if (!dirName.isEmpty()) {
                    _ftpDirsToList.append(dirName);
                }
            }
        }

        _ftpDirsToList.sort();
        qCDebug(OnboardLogControllerLog) << "ftp: root listing of" << _ftpLogRoot
            << "found" << flatLogs << "flat logs and" << _ftpDirsToList.size() << "subdirectories";

        _ftpListState = FtpListState::ListingSubdir;
        _ftpListNextSubdir();
        return;
    }

    const QString currentDir = _ftpDirsToList.isEmpty() ? QString() : _ftpDirsToList.first();
    const uint logsFoundInDir = _ftpProcessFileEntries(dirList, currentDir);

    qCDebug(OnboardLogControllerLog) << "ftp:" << currentDir << "->" << logsFoundInDir << "logs";

    if (!_ftpDirsToList.isEmpty()) {
        _ftpDirsToList.removeFirst();
    }

    _ftpListNextSubdir();
}

uint OnboardLogController::_ftpProcessFileEntries(const QStringList &dirList, const QString &subdir)
{
    const QDate dirDate = subdir.isEmpty() ? QDate() : QDate::fromString(subdir, QStringLiteral("yyyy-MM-dd"));
    uint logsFound = 0;

    for (const QString &entry : dirList) {
        if (!entry.startsWith(QLatin1Char('F'))) {
            continue;
        }

        // Entry format is "F<name>\t<size>" and, when the server supports kCmdListDirectoryWithTime,
        // "F<name>\t<size>\t<modification time in seconds since UNIX epoch UTC>".
        const QString fileInfo = entry.mid(1);
        const int tabIdx = fileInfo.indexOf(QLatin1Char('\t'));
        if (tabIdx < 0) {
            continue;
        }

        const QString fileName = fileInfo.left(tabIdx);
        const QString sizeStr = fileInfo.section(QLatin1Char('\t'), 1, 1);
        const QString mtimeStr = fileInfo.section(QLatin1Char('\t'), 2, 2);

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

        // Prefer the modification time reported by the vehicle when available (0 means unknown).
        bool mtimeOk = false;
        const qint64 mtimeSecs = mtimeStr.toLongLong(&mtimeOk);
        if (mtimeOk && (mtimeSecs > 0)) {
            dateTime = QDateTime::fromSecsSinceEpoch(mtimeSecs, QTimeZone::UTC);
        }

        // Otherwise reconstruct the date from the date sub-directory name and the filename time.
        if (!dateTime.isValid() && dirDate.isValid()) {
            const QString baseName = fileName.left(fileName.lastIndexOf(QLatin1Char('.')));
            const QTime fileTime = QTime::fromString(baseName, QStringLiteral("HH_mm_ss"));
            if (fileTime.isValid()) {
                dateTime = QDateTime(dirDate, fileTime, QTimeZone::UTC);
            } else {
                dateTime = QDateTime(dirDate, QTime(), QTimeZone::UTC);
            }
        }

        const QString ftpPath = subdir.isEmpty()
            ? (_ftpLogRoot + QStringLiteral("/") + fileName)
            : (_ftpLogRoot + QStringLiteral("/") + subdir + QStringLiteral("/") + fileName);

        QGCOnboardLogEntry *const logEntry = new QGCOnboardLogEntry(_ftpLogIdCounter++, dateTime, fileSize, true);
        logEntry->setFtpPath(ftpPath);
        logEntry->setStatus(tr("Available"));
        (void) connect(logEntry, &QGCOnboardLogEntry::selectedChanged, this, &OnboardLogController::selectionChanged);
        _logEntriesModel->append(logEntry);
        logsFound++;
    }

    return logsFound;
}

void OnboardLogController::_ftpListNextSubdir()
{
    if (_ftpDirsToList.isEmpty()) {
        qCDebug(OnboardLogControllerLog) << "ftp: listing complete, found" << _logEntriesModel->count() << "logs";
        _ftpFinishListing();
        return;
    }

    const QString subdir = _ftpDirsToList.first();
    const QString path = _ftpLogRoot + QStringLiteral("/") + subdir;

    qCDebug(OnboardLogControllerLog) << "ftp: listing subdir" << path;

    if (!_vehicle->ftpManager()->listDirectory(MAV_COMP_ID_AUTOPILOT1, path)) {
        qCWarning(OnboardLogControllerLog) << "ftp: failed to list subdir" << path;
        _ftpDirsToList.removeFirst();
        _ftpListNextSubdir();
    }
}

void OnboardLogController::_ftpFinishListing()
{
    _ftpListState = FtpListState::Idle;
    _setListing(false);
}

void OnboardLogController::_ftpFallbackToMessages()
{
    qCDebug(OnboardLogControllerLog) << "ftp: transport failed, falling back to message based log download";

    _ftpListState = FtpListState::Idle;
    _ftpDirsToList.clear();
    _ftpDisabled = true;
    _setTransport(Transport::Messages);

    _logEntriesModel->clearAndDeleteContents();
    emit selectionChanged();

    // Listing state stays active across the transport switch so the UI sees a single refresh
    _requestLogList(0, 0xffff);
}

void OnboardLogController::_ftpDownloadToDirectory(const QString &dir)
{
    _downloadPath = dir;
    if (_downloadPath.isEmpty()) {
        return;
    }

    if (!_downloadPath.endsWith(QDir::separator())) {
        _downloadPath += QDir::separator();
    }

    _ftpDownloadQueue.clear();
    _ftpDownloadHadError = false;
    const int numLogs = _logEntriesModel->count();
    for (int i = 0; i < numLogs; i++) {
        QGCOnboardLogEntry *const entry = _logEntriesModel->value<QGCOnboardLogEntry*>(i);
        if (entry && entry->selected() && !entry->ftpPath().isEmpty()) {
            entry->setStatus(tr("Waiting"));
            _ftpDownloadQueue.enqueue(entry);
        }
    }

    if (_ftpDownloadQueue.isEmpty()) {
        qCWarning(OnboardLogControllerLog) << "ftp: no selected logs have FTP paths for download";
        return;
    }

    qCDebug(OnboardLogControllerLog) << "ftp: queued" << _ftpDownloadQueue.size() << "logs for download to" << _downloadPath;
    _setDownloading(true);

    _ftpDownloadEntry(_ftpDownloadQueue.dequeue());
}

void OnboardLogController::_ftpDownloadEntry(QGCOnboardLogEntry *entry)
{
    if (!entry || !_vehicle) {
        return;
    }

    entry->setSelected(false);

    _ftpCurrentDownloadEntry = entry;
    _ftpDownloadBytesAtLastUpdate = 0;
    _ftpDownloadRateAvg = 0.;
    _ftpDownloadElapsed.start();

    entry->setStatus(tr("Downloading"));

    // Save under the remote log file name
    QString localFilename = entry->ftpPath().section(QLatin1Char('/'), -1);
    if (localFilename.isEmpty()) {
        localFilename = QStringLiteral("log_") + QString::number(entry->id()) + QStringLiteral(".ulg");
    }

    if (QFile::exists(_downloadPath + localFilename)) {
        const int dotIdx = localFilename.lastIndexOf(QLatin1Char('.'));
        const QString base = (dotIdx > 0) ? localFilename.left(dotIdx) : localFilename;
        const QString ext = (dotIdx > 0) ? localFilename.mid(dotIdx) : QString();
        uint numDups = 0;
        do {
            numDups++;
            localFilename = base + QStringLiteral("_") + QString::number(numDups) + ext;
        } while (QFile::exists(_downloadPath + localFilename));
    }

    qCDebug(OnboardLogControllerLog) << "ftp: downloading" << entry->ftpPath() << "to" << _downloadPath + localFilename;

    if (!_vehicle->ftpManager()->download(MAV_COMP_ID_AUTOPILOT1, entry->ftpPath(), _downloadPath, localFilename, true)) {
        qCWarning(OnboardLogControllerLog) << "ftp: failed to start download for" << entry->ftpPath();
        entry->setStatus(tr("Error"));
        _ftpCurrentDownloadEntry = nullptr;
        _ftpDownloadHadError = true;
        _ftpDownloadQueueNext();
    }
}

void OnboardLogController::_ftpDownloadQueueNext()
{
    if (!_ftpDownloadQueue.isEmpty()) {
        _ftpDownloadEntry(_ftpDownloadQueue.dequeue());
        return;
    }

    if (_ftpDownloadHadError) {
        qCDebug(OnboardLogControllerLog) << "ftp: download errors occurred, using message based transport for subsequent refreshes";
        _ftpDisabled = true;
    }

    _setDownloading(false);
}

void OnboardLogController::_ftpDownloadComplete(const QString &file, const QString &errorMsg)
{
    if (!_ftpCurrentDownloadEntry) {
        return;
    }

    if (errorMsg.isEmpty()) {
        _ftpCurrentDownloadEntry->setStatus(tr("Downloaded"));
        qCDebug(OnboardLogControllerLog) << "ftp: download complete" << file;
    } else {
        _ftpCurrentDownloadEntry->setStatus(tr("Error"));
        _ftpDownloadHadError = true;
        qCWarning(OnboardLogControllerLog) << "ftp: download error:" << errorMsg;
    }

    _ftpCurrentDownloadEntry = nullptr;
    _ftpDownloadQueueNext();
}

void OnboardLogController::_ftpDownloadProgress(float value)
{
    if (!_ftpCurrentDownloadEntry) {
        return;
    }

    if (_ftpDownloadElapsed.elapsed() < kGUIRateMs) {
        return;
    }

    const size_t totalBytes = static_cast<size_t>(static_cast<qreal>(_ftpCurrentDownloadEntry->size()) * static_cast<qreal>(value));
    if (totalBytes < _ftpDownloadBytesAtLastUpdate) {
        // Guard against non-monotonic progress which would underflow the unsigned delta
        _ftpDownloadBytesAtLastUpdate = totalBytes;
        return;
    }

    const size_t bytesSinceLastUpdate = totalBytes - _ftpDownloadBytesAtLastUpdate;
    const qreal elapsedSec = _ftpDownloadElapsed.elapsed() / 1000.0;
    const qreal rate = (elapsedSec > 0) ? (bytesSinceLastUpdate / elapsedSec) : 0;
    _ftpDownloadRateAvg = (_ftpDownloadRateAvg * 0.95) + (rate * 0.05);
    _ftpDownloadBytesAtLastUpdate = totalBytes;
    _ftpDownloadElapsed.start();

    const QString status = QStringLiteral("%1 (%2/s)").arg(
        QGC::bigSizeToString(totalBytes),
        QGC::bigSizeToString(_ftpDownloadRateAvg));

    _ftpCurrentDownloadEntry->setStatus(status);
}

void OnboardLogController::setCompressLogs(bool compress)
{
    if (_compressLogs != compress) {
        _compressLogs = compress;
        emit compressLogsChanged();
    }
}

bool OnboardLogController::compressLogFile(const QString &logPath)
{
    Q_UNUSED(logPath)
    qCWarning(OnboardLogControllerLog) << "Log compression not yet implemented (decompression-only API)";
    return false;
}

void OnboardLogController::cancelCompression()
{
    // Not implemented - compression API is decompression-only
}

void OnboardLogController::_handleCompressionProgress(qreal progress)
{
    Q_UNUSED(progress)
    // Not implemented - compression API is decompression-only
}

void OnboardLogController::_handleCompressionFinished(bool success)
{
    Q_UNUSED(success)
    // Not implemented - compression API is decompression-only
}
