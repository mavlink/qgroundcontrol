#include "LogProtocolTransport.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QTimer>

#include "AppSettings.h"
#include "MAVLinkLib.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "OnboardLogDownloadData.h"
#include "OnboardLogEntry.h"
#include "ParameterManager.h"
#include "QGCFormat.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

QGC_LOGGING_CATEGORY(LogProtocolTransportLog, "AnalyzeView.LogProtocolTransport")

LogProtocolTransport::LogProtocolTransport(QObject* parent) : OnboardLogTransport(parent), _timer(new QTimer(this))
{
    qCDebug(LogProtocolTransportLog) << this;

    (void)connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this,
                  &LogProtocolTransport::_setActiveVehicle);
    (void)connect(_timer, &QTimer::timeout, this, &LogProtocolTransport::_processDownload);

    _timer->setSingleShot(false);

    _setActiveVehicle(MultiVehicleManager::instance()->activeVehicle());
}

LogProtocolTransport::~LogProtocolTransport()
{
    qCDebug(LogProtocolTransportLog) << this;
}

void LogProtocolTransport::download(const QString& path)
{
    const QString dir = path.isEmpty() ? SettingsManager::instance()->appSettings()->logSavePath() : path;
    _downloadToDirectory(dir);
}

void LogProtocolTransport::_downloadToDirectory(const QString& dir)
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

    OnboardLogEntry* const log = _getNextSelected();
    if (log) {
        log->setStatus(tr("Waiting"));
    }

    _setDownloading(true);
    _receivedAllData();
}

void LogProtocolTransport::_processDownload()
{
    if (_requestingLogEntries) {
        _findMissingEntries();
    } else if (_downloadingLogs) {
        _findMissingData();
    }
}

void LogProtocolTransport::_findMissingEntries()
{
    const int num_logs = _logEntriesModel->count();
    int start = -1;
    int end = -1;
    for (int i = 0; i < num_logs; i++) {
        const OnboardLogEntry* const entry = _logEntriesModel->value<const OnboardLogEntry*>(i);
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
            OnboardLogEntry* const entry = _logEntriesModel->value<OnboardLogEntry*>(i);
            if (entry && !entry->received()) {
                entry->setStatus(tr("Error"));
            }
        }

        _receivedAllEntries();
        qCWarning(LogProtocolTransportLog) << "Too many errors retreiving log list. Giving up.";
        return;
    }

    if (end < 0) {
        end = start;
    }

    start += _apmOffset;
    end += _apmOffset;

    _requestLogList(static_cast<uint32_t>(start), static_cast<uint32_t>(end));
}

void LogProtocolTransport::_setActiveVehicle(Vehicle* vehicle)
{
    if (vehicle == _vehicle) {
        return;
    }

    if (_vehicle) {
        _logEntriesModel->clearAndDeleteContents();
        (void)disconnect(_vehicle, &Vehicle::logEntry, this, &LogProtocolTransport::_logEntry);
        (void)disconnect(_vehicle, &Vehicle::logData, this, &LogProtocolTransport::_logData);
    }

    _vehicle = vehicle;

    if (_vehicle) {
        (void)connect(_vehicle, &Vehicle::logEntry, this, &LogProtocolTransport::_logEntry);
        (void)connect(_vehicle, &Vehicle::logData, this, &LogProtocolTransport::_logData);
    }
}

void LogProtocolTransport::_logEntry(uint32_t time_utc, uint32_t size, uint16_t id, uint16_t num_logs,
                                     uint16_t last_log_num)
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
            OnboardLogEntry* const entry = new OnboardLogEntry(i);
            _logEntriesModel->append(entry);
        }
    }

    if (num_logs > 0) {
        if ((size > 0) || (_vehicle->firmwareType() != MAV_AUTOPILOT_ARDUPILOTMEGA)) {
            id -= _apmOffset;
            if (id < _logEntriesModel->count()) {
                OnboardLogEntry* const entry = _logEntriesModel->value<OnboardLogEntry*>(id);
                entry->setSize(size);
                entry->setTime(QDateTime::fromSecsSinceEpoch(time_utc));
                entry->setReceived(true);
                entry->setStatus(tr("Available"));
            } else {
                qCWarning(LogProtocolTransportLog) << "Received onboard log entry for out-of-bound index:" << id;
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

void LogProtocolTransport::_receivedAllEntries()
{
    _timer->stop();
    _setListing(false);
}

bool LogProtocolTransport::_entriesComplete() const
{
    const int num_logs = _logEntriesModel->count();
    for (int i = 0; i < num_logs; i++) {
        const OnboardLogEntry* const entry = _logEntriesModel->value<const OnboardLogEntry*>(i);
        if (!entry) {
            continue;
        }

        if (!entry->received()) {
            return false;
        }
    }

    return true;
}

void LogProtocolTransport::_logData(uint32_t ofs, uint16_t id, uint8_t count, const uint8_t* data)
{
    if (!_downloadingLogs || !_downloadData) {
        return;
    }

    id -= _apmOffset;
    if (_downloadData->ID != id) {
        qCWarning(LogProtocolTransportLog) << "Received log data for wrong log";
        return;
    }

    if ((ofs % MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN) != 0) {
        qCWarning(LogProtocolTransportLog) << "Ignored misaligned incoming packet @" << ofs;
        return;
    }

    bool result = false;
    if (ofs <= _downloadData->entry->size()) {
        const uint32_t chunk = ofs / OnboardLogDownloadData::kChunkSize;
        // qCDebug(LogProtocolTransportLog) << "Received data - Offset:" << ofs << "Chunk:" << chunk;
        if (chunk != _downloadData->current_chunk) {
            qCWarning(LogProtocolTransportLog)
                << "Ignored packet for out of order chunk actual:expected" << chunk << _downloadData->current_chunk;
            return;
        }

        const uint16_t bin = (ofs - (chunk * OnboardLogDownloadData::kChunkSize)) / MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN;
        if (bin >= _downloadData->chunk_table.size()) {
            qCWarning(LogProtocolTransportLog) << "Out of range bin received";
        } else {
            _downloadData->chunk_table.setBit(bin);
        }

        if (_downloadData->file.pos() != ofs) {
            if (!_downloadData->file.seek(ofs)) {
                qCWarning(LogProtocolTransportLog) << "Error while seeking log file offset";
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
                _requestLogData(_downloadData->ID, _downloadData->current_chunk * OnboardLogDownloadData::kChunkSize,
                                _downloadData->chunk_table.size() * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN);
            } else if ((bin < (_downloadData->chunk_table.size() - 1)) && _downloadData->chunk_table.at(bin + 1)) {
                // Likely to be grabbing fragments and got to the end of a gap
                _findMissingData();
            }
        } else {
            qCWarning(LogProtocolTransportLog) << "Error while writing log file chunk";
        }
    } else {
        qCWarning(LogProtocolTransportLog) << "Received log offset greater than expected";
    }

    if (!result) {
        _downloadData->entry->setStatus(tr("Error"));
    }
}

void LogProtocolTransport::_findMissingData()
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

    const uint32_t pos = (_downloadData->current_chunk * OnboardLogDownloadData::kChunkSize) +
                         (start * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN);
    const uint32_t len = (end - start) * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN;
    _requestLogData(_downloadData->ID, pos, len, _retries);
}

void LogProtocolTransport::_updateDataRate()
{
    constexpr uint kSizeUpdateThreshold = 102400;  // 0.1 MB
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

        status = QStringLiteral("%1 (%2/s)")
                     .arg(QGC::bigSizeToString(_downloadData->written), QGC::bigSizeToString(_downloadData->rate_avg));
        _downloadData->elapsed.start();
    } else {
        // Update size only, keep previous rate
        status = QStringLiteral("%1 (%2/s)")
                     .arg(QGC::bigSizeToString(_downloadData->written), QGC::bigSizeToString(_downloadData->rate_avg));
    }

    _downloadData->entry->setStatus(status);
    _downloadData->last_status_written = _downloadData->written;
}

bool LogProtocolTransport::_chunkComplete() const
{
    return _downloadData->chunkEquals(true);
}

bool LogProtocolTransport::_logComplete() const
{
    return (_chunkComplete() && ((_downloadData->current_chunk + 1) == _downloadData->numChunks()));
}

void LogProtocolTransport::_receivedAllData()
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

bool LogProtocolTransport::_prepareLogDownload()
{
    _downloadData.reset();

    OnboardLogEntry* const entry = _getNextSelected();
    if (!entry) {
        return false;
    }

    entry->setSelected(false);
    emit selectionChanged();

    const QString ftime = (entry->time().date().year() >= 2010)
                              ? entry->time().toString(QStringLiteral("yyyy-M-d-hh-mm-ss"))
                              : QStringLiteral("UnknownDate");

    _downloadData = std::make_unique<OnboardLogDownloadData>(entry);
    _downloadData->filename = QStringLiteral("log_") + QString::number(entry->id()) + "_" + ftime;

    if (_vehicle->firmwareType() == MAV_AUTOPILOT_PX4) {
        const QString loggerParam = QStringLiteral("SYS_LOGGER");
        ParameterManager* const parameterManager = _vehicle->parameterManager();
        Fact* const loggerFact = parameterManager->parameterExists(ParameterManager::defaultComponentId, loggerParam)
                                     ? parameterManager->getParameter(ParameterManager::defaultComponentId, loggerParam)
                                     : nullptr;
        if (loggerFact && loggerFact->rawValue().toInt() == 0) {
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
        } while (_downloadData->file.exists());
    }

    bool result = false;
    if (!_downloadData->file.open(QIODevice::WriteOnly)) {
        qCWarning(LogProtocolTransportLog) << "Failed to create log file:" << _downloadData->filename;
    } else if (!_downloadData->file.resize(entry->size())) {
        qCWarning(LogProtocolTransportLog) << "Failed to allocate space for log file:" << _downloadData->filename;
    } else {
        _downloadData->current_chunk = 0;
        _downloadData->chunk_table = QBitArray(_downloadData->chunkBins(), false);
        _downloadData->elapsed.start();
        result = true;
    }

    if (!result) {
        if (_downloadData->file.exists()) {
            (void)_downloadData->file.remove();
        }

        _downloadData->entry->setStatus(QStringLiteral("Error"));
        _downloadData.reset();
    }

    return result;
}

void LogProtocolTransport::refresh()
{
    _logEntriesModel->clearAndDeleteContents();
    _requestLogList(0, 0xffff);
}

OnboardLogEntry* LogProtocolTransport::_getNextSelected() const
{
    const int numLogs = _logEntriesModel->count();
    for (int i = 0; i < numLogs; i++) {
        OnboardLogEntry* const entry = _logEntriesModel->value<OnboardLogEntry*>(i);
        if (!entry) {
            continue;
        }

        if (entry->selected()) {
            return entry;
        }
    }

    return nullptr;
}

void LogProtocolTransport::cancel()
{
    _requestLogEnd();
    _receivedAllEntries();

    if (_downloadData) {
        _downloadData->entry->setStatus(QStringLiteral("Canceled"));
        if (_downloadData->file.exists()) {
            (void)_downloadData->file.remove();
        }

        _downloadData.reset();
    }

    _resetSelection(true);
    _setDownloading(false);
}

void LogProtocolTransport::_resetSelection(bool canceled)
{
    const int num_logs = _logEntriesModel->count();
    for (int i = 0; i < num_logs; i++) {
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

void LogProtocolTransport::eraseAll()
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

    mavlink_message_t msg{};
    (void)mavlink_msg_log_erase_pack_chan(MAVLinkProtocol::instance()->getSystemId(), MAVLinkProtocol::getComponentId(),
                                          sharedLink->mavlinkChannel(), &msg, _vehicle->id(),
                                          _vehicle->defaultComponentId());

    if (!_vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg)) {
        qCWarning(LogProtocolTransportLog) << "Failed to send";
        return;
    }

    refresh();
}

void LogProtocolTransport::_requestLogList(uint32_t start, uint32_t end)
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

    mavlink_message_t msg{};
    (void)mavlink_msg_log_request_list_pack_chan(MAVLinkProtocol::instance()->getSystemId(),
                                                 MAVLinkProtocol::getComponentId(), sharedLink->mavlinkChannel(), &msg,
                                                 _vehicle->id(), _vehicle->defaultComponentId(), start, end);

    if (!_vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg)) {
        qCWarning(LogProtocolTransportLog) << "Failed to send";
        return;
    }

    qCDebug(LogProtocolTransportLog) << "Request onboard log entry list (" << start << "through" << end << ")";
    _setListing(true);
    _timer->start(kRequestLogListTimeoutMs);
}

void LogProtocolTransport::_requestLogData(uint16_t id, uint32_t offset, uint32_t count, int retryCount)
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

    id += _apmOffset;
    qCDebug(LogProtocolTransportLog) << "Request log data (id:" << id << "offset:" << offset << "size:" << count
                                     << "retryCount" << retryCount << ")";

    mavlink_message_t msg{};
    (void)mavlink_msg_log_request_data_pack_chan(MAVLinkProtocol::instance()->getSystemId(),
                                                 MAVLinkProtocol::getComponentId(), sharedLink->mavlinkChannel(), &msg,
                                                 _vehicle->id(), _vehicle->defaultComponentId(), id, offset, count);

    if (!_vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg)) {
        qCWarning(LogProtocolTransportLog) << "Failed to send";
    }
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

    mavlink_message_t msg{};
    (void)mavlink_msg_log_request_end_pack_chan(MAVLinkProtocol::instance()->getSystemId(),
                                                MAVLinkProtocol::getComponentId(), sharedLink->mavlinkChannel(), &msg,
                                                _vehicle->id(), _vehicle->defaultComponentId());

    if (!_vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg)) {
        qCWarning(LogProtocolTransportLog) << "Failed to send";
    }
}

void LogProtocolTransport::_setDownloading(bool active)
{
    if (_downloadingLogs != active) {
        _downloadingLogs = active;
        _vehicle->vehicleLinkManager()->setCommunicationLostEnabled(!active);
        emit downloadingLogsChanged();
    }
}

void LogProtocolTransport::_setListing(bool active)
{
    if (_requestingLogEntries != active) {
        _requestingLogEntries = active;
        _vehicle->vehicleLinkManager()->setCommunicationLostEnabled(!active);
        emit requestingListChanged();
    }
}

void LogProtocolTransport::setCompressLogs(bool compress)
{
    if (_compressLogs != compress) {
        _compressLogs = compress;
        emit compressLogsChanged();
    }
}

bool LogProtocolTransport::compressLogFile(const QString& logPath)
{
    Q_UNUSED(logPath)
    qCWarning(LogProtocolTransportLog) << "Log compression not yet implemented (decompression-only API)";
    return false;
}

void LogProtocolTransport::cancelCompression()
{
    // Not implemented - compression API is decompression-only
}

void LogProtocolTransport::_handleCompressionProgress(qreal progress)
{
    Q_UNUSED(progress)
    // Not implemented - compression API is decompression-only
}

void LogProtocolTransport::_handleCompressionFinished(bool success)
{
    Q_UNUSED(success)
    // Not implemented - compression API is decompression-only
}
