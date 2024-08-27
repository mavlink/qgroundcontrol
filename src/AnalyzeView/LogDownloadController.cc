/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "LogDownloadController.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "QGCToolbox.h"
#include "ParameterManager.h"
#include "Vehicle.h"
#include "SettingsManager.h"
#include "MAVLinkProtocol.h"
#include "LogEntry.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"

#include <QtConcurrent/QtConcurrent>
#include <QtCore/QTimer>

QGC_LOGGING_CATEGORY(LogDownloadControllerLog, "qgc.analyzeview.logdownloadcontroller")

LogDownloadController::LogDownloadController(QObject *parent)
    : QObject(parent)
    , _timer(new QTimer(this))
    , _logEntriesModel(new QmlObjectListModel(this))
{
    // qCDebug(LogDownloadControllerLog) << Q_FUNC_INFO << this;

    MultiVehicleManager* const manager = qgcApp()->toolbox()->multiVehicleManager();
    (void) connect(manager, &MultiVehicleManager::activeVehicleChanged, this, &LogDownloadController::_setActiveVehicle);
    (void) connect(_timer, &QTimer::timeout, this, &LogDownloadController::_processDownload);

    _timer->setSingleShot(false);

    _setActiveVehicle(manager->activeVehicle());
}

LogDownloadController::~LogDownloadController()
{
    // qCDebug(LogDownloadControllerLog) << Q_FUNC_INFO << this;
}

void LogDownloadController::_processDownload()
{
    if (_requestingLogEntries) {
        _findMissingEntries();
    } else if (_downloadingLogs) {
        _findMissingData();
    }
}

void LogDownloadController::_setActiveVehicle(Vehicle *vehicle)
{
    if (_vehicle) {
        _logEntriesModel->clearAndDeleteContents();
        (void) disconnect(_vehicle, &Vehicle::logEntry, this, &LogDownloadController::_logEntry);
        (void) disconnect(_vehicle, &Vehicle::logData,  this, &LogDownloadController::_logData);
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

    //-- If this is the first, pre-fill it
    if ((_logEntriesModel->count() == 0) && (num_logs > 0)) {
        //-- Is this APM? They send a first entry with bogus ID and only the
        //   count is valid. From now on, all entries are 1-based.
        if(_vehicle->firmwareType() == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            _apmOneBased = 1;
        }

        for (int i = 0; i < num_logs; i++) {
            QGCLogEntry* const entry = new QGCLogEntry(i);
            (void) _logEntriesModel->append(entry);
        }
    }

    if (num_logs > 0) {
        if ((size > 0) || (_vehicle->firmwareType() != MAV_AUTOPILOT_ARDUPILOTMEGA)) {
            id -= _apmOneBased;
            if (id < _logEntriesModel->count()) {
                QGCLogEntry* const entry = _logEntriesModel->value<QGCLogEntry*>(id);
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
        _timer->start(kTimeOutMilliseconds);
    }
}

bool LogDownloadController::_entriesComplete()
{
    const int num_logs = _logEntriesModel->count();
    for (int i = 0; i < num_logs; i++) {
        QGCLogEntry* const entry = _logEntriesModel->value<QGCLogEntry*>(i);
        if (!entry) {
            continue;
        }

        if (!entry->received()) {
           return false;
        }
    }

    return true;
}

void LogDownloadController::_resetSelection(bool canceled)
{
    const int num_logs = _logEntriesModel->count();
    for (int i = 0; i < num_logs; i++) {
        QGCLogEntry* const entry = _logEntriesModel->value<QGCLogEntry*>(i);
        if (!entry) {
            continue;
        }

        if (entry->selected()) {
            if (canceled) {
                entry->setStatus(QStringLiteral("Canceled"));
            }
            entry->setSelected(false);
        }
    }

    emit selectionChanged();
}

void LogDownloadController::_receivedAllEntries()
{
    _timer->stop();
    _setListing(false);
}

void LogDownloadController::_findMissingEntries()
{
    int start = -1;
    int end = -1;
    int num_logs = _logEntriesModel->count();
    for (int i = 0; i < num_logs; i++) {
        QGCLogEntry* const entry = _logEntriesModel->value<QGCLogEntry*>(i);
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

    if (start >= 0) {
        if (_retries++ > 2) {
            for (int i = 0; i < num_logs; i++) {
                QGCLogEntry* const entry = _logEntriesModel->value<QGCLogEntry*>(i);
                if (entry && !entry->received()) {
                    entry->setStatus(QStringLiteral("Error"));
                }
            }

            _receivedAllEntries();
            qCWarning(LogDownloadControllerLog) << "Too many errors retreiving log list. Giving up.";
            return;
        }

        //-- Is it a sequence or just one entry?
        if (end < 0) {
            end = start;
        }

        //-- APM Offset
        start += _apmOneBased;
        end += _apmOneBased;

        _requestLogList(static_cast<uint32_t>(start), static_cast<uint32_t>(end));
    } else {
        _receivedAllEntries();
    }
}

void LogDownloadController::_updateDataRate()
{
    if (_downloadData->elapsed.elapsed() < kGUIRateMilliseconds) {
        return;
    }

    const qreal rrate = _downloadData->rate_bytes / (_downloadData->elapsed.elapsed() / 1000.0);
    _downloadData->rate_avg = (_downloadData->rate_avg * 0.95) + (rrate * 0.05);
    _downloadData->rate_bytes = 0;

    const QString status = QString("%1 (%2/s)").arg(qgcApp()->bigSizeToString(_downloadData->written),
                                                    qgcApp()->bigSizeToString(_downloadData->rate_avg));

    _downloadData->entry->setStatus(status);
    _downloadData->elapsed.start();
}

void LogDownloadController::_logData(uint32_t ofs, uint16_t id, uint8_t count, const uint8_t* data)
{
    if (!_downloadData) {
        return;
    }
    //-- APM Offset
    id -= _apmOneBased;
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
            _timer->start(kTimeOutMilliseconds);
            if(_logComplete()) {
                _downloadData->entry->setStatus(QStringLiteral("Downloaded"));
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
        _downloadData->entry->setStatus(QStringLiteral("Error"));
    }
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
        _timer->start(kTimeOutMilliseconds);
    } else {
        _resetSelection();
        _setDownloading(false);
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

#if 0
    // Trying the change to infinite log download. This way if retries hit 100% failure the data rate will
    // slowly fall to 0 and the user can Cancel. This should work better on really crappy links.
    if (_retries > 5) {
        _downloadData->entry->setStatus(QStringLiteral("Timed Out"));
        //-- Give up
        qCWarning(LogDownloadControllerLog) << "Too many errors retreiving log data. Giving up.";
        _receivedAllData();
        return;
    }
#endif

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

void LogDownloadController::_requestLogData(uint16_t id, uint32_t offset, uint32_t count, int retryCount)
{
    if (!_vehicle) {
        return;
    }

    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (sharedLink) {
        //-- APM Offset
        id += _apmOneBased;
        qCDebug(LogDownloadControllerLog) << "Request log data (id:" << id << "offset:" << offset << "size:" << count << "retryCount" << retryCount << ")";

        const MAVLinkProtocol* const mavlinkProtocol = qgcApp()->toolbox()->mavlinkProtocol();
        mavlink_message_t msg;
        (void) mavlink_msg_log_request_data_pack_chan(
            mavlinkProtocol->getSystemId(),
            mavlinkProtocol->getComponentId(),
            sharedLink->mavlinkChannel(),
            &msg,
            _vehicle->id(),
            _vehicle->defaultComponentId(),
            id,
            offset,
            count
        );
        _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }
}

void LogDownloadController::refresh()
{
    _logEntriesModel->clearAndDeleteContents();
    _requestLogList(0, 49);
}

void LogDownloadController::_requestLogList(uint32_t start, uint32_t end)
{
    if (!_vehicle) {
        return;
    }

    qCDebug(LogDownloadControllerLog) << "Request log entry list (" << start << "through" << end << ")";
    _setListing(true);

    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (sharedLink) {
        const MAVLinkProtocol* const mavlinkProtocol = qgcApp()->toolbox()->mavlinkProtocol();
        mavlink_message_t msg;
        (void) mavlink_msg_log_request_list_pack_chan(
            mavlinkProtocol->getSystemId(),
            mavlinkProtocol->getComponentId(),
            sharedLink->mavlinkChannel(),
            &msg,
            _vehicle->id(),
            _vehicle->defaultComponentId(),
            start,
            end
        );
        _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }

    _timer->start(5000);
}

void LogDownloadController::download(const QString &path)
{
    QString dir = path;
    if (dir.isEmpty()) {
        dir = qgcApp()->toolbox()->settingsManager()->appSettings()->logSavePath();
    }

    (void) QtConcurrent::run([this, dir]() {
        downloadToDirectory(dir);
    });
}

void LogDownloadController::downloadToDirectory(const QString &dir)
{
    _receivedAllEntries();

    delete _downloadData;
    _downloadData = nullptr;

    _downloadPath = dir;
    if (_downloadPath.isEmpty()) {
        return;
    }

    if (!_downloadPath.endsWith(QDir::separator())) {
        _downloadPath += QDir::separator();
    }

    QGCLogEntry *const log = _getNextSelected();
    if (log) {
        log->setStatus(QStringLiteral("Waiting"));
    }

    _setDownloading(true);
    _receivedAllData();
}

QGCLogEntry *LogDownloadController::_getNextSelected()
{
    const int num_logs = _logEntriesModel->count();
    for (int i = 0; i < num_logs; i++) {
        QGCLogEntry* const entry = _logEntriesModel->value<QGCLogEntry*>(i);
        if (!entry) {
            continue;
        }

        if (entry->selected()) {
           return entry;
        }
    }

    return nullptr;
}

bool LogDownloadController::_prepareLogDownload()
{
    delete _downloadData;
    _downloadData = nullptr;

    QGCLogEntry* const entry = _getNextSelected();
    if (!entry) {
        return false;
    }
    entry->setSelected(false);
    emit selectionChanged();

    bool result = false;
    QString ftime;
    if (entry->time().date().year() < 2010) {
        ftime = QStringLiteral("UnknownDate");
    } else {
        ftime = entry->time().toString(QStringLiteral("yyyy-M-d-hh-mm-ss"));
    }

    _downloadData = new LogDownloadData(entry);
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

    //-- Append a number to the end if the filename already exists
    if (_downloadData->file.exists()) {
        uint num_dups = 0;
        const QStringList filename_spl = _downloadData->filename.split('.');
        do {
            num_dups +=1;
            _downloadData->file.setFileName(filename_spl[0] + '_' + QString::number(num_dups) + '.' + filename_spl[1]);
        } while( _downloadData->file.exists());
    }

    if (!_downloadData->file.open(QIODevice::WriteOnly)) {
        qCWarning(LogDownloadControllerLog) << "Failed to create log file:" <<  _downloadData->filename;
    } else {
        if (!_downloadData->file.resize(entry->size())) {
            qCWarning(LogDownloadControllerLog) << "Failed to allocate space for log file:" <<  _downloadData->filename;
        } else {
            _downloadData->current_chunk = 0;
            _downloadData->chunk_table = QBitArray(_downloadData->chunkBins(), false);
            _downloadData->elapsed.start();
            result = true;
        }
    }

    if (!result) {
        if (_downloadData->file.exists()) {
            _downloadData->file.remove();
        }
        _downloadData->entry->setStatus(QStringLiteral("Error"));
        delete _downloadData;
        _downloadData = nullptr;
    }

    return result;
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

void LogDownloadController::eraseAll()
{
    if (!_vehicle) {
        return;
    }

    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (sharedLink) {
        const MAVLinkProtocol* const mavlinkProtocol = qgcApp()->toolbox()->mavlinkProtocol();
        MultiVehicleManager* const multiVehicleManager = qgcApp()->toolbox()->multiVehicleManager();
        mavlink_message_t msg;
        (void) mavlink_msg_log_erase_pack_chan(
            mavlinkProtocol->getSystemId(),
            mavlinkProtocol->getComponentId(),
            sharedLink->mavlinkChannel(),
            &msg,
            multiVehicleManager->activeVehicle()->id(),
            multiVehicleManager->activeVehicle()->defaultComponentId()
        );
        _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }

    refresh();
}

void LogDownloadController::cancel()
{
    _receivedAllEntries();

    if (_downloadData) {
        _downloadData->entry->setStatus(QStringLiteral("Canceled"));
        if (_downloadData->file.exists()) {
            _downloadData->file.remove();
        }
        delete _downloadData;
        _downloadData = 0;
    }

    _resetSelection(true);
    _setDownloading(false);
}
