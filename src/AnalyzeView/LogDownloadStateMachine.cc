#include "LogDownloadStateMachine.h"
#include "LogDownloadController.h"
#include "LogEntry.h"
#include "MAVLinkProtocol.h"
#include "ParameterManager.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(LogDownloadStateMachineLog, "AnalyzeView.LogDownloadStateMachine")

LogDownloadStateMachine::LogDownloadStateMachine(LogDownloadController* controller, QObject* parent)
    : QGCStateMachine(QStringLiteral("LogDownload"), nullptr, parent)
    , _controller(controller)
{
    _buildListStateMachine();
    _buildDownloadStateMachine();
    _setupTransitions();
}

LogDownloadStateMachine::~LogDownloadStateMachine()
{
}

void LogDownloadStateMachine::_buildListStateMachine()
{
    _listIdleState = new QGCState("ListIdle", this);
    _requestingListState = new QGCState("RequestingList", this);
    _findingMissingEntriesState = new QGCState("FindingMissingEntries", this);
    _listCompleteState = new QGCFinalState("ListComplete", this);
    _listFailedState = new QGCFinalState("ListFailed", this);

    setInitialState(_listIdleState);

    // State entry actions
    connect(_requestingListState, &QGCState::entered, this, [this]() {
        _retries = 0;
        _requestLogList(0, 0xffff);
    });

    connect(_findingMissingEntriesState, &QGCState::entered, this, [this]() {
        _findMissingEntries();
    });

    connect(_listCompleteState, &QGCFinalState::entered, this, [this]() {
        _setRequestingList(false);
        emit listComplete();
    });

    connect(_listFailedState, &QGCFinalState::entered, this, [this]() {
        _setRequestingList(false);
        emit operationFailed(tr("Failed to retrieve log list"));
    });
}

void LogDownloadStateMachine::_buildDownloadStateMachine()
{
    _downloadIdleState = new QGCState("DownloadIdle", this);
    _preparingState = new QGCState("Preparing", this);
    _downloadingState = new QGCState("Downloading", this);
    _findingGapsState = new QGCState("FindingGaps", this);
    _nextChunkState = new QGCState("NextChunk", this);
    _nextLogState = new QGCState("NextLog", this);
    _downloadCompleteState = new QGCFinalState("DownloadComplete", this);
    _downloadFailedState = new QGCFinalState("DownloadFailed", this);
    _downloadCancelledState = new QGCFinalState("DownloadCancelled", this);

    // State entry actions
    connect(_preparingState, &QGCState::entered, this, [this]() {
        _prepareNextLog();
    });

    connect(_downloadingState, &QGCState::entered, this, [this]() {
        _retries = 0;
    });

    connect(_findingGapsState, &QGCState::entered, this, [this]() {
        _findMissingData();
    });

    connect(_nextChunkState, &QGCState::entered, this, [this]() {
        _advanceChunk();
    });

    connect(_nextLogState, &QGCState::entered, this, [this]() {
        _finishCurrentLog();
    });

    connect(_downloadCompleteState, &QGCFinalState::entered, this, [this]() {
        _setDownloading(false);
        emit downloadComplete();
    });

    connect(_downloadFailedState, &QGCFinalState::entered, this, [this]() {
        _setDownloading(false);
        emit operationFailed(tr("Download failed"));
    });

    connect(_downloadCancelledState, &QGCFinalState::entered, this, [this]() {
        _setDownloading(false);
    });
}

void LogDownloadStateMachine::_setupTransitions()
{
    // List workflow transitions
    _listIdleState->addTransition(new MachineEventTransition("refresh", _requestingListState));

    auto* listReceivedTransition = new MachineEventTransition("entries_received", _findingMissingEntriesState);
    _requestingListState->addTransition(listReceivedTransition);

    auto* allEntriesReceivedTransition = new MachineEventTransition("all_entries_received", _listCompleteState);
    _findingMissingEntriesState->addTransition(allEntriesReceivedTransition);

    auto* retryListTransition = new MachineEventTransition("retry_list", _requestingListState);
    _findingMissingEntriesState->addTransition(retryListTransition);

    auto* listFailedTransition = new MachineEventTransition("list_failed", _listFailedState);
    _findingMissingEntriesState->addTransition(listFailedTransition);

    // Timeout on list request
    addTimeoutTransition(_requestingListState, _listTimeoutMs, _findingMissingEntriesState);

    // Download workflow transitions
    _downloadIdleState->addTransition(new MachineEventTransition("start_download", _preparingState));

    auto* preparedTransition = new MachineEventTransition("prepared", _downloadingState);
    _preparingState->addTransition(preparedTransition);

    auto* noMoreLogsTransition = new MachineEventTransition("no_more_logs", _downloadCompleteState);
    _preparingState->addTransition(noMoreLogsTransition);

    auto* prepareFailedTransition = new MachineEventTransition("prepare_failed", _downloadFailedState);
    _preparingState->addTransition(prepareFailedTransition);

    auto* dataReceivedTransition = new MachineEventTransition("data_received", _findingGapsState);
    _downloadingState->addTransition(dataReceivedTransition);

    auto* chunkCompleteTransition = new MachineEventTransition("chunk_complete", _nextChunkState);
    _findingGapsState->addTransition(chunkCompleteTransition);

    auto* gapsFoundTransition = new MachineEventTransition("gaps_found", _downloadingState);
    _findingGapsState->addTransition(gapsFoundTransition);

    auto* logCompleteTransition = new MachineEventTransition("log_complete", _nextLogState);
    _findingGapsState->addTransition(logCompleteTransition);

    auto* nextChunkStartTransition = new MachineEventTransition("next_chunk_started", _downloadingState);
    _nextChunkState->addTransition(nextChunkStartTransition);

    auto* nextLogTransition = new MachineEventTransition("next_log", _preparingState);
    _nextLogState->addTransition(nextLogTransition);

    auto* allLogsCompleteTransition = new MachineEventTransition("all_logs_complete", _downloadCompleteState);
    _nextLogState->addTransition(allLogsCompleteTransition);

    // Timeout on data request
    addTimeoutTransition(_downloadingState, _dataTimeoutMs, _findingGapsState);

    // Cancel transitions - can cancel from any active state
    auto* cancelListTransition = new MachineEventTransition("cancel", _listCompleteState);
    _requestingListState->addTransition(cancelListTransition);

    auto* cancelFindingTransition = new MachineEventTransition("cancel", _listCompleteState);
    _findingMissingEntriesState->addTransition(cancelFindingTransition);

    auto* cancelPrepTransition = new MachineEventTransition("cancel", _downloadCancelledState);
    _preparingState->addTransition(cancelPrepTransition);

    auto* cancelDownloadTransition = new MachineEventTransition("cancel", _downloadCancelledState);
    _downloadingState->addTransition(cancelDownloadTransition);

    auto* cancelGapsTransition = new MachineEventTransition("cancel", _downloadCancelledState);
    _findingGapsState->addTransition(cancelGapsTransition);
}

void LogDownloadStateMachine::startListRequest()
{
    if (_requestingList || _downloading) {
        return;
    }

    _vehicle = _controller->_vehicle;
    if (!_vehicle) {
        emit operationFailed(tr("No vehicle connected"));
        return;
    }

    _apmOffset = 0;
    _retries = 0;

    setInitialState(_listIdleState);
    start();
    postEvent("refresh");
}

void LogDownloadStateMachine::startDownload(const QString& downloadPath)
{
    if (_downloading) {
        return;
    }

    _vehicle = _controller->_vehicle;
    if (!_vehicle) {
        emit operationFailed(tr("No vehicle connected"));
        return;
    }

    _downloadPath = downloadPath;
    _retries = 0;

    setInitialState(_downloadIdleState);
    start();
    postEvent("start_download");
}

void LogDownloadStateMachine::cancel()
{
    _requestLogEnd();
    postEvent("cancel");
}

void LogDownloadStateMachine::handleLogEntry(uint32_t time_utc, uint32_t size, uint16_t id, uint16_t num_logs)
{
    if (!_requestingList) {
        return;
    }

    QmlObjectListModel* model = _controller->_logEntriesModel;

    if ((model->count() == 0) && (num_logs > 0)) {
        if (_vehicle->firmwareType() == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            _apmOffset = 1;
        }

        for (int i = 0; i < num_logs; i++) {
            QGCLogEntry* entry = new QGCLogEntry(i);
            model->append(entry);
        }
    }

    if (num_logs > 0) {
        if ((size > 0) || (_vehicle->firmwareType() != MAV_AUTOPILOT_ARDUPILOTMEGA)) {
            id -= _apmOffset;
            if (id < model->count()) {
                QGCLogEntry* entry = model->value<QGCLogEntry*>(id);
                entry->setSize(size);
                entry->setTime(QDateTime::fromSecsSinceEpoch(time_utc));
                entry->setReceived(true);
                entry->setStatus(tr("Available"));
            } else {
                qCWarning(LogDownloadStateMachineLog) << "Received log entry for out-of-bound index:" << id;
            }
        }
    } else {
        postEvent("all_entries_received");
        return;
    }

    _retries = 0;

    if (_entriesComplete()) {
        postEvent("all_entries_received");
    } else {
        postEvent("entries_received");
    }
}

void LogDownloadStateMachine::handleLogData(uint32_t offset, uint16_t id, uint8_t count, const uint8_t* data)
{
    if (!_downloading || !_controller->_downloadData) {
        return;
    }

    LogDownloadData* downloadData = _controller->_downloadData.get();

    id -= _apmOffset;
    if (downloadData->ID != id) {
        qCWarning(LogDownloadStateMachineLog) << "Received log data for wrong log";
        return;
    }

    if ((offset % MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN) != 0) {
        qCWarning(LogDownloadStateMachineLog) << "Ignored misaligned incoming packet @" << offset;
        return;
    }

    bool result = false;
    if (offset <= downloadData->entry->size()) {
        const uint32_t chunk = offset / LogDownloadData::kChunkSize;
        if (chunk != downloadData->current_chunk) {
            qCWarning(LogDownloadStateMachineLog) << "Ignored packet for out of order chunk actual:expected"
                                                   << chunk << downloadData->current_chunk;
            return;
        }

        const uint16_t bin = (offset - (chunk * LogDownloadData::kChunkSize)) / MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN;
        if (bin >= downloadData->chunk_table.size()) {
            qCWarning(LogDownloadStateMachineLog) << "Out of range bin received";
        } else {
            downloadData->chunk_table.setBit(bin);
        }

        if (downloadData->file.pos() != offset) {
            if (!downloadData->file.seek(offset)) {
                qCWarning(LogDownloadStateMachineLog) << "Error while seeking log file offset";
                return;
            }
        }

        if (downloadData->file.write(reinterpret_cast<const char*>(data), count)) {
            downloadData->written += count;
            downloadData->rate_bytes += count;
            _controller->_updateDataRate();

            result = true;
            _retries = 0;

            if (_logComplete()) {
                downloadData->entry->setStatus(tr("Downloaded"));
                postEvent("log_complete");
            } else if (_chunkComplete()) {
                postEvent("chunk_complete");
            } else {
                postEvent("data_received");
            }
        } else {
            qCWarning(LogDownloadStateMachineLog) << "Error while writing log file chunk";
        }
    } else {
        qCWarning(LogDownloadStateMachineLog) << "Received log offset greater than expected";
    }

    if (!result) {
        downloadData->entry->setStatus(tr("Error"));
    }
}

void LogDownloadStateMachine::_setRequestingList(bool requesting)
{
    if (_requestingList != requesting) {
        _requestingList = requesting;
        if (_vehicle) {
            _vehicle->vehicleLinkManager()->setCommunicationLostEnabled(!requesting);
        }
        emit requestingListChanged();
    }
}

void LogDownloadStateMachine::_setDownloading(bool downloading)
{
    if (_downloading != downloading) {
        _downloading = downloading;
        if (_vehicle) {
            _vehicle->vehicleLinkManager()->setCommunicationLostEnabled(!downloading);
        }
        emit downloadingChanged();
    }
}

void LogDownloadStateMachine::_requestLogList(uint32_t start, uint32_t end)
{
    if (!_vehicle) {
        qCWarning(LogDownloadStateMachineLog) << "Vehicle Unavailable";
        return;
    }

    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCWarning(LogDownloadStateMachineLog) << "Link Unavailable";
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
        qCWarning(LogDownloadStateMachineLog) << "Failed to send";
        return;
    }

    qCDebug(LogDownloadStateMachineLog) << "Request log entry list (" << start << "through" << end << ")";
    _setRequestingList(true);
}

void LogDownloadStateMachine::_requestLogData(uint16_t id, uint32_t offset, uint32_t count)
{
    if (!_vehicle) {
        qCWarning(LogDownloadStateMachineLog) << "Vehicle Unavailable";
        return;
    }

    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCWarning(LogDownloadStateMachineLog) << "Link Unavailable";
        return;
    }

    id += _apmOffset;
    qCDebug(LogDownloadStateMachineLog) << "Request log data (id:" << id << "offset:" << offset
                                         << "size:" << count << "retries:" << _retries << ")";

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
        qCWarning(LogDownloadStateMachineLog) << "Failed to send";
    }

    _setDownloading(true);
}

void LogDownloadStateMachine::_requestLogEnd()
{
    if (!_vehicle) {
        qCWarning(LogDownloadStateMachineLog) << "Vehicle Unavailable";
        return;
    }

    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCWarning(LogDownloadStateMachineLog) << "Link Unavailable";
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
        qCWarning(LogDownloadStateMachineLog) << "Failed to send";
    }
}

void LogDownloadStateMachine::_findMissingEntries()
{
    QmlObjectListModel* model = _controller->_logEntriesModel;
    const int num_logs = model->count();
    int start = -1;
    int end = -1;

    for (int i = 0; i < num_logs; i++) {
        const QGCLogEntry* entry = model->value<const QGCLogEntry*>(i);
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
        postEvent("all_entries_received");
        return;
    }

    if (_retries++ > _maxRetries) {
        for (int i = 0; i < num_logs; i++) {
            QGCLogEntry* entry = model->value<QGCLogEntry*>(i);
            if (entry && !entry->received()) {
                entry->setStatus(tr("Error"));
            }
        }
        postEvent("list_failed");
        qCWarning(LogDownloadStateMachineLog) << "Too many errors retrieving log list. Giving up.";
        return;
    }

    if (end < 0) {
        end = start;
    }

    start += _apmOffset;
    end += _apmOffset;

    _requestLogList(static_cast<uint32_t>(start), static_cast<uint32_t>(end));
    postEvent("retry_list");
}

void LogDownloadStateMachine::_findMissingData()
{
    if (_logComplete()) {
        postEvent("log_complete");
        return;
    }

    if (_chunkComplete()) {
        postEvent("chunk_complete");
        return;
    }

    _retries++;

    _controller->_updateDataRate();

    LogDownloadData* downloadData = _controller->_downloadData.get();
    if (!downloadData) {
        postEvent("prepare_failed");
        return;
    }

    uint16_t start = 0, end = 0;
    const int size = downloadData->chunk_table.size();
    for (; start < size; start++) {
        if (!downloadData->chunk_table.testBit(start)) {
            break;
        }
    }

    for (end = start; end < size; end++) {
        if (downloadData->chunk_table.testBit(end)) {
            break;
        }
    }

    const uint32_t pos = (downloadData->current_chunk * LogDownloadData::kChunkSize) + (start * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN);
    const uint32_t len = (end - start) * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN;
    _requestLogData(downloadData->ID, pos, len);
    postEvent("gaps_found");
}

bool LogDownloadStateMachine::_entriesComplete() const
{
    QmlObjectListModel* model = _controller->_logEntriesModel;
    const int num_logs = model->count();
    for (int i = 0; i < num_logs; i++) {
        const QGCLogEntry* entry = model->value<const QGCLogEntry*>(i);
        if (!entry) {
            continue;
        }

        if (!entry->received()) {
            return false;
        }
    }

    return true;
}

bool LogDownloadStateMachine::_chunkComplete() const
{
    LogDownloadData* downloadData = _controller->_downloadData.get();
    return downloadData && downloadData->chunkEquals(true);
}

bool LogDownloadStateMachine::_logComplete() const
{
    LogDownloadData* downloadData = _controller->_downloadData.get();
    return downloadData && (_chunkComplete() && ((downloadData->current_chunk + 1) == downloadData->numChunks()));
}

void LogDownloadStateMachine::_prepareNextLog()
{
    _controller->_downloadData.reset();

    QGCLogEntry* entry = _controller->_getNextSelected();
    if (!entry) {
        postEvent("no_more_logs");
        return;
    }

    entry->setSelected(false);
    emit _controller->selectionChanged();

    const QString ftime = (entry->time().date().year() >= 2010)
        ? entry->time().toString(QStringLiteral("yyyy-M-d-hh-mm-ss"))
        : QStringLiteral("UnknownDate");

    _controller->_downloadData = std::make_unique<LogDownloadData>(entry);
    LogDownloadData* downloadData = _controller->_downloadData.get();
    downloadData->filename = QStringLiteral("log_") + QString::number(entry->id()) + "_" + ftime;

    if (_vehicle->firmwareType() == MAV_AUTOPILOT_PX4) {
        const QString loggerParam = QStringLiteral("SYS_LOGGER");
        ParameterManager* parameterManager = _vehicle->parameterManager();
        if (parameterManager->parameterExists(ParameterManager::defaultComponentId, loggerParam)
            && parameterManager->getParameter(ParameterManager::defaultComponentId, loggerParam)->rawValue().toInt() == 0) {
            downloadData->filename += ".px4log";
        } else {
            downloadData->filename += ".ulg";
        }
    } else {
        downloadData->filename += ".bin";
    }

    downloadData->file.setFileName(_downloadPath + downloadData->filename);

    if (downloadData->file.exists()) {
        uint32_t numDups = 0;
        const QStringList filename_spl = downloadData->filename.split('.');
        do {
            numDups += 1;
            const QString filename = filename_spl[0] + '_' + QString::number(numDups) + '.' + filename_spl[1];
            downloadData->file.setFileName(filename);
        } while (downloadData->file.exists());
    }

    bool result = false;
    if (!downloadData->file.open(QIODevice::WriteOnly)) {
        qCWarning(LogDownloadStateMachineLog) << "Failed to create log file:" << downloadData->filename;
    } else if (!downloadData->file.resize(entry->size())) {
        qCWarning(LogDownloadStateMachineLog) << "Failed to allocate space for log file:" << downloadData->filename;
    } else {
        downloadData->current_chunk = 0;
        downloadData->chunk_table = QBitArray(downloadData->chunkBins(), false);
        downloadData->elapsed.start();
        result = true;
    }

    if (!result) {
        if (downloadData->file.exists()) {
            (void) downloadData->file.remove();
        }
        downloadData->entry->setStatus(QStringLiteral("Error"));
        _controller->_downloadData.reset();
        postEvent("prepare_failed");
        return;
    }

    _requestLogData(downloadData->ID, 0, downloadData->chunk_table.size() * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN);
    postEvent("prepared");
}

void LogDownloadStateMachine::_advanceChunk()
{
    LogDownloadData* downloadData = _controller->_downloadData.get();
    if (!downloadData) {
        postEvent("prepare_failed");
        return;
    }

    downloadData->advanceChunk();
    _requestLogData(downloadData->ID,
                    downloadData->current_chunk * LogDownloadData::kChunkSize,
                    downloadData->chunk_table.size() * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN);
    postEvent("next_chunk_started");
}

void LogDownloadStateMachine::_finishCurrentLog()
{
    LogDownloadData* downloadData = _controller->_downloadData.get();
    if (downloadData) {
        downloadData->entry->setStatus(tr("Downloaded"));
    }

    QGCLogEntry* nextEntry = _controller->_getNextSelected();
    if (nextEntry) {
        nextEntry->setStatus(tr("Waiting"));
        postEvent("next_log");
    } else {
        postEvent("all_logs_complete");
    }
}

void LogDownloadStateMachine::_processTimeout()
{
    if (_requestingList) {
        _findMissingEntries();
    } else if (_downloading) {
        _findMissingData();
    }
}
