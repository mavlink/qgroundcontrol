/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

#include "LogDownloadController.h"
#include "MultiVehicleManager.h"
#include "QGCMAVLink.h"
#include "QGCFileDialog.h"
#include "UAS.h"
#include "QGCApplication.h"
#include "QGCToolbox.h"
#include "Vehicle.h"
#include "MainWindow.h"

#include <QDebug>
#include <QSettings>
#include <QUrl>

#define kTimeOutMilliseconds 500

QGC_LOGGING_CATEGORY(LogDownloadLog, "LogDownloadLog")

//----------------------------------------------------------------------------------------
LogDownloadData::LogDownloadData(QGCLogEntry* entry_)
    : ID(entry_->id())
    , entry(entry_)
    , written(0)
{

}

//----------------------------------------------------------------------------------------
QGCLogEntry:: QGCLogEntry(uint logId, const QDateTime& dateTime, uint logSize, bool received)
    : _logID(logId)
    , _logSize(logSize)
    , _logTimeUTC(dateTime)
    , _received(received)
    , _selected(false)
{
    _status = "Pending";
}

//----------------------------------------------------------------------------------------
LogDownloadController::LogDownloadController(void)
    : _uas(NULL)
    , _downloadData(NULL)
    , _vehicle(NULL)
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
    if((_uas && vehicle && _uas == vehicle->uas()) || !vehicle ) {
        return;
    }
    _vehicle = vehicle;
    if (_uas) {
        _logEntriesModel.clear();
        disconnect(_uas, &UASInterface::logEntry, this, &LogDownloadController::_logEntry);
        disconnect(_uas, &UASInterface::logData,  this, &LogDownloadController::_logData);
        _uas = NULL;
    }
    _uas = vehicle->uas();
    connect(_uas, &UASInterface::logEntry, this, &LogDownloadController::_logEntry);
    connect(_uas, &UASInterface::logData,  this, &LogDownloadController::_logData);
}

//----------------------------------------------------------------------------------------
void
LogDownloadController::_logEntry(UASInterface* uas, uint32_t time_utc, uint32_t size, uint16_t id, uint16_t num_logs, uint16_t /*last_log_num*/)
{
    //-- Do we care?
    if(!_uas || uas != _uas || !_requestingLogEntries) {
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
        if(size) {
            id -= _apmOneBased;
            if(id < _logEntriesModel.count()) {
                QGCLogEntry* entry = _logEntriesModel[id];
                entry->setSize(size);
                entry->setTime(QDateTime::fromTime_t(time_utc));
                entry->setReceived(true);
                entry->setStatus(QString("Available"));
            } else {
                qWarning() << "Received log entry for out-of-bound index:" << id;
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
        QGCLogEntry* entry = _logEntriesModel[i];
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
        QGCLogEntry* entry = _logEntriesModel[i];
        if(entry) {
            if(entry->selected()) {
                if(canceled) {
                    entry->setStatus(QString("Canceled"));
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
    _requestingLogEntries = false;
    emit requestingListChanged();
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
        QGCLogEntry* entry = _logEntriesModel[i];
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
                QGCLogEntry* entry = _logEntriesModel[i];
                if(entry && !entry->received()) {
                    entry->setStatus(QString("Error"));
                }
            }
            //-- Give up
            _receivedAllEntries();
            qWarning() << "Too many errors retreiving log list. Giving up.";
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

//----------------------------------------------------------------------------------------
void
LogDownloadController::_logData(UASInterface* uas, uint32_t ofs, uint16_t id, uint8_t count, const uint8_t* data)
{
    if(!_uas || uas != _uas || !_downloadData) {
        return;
    }
    //-- APM "Fix"
    id -= _apmOneBased;
    if(_downloadData->ID != id) {
        qWarning() << "Received log data for wrong log";
        return;
    }
    bool result = false;
    //-- Find offset table entry
    uint o_index = ofs / MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN;
    if(o_index <= (uint)_downloadData->offsets.count()) {
        _downloadData->offsets[o_index] = count;
        //-- Write chunk to file
        if(_downloadData->file.seek(ofs)) {
            if(_downloadData->file.write((const char*)data, count)) {
                _downloadData->written += count;
                //-- Update status
                _downloadData->entry->setStatus(QString::number(_downloadData->written));
                result = true;
                //-- reset retries
                _retries = 0;
                //-- Reset timer
                _timer.start(kTimeOutMilliseconds);
                //-- Do we have it all?
                if(_logComplete()) {
                    _downloadData->entry->setStatus(QString("Downloaded"));
                    //-- Check for more
                    _receivedAllData();
                }
            } else {
                qWarning() << "Error while writing log file chunk";
            }
        } else {
            qWarning() << "Error while seeking log file offset";
        }
    } else {
        qWarning() << "Received log offset greater than expected";
    }
    if(!result) {
        _downloadData->entry->setStatus(QString("Error"));
    }
}

//----------------------------------------------------------------------------------------
bool
LogDownloadController::_logComplete()
{
    //-- Iterate entries and look for a gap
    int num_ofs = _downloadData->offsets.count();
    for(int i = 0; i < num_ofs; i++) {
        if(_downloadData->offsets[i] == 0) {
           return false;
        }
    }
    return true;
}

//----------------------------------------------------------------------------------------
void
LogDownloadController::_receivedAllData()
{
    _timer.stop();
    //-- Anything queued up for download?
    if(_prepareLogDownload()) {
        //-- Request Log
        _requestLogData(_downloadData->ID, 0, _downloadData->entry->size());
    } else {
        _resetSelection();
        _setDownloading(false);
    }
}

//----------------------------------------------------------------------------------------
void
LogDownloadController::_findMissingData()
{
    int start = -1;
    int end   = -1;
    int num_ofs = _downloadData->offsets.count();
    //-- Iterate offsets and look for a gap
    for(int i = 0; i < num_ofs; i++) {
        if(_downloadData->offsets[i] == 0) {
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
    //-- Is there something missing?
    if(start >= 0) {
        //-- Have we tried too many times?
        if(_retries++ > 2) {
            _downloadData->entry->setStatus(QString("Timed Out"));
            //-- Give up
            qWarning() << "Too many errors retreiving log data. Giving up.";
            _receivedAllData();
            return;
        }
        //-- Is it a sequence or just one entry?
        if(end < 0) {
            end = start;
        }
        //-- Request these log chunks again
        _requestLogData(
            _downloadData->ID,
            (uint32_t)(start * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN),
            (uint32_t)((end - start + 1) * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN));
    } else {
        _receivedAllData();
    }
}

//----------------------------------------------------------------------------------------
void
LogDownloadController::_requestLogData(uint8_t id, uint32_t offset, uint32_t count)
{
    if(_vehicle) {
        //-- APM "Fix"
        id += _apmOneBased;
        qCDebug(LogDownloadLog) << "Request log data (id:" << id << "offset:" << offset << "size:" << count << ")";
        mavlink_message_t msg;
        mavlink_msg_log_request_data_pack(
            qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
            qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
            &msg,
            qgcApp()->toolbox()->multiVehicleManager()->activeVehicle()->id(), MAV_COMP_ID_ALL,
            id, offset, count);
        _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
    }
}

//----------------------------------------------------------------------------------------
void
LogDownloadController::refresh(void)
{
    _logEntriesModel.clear();
    _requestLogList();
}

//----------------------------------------------------------------------------------------
void
LogDownloadController::_requestLogList(uint32_t start, uint32_t end)
{
    if(_vehicle && _uas) {
        qCDebug(LogDownloadLog) << "Request log entry list (" << start << "through" << end << ")";
        _requestingLogEntries = true;
        emit requestingListChanged();
        mavlink_message_t msg;
        mavlink_msg_log_request_list_pack(
            qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
            qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
            &msg,
            _vehicle->id(),
            MAV_COMP_ID_ALL,
            start,
            end);
        _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
        //-- Wait 2 seconds before bitching about not getting anything
        _timer.start(2000);
    }
}

//----------------------------------------------------------------------------------------
void
LogDownloadController::download(void)
{
    //-- Stop listing just in case
    _receivedAllEntries();
    //-- Reset downloads, again just in case
    if(_downloadData) {
        delete _downloadData;
        _downloadData = 0;
    }
    _downloadPath.clear();
    _downloadPath = QGCFileDialog::getExistingDirectory(
        MainWindow::instance(),
        "Log Download Directory",
        QDir::homePath(),
        QGCFileDialog::ShowDirsOnly | QGCFileDialog::DontResolveSymlinks);
    if(!_downloadPath.isEmpty()) {
        if(!_downloadPath.endsWith(QDir::separator()))
            _downloadPath += QDir::separator();
        //-- Iterate selected entries and shown them as waiting
        int num_logs = _logEntriesModel.count();
        for(int i = 0; i < num_logs; i++) {
            QGCLogEntry* entry = _logEntriesModel[i];
            if(entry) {
                if(entry->selected()) {
                   entry->setStatus(QString("Waiting"));
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
        QGCLogEntry* entry = _logEntriesModel[i];
        if(entry) {
            if(entry->selected()) {
               return entry;
            }
        }
    }
    return NULL;
}

//----------------------------------------------------------------------------------------
bool
LogDownloadController::_prepareLogDownload()
{
    if(_downloadData) {
        delete _downloadData;
        _downloadData = NULL;
    }
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
        ftime = "UnknownDate";
    } else {
        ftime = entry->time().toString("yyyy-M-d-hh-mm-ss");
    }
    _downloadData = new LogDownloadData(entry);
    _downloadData->filename = QString("log_") + QString::number(entry->id()) + "_" + ftime;
    if(_vehicle->firmwareType() == MAV_AUTOPILOT_PX4) {
        _downloadData->filename += ".px4log";
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
        qWarning() << "Failed to create log file:" <<  _downloadData->filename;
    } else {
        //-- Preallocate file
        if(!_downloadData->file.resize(entry->size())) {
            qWarning() << "Failed to allocate space for log file:" <<  _downloadData->filename;
        } else {
            //-- Prepare Offset Table
            uint o_count = (uint)ceil(entry->size() / (double)MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN);
            for(uint i = 0; i < o_count; i++) {
                _downloadData->offsets.append(0);
            }
            result = true;
        }
    }
    if(!result) {
        if (_downloadData->file.exists()) {
            _downloadData->file.remove();
        }
        _downloadData->entry->setStatus(QString("Error"));
        delete _downloadData;
        _downloadData = NULL;
    }
    return result;
}

//----------------------------------------------------------------------------------------
void
LogDownloadController::_setDownloading(bool active)
{
    _downloadingLogs = active;
    _vehicle->setConnectionLostEnabled(!active);
    emit downloadingLogsChanged();
}

//----------------------------------------------------------------------------------------
void
LogDownloadController::eraseAll(void)
{
    if(_vehicle && _uas) {
        mavlink_message_t msg;
        mavlink_msg_log_erase_pack(
            qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
            qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
            &msg,
            qgcApp()->toolbox()->multiVehicleManager()->activeVehicle()->id(), MAV_COMP_ID_ALL);
        _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
        refresh();
    }
}

//----------------------------------------------------------------------------------------
void
LogDownloadController::cancel(void)
{
    if(_uas){
        _receivedAllEntries();
    }
    if(_downloadData) {
        _downloadData->entry->setStatus(QString("Canceled"));
        if (_downloadData->file.exists()) {
            _downloadData->file.remove();
        }
        delete _downloadData;
        _downloadData = 0;
    }
    _resetSelection(true);
    _setDownloading(false);
}

//-----------------------------------------------------------------------------
QGCLogModel::QGCLogModel(QObject* parent)
    : QAbstractListModel(parent)
{

}

//-----------------------------------------------------------------------------
QGCLogEntry*
QGCLogModel::get(int index)
{
    if (index < 0 || index >= _logEntries.count()) {
        return NULL;
    }
    return _logEntries[index];
}

//-----------------------------------------------------------------------------
int
QGCLogModel::count() const
{
    return _logEntries.count();
}

//-----------------------------------------------------------------------------
void
QGCLogModel::append(QGCLogEntry* object)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    _logEntries.append(object);
    endInsertRows();
    emit countChanged();
}

//-----------------------------------------------------------------------------
void
QGCLogModel::clear(void)
{
    if(!_logEntries.isEmpty()) {
        beginRemoveRows(QModelIndex(), 0, _logEntries.count());
        while (_logEntries.count()) {
            QGCLogEntry* entry = _logEntries.last();
            if(entry) delete entry;
            _logEntries.removeLast();
        }
        endRemoveRows();
        emit countChanged();
    }
}

//-----------------------------------------------------------------------------
QGCLogEntry*
QGCLogModel::operator[](int index)
{
    return get(index);
}

//-----------------------------------------------------------------------------
int
QGCLogModel::rowCount(const QModelIndex& /*parent*/) const
{
    return _logEntries.count();
}

//-----------------------------------------------------------------------------
QVariant
QGCLogModel::data(const QModelIndex & index, int role) const {
    if (index.row() < 0 || index.row() >= _logEntries.count())
        return QVariant();
    if (role == ObjectRole)
        return QVariant::fromValue(_logEntries[index.row()]);
    return QVariant();
}

//-----------------------------------------------------------------------------
QHash<int, QByteArray>
QGCLogModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[ObjectRole] = "logEntry";
    return roles;
}
