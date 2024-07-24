/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

#define kTimeOutMilliseconds 500
#define kGUIRateMilliseconds 17
#define kTableBins           512
#define kChunkSize           (kTableBins * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN)

QGC_LOGGING_CATEGORY(LogDownloadControllerLog, "qgc.analyzeview.logdownloadcontroller")

//----------------------------------------------------------------------------------------
LogDownloadController::LogDownloadController(void)
    : _downloadData(nullptr)
    , _vehicle(nullptr)
    , _requestingLogEntries(false)
    , _downloadingLogs(false)
    , _retries(0)
    , _apmOneBased(0)
{
    MultiVehicleManager *manager = qgcApp()->toolbox()->multiVehicleManager();
    connect(manager, &MultiVehicleManager::activeVehicleChanged, this, &LogDownloadController::_setActiveVehicle);
    connect(&_timer, &QTimer::timeout, this, &LogDownloadController::_processDownload);
    _setActiveVehicle(manager->activeVehicle());
}

//----------------------------------------------------------------------------------------
void
LogDownloadController::_processDownload()
{
    if(_requestingLogEntries) {
        _findMissingEntries();
    } else if(_downloadingLogs) {
        _findMissingData();
    }
}

//----------------------------------------------------------------------------------------
void
LogDownloadController::_setActiveVehicle(Vehicle* vehicle)
{
    if(_vehicle) {
        _logEntriesModel.clearAndDeleteContents();
        disconnect(_vehicle, &Vehicle::logEntry, this, &LogDownloadController::_logEntry);
        disconnect(_vehicle, &Vehicle::logData,  this, &LogDownloadController::_logData);
    }
    _vehicle = vehicle;
    if(_vehicle) {
        connect(_vehicle, &Vehicle::logEntry, this, &LogDownloadController::_logEntry);
        connect(_vehicle, &Vehicle::logData,  this, &LogDownloadController::_logData);
    }
}

//----------------------------------------------------------------------------------------
void
LogDownloadController::_logEntry(uint32_t time_utc, uint32_t size, uint16_t id, uint16_t num_logs, uint16_t /*last_log_num*/)
{
    //-- Do we care?
    if(!_requestingLogEntries) {
        return;
    }
    //-- If this is the first, pre-fill it
    if(!_logEntriesModel.count() && num_logs > 0) {
        //-- Is this APM? They send a first entry with bogus ID and only the
        //   count is valid. From now on, all entries are 1-based.
        if(_vehicle->firmwareType() == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            _apmOneBased = 1;
        }
        for(int i = 0; i < num_logs; i++) {
            QGCLogEntry *entry = new QGCLogEntry(i);
            _logEntriesModel.append(entry);
        }
    }
    //-- Update this log record
    if(num_logs > 0) {
        //-- Skip if empty (APM first packet)
        if(size || _vehicle->firmwareType() != MAV_AUTOPILOT_ARDUPILOTMEGA) {
            id -= _apmOneBased;
            if(id < _logEntriesModel.count()) {
                QGCLogEntry* entry = _logEntriesModel.value<QGCLogEntry*>(id);
                entry->setSize(size);
                entry->setTime(QDateTime::fromSecsSinceEpoch(time_utc));
                entry->setReceived(true);
                entry->setStatus(tr("Available"));
            } else {
                qCWarning(LogDownloadControllerLog) << "Received log entry for out-of-bound index:" << id;
            }
        }
    } else {
        //-- No logs to list
        _receivedAllEntries();
    }
    //-- Reset retry count
    _retries = 0;
    //-- Do we have it all?
    if(_entriesComplete()) {
        _receivedAllEntries();
    } else {
        //-- Reset timer
        _timer.start(kTimeOutMilliseconds);
    }
}

//----------------------------------------------------------------------------------------
bool
LogDownloadController::_entriesComplete()
{
    //-- Iterate entries and look for a gap
    int num_logs = _logEntriesModel.count();
    for(int i = 0; i < num_logs; i++) {
        QGCLogEntry* entry = _logEntriesModel.value<QGCLogEntry*>(i);
        if(entry) {
            if(!entry->received()) {
               return false;
            }
        }
    }
    return true;
}

//----------------------------------------------------------------------------------------
void
LogDownloadController::_resetSelection(bool canceled)
{
    int num_logs = _logEntriesModel.count();
    for(int i = 0; i < num_logs; i++) {
        QGCLogEntry* entry = _logEntriesModel.value<QGCLogEntry*>(i);
        if(entry) {
            if(entry->selected()) {
                if(canceled) {
                    entry->setStatus(tr("Canceled"));
                }
                entry->setSelected(false);
            }
        }
    }
    emit selectionChanged();
}

//----------------------------------------------------------------------------------------
void
LogDownloadController::_receivedAllEntries()
{
    _timer.stop();
    _setListing(false);
}

//----------------------------------------------------------------------------------------
void
LogDownloadController::_findMissingEntries()
{
    int start = -1;
    int end   = -1;
    int num_logs = _logEntriesModel.count();
    //-- Iterate entries and look for a gap
    for(int i = 0; i < num_logs; i++) {
        QGCLogEntry* entry = _logEntriesModel.value<QGCLogEntry*>(i);
        if(entry) {
            if(!entry->received()) {
                if(start < 0)
                    start = i;
                else
                    end = i;
            } else {
                if(start >= 0) {
                    break;
                }
            }
        }
    }
    //-- Is there something missing?
    if(start >= 0) {
        //-- Have we tried too many times?
        if(_retries++ > 2) {
            for(int i = 0; i < num_logs; i++) {
                QGCLogEntry* entry = _logEntriesModel.value<QGCLogEntry*>(i);
                if(entry && !entry->received()) {
                    entry->setStatus(tr("Error"));
                }
            }
            //-- Give up
            _receivedAllEntries();
            qCWarning(LogDownloadControllerLog) << "Too many errors retreiving log list. Giving up.";
            return;
        }
        //-- Is it a sequence or just one entry?
        if(end < 0) {
            end = start;
        }
        //-- APM "Fix"
        start += _apmOneBased;
        end   += _apmOneBased;
        //-- Request these entries again
        _requestLogList((uint32_t)start, (uint32_t) end);
    } else {
        _receivedAllEntries();
    }
}

void LogDownloadController::_updateDataRate(void)
{
    if (_downloadData->elapsed.elapsed() >= kGUIRateMilliseconds) {
        //-- Update download rate
        qreal rrate = _downloadData->rate_bytes / (_downloadData->elapsed.elapsed() / 1000.0);
        _downloadData->rate_avg = (_downloadData->rate_avg * 0.95) + (rrate * 0.05);
        _downloadData->rate_bytes = 0;

        //-- Update status
        const QString status = QString("%1 (%2/s)").arg(qgcApp()->bigSizeToString(_downloadData->written),
                                                        qgcApp()->bigSizeToString(_downloadData->rate_avg));

        _downloadData->entry->setStatus(status);
        _downloadData->elapsed.start();
    }
}

//----------------------------------------------------------------------------------------
void
LogDownloadController::_logData(uint32_t ofs, uint16_t id, uint8_t count, const uint8_t* data)
{
    if(!_downloadData) {
        return;
    }
    //-- APM "Fix"
    id -= _apmOneBased;
    if(_downloadData->ID != id) {
        qCWarning(LogDownloadControllerLog) << "Received log data for wrong log";
        return;
    }

    if ((ofs % MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN) != 0) {
        qCWarning(LogDownloadControllerLog) << "Ignored misaligned incoming packet @" << ofs;
        return;
    }

    bool result = false;
    uint32_t timeout_time = kTimeOutMilliseconds;
    if(ofs <= _downloadData->entry->size()) {
        const uint32_t chunk = ofs / kChunkSize;
        if (chunk != _downloadData->current_chunk) {
            qCWarning(LogDownloadControllerLog) << "Ignored packet for out of order chunk actual:expected" << chunk << _downloadData->current_chunk;
            return;
        }
        const uint16_t bin = (ofs - chunk*kChunkSize) / MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN;
        if (bin >= _downloadData->chunk_table.size()) {
            qCWarning(LogDownloadControllerLog) << "Out of range bin received";
        } else
            _downloadData->chunk_table.setBit(bin);
        if (_downloadData->file.pos() != ofs) {
            // Seek to correct position
            if (!_downloadData->file.seek(ofs)) {
                qCWarning(LogDownloadControllerLog) << "Error while seeking log file offset";
                return;
            }
        }

        //-- Write chunk to file
        if(_downloadData->file.write((const char*)data, count)) {
            _downloadData->written += count;
            _downloadData->rate_bytes += count;
            _updateDataRate();
            result = true;
            //-- reset retries
            _retries = 0;
            //-- Reset timer
            _timer.start(timeout_time);
            //-- Do we have it all?
            if(_logComplete()) {
                _downloadData->entry->setStatus(tr("Downloaded"));
                //-- Check for more
                _receivedAllData();
            } else if (_chunkComplete()) {
                _downloadData->advanceChunk();
                _requestLogData(_downloadData->ID,
                                _downloadData->current_chunk*kChunkSize,
                                _downloadData->chunk_table.size()*MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN);
            } else if (bin < _downloadData->chunk_table.size() - 1 && _downloadData->chunk_table.at(bin+1)) {
                // Likely to be grabbing fragments and got to the end of a gap
                _findMissingData();
            }
        } else {
            qCWarning(LogDownloadControllerLog) << "Error while writing log file chunk";
        }
    } else {
        qCWarning(LogDownloadControllerLog) << "Received log offset greater than expected";
    }
    if(!result) {
        _downloadData->entry->setStatus(tr("Error"));
    }
}


//----------------------------------------------------------------------------------------
bool
LogDownloadController::_chunkComplete() const
{
    return _downloadData->chunkEquals(true);
}

//----------------------------------------------------------------------------------------
bool
LogDownloadController::_logComplete() const
{
    return _chunkComplete() && (_downloadData->current_chunk+1) == _downloadData->numChunks();
}

//----------------------------------------------------------------------------------------
void
LogDownloadController::_receivedAllData()
{
    _timer.stop();
    //-- Anything queued up for download?
    if(_prepareLogDownload()) {
        //-- Request Log
        _requestLogData(_downloadData->ID, 0, _downloadData->chunk_table.size()*MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN);
        _timer.start(kTimeOutMilliseconds);
    } else {
        _resetSelection();
        _setDownloading(false);
    }
}

//----------------------------------------------------------------------------------------
void
LogDownloadController::_findMissingData()
{
    if (_logComplete()) {
         _receivedAllData();
         return;
    } else if (_chunkComplete()) {
        _downloadData->advanceChunk();
    }

    _retries++;
#if 0
    // Trying the change to infinite log download. This way if retries hit 100% failure the data rate will
    // slowly fall to 0 and the user can Cancel. This should work better on really crappy links.
    if(_retries > 5) {
        _downloadData->entry->setStatus(tr("Timed Out"));
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

    const uint32_t pos = _downloadData->current_chunk*kChunkSize + start*MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN,
                   len = (end - start)*MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN;
    _requestLogData(_downloadData->ID, pos, len, _retries);
}

//----------------------------------------------------------------------------------------
void
LogDownloadController::_requestLogData(uint16_t id, uint32_t offset, uint32_t count, int retryCount)
{
    if (_vehicle) {
        WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();
        if (!weakLink.expired()) {
            SharedLinkInterfacePtr sharedLink = weakLink.lock();

            //-- APM "Fix"
            id += _apmOneBased;
            qCDebug(LogDownloadControllerLog) << "Request log data (id:" << id << "offset:" << offset << "size:" << count << "retryCount" << retryCount << ")";
            mavlink_message_t msg;
            mavlink_msg_log_request_data_pack_chan(
                        qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                        qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                        sharedLink->mavlinkChannel(),
                        &msg,
                        _vehicle->id(), _vehicle->defaultComponentId(),
                        id, offset, count);
            _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
        }
    }
}

//----------------------------------------------------------------------------------------
void
LogDownloadController::refresh(void)
{
    _logEntriesModel.clearAndDeleteContents();
    //-- Get first 50 entries
    _requestLogList(0, 49);
}

//----------------------------------------------------------------------------------------
void
LogDownloadController::_requestLogList(uint32_t start, uint32_t end)
{
    if(_vehicle) {
        qCDebug(LogDownloadControllerLog) << "Request log entry list (" << start << "through" << end << ")";
        _setListing(true);
        WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();
        if (!weakLink.expired()) {
            SharedLinkInterfacePtr sharedLink = weakLink.lock();

            mavlink_message_t msg;
            mavlink_msg_log_request_list_pack_chan(
                        qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                        qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                        sharedLink->mavlinkChannel(),
                        &msg,
                        _vehicle->id(),
                        _vehicle->defaultComponentId(),
                        start,
                        end);
            _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
        }
        //-- Wait 5 seconds before bitching about not getting anything
        _timer.start(5000);
    }
}

//----------------------------------------------------------------------------------------
void
LogDownloadController::download(QString path)
{
    QString dir = path;
    if (dir.isEmpty()) {
        dir = qgcApp()->toolbox()->settingsManager()->appSettings()->logSavePath();
    }
    downloadToDirectory(dir);
}

void LogDownloadController::downloadToDirectory(const QString& dir)
{
    //-- Stop listing just in case
    _receivedAllEntries();
    //-- Reset downloads, again just in case
    delete _downloadData;
    _downloadData = nullptr;

    _downloadPath = dir;
    if(!_downloadPath.isEmpty()) {
        if(!_downloadPath.endsWith(QDir::separator()))
            _downloadPath += QDir::separator();
        //-- Iterate selected entries and shown them as waiting
        int num_logs = _logEntriesModel.count();
        for(int i = 0; i < num_logs; i++) {
            QGCLogEntry* entry = _logEntriesModel.value<QGCLogEntry*>(i);
            if(entry) {
                if(entry->selected()) {
                   entry->setStatus(tr("Waiting"));
                }
            }
        }
        //-- Start download process
        _setDownloading(true);
        _receivedAllData();
    }
}

//----------------------------------------------------------------------------------------
QGCLogEntry*
LogDownloadController::_getNextSelected()
{
    //-- Iterate entries and look for a selected file
    int num_logs = _logEntriesModel.count();
    for(int i = 0; i < num_logs; i++) {
        QGCLogEntry* entry = _logEntriesModel.value<QGCLogEntry*>(i);
        if(entry) {
            if(entry->selected()) {
               return entry;
            }
        }
    }
    return nullptr;
}

//----------------------------------------------------------------------------------------
bool
LogDownloadController::_prepareLogDownload()
{
    delete _downloadData;
    _downloadData = nullptr;

    QGCLogEntry* entry = _getNextSelected();
    if(!entry) {
        return false;
    }
    //-- Deselect file
    entry->setSelected(false);
    emit selectionChanged();
    bool result = false;
    QString ftime;
    if(entry->time().date().year() < 2010) {
        ftime = tr("UnknownDate");
    } else {
        ftime = entry->time().toString(QStringLiteral("yyyy-M-d-hh-mm-ss"));
    }
    _downloadData = new LogDownloadData(entry);
    _downloadData->filename = QString("log_") + QString::number(entry->id()) + "_" + ftime;
    if (_vehicle->firmwareType() == MAV_AUTOPILOT_PX4) {
        QString loggerParam = QStringLiteral("SYS_LOGGER");
        if (_vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, loggerParam) &&
                _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, loggerParam)->rawValue().toInt() == 0) {
            _downloadData->filename += ".px4log";
        } else {
            _downloadData->filename += ".ulg";
        }
    } else {
        _downloadData->filename += ".bin";
    }
    _downloadData->file.setFileName(_downloadPath + _downloadData->filename);
    //-- Append a number to the end if the filename already exists
    if (_downloadData->file.exists()){
        uint num_dups = 0;
        QStringList filename_spl = _downloadData->filename.split('.');
        do {
            num_dups +=1;
            _downloadData->file.setFileName(filename_spl[0] + '_' + QString::number(num_dups) + '.' + filename_spl[1]);
        } while( _downloadData->file.exists());
    }
    //-- Create file
    if (!_downloadData->file.open(QIODevice::WriteOnly)) {
        qCWarning(LogDownloadControllerLog) << "Failed to create log file:" <<  _downloadData->filename;
    } else {
        //-- Preallocate file
        if(!_downloadData->file.resize(entry->size())) {
            qCWarning(LogDownloadControllerLog) << "Failed to allocate space for log file:" <<  _downloadData->filename;
        } else {
            _downloadData->current_chunk = 0;
            _downloadData->chunk_table = QBitArray(_downloadData->chunkBins(), false);
            _downloadData->elapsed.start();
            result = true;
        }
    }
    if(!result) {
        if (_downloadData->file.exists()) {
            _downloadData->file.remove();
        }
        _downloadData->entry->setStatus(tr("Error"));
        delete _downloadData;
        _downloadData = nullptr;
    }
    return result;
}

//----------------------------------------------------------------------------------------
void
LogDownloadController::_setDownloading(bool active)
{
    if (_downloadingLogs != active) {
        _downloadingLogs = active;
        _vehicle->vehicleLinkManager()->setCommunicationLostEnabled(!active);
        emit downloadingLogsChanged();
    }
}

//----------------------------------------------------------------------------------------
void
LogDownloadController::_setListing(bool active)
{
    if (_requestingLogEntries != active) {
        _requestingLogEntries = active;
        _vehicle->vehicleLinkManager()->setCommunicationLostEnabled(!active);
        emit requestingListChanged();
    }
}

//----------------------------------------------------------------------------------------
void
LogDownloadController::eraseAll(void)
{
    if(_vehicle) {
        WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();
        if (!weakLink.expired()) {
            SharedLinkInterfacePtr sharedLink = weakLink.lock();

            mavlink_message_t msg;
            mavlink_msg_log_erase_pack_chan(
                        qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                        qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                        sharedLink->mavlinkChannel(),
                        &msg,
                        qgcApp()->toolbox()->multiVehicleManager()->activeVehicle()->id(), qgcApp()->toolbox()->multiVehicleManager()->activeVehicle()->defaultComponentId());
            _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
        }
        refresh();
    }
}

//----------------------------------------------------------------------------------------
void
LogDownloadController::cancel(void)
{
    _receivedAllEntries();
    if(_downloadData) {
        _downloadData->entry->setStatus(tr("Canceled"));
        if (_downloadData->file.exists()) {
            _downloadData->file.remove();
        }
        delete _downloadData;
        _downloadData = 0;
    }
    _resetSelection(true);
    _setDownloading(false);
}
