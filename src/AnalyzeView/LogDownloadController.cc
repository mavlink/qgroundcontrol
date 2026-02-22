#include "LogDownloadController.h"
#include "AppSettings.h"
#include "LogEntry.h"

#ifdef QGC_MAVFTP_LOG_DOWNLOAD
#include "FTPManager.h"
#endif
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "ParameterManager.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"
#include "Vehicle.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QDir>
#include <QtCore/QTimer>

QGC_LOGGING_CATEGORY(LogDownloadControllerLog, "AnalyzeView.LogDownloadController")

LogDownloadController::LogDownloadController(QObject *parent)
    : QObject(parent)
    , _timer(new QTimer(this))
    , _logEntriesModel(new QmlObjectListModel(this))
{
    qCDebug(LogDownloadControllerLog) << this;

    (void) connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this, &LogDownloadController::_setActiveVehicle);
    (void) connect(_timer, &QTimer::timeout, this, &LogDownloadController::_processDownload);

    _timer->setSingleShot(false);

    _setActiveVehicle(MultiVehicleManager::instance()->activeVehicle());
}

LogDownloadController::~LogDownloadController()
{
    qCDebug(LogDownloadControllerLog) << this;
}

void LogDownloadController::download(const QString &path)
{
    const QString dir = path.isEmpty() ? SettingsManager::instance()->appSettings()->logSavePath() : path;
    _downloadToDirectory(dir);
}

void LogDownloadController::_downloadToDirectory(const QString &dir)
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

#ifdef QGC_MAVFTP_LOG_DOWNLOAD
    if (_useMavftp) {
        // Build queue of selected entries that have FTP paths
        _ftpDownloadQueue.clear();
        const int numLogs = _logEntriesModel->count();
        for (int i = 0; i < numLogs; i++) {
            QGCLogEntry *const entry = _logEntriesModel->value<QGCLogEntry*>(i);
            if (entry && entry->selected() && !entry->ftpPath().isEmpty()) {
                entry->setStatus(tr("Waiting"));
                _ftpDownloadQueue.enqueue(entry);
            }
        }

        if (_ftpDownloadQueue.isEmpty()) {
            qCWarning(LogDownloadControllerLog) << "MAVFTP: no selected logs have FTP paths for download";
            return;
        }

        qCDebug(LogDownloadControllerLog) << "MAVFTP: queued" << _ftpDownloadQueue.size() << "logs for download to" << _downloadPath;
        _setDownloading(true);

        // Start the first download
        QGCLogEntry *first = _ftpDownloadQueue.dequeue();
        _downloadLogMavftp(first);
        return;
    }
#endif

    QGCLogEntry *const log = _getNextSelected();
    if (log) {
        log->setStatus(tr("Waiting"));
    }

    _setDownloading(true);
    _receivedAllData();
}

void LogDownloadController::_processDownload()
{
    if (_requestingLogEntries) {
        _findMissingEntries();
    } else if (_downloadingLogs) {
        _findMissingData();
    }
}

void LogDownloadController::_findMissingEntries()
{
    const int num_logs = _logEntriesModel->count();
    int start = -1;
    int end = -1;
    for (int i = 0; i < num_logs; i++) {
        const QGCLogEntry *const entry = _logEntriesModel->value<const QGCLogEntry*>(i);
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
            QGCLogEntry *const entry = _logEntriesModel->value<QGCLogEntry*>(i);
            if (entry && !entry->received()) {
                entry->setStatus(tr("Error"));
            }
        }

        _receivedAllEntries();
        qCWarning(LogDownloadControllerLog) << "Too many errors retreiving log list. Giving up.";
        return;
    }

    if (end < 0) {
        end = start;
    }

    start += _apmOffset;
    end += _apmOffset;

    _requestLogList(static_cast<uint32_t>(start), static_cast<uint32_t>(end));
}

void LogDownloadController::_setActiveVehicle(Vehicle *vehicle)
{
    if (vehicle == _vehicle) {
        return;
    }

    if (_vehicle) {
        _logEntriesModel->clearAndDeleteContents();
        (void) disconnect(_vehicle, &Vehicle::logEntry, this, &LogDownloadController::_logEntry);
        (void) disconnect(_vehicle, &Vehicle::logData,  this, &LogDownloadController::_logData);

#ifdef QGC_MAVFTP_LOG_DOWNLOAD
        // Disconnect MAVFTP signals and reset state
        FTPManager *ftp = _vehicle->ftpManager();
        (void) disconnect(ftp, &FTPManager::listDirectoryComplete, this, &LogDownloadController::_mavftpListDirComplete);
        (void) disconnect(ftp, &FTPManager::downloadComplete, this, &LogDownloadController::_ftpDownloadComplete);
        (void) disconnect(ftp, &FTPManager::commandProgress, this, &LogDownloadController::_ftpDownloadProgress);
        _useMavftp = false;
        _mavftpListState = MavftpIdle;
        _mavftpDirsToList.clear();
        _mavftpLogIdCounter = 0;
        _ftpDownloadQueue.clear();
        _ftpCurrentDownloadEntry = nullptr;
#endif
    }

    _vehicle = vehicle;

    if (_vehicle) {
        (void) connect(_vehicle, &Vehicle::logEntry, this, &LogDownloadController::_logEntry);
        (void) connect(_vehicle, &Vehicle::logData,  this, &LogDownloadController::_logData);
    }
}

void LogDownloadController::_logEntry(uint32_t time_utc, uint32_t size, uint16_t id, uint16_t num_logs, uint16_t last_log_num)
{
    Q_UNUSED(last_log_num);

    if (!_requestingLogEntries) {
        return;
    }

    if ((_logEntriesModel->count() == 0) && (num_logs > 0)) {
        if (_vehicle->firmwareType() == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            // APM ID starts at 1
            _apmOffset = 1;
        }

        for (int i = 0; i < num_logs; i++) {
            QGCLogEntry *const entry = new QGCLogEntry(i);
            _logEntriesModel->append(entry);
        }
    }

    if (num_logs > 0) {
        if ((size > 0) || (_vehicle->firmwareType() != MAV_AUTOPILOT_ARDUPILOTMEGA)) {
            id -= _apmOffset;
            if (id < _logEntriesModel->count()) {
                QGCLogEntry *const entry = _logEntriesModel->value<QGCLogEntry*>(id);
                entry->setSize(size);
                entry->setTime(QDateTime::fromSecsSinceEpoch(time_utc));
                entry->setReceived(true);
                entry->setStatus(tr("Available"));
            } else {
                qCWarning(LogDownloadControllerLog) << "Received log entry for out-of-bound index:" << id;
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

void LogDownloadController::_receivedAllEntries()
{
    _timer->stop();
    _setListing(false);
}

bool LogDownloadController::_entriesComplete() const
{
    const int num_logs = _logEntriesModel->count();
    for (int i = 0; i < num_logs; i++) {
        const QGCLogEntry *const entry = _logEntriesModel->value<const QGCLogEntry*>(i);
        if (!entry) {
            continue;
        }

        if (!entry->received()) {
            return false;
        }
    }

    return true;
}

void LogDownloadController::_logData(uint32_t ofs, uint16_t id, uint8_t count, const uint8_t *data)
{
    if (!_downloadingLogs || !_downloadData) {
        return;
    }

    id -= _apmOffset;
    if (_downloadData->ID != id) {
        qCWarning(LogDownloadControllerLog) << "Received log data for wrong log";
        return;
    }

    if ((ofs % MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN) != 0) {
        qCWarning(LogDownloadControllerLog) << "Ignored misaligned incoming packet @" << ofs;
        return;
    }

    bool result = false;
    if (ofs <= _downloadData->entry->size()) {
        const uint32_t chunk = ofs / LogDownloadData::kChunkSize;
        // qCDebug(LogDownloadControllerLog) << "Received data - Offset:" << ofs << "Chunk:" << chunk;
        if (chunk != _downloadData->current_chunk) {
            qCWarning(LogDownloadControllerLog) << "Ignored packet for out of order chunk actual:expected" << chunk << _downloadData->current_chunk;
            return;
        }

        const uint16_t bin = (ofs - (chunk * LogDownloadData::kChunkSize)) / MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN;
        if (bin >= _downloadData->chunk_table.size()) {
            qCWarning(LogDownloadControllerLog) << "Out of range bin received";
        } else {
            _downloadData->chunk_table.setBit(bin);
        }

        if (_downloadData->file.pos() != ofs) {
            if (!_downloadData->file.seek(ofs)) {
                qCWarning(LogDownloadControllerLog) << "Error while seeking log file offset";
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
                                _downloadData->current_chunk * LogDownloadData::kChunkSize,
                                _downloadData->chunk_table.size() * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN);
            } else if ((bin < (_downloadData->chunk_table.size() - 1)) && _downloadData->chunk_table.at(bin + 1)) {
                // Likely to be grabbing fragments and got to the end of a gap
                _findMissingData();
            }
        } else {
            qCWarning(LogDownloadControllerLog) << "Error while writing log file chunk";
        }
    } else {
        qCWarning(LogDownloadControllerLog) << "Received log offset greater than expected";
    }

    if (!result) {
        _downloadData->entry->setStatus(tr("Error"));
    }
}

void LogDownloadController::_findMissingData()
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

    const uint32_t pos = (_downloadData->current_chunk * LogDownloadData::kChunkSize) + (start * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN);
    const uint32_t len = (end - start) * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN;
    _requestLogData(_downloadData->ID, pos, len, _retries);
}

void LogDownloadController::_updateDataRate()
{
    if (_downloadData->elapsed.elapsed() < kGUIRateMs) {
        return;
    }

    const qreal rate = _downloadData->rate_bytes / (_downloadData->elapsed.elapsed() / 1000.0);
    _downloadData->rate_avg = (_downloadData->rate_avg * 0.95) + (rate * 0.05);
    _downloadData->rate_bytes = 0;

    const QString status = QStringLiteral("%1 (%2/s)").arg(qgcApp()->bigSizeToString(_downloadData->written),
                                                           qgcApp()->bigSizeToString(_downloadData->rate_avg));

    _downloadData->entry->setStatus(status);
    _downloadData->elapsed.start();
}

bool LogDownloadController::_chunkComplete() const
{
    return _downloadData->chunkEquals(true);
}

bool LogDownloadController::_logComplete() const
{
    return (_chunkComplete() && ((_downloadData->current_chunk + 1) == _downloadData->numChunks()));
}

void LogDownloadController::_receivedAllData()
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

bool LogDownloadController::_prepareLogDownload()
{
    _downloadData.reset();

    QGCLogEntry *const entry = _getNextSelected();
    if (!entry) {
        return false;
    }

    entry->setSelected(false);
    emit selectionChanged();

    const QString ftime = (entry->time().date().year() >= 2010) ? entry->time().toString(QStringLiteral("yyyy-M-d-hh-mm-ss")) : QStringLiteral("UnknownDate");

    _downloadData = std::make_unique<LogDownloadData>(entry);
    _downloadData->filename = QStringLiteral("log_") + QString::number(entry->id()) + "_" + ftime;

    if (_vehicle->firmwareType() == MAV_AUTOPILOT_PX4) {
        const QString loggerParam = QStringLiteral("SYS_LOGGER");
        ParameterManager *const parameterManager = _vehicle->parameterManager();
        if (parameterManager->parameterExists(ParameterManager::defaultComponentId, loggerParam) && parameterManager->getParameter(ParameterManager::defaultComponentId, loggerParam)->rawValue().toInt() == 0) {
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
            _downloadData->file.setFileName(filename);
        } while ( _downloadData->file.exists());
    }

    bool result = false;
    if (!_downloadData->file.open(QIODevice::WriteOnly)) {
        qCWarning(LogDownloadControllerLog) << "Failed to create log file:" <<  _downloadData->filename;
    } else if (!_downloadData->file.resize(entry->size())) {
        qCWarning(LogDownloadControllerLog) << "Failed to allocate space for log file:" <<  _downloadData->filename;
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

        _downloadData->entry->setStatus(QStringLiteral("Error"));
        _downloadData.reset();
    }

    return result;
}

void LogDownloadController::refresh()
{
    _logEntriesModel->clearAndDeleteContents();

#ifdef QGC_MAVFTP_LOG_DOWNLOAD
    if (_vehicle
        && _vehicle->px4Firmware()
        && _vehicle->capabilitiesKnown()
        && (_vehicle->capabilityBits() & MAV_PROTOCOL_CAPABILITY_FTP)) {
        _useMavftp = true;
        qCDebug(LogDownloadControllerLog) << "refresh: using MAVFTP (px4:" << _vehicle->px4Firmware()
            << "capsKnown:" << _vehicle->capabilitiesKnown()
            << "ftpCap:" << bool(_vehicle->capabilityBits() & MAV_PROTOCOL_CAPABILITY_FTP) << ")";
        _requestLogListMavftp();
        return;
    }
    _useMavftp = false;
#endif

    qCDebug(LogDownloadControllerLog) << "refresh: using legacy LOG_REQUEST protocol";
    _requestLogList(0, 0xffff);
}

QGCLogEntry *LogDownloadController::_getNextSelected() const
{
    const int numLogs = _logEntriesModel->count();
    for (int i = 0; i < numLogs; i++) {
        QGCLogEntry *const entry = _logEntriesModel->value<QGCLogEntry*>(i);
        if (!entry) {
            continue;
        }

        if (entry->selected()) {
           return entry;
        }
    }

    return nullptr;
}

void LogDownloadController::cancel()
{
#ifdef QGC_MAVFTP_LOG_DOWNLOAD
    if (_useMavftp) {
        qCDebug(LogDownloadControllerLog) << "cancel: mode=MAVFTP";

        if (!_vehicle) {
            return;
        }

        if (_requestingLogEntries) {
            _vehicle->ftpManager()->cancelListDirectory();
            _mavftpDirsToList.clear();
            _mavftpListState = MavftpIdle;
            _receivedAllEntries();
        }

        if (_downloadingLogs) {
            _vehicle->ftpManager()->cancelDownload();
            if (_ftpCurrentDownloadEntry) {
                _ftpCurrentDownloadEntry->setStatus(tr("Canceled"));
                _ftpCurrentDownloadEntry = nullptr;
            }
            _ftpDownloadQueue.clear();
        }

        _resetSelection(true);
        _setDownloading(false);
        return;
    }
#endif

    qCDebug(LogDownloadControllerLog) << "cancel: mode=legacy";
    _requestLogEnd();
    _receivedAllEntries();

    if (_downloadData) {
        _downloadData->entry->setStatus(tr("Canceled"));
        if (_downloadData->file.exists()) {
            (void) _downloadData->file.remove();
        }

        _downloadData.reset();
    }

    _resetSelection(true);
    _setDownloading(false);
}

void LogDownloadController::_resetSelection(bool canceled)
{
    const int num_logs = _logEntriesModel->count();
    for (int i = 0; i < num_logs; i++) {
        QGCLogEntry *const entry = _logEntriesModel->value<QGCLogEntry*>(i);
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

void LogDownloadController::eraseAll()
{
    if (!_vehicle) {
        qCWarning(LogDownloadControllerLog) << "Vehicle Unavailable";
        return;
    }

#ifdef QGC_MAVFTP_LOG_DOWNLOAD
    if (_useMavftp) {
        // TODO: Implement MAVFTP-based log erasure (FTP file removal)
        qCWarning(LogDownloadControllerLog) << "eraseAll: MAVFTP erase not yet implemented, using legacy LOG_ERASE";
    }
#endif

    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCWarning(LogDownloadControllerLog) << "Link Unavailable";
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
        qCWarning(LogDownloadControllerLog) << "Failed to send";
        return;
    }

    refresh();
}

void LogDownloadController::_requestLogList(uint32_t start, uint32_t end)
{
    if (!_vehicle) {
        qCWarning(LogDownloadControllerLog) << "Vehicle Unavailable";
        return;
    }

    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCWarning(LogDownloadControllerLog) << "Link Unavailable";
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
        qCWarning(LogDownloadControllerLog) << "Failed to send";
        return;
    }

    qCDebug(LogDownloadControllerLog) << "Request log entry list (" << start << "through" << end << ")";
    _setListing(true);
    _timer->start(kRequestLogListTimeoutMs);
}

void LogDownloadController::_requestLogData(uint16_t id, uint32_t offset, uint32_t count, int retryCount)
{
    if (!_vehicle) {
        qCWarning(LogDownloadControllerLog) << "Vehicle Unavailable";
        return;
    }

    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCWarning(LogDownloadControllerLog) << "Link Unavailable";
        return;
    }

    id += _apmOffset;
    qCDebug(LogDownloadControllerLog) << "Request log data (id:" << id << "offset:" << offset << "size:" << count << "retryCount" << retryCount << ")";

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
        qCWarning(LogDownloadControllerLog) << "Failed to send";
    }
}

void LogDownloadController::_requestLogEnd()
{
    if (!_vehicle) {
        qCWarning(LogDownloadControllerLog) << "Vehicle Unavailable";
        return;
    }

    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCWarning(LogDownloadControllerLog) << "Link Unavailable";
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
        qCWarning(LogDownloadControllerLog) << "Failed to send";
    }
}

void LogDownloadController::_setDownloading(bool active)
{
    if (_downloadingLogs != active) {
        _downloadingLogs = active;
        _vehicle->vehicleLinkManager()->setCommunicationLostEnabled(!active);
        emit downloadingLogsChanged();
    }
}

void LogDownloadController::_setListing(bool active)
{
    if (_requestingLogEntries != active) {
        _requestingLogEntries = active;
        _vehicle->vehicleLinkManager()->setCommunicationLostEnabled(!active);
        emit requestingListChanged();
    }
}

#ifdef QGC_MAVFTP_LOG_DOWNLOAD

// --- MAVFTP log listing ---

static constexpr const char *kPx4LogRoot = "/fs/microsd/log";

void LogDownloadController::_requestLogListMavftp()
{
    if (!_vehicle) {
        qCWarning(LogDownloadControllerLog) << "Vehicle unavailable for MAVFTP listing";
        return;
    }

    _mavftpDirsToList.clear();
    _mavftpLogIdCounter = 0;
    _mavftpListState = MavftpListingRoot;

    FTPManager *ftp = _vehicle->ftpManager();
    (void) disconnect(ftp, &FTPManager::listDirectoryComplete, this, &LogDownloadController::_mavftpListDirComplete);
    (void) connect(ftp, &FTPManager::listDirectoryComplete, this, &LogDownloadController::_mavftpListDirComplete);

    qCDebug(LogDownloadControllerLog) << "MAVFTP: listing root" << kPx4LogRoot;
    _setListing(true);

    if (!ftp->listDirectory(MAV_COMP_ID_AUTOPILOT1, QString(kPx4LogRoot))) {
        qCWarning(LogDownloadControllerLog) << "MAVFTP: failed to start root listing";
        _mavftpListState = MavftpIdle;
        _receivedAllEntries();
    }
}

void LogDownloadController::_mavftpListDirComplete(const QStringList &dirList, const QString &errorMsg)
{
    if (!errorMsg.isEmpty()) {
        qCWarning(LogDownloadControllerLog) << "MAVFTP listing error:" << errorMsg;
        _mavftpListState = MavftpIdle;
        _receivedAllEntries();
        return;
    }

    if (_mavftpListState == MavftpListingRoot) {
        // Root listing: collect date/session subdirectories
        for (const QString &entry : dirList) {
            if (entry.startsWith(QLatin1Char('D'))) {
                const QString dirName = entry.mid(1);
                if (!dirName.isEmpty()) {
                    _mavftpDirsToList.append(dirName);
                }
            }
        }

        // Sort directories so logs appear in chronological order
        _mavftpDirsToList.sort();
        qCDebug(LogDownloadControllerLog) << "MAVFTP: root listing found" << _mavftpDirsToList.size() << "subdirectories";

        _mavftpListState = MavftpListingSubdir;
        _listNextMavftpSubdir();
    } else {
        // Subdirectory listing: parse file entries
        const QString currentDir = _mavftpDirsToList.isEmpty() ? QString() : _mavftpDirsToList.first();

        // Parse date from directory name
        QDate dirDate = QDate::fromString(currentDir, QStringLiteral("yyyy-MM-dd"));
        uint logsFoundInDir = 0;

        for (const QString &entry : dirList) {
            if (entry.startsWith(QLatin1Char('F'))) {
                const QString fileInfo = entry.mid(1);
                const int tabIdx = fileInfo.indexOf(QLatin1Char('\t'));
                if (tabIdx < 0) {
                    continue;
                }

                const QString fileName = fileInfo.left(tabIdx);
                const QString sizeStr = fileInfo.mid(tabIdx + 1);

                if (!fileName.endsWith(QStringLiteral(".ulg"), Qt::CaseInsensitive)) {
                    continue;
                }

                bool sizeOk = false;
                const uint fileSize = sizeStr.toUInt(&sizeOk);
                if (!sizeOk) {
                    continue;
                }

                // Parse time from filename (HH_MM_SS.ulg)
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

                const QString ftpPath = QString(kPx4LogRoot) + QStringLiteral("/") + currentDir + QStringLiteral("/") + fileName;

                QGCLogEntry *logEntry = new QGCLogEntry(_mavftpLogIdCounter++, dateTime, fileSize, true, this);
                logEntry->setFtpPath(ftpPath);
                logEntry->setStatus(tr("Available"));
                _logEntriesModel->append(logEntry);
                logsFoundInDir++;
            }
        }

        qCDebug(LogDownloadControllerLog) << "MAVFTP:" << currentDir << "->" << logsFoundInDir << "logs";

        // Remove the directory we just processed and continue
        if (!_mavftpDirsToList.isEmpty()) {
            _mavftpDirsToList.removeFirst();
        }

        _listNextMavftpSubdir();
    }
}

void LogDownloadController::_listNextMavftpSubdir()
{
    if (_mavftpDirsToList.isEmpty()) {
        qCDebug(LogDownloadControllerLog) << "MAVFTP: listing complete, found" << _logEntriesModel->count() << "logs";
        _mavftpListState = MavftpIdle;
        _receivedAllEntries();
        return;
    }

    const QString subdir = _mavftpDirsToList.first();
    const QString path = QString(kPx4LogRoot) + QStringLiteral("/") + subdir;

    qCDebug(LogDownloadControllerLog) << "MAVFTP: listing subdir" << path;

    if (!_vehicle->ftpManager()->listDirectory(MAV_COMP_ID_AUTOPILOT1, path)) {
        qCWarning(LogDownloadControllerLog) << "MAVFTP: failed to list subdir" << path;
        _mavftpDirsToList.removeFirst();
        _listNextMavftpSubdir();
    }
}

// --- MAVFTP log download ---

void LogDownloadController::_downloadLogMavftp(QGCLogEntry *entry)
{
    if (!entry || !_vehicle) {
        return;
    }

    entry->setSelected(false);
    emit selectionChanged();

    _ftpCurrentDownloadEntry = entry;
    _ftpDownloadBytesAtLastUpdate = 0;
    _ftpDownloadRateAvg = 0.;
    _ftpDownloadElapsed.start();

    entry->setStatus(tr("Downloading"));

    // Build local filename
    QString localFilename;
    if (entry->time().isValid() && entry->time().date().year() >= 2010) {
        localFilename = entry->time().toString(QStringLiteral("yyyy-M-d-hh-mm-ss")) + QStringLiteral(".ulg");
    } else {
        localFilename = QStringLiteral("log_") + QString::number(entry->id()) + QStringLiteral(".ulg");
    }

    // Handle duplicate filenames
    if (QFile::exists(_downloadPath + localFilename)) {
        const QStringList parts = localFilename.split(QLatin1Char('.'));
        uint numDups = 0;
        do {
            numDups++;
            localFilename = parts[0] + QStringLiteral("_") + QString::number(numDups) + QStringLiteral(".") + parts[1];
        } while (QFile::exists(_downloadPath + localFilename));
    }

    FTPManager *ftp = _vehicle->ftpManager();
    (void) disconnect(ftp, &FTPManager::downloadComplete, this, &LogDownloadController::_ftpDownloadComplete);
    (void) disconnect(ftp, &FTPManager::commandProgress, this, &LogDownloadController::_ftpDownloadProgress);
    (void) connect(ftp, &FTPManager::downloadComplete, this, &LogDownloadController::_ftpDownloadComplete);
    (void) connect(ftp, &FTPManager::commandProgress, this, &LogDownloadController::_ftpDownloadProgress);

    qCDebug(LogDownloadControllerLog) << "MAVFTP: downloading" << entry->ftpPath() << "to" << _downloadPath + localFilename;

    if (!ftp->download(MAV_COMP_ID_AUTOPILOT1, entry->ftpPath(), _downloadPath, localFilename, true)) {
        qCWarning(LogDownloadControllerLog) << "MAVFTP: failed to start download for" << entry->ftpPath();
        entry->setStatus(tr("Error"));
        _ftpCurrentDownloadEntry = nullptr;

        // Try next in queue
        if (!_ftpDownloadQueue.isEmpty()) {
            _downloadLogMavftp(_ftpDownloadQueue.dequeue());
        } else {
            _setDownloading(false);
        }
    }
}

void LogDownloadController::_ftpDownloadComplete(const QString &file, const QString &errorMsg)
{
    if (!_ftpCurrentDownloadEntry) {
        return;
    }

    if (errorMsg.isEmpty()) {
        _ftpCurrentDownloadEntry->setStatus(tr("Downloaded"));
        qCDebug(LogDownloadControllerLog) << "MAVFTP: download complete" << file;
    } else {
        _ftpCurrentDownloadEntry->setStatus(tr("Error"));
        qCWarning(LogDownloadControllerLog) << "MAVFTP: download error:" << errorMsg;
    }

    _ftpCurrentDownloadEntry = nullptr;

    // Start next queued download
    if (!_ftpDownloadQueue.isEmpty()) {
        _downloadLogMavftp(_ftpDownloadQueue.dequeue());
    } else {
        _setDownloading(false);
    }
}

void LogDownloadController::_ftpDownloadProgress(float value)
{
    if (!_ftpCurrentDownloadEntry) {
        return;
    }

    if (_ftpDownloadElapsed.elapsed() < kGUIRateMs) {
        return;
    }

    const size_t totalBytes = static_cast<size_t>(static_cast<qreal>(_ftpCurrentDownloadEntry->size()) * static_cast<qreal>(value));
    const size_t bytesSinceLastUpdate = totalBytes - _ftpDownloadBytesAtLastUpdate;
    const qreal elapsedSec = _ftpDownloadElapsed.elapsed() / 1000.0;
    const qreal rate = (elapsedSec > 0) ? (bytesSinceLastUpdate / elapsedSec) : 0;
    _ftpDownloadRateAvg = (_ftpDownloadRateAvg * 0.95) + (rate * 0.05);
    _ftpDownloadBytesAtLastUpdate = totalBytes;
    _ftpDownloadElapsed.start();

    const QString status = QStringLiteral("%1 (%2/s)").arg(
        qgcApp()->bigSizeToString(totalBytes),
        qgcApp()->bigSizeToString(_ftpDownloadRateAvg));

    _ftpCurrentDownloadEntry->setStatus(status);
}

#endif // QGC_MAVFTP_LOG_DOWNLOAD

void LogDownloadController::setCompressLogs(bool compress)
{
    if (_compressLogs != compress) {
        _compressLogs = compress;
        emit compressLogsChanged();
    }
}

bool LogDownloadController::compressLogFile(const QString &logPath)
{
    Q_UNUSED(logPath)
    qCWarning(LogDownloadControllerLog) << "Log compression not yet implemented (decompression-only API)";
    return false;
}

void LogDownloadController::cancelCompression()
{
    // Not implemented - compression API is decompression-only
}

void LogDownloadController::_handleCompressionProgress(qreal progress)
{
    Q_UNUSED(progress)
    // Not implemented - compression API is decompression-only
}

void LogDownloadController::_handleCompressionFinished(bool success)
{
    Q_UNUSED(success)
    // Not implemented - compression API is decompression-only
}
