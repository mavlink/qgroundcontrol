#include "ParameterLoadStateMachine.h"
#include "ParameterManager.h"
#include "FTPManager.h"
#include "MAVLinkProtocol.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

#include <QtCore/QStandardPaths>

Q_DECLARE_LOGGING_CATEGORY(ParameterManagerLog)
Q_DECLARE_LOGGING_CATEGORY(ParameterManagerVerbose1Log)

ParameterLoadStateMachine::ParameterLoadStateMachine(ParameterManager* paramManager, QObject* parent)
    : QGCStateMachine("ParameterLoadStateMachine", paramManager->vehicle(), parent)
    , _paramManager(paramManager)
    , _useFTP(paramManager->vehicle()->apmFirmware())
    , _disableAllRetries(paramManager->vehicle()->vehicleLinkManager()->primaryLink().lock() &&
                          paramManager->vehicle()->vehicleLinkManager()->primaryLink().lock()->isLogReplay())
{
    _initialRequestTimeoutTimer = new QTimer(this);
    _initialRequestTimeoutTimer->setSingleShot(true);
    _initialRequestTimeoutTimer->setInterval(_initialRequestTimeoutMs);
    connect(_initialRequestTimeoutTimer, &QTimer::timeout, this, &ParameterLoadStateMachine::_initialRequestTimeout);

    _waitingParamTimeoutTimer = new QTimer(this);
    _waitingParamTimeoutTimer->setSingleShot(true);
    _waitingParamTimeoutTimer->setInterval(_waitingParamTimeoutMs);
    if (!_disableAllRetries) {
        connect(_waitingParamTimeoutTimer, &QTimer::timeout, this, &ParameterLoadStateMachine::_waitingParamTimeout);
    }

    _buildStateMachine();
}

ParameterLoadStateMachine::~ParameterLoadStateMachine()
{
    _initialRequestTimeoutTimer->stop();
    _waitingParamTimeoutTimer->stop();
}

void ParameterLoadStateMachine::_buildStateMachine()
{
    // Create states
    _idleState = new QGCState("Idle", this);
    _ftpDownloadingState = new QGCState("FTPDownloading", this);
    _sendingRequestListState = new QGCState("SendingRequestList", this);
    _receivingParamsState = new QGCState("ReceivingParams", this);
    _batchRetryingState = new QGCState("BatchRetrying", this);
    _waitingDefaultComponentState = new QGCState("WaitingDefaultComponent", this);
    _completeState = new QGCFinalState("Complete", this);

    setInitialState(_idleState);

    // Idle state - entry point
    connect(_idleState, &QAbstractState::entered, this, &ParameterLoadStateMachine::_onIdleEntry);

    // FTP downloading state
    connect(_ftpDownloadingState, &QAbstractState::entered, this, &ParameterLoadStateMachine::_onFTPDownloadingEntry);

    // Sending request list state
    connect(_sendingRequestListState, &QAbstractState::entered, this, &ParameterLoadStateMachine::_onSendingRequestListEntry);

    // Receiving params state
    connect(_receivingParamsState, &QAbstractState::entered, this, &ParameterLoadStateMachine::_onReceivingParamsEntry);

    // Batch retrying state
    connect(_batchRetryingState, &QAbstractState::entered, this, &ParameterLoadStateMachine::_onBatchRetryingEntry);

    // Waiting for default component state
    connect(_waitingDefaultComponentState, &QAbstractState::entered, this, &ParameterLoadStateMachine::_onWaitingDefaultComponentEntry);

    // Complete state
    connect(_completeState, &QAbstractState::entered, this, &ParameterLoadStateMachine::_onCompleteEntry);

    // Set up transitions
    // Idle -> FTPDownloading (for ArduPilot with FTP)
    _idleState->addTransition(new MachineEventTransition("start_ftp", _ftpDownloadingState));

    // Idle -> SendingRequestList (conventional path)
    _idleState->addTransition(new MachineEventTransition("start_conventional", _sendingRequestListState));

    // Idle -> Complete (high-latency/log replay)
    _idleState->addTransition(new MachineEventTransition("complete_immediate", _completeState));

    // FTPDownloading -> Complete (FTP success with parsed file)
    _ftpDownloadingState->addTransition(new MachineEventTransition("ftp_success", _completeState));

    // FTPDownloading -> SendingRequestList (FTP failed, fallback)
    _ftpDownloadingState->addTransition(new MachineEventTransition("ftp_fallback", _sendingRequestListState));

    // FTPDownloading -> FTPDownloading (retry)
    _ftpDownloadingState->addTransition(new MachineEventTransition("ftp_retry", _ftpDownloadingState));

    // SendingRequestList -> ReceivingParams (first param received)
    _sendingRequestListState->addTransition(new MachineEventTransition("param_received", _receivingParamsState));

    // SendingRequestList -> SendingRequestList (timeout retry)
    _sendingRequestListState->addTransition(new MachineEventTransition("retry_request", _sendingRequestListState));

    // ReceivingParams -> BatchRetrying (timeout with missing params)
    _receivingParamsState->addTransition(new MachineEventTransition("start_batch_retry", _batchRetryingState));

    // ReceivingParams -> WaitingDefaultComponent (all params received but no default component)
    _receivingParamsState->addTransition(new MachineEventTransition("wait_default_component", _waitingDefaultComponentState));

    // ReceivingParams -> Complete (all params received including default component)
    _receivingParamsState->addTransition(new MachineEventTransition("all_received", _completeState));

    // BatchRetrying -> BatchRetrying (still missing params, re-request)
    _batchRetryingState->addTransition(new MachineEventTransition("continue_retry", _batchRetryingState));

    // BatchRetrying -> WaitingDefaultComponent (no more missing but no default component)
    _batchRetryingState->addTransition(new MachineEventTransition("wait_default_component", _waitingDefaultComponentState));

    // BatchRetrying -> Complete (all done)
    _batchRetryingState->addTransition(new MachineEventTransition("batch_complete", _completeState));

    // WaitingDefaultComponent -> Complete (got default component or timeout)
    _waitingDefaultComponentState->addTransition(new MachineEventTransition("default_component_done", _completeState));

    // Set up progress weights
    setProgressWeights({
        {_idleState, 0},
        {_ftpDownloadingState, 3},
        {_sendingRequestListState, 1},
        {_receivingParamsState, 5},
        {_batchRetryingState, 2},
        {_waitingDefaultComponentState, 1},
        {_completeState, 0}
    });
}

void ParameterLoadStateMachine::startLoad(uint8_t componentId)
{
    if (isRunning()) {
        qCDebug(ParameterManagerLog) << "ParameterLoadStateMachine: Already running";
        return;
    }

    _targetComponentId = componentId;
    _initialLoadComplete = false;
    _indexBatchQueueActive = false;
    _waitingForDefaultComponent = false;
    _initialRequestRetryCount = 0;
    _indexBatchQueue.clear();
    _prevWaitingReadParamIndexCount = 0;

    const SharedLinkInterfacePtr sharedLink = _paramManager->vehicle()->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCDebug(ParameterManagerLog) << "ParameterLoadStateMachine: No primary link";
        return;
    }

    start();

    // Check for high-latency or log replay - immediate completion
    if (sharedLink->linkConfiguration()->isHighLatency() || _disableAllRetries) {
        qCDebug(ParameterManagerLog) << "ParameterLoadStateMachine: High-latency or log replay, immediate completion";
        QTimer::singleShot(0, this, [this]() {
            postEvent("complete_immediate");
        });
        return;
    }

    // Determine which path to take
    QTimer::singleShot(0, this, [this]() {
        if (_useFTP && ((_targetComponentId == MAV_COMP_ID_ALL) || (_targetComponentId == MAV_COMP_ID_AUTOPILOT1))) {
            postEvent("start_ftp");
        } else {
            postEvent("start_conventional");
        }
    });
}

void ParameterLoadStateMachine::cancel()
{
    if (!isRunning()) {
        return;
    }

    _initialRequestTimeoutTimer->stop();
    _waitingParamTimeoutTimer->stop();
    stop();
}

bool ParameterLoadStateMachine::isLoading() const
{
    return isRunning() && !_initialLoadComplete;
}

void ParameterLoadStateMachine::setTestTimeouts(int initialRequestTimeoutMs, int waitingParamTimeoutMs)
{
    _initialRequestTimeoutMs = initialRequestTimeoutMs;
    _waitingParamTimeoutMs = waitingParamTimeoutMs;
    _initialRequestTimeoutTimer->setInterval(initialRequestTimeoutMs);
    _waitingParamTimeoutTimer->setInterval(waitingParamTimeoutMs);
    qCDebug(ParameterManagerLog) << "Test timeouts set: initial=" << initialRequestTimeoutMs << "ms, waiting=" << waitingParamTimeoutMs << "ms";
}

void ParameterLoadStateMachine::_onIdleEntry()
{
    qCDebug(ParameterManagerLog) << "ParameterLoadStateMachine: Idle state entered";
}

void ParameterLoadStateMachine::_onFTPDownloadingEntry()
{
    qCDebug(ParameterManagerLog) << "ParameterLoadStateMachine: FTPDownloading state entered";

    _initialRequestTimeoutTimer->start();

    FTPManager* ftpManager = _paramManager->vehicle()->ftpManager();
    connect(ftpManager, &FTPManager::downloadComplete, this, &ParameterLoadStateMachine::handleFTPComplete, Qt::UniqueConnection);
    connect(ftpManager, &FTPManager::commandProgress, this, &ParameterLoadStateMachine::handleFTPProgress, Qt::UniqueConnection);

    _waitingParamTimeoutTimer->stop();

    if (!ftpManager->download(MAV_COMP_ID_AUTOPILOT1,
                              QStringLiteral("@PARAM/param.pck"),
                              QStandardPaths::writableLocation(QStandardPaths::TempLocation),
                              QStringLiteral(""),
                              false /* No filesize check */)) {
        qCWarning(ParameterManagerLog) << "ParameterLoadStateMachine: FTPManager::download returned failure";
        disconnect(ftpManager, &FTPManager::downloadComplete, this, &ParameterLoadStateMachine::handleFTPComplete);
        disconnect(ftpManager, &FTPManager::commandProgress, this, &ParameterLoadStateMachine::handleFTPProgress);
        postEvent("ftp_fallback");
    }
}

void ParameterLoadStateMachine::_onSendingRequestListEntry()
{
    qCDebug(ParameterManagerLog) << "ParameterLoadStateMachine: SendingRequestList state entered";

    if (!_initialLoadComplete) {
        _initialRequestTimeoutTimer->start();
    }

    _sendParamRequestList(_targetComponentId);
}

void ParameterLoadStateMachine::_onReceivingParamsEntry()
{
    qCDebug(ParameterManagerLog) << "ParameterLoadStateMachine: ReceivingParams state entered";
    // Timer management happens in handleParamValue
}

void ParameterLoadStateMachine::_onBatchRetryingEntry()
{
    qCDebug(ParameterManagerLog) << "ParameterLoadStateMachine: BatchRetrying state entered";
    _indexBatchQueueActive = true;

    // Attempt to fill the batch queue and request missing parameters
    bool paramsRequested = _fillIndexBatchQueue(true /* waitingParamTimeout */);

    if (!paramsRequested) {
        // No params requested, check if we have default component
        if (!_hasDefaultComponentParams() && !_waitingForDefaultComponent) {
            postEvent("wait_default_component");
        } else {
            postEvent("batch_complete");
        }
    } else {
        _waitingParamTimeoutTimer->start();
    }
}

void ParameterLoadStateMachine::_onWaitingDefaultComponentEntry()
{
    qCDebug(ParameterManagerLog) << "ParameterLoadStateMachine: WaitingDefaultComponent state entered";
    _waitingForDefaultComponent = true;
    _waitingParamTimeoutTimer->start();
}

void ParameterLoadStateMachine::_onCompleteEntry()
{
    qCDebug(ParameterManagerLog) << "ParameterLoadStateMachine: Complete state entered";

    _initialRequestTimeoutTimer->stop();
    _waitingParamTimeoutTimer->stop();
    _initialLoadComplete = true;
    _waitingForDefaultComponent = false;

    // Determine if we're missing parameters
    bool missingParameters = false;
    const QMap<int, QList<int>>& failedMap = _paramManager->_failedReadParamIndexMap;
    for (const int componentId : failedMap.keys()) {
        if (!failedMap[componentId].isEmpty()) {
            missingParameters = true;
            break;
        }
    }

    // Check for high-latency special case
    const SharedLinkInterfacePtr sharedLink = _paramManager->vehicle()->vehicleLinkManager()->primaryLink().lock();
    if (sharedLink && (sharedLink->linkConfiguration()->isHighLatency() || _disableAllRetries)) {
        missingParameters = true;
    }

    emit loadComplete(true, missingParameters);
}

void ParameterLoadStateMachine::handleFTPComplete(const QString& fileName, const QString& errorMsg)
{
    FTPManager* ftpManager = _paramManager->vehicle()->ftpManager();
    disconnect(ftpManager, &FTPManager::downloadComplete, this, &ParameterLoadStateMachine::handleFTPComplete);
    disconnect(ftpManager, &FTPManager::commandProgress, this, &ParameterLoadStateMachine::handleFTPProgress);

    bool continueWithDefaultDownload = true;
    bool immediateRetry = false;

    if (errorMsg.isEmpty()) {
        qCDebug(ParameterManagerLog) << "ParameterLoadStateMachine: FTP download complete:" << fileName;
        if (_paramManager->_parseParamFile(fileName)) {
            qCDebug(ParameterManagerLog) << "ParameterLoadStateMachine: Param file parsed successfully";
            postEvent("ftp_success");
            return;
        } else {
            qCDebug(ParameterManagerLog) << "ParameterLoadStateMachine: Error parsing param file";
        }
    } else if (errorMsg.contains("File Not Found")) {
        qCDebug(ParameterManagerLog) << "ParameterLoadStateMachine: No param file on vehicle, fallback to conventional";
        if (_initialRequestRetryCount == 0) {
            immediateRetry = true;
        }
    } else if ((_loadProgress > 0.0001) && (_loadProgress < 0.01)) {
        qCDebug(ParameterManagerLog) << "ParameterLoadStateMachine: FTP progress too slow, fallback";
    } else if (_initialRequestRetryCount == 1) {
        qCDebug(ParameterManagerLog) << "ParameterLoadStateMachine: Too many FTP retries, fallback";
    } else {
        qCDebug(ParameterManagerLog) << "ParameterLoadStateMachine: FTP retry:" << _initialRequestRetryCount;
        continueWithDefaultDownload = false;
    }

    if (continueWithDefaultDownload) {
        _useFTP = false;
        _initialRequestRetryCount = 0;
        if (immediateRetry) {
            postEvent("ftp_fallback");
        } else {
            // Let timeout trigger fallback
            _initialRequestTimeoutTimer->start();
        }
    } else {
        // Retry FTP
        _initialRequestTimeoutTimer->start();
    }
}

void ParameterLoadStateMachine::handleFTPProgress(float progress)
{
    qCDebug(ParameterManagerVerbose1Log) << "ParameterLoadStateMachine: FTP progress:" << progress;
    _setLoadProgress(static_cast<double>(progress));
    if (progress > 0.001) {
        _initialRequestTimeoutTimer->stop();
    }
}

void ParameterLoadStateMachine::handleParamValue(int componentId, const QString& parameterName,
                                                   int parameterCount, int parameterIndex,
                                                   MAV_PARAM_TYPE mavParamType, const QVariant& parameterValue)
{
    Q_UNUSED(parameterName)
    Q_UNUSED(mavParamType)
    Q_UNUSED(parameterValue)

    // Stop timers since we received a parameter
    _initialRequestTimeoutTimer->stop();
    _waitingParamTimeoutTimer->stop();

    // Transition from SendingRequestList to ReceivingParams if this is the first param
    if (isRunning() && configuration().contains(_sendingRequestListState)) {
        postEvent("param_received");
    }

    // Update param count tracking in ParameterManager
    QMap<int, int>& paramCountMap = _paramManager->_paramCountMap;
    int& totalParamCount = _paramManager->_totalParamCount;

    if (!paramCountMap.contains(componentId)) {
        paramCountMap[componentId] = parameterCount;
        totalParamCount += parameterCount;
    }

    // Update waiting list
    QMap<int, QMap<int, int>>& waitingMap = _paramManager->_waitingReadParamIndexMap;
    if (!waitingMap.contains(componentId)) {
        for (int i = 0; i < parameterCount; i++) {
            waitingMap[componentId][i] = 0;
        }
        qCDebug(ParameterManagerLog) << "Seeing component for first time - paramcount:" << parameterCount;
    }

    // Remove received param from waiting list
    if (waitingMap[componentId].contains(parameterIndex)) {
        waitingMap[componentId].remove(parameterIndex);
        _indexBatchQueue.removeOne(parameterIndex);
        _fillIndexBatchQueue(false /* waitingParamTimeout */);
    }

    // Count remaining params
    int waitingReadParamIndexCount = 0;
    for (const int cid : waitingMap.keys()) {
        waitingReadParamIndexCount += waitingMap[cid].count();
    }

    // Update progress
    _updateProgress();

    // Determine if we need to keep waiting
    if (waitingReadParamIndexCount > 0) {
        _waitingParamTimeoutTimer->start();
    } else if (!_paramManager->_mapCompId2FactMap.contains(_paramManager->vehicle()->defaultComponentId())) {
        qCDebug(ParameterManagerLog) << "Restarting wait timer - still waiting for default component";
        _waitingParamTimeoutTimer->start();
    } else {
        // All params received with default component
        _checkInitialLoadComplete();
    }

    _prevWaitingReadParamIndexCount = waitingReadParamIndexCount;
}

void ParameterLoadStateMachine::_waitingParamTimeout()
{
    if (_disableAllRetries) {
        return;
    }

    qCDebug(ParameterManagerLog) << "ParameterLoadStateMachine: _waitingParamTimeout";

    // Activate batch queue for index-based retries
    _indexBatchQueueActive = true;

    bool paramsRequested = _fillIndexBatchQueue(true /* waitingParamTimeout */);

    if (!paramsRequested && !_waitingForDefaultComponent && !_hasDefaultComponentParams()) {
        // No params to request but still missing default component
        qCDebug(ParameterManagerLog) << "Waiting for default component params";
        _waitingParamTimeoutTimer->start();
        _waitingForDefaultComponent = true;
        return;
    }
    _waitingForDefaultComponent = false;

    _checkInitialLoadComplete();

    if (paramsRequested) {
        _waitingParamTimeoutTimer->start();
    }
}

void ParameterLoadStateMachine::_initialRequestTimeout()
{
    if (_disableAllRetries) {
        // Log replay: signal completion
        qCDebug(ParameterManagerLog) << "ParameterLoadStateMachine: Initial timeout (log replay) - completing";
        _initialLoadComplete = true;
        emit loadComplete(true, false);
        return;
    }

    if (++_initialRequestRetryCount <= _maxInitialRequestListRetry) {
        qCDebug(ParameterManagerLog) << "ParameterLoadStateMachine: Retrying initial request, attempt:" << _initialRequestRetryCount;

        if (configuration().contains(_ftpDownloadingState)) {
            // Retry FTP or fallback
            FTPManager* ftpManager = _paramManager->vehicle()->ftpManager();
            connect(ftpManager, &FTPManager::downloadComplete, this, &ParameterLoadStateMachine::handleFTPComplete, Qt::UniqueConnection);
            connect(ftpManager, &FTPManager::commandProgress, this, &ParameterLoadStateMachine::handleFTPProgress, Qt::UniqueConnection);

            if (!ftpManager->download(MAV_COMP_ID_AUTOPILOT1,
                                      QStringLiteral("@PARAM/param.pck"),
                                      QStandardPaths::writableLocation(QStandardPaths::TempLocation),
                                      QStringLiteral(""),
                                      false)) {
                disconnect(ftpManager, &FTPManager::downloadComplete, this, &ParameterLoadStateMachine::handleFTPComplete);
                disconnect(ftpManager, &FTPManager::commandProgress, this, &ParameterLoadStateMachine::handleFTPProgress);
                _useFTP = false;
                postEvent("ftp_fallback");
            }
        } else {
            // Retry conventional request
            _sendParamRequestList(_targetComponentId);
        }
        _initialRequestTimeoutTimer->start();
    } else {
        // Max retries exceeded
        if (!_paramManager->vehicle()->genericFirmware()) {
            qCWarning(ParameterManagerLog) << "Vehicle did not respond to parameter request";
        }
        _initialLoadComplete = true;
        emit loadComplete(false, true);
    }
}

bool ParameterLoadStateMachine::_fillIndexBatchQueue(bool waitingParamTimeout)
{
    if (!_indexBatchQueueActive) {
        return false;
    }

    if (waitingParamTimeout) {
        qCDebug(ParameterManagerLog) << "Refilling batch queue due to timeout";
        _indexBatchQueue.clear();
    } else {
        qCDebug(ParameterManagerLog) << "Refilling batch queue due to received parameter";
    }

    QMap<int, QMap<int, int>>& waitingMap = _paramManager->_waitingReadParamIndexMap;
    QMap<int, QList<int>>& failedMap = _paramManager->_failedReadParamIndexMap;

    for (const int componentId : waitingMap.keys()) {
        if (waitingMap[componentId].count()) {
            qCDebug(ParameterManagerLog) << "Component" << componentId << "waiting count:" << waitingMap[componentId].count();
        }

        for (const int paramIndex : waitingMap[componentId].keys()) {
            if (_indexBatchQueue.contains(paramIndex)) {
                continue;
            }

            if (_indexBatchQueue.count() > _maxBatchSize) {
                break;
            }

            waitingMap[componentId][paramIndex]++;  // Bump retry count

            if (_disableAllRetries || (waitingMap[componentId][paramIndex] > _maxInitialLoadRetrySingleParam)) {
                // Give up on this index
                failedMap[componentId] << paramIndex;
                qCDebug(ParameterManagerLog) << "Giving up on paramIndex:" << paramIndex << "retryCount:" << waitingMap[componentId][paramIndex];
                waitingMap[componentId].remove(paramIndex);
            } else {
                // Retry again
                _indexBatchQueue.append(paramIndex);
                _paramManager->_mavlinkParamRequestRead(componentId, QString(), paramIndex, false /* notifyFailure */);
                qCDebug(ParameterManagerLog) << "Re-requesting paramIndex:" << paramIndex << "retryCount:" << waitingMap[componentId][paramIndex];
            }
        }
    }

    return !_indexBatchQueue.isEmpty();
}

void ParameterLoadStateMachine::_updateProgress()
{
    int waitingReadParamIndexCount = 0;

    const QMap<int, QMap<int, int>>& waitingMap = _paramManager->_waitingReadParamIndexMap;
    for (const int compId : waitingMap.keys()) {
        waitingReadParamIndexCount += waitingMap[compId].count();
    }

    if (waitingReadParamIndexCount == 0) {
        if (_readParamIndexProgressActive) {
            _readParamIndexProgressActive = false;
            _setLoadProgress(0.0);
            return;
        }
    } else {
        _readParamIndexProgressActive = true;
        int totalParamCount = _paramManager->_totalParamCount;
        if (totalParamCount > 0) {
            _setLoadProgress(static_cast<double>(totalParamCount - waitingReadParamIndexCount) / static_cast<double>(totalParamCount));
        }
    }
}

bool ParameterLoadStateMachine::_allParamsReceived() const
{
    const QMap<int, QMap<int, int>>& waitingMap = _paramManager->_waitingReadParamIndexMap;
    for (const int componentId : waitingMap.keys()) {
        if (!waitingMap[componentId].isEmpty()) {
            return false;
        }
    }
    return true;
}

bool ParameterLoadStateMachine::_hasDefaultComponentParams() const
{
    return _paramManager->_mapCompId2FactMap.contains(_paramManager->vehicle()->defaultComponentId());
}

void ParameterLoadStateMachine::_sendParamRequestList(uint8_t componentId)
{
    const SharedLinkInterfacePtr sharedLink = _paramManager->vehicle()->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        return;
    }

    // Reset index wait lists for conventional path
    QMap<int, int>& paramCountMap = _paramManager->_paramCountMap;
    QMap<int, QMap<int, int>>& waitingMap = _paramManager->_waitingReadParamIndexMap;

    for (int cid : paramCountMap.keys()) {
        if ((componentId != MAV_COMP_ID_ALL) && (componentId != cid)) {
            continue;
        }
        for (int waitingIndex = 0; waitingIndex < paramCountMap[cid]; waitingIndex++) {
            waitingMap[cid][waitingIndex] = 0;
        }
    }

    mavlink_message_t msg{};
    mavlink_msg_param_request_list_pack_chan(MAVLinkProtocol::instance()->getSystemId(),
                                             MAVLinkProtocol::getComponentId(),
                                             sharedLink->mavlinkChannel(),
                                             &msg,
                                             _paramManager->vehicle()->id(),
                                             componentId);
    _paramManager->vehicle()->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);

    qCDebug(ParameterManagerLog) << "Request to refresh all parameters for component:" << componentId;
}

void ParameterLoadStateMachine::_checkInitialLoadComplete()
{
    if (_initialLoadComplete) {
        return;
    }

    // Check if we're still waiting on params
    if (!_allParamsReceived()) {
        return;
    }

    // Check if we have default component params
    if (!_hasDefaultComponentParams()) {
        return;
    }

    _initialLoadComplete = true;
    qCDebug(ParameterManagerLog) << "Initial load complete";

    // Check for failures
    bool missingParameters = false;
    const QMap<int, QList<int>>& failedMap = _paramManager->_failedReadParamIndexMap;
    for (const int componentId : failedMap.keys()) {
        if (!failedMap[componentId].isEmpty()) {
            missingParameters = true;
            break;
        }
    }

    emit loadComplete(true, missingParameters);
}

void ParameterLoadStateMachine::_setLoadProgress(double progress)
{
    if (_loadProgress != progress) {
        _loadProgress = progress;
        emit progressChanged(progress);
    }
}
