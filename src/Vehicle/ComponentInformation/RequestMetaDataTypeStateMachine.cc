#include "RequestMetaDataTypeStateMachine.h"
#include "ComponentInformationManager.h"
#include "ComponentInformationTranslation.h"
#include "ComponentInformationCache.h"
#include "Vehicle.h"
#include "FTPManager.h"
#include "QGCCompression.h"
#include "CompInfoGeneral.h"
#include "QGCApplication.h"
#include "QGCCachedFileDownload.h"
#include "QGCLoggingCategory.h"

// State types included via QGCStateMachine.h in header

#include <QtCore/QCoreApplication>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtCore/QFile>

QGC_LOGGING_CATEGORY(RequestMetaDataTypeStateMachineLog, "ComponentInformation.RequestMetaDataTypeStateMachine")

RequestMetaDataTypeStateMachine::RequestMetaDataTypeStateMachine(ComponentInformationManager* compMgr, QObject* parent)
    : QGCStateMachine("RequestMetaDataType", compMgr->vehicle(), parent)
    , _compMgr(compMgr)
{
    qCDebug(RequestMetaDataTypeStateMachineLog) << Q_FUNC_INFO << this;

    _createStates();
    _wireTransitions();
    _wireTimeoutHandling();
}

RequestMetaDataTypeStateMachine::~RequestMetaDataTypeStateMachine()
{
    qCDebug(RequestMetaDataTypeStateMachineLog) << this;
}

void RequestMetaDataTypeStateMachine::_createStates()
{
    // State 1: Request COMPONENT_METADATA message
    _stateRequestCompInfo = new AsyncFunctionState(
        "RequestCompMetadata",
        this,
        [this](AsyncFunctionState*) { _requestCompInfo(); },
        _timeoutCompInfoRequest
    );
    registerState(_stateRequestCompInfo);

    // State 2: Fallback to deprecated COMPONENT_INFORMATION if needed
    _stateRequestDeprecated = new SkippableAsyncState(
        "RequestCompInfoDeprecated",
        this,
        [this]() { return _shouldSkipDeprecatedRequest(); },
        [this](SkippableAsyncState*) { _requestCompInfoDeprecated(); },
        nullptr,
        _timeoutCompInfoRequest
    );
    registerState(_stateRequestDeprecated);

    // State 3: Download metadata JSON
    _stateRequestMetaDataJson = new AsyncFunctionState(
        "RequestMetaDataJson",
        this,
        [this](AsyncFunctionState*) { _requestMetaDataJson(); },
        _timeoutMetaDataDownload
    );
    registerState(_stateRequestMetaDataJson);

    // State 4: Try fallback URI if primary download failed
    _stateRequestMetaDataJsonFallback = new SkippableAsyncState(
        "RequestMetaDataJsonFallback",
        this,
        [this]() { return _shouldSkipFallback(); },
        [this](SkippableAsyncState*) { _requestMetaDataJsonFallback(); },
        nullptr,
        _timeoutMetaDataDownload
    );
    registerState(_stateRequestMetaDataJsonFallback);

    // State 5: Download translation JSON
    _stateRequestTranslationJson = new AsyncFunctionState(
        "RequestTranslationJson",
        this,
        [this](AsyncFunctionState*) { _requestTranslationJson(); },
        _timeoutMetaDataDownload
    );
    registerState(_stateRequestTranslationJson);

    // State 6: Translate metadata
    _stateRequestTranslate = new SkippableAsyncState(
        "RequestTranslate",
        this,
        [this]() { return _shouldSkipTranslation(); },
        [this](SkippableAsyncState*) { _requestTranslate(); },
        nullptr,
        _timeoutTranslation
    );
    registerState(_stateRequestTranslate);

    // State 7: Complete request
    // Use ErrorRecoveryState shape (without retries/fallback) so completion
    // stays in the unified recovery framework used by this utility.
    _stateComplete = addErrorRecoveryState(
        "CompleteRequest",
        [this]() {
            _completeRequest();
            return true;
        },
        0,
        0,
        ErrorRecoveryBuilder::EmitError
    );

    _stateFinal = addFinalState("Final");

    setInitialState(_stateRequestCompInfo);
}

void RequestMetaDataTypeStateMachine::_wireTransitions()
{
    // Use completed() for WaitStateBase-derived states (more semantic than advance())

    // RequestCompMetadata -> RequestDeprecated
    _stateRequestCompInfo->addTransition(_stateRequestCompInfo, &WaitStateBase::completed, _stateRequestDeprecated);

    // RequestDeprecated -> RequestMetaDataJson (either via completed or skipped)
    _stateRequestDeprecated->addTransition(_stateRequestDeprecated, &WaitStateBase::completed, _stateRequestMetaDataJson);
    _stateRequestDeprecated->addTransition(_stateRequestDeprecated, &SkippableAsyncState::skipped, _stateRequestMetaDataJson);

    // RequestMetaDataJson -> RequestMetaDataJsonFallback
    _stateRequestMetaDataJson->addTransition(_stateRequestMetaDataJson, &WaitStateBase::completed, _stateRequestMetaDataJsonFallback);

    // RequestMetaDataJsonFallback -> RequestTranslationJson
    _stateRequestMetaDataJsonFallback->addTransition(_stateRequestMetaDataJsonFallback, &WaitStateBase::completed, _stateRequestTranslationJson);
    _stateRequestMetaDataJsonFallback->addTransition(_stateRequestMetaDataJsonFallback, &SkippableAsyncState::skipped, _stateRequestTranslationJson);

    // RequestTranslationJson -> RequestTranslate
    _stateRequestTranslationJson->addTransition(_stateRequestTranslationJson, &WaitStateBase::completed, _stateRequestTranslate);

    // RequestTranslate -> CompleteRequest
    _stateRequestTranslate->addTransition(_stateRequestTranslate, &WaitStateBase::completed, _stateComplete);
    _stateRequestTranslate->addTransition(_stateRequestTranslate, &SkippableAsyncState::skipped, _stateComplete);

    // CompleteRequest -> Final (QGCState advance())
    _stateComplete->addTransition(_stateComplete, &QGCState::advance, _stateFinal);

    // Emit requestComplete when machine finishes (reaches final state and stops)
    connect(this, &QStateMachine::finished, this, &RequestMetaDataTypeStateMachine::requestComplete);
}

void RequestMetaDataTypeStateMachine::_wireTimeoutHandling()
{
    // On timeout, gracefully advance to next state (don't block the connection sequence)
    // Timeout indicates the operation failed, but we continue with degraded functionality

    _stateRequestCompInfo->addTransition(_stateRequestCompInfo, &WaitStateBase::timedOut, _stateRequestDeprecated);

    _stateRequestDeprecated->addTransition(_stateRequestDeprecated, &WaitStateBase::timedOut, _stateRequestMetaDataJson);

    _stateRequestMetaDataJson->addTransition(_stateRequestMetaDataJson, &WaitStateBase::timedOut, _stateRequestMetaDataJsonFallback);

    _stateRequestMetaDataJsonFallback->addTransition(_stateRequestMetaDataJsonFallback, &WaitStateBase::timedOut, _stateRequestTranslationJson);

    _stateRequestTranslationJson->addTransition(_stateRequestTranslationJson, &WaitStateBase::timedOut, _stateRequestTranslate);

    _stateRequestTranslate->addTransition(_stateRequestTranslate, &WaitStateBase::timedOut, _stateComplete);
}

void RequestMetaDataTypeStateMachine::request(CompInfo* compInfo)
{
    qCDebug(RequestMetaDataTypeStateMachineLog) << Q_FUNC_INFO << typeToString();

    _compInfo = compInfo;
    _jsonMetadataFileName.clear();
    _jsonMetadataTranslatedFileName.clear();
    _jsonTranslationFileName.clear();
    _activeAsyncState = nullptr;
    _activeSkippableState = nullptr;

    start();
}

QString RequestMetaDataTypeStateMachine::typeToString() const
{
    if (!_compInfo) return "Unknown";

    switch (_compInfo->type) {
    case COMP_METADATA_TYPE_GENERAL: return "COMP_METADATA_TYPE_GENERAL";
    case COMP_METADATA_TYPE_PARAMETER: return "COMP_METADATA_TYPE_PARAMETER";
    case COMP_METADATA_TYPE_COMMANDS: return "COMP_METADATA_TYPE_COMMANDS";
    case COMP_METADATA_TYPE_PERIPHERALS: return "COMP_METADATA_TYPE_PERIPHERALS";
    case COMP_METADATA_TYPE_EVENTS: return "COMP_METADATA_TYPE_EVENTS";
    case COMP_METADATA_TYPE_ACTUATORS: return "COMP_METADATA_TYPE_ACTUATORS";
    default: return "Unknown";
    }
}

bool RequestMetaDataTypeStateMachine::_shouldSkipCompInfoRequest() const
{
    return _compInfo->type != COMP_METADATA_TYPE_GENERAL;
}

bool RequestMetaDataTypeStateMachine::_shouldSkipDeprecatedRequest() const
{
    // Skip if not GENERAL type
    if (_compInfo->type != COMP_METADATA_TYPE_GENERAL) {
        return true;
    }
    // Skip if we already got valid CRC from COMPONENT_METADATA
    if (_compInfo->crcMetaDataValid()) {
        qCDebug(RequestMetaDataTypeStateMachineLog) << "COMPONENT_METADATA available, skipping COMPONENT_INFORMATION";
        return true;
    }
    return false;
}

bool RequestMetaDataTypeStateMachine::_shouldSkipFallback() const
{
    // Skip fallback if primary download succeeded
    return !_jsonMetadataFileName.isEmpty();
}

bool RequestMetaDataTypeStateMachine::_shouldSkipTranslation() const
{
    // Skip if no translation file was downloaded
    return _jsonTranslationFileName.isEmpty();
}

void RequestMetaDataTypeStateMachine::_requestCompInfo()
{
    if (_shouldSkipCompInfoRequest()) {
        _stateRequestCompInfo->complete();
        return;
    }

    Vehicle* vehicle = _compMgr->vehicle();
    SharedLinkInterfacePtr sharedLink = vehicle->vehicleLinkManager()->primaryLink().lock();

    if (!sharedLink) {
        qCDebug(RequestMetaDataTypeStateMachineLog) << "Skipping component information request due to no primary link" << typeToString();
        _stateRequestCompInfo->complete();
        return;
    }

    if (sharedLink->linkConfiguration()->isHighLatency() || sharedLink->isLogReplay()) {
        qCDebug(RequestMetaDataTypeStateMachineLog) << "Skipping component information request due to link type" << typeToString();
        _stateRequestCompInfo->complete();
        return;
    }

    qCDebug(RequestMetaDataTypeStateMachineLog) << "Requesting component metadata" << typeToString();

    vehicle->requestMessage(
        [](void* resultHandlerData, MAV_RESULT result, Vehicle::RequestMessageResultHandlerFailureCode_t, const mavlink_message_t& message) {
            auto* self = static_cast<RequestMetaDataTypeStateMachine*>(resultHandlerData);
            self->_handleCompMetadataResult(result, message);
        },
        this,
        MAV_COMP_ID_AUTOPILOT1,
        MAVLINK_MSG_ID_COMPONENT_METADATA
    );
}

void RequestMetaDataTypeStateMachine::_handleCompMetadataResult(MAV_RESULT result, const mavlink_message_t& message)
{
    if (result == MAV_RESULT_ACCEPTED) {
        mavlink_component_metadata_t componentMetadata;
        mavlink_msg_component_metadata_decode(&message, &componentMetadata);
        _compInfo->setUriMetaData(componentMetadata.uri, componentMetadata.file_crc);
    }
    // else: try deprecated COMPONENT_INFORMATION in next state

    _stateRequestCompInfo->complete();
}

void RequestMetaDataTypeStateMachine::_requestCompInfoDeprecated()
{
    Vehicle* vehicle = _compMgr->vehicle();
    SharedLinkInterfacePtr sharedLink = vehicle->vehicleLinkManager()->primaryLink().lock();

    if (!sharedLink) {
        qCDebug(RequestMetaDataTypeStateMachineLog) << "Skipping deprecated component information request due to no primary link" << typeToString();
        _stateRequestDeprecated->complete();
        return;
    }

    if (sharedLink->linkConfiguration()->isHighLatency() || sharedLink->isLogReplay()) {
        qCDebug(RequestMetaDataTypeStateMachineLog) << "Skipping deprecated component information request due to link type" << typeToString();
        _stateRequestDeprecated->complete();
        return;
    }

    qCDebug(RequestMetaDataTypeStateMachineLog) << "Requesting component information (deprecated)" << typeToString();

    vehicle->requestMessage(
        [](void* resultHandlerData, MAV_RESULT result, Vehicle::RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t& message) {
            auto* self = static_cast<RequestMetaDataTypeStateMachine*>(resultHandlerData);
            self->_handleCompInfoResult(result, failureCode, message);
        },
        this,
        MAV_COMP_ID_AUTOPILOT1,
        MAVLINK_MSG_ID_COMPONENT_INFORMATION
    );
}

void RequestMetaDataTypeStateMachine::_handleCompInfoResult(MAV_RESULT result, Vehicle::RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t& message)
{
    if (result == MAV_RESULT_ACCEPTED) {
        mavlink_component_information_t componentInformation;
        mavlink_msg_component_information_decode(&message, &componentInformation);
        _compInfo->setUriMetaData(componentInformation.general_metadata_uri, componentInformation.general_metadata_file_crc);
    } else {
        switch (failureCode) {
        case Vehicle::RequestMessageFailureCommandError:
            qCDebug(RequestMetaDataTypeStateMachineLog) << "MAV_CMD_REQUEST_MESSAGE COMPONENT_INFORMATION error:" << QGCMAVLink::mavResultToString(result) << typeToString();
            break;
        case Vehicle::RequestMessageFailureCommandNotAcked:
            qCDebug(RequestMetaDataTypeStateMachineLog) << "MAV_CMD_REQUEST_MESSAGE COMPONENT_INFORMATION no response from vehicle" << typeToString();
            break;
        case Vehicle::RequestMessageFailureMessageNotReceived:
            qCDebug(RequestMetaDataTypeStateMachineLog) << "MAV_CMD_REQUEST_MESSAGE COMPONENT_INFORMATION message not received" << typeToString();
            break;
        default:
            break;
        }
    }

    _stateRequestDeprecated->complete();
}

void RequestMetaDataTypeStateMachine::_requestMetaDataJson()
{
    CompInfo* compInfo = _compInfo;
    const QString fileTag = ComponentInformationManager::_getFileCacheTag(compInfo->type, compInfo->crcMetaData(), false);
    const QString uri = compInfo->uriMetaData();
    _jsonMetadataCrcValid = compInfo->crcMetaDataValid();

    _activeAsyncState = _stateRequestMetaDataJson;
    _activeSkippableState = nullptr;
    _requestFile(fileTag, compInfo->crcMetaDataValid(), uri, _jsonMetadataFileName);
}

void RequestMetaDataTypeStateMachine::_requestMetaDataJsonFallback()
{
    qCDebug(RequestMetaDataTypeStateMachineLog) << "Trying fallback download for" << typeToString();

    CompInfo* compInfo = _compInfo;
    const QString fileTag = ComponentInformationManager::_getFileCacheTag(compInfo->type, compInfo->crcMetaDataFallback(), false);
    const QString uri = compInfo->uriMetaDataFallback();
    _jsonMetadataCrcValid = compInfo->crcMetaDataFallbackValid();

    _activeAsyncState = nullptr;
    _activeSkippableState = _stateRequestMetaDataJsonFallback;
    _requestFile(fileTag, compInfo->crcMetaDataFallbackValid(), uri, _jsonMetadataFileName);
}

void RequestMetaDataTypeStateMachine::_requestTranslationJson()
{
    CompInfo* compInfo = _compInfo;
    const QString uri = compInfo->uriTranslation();

    _activeAsyncState = _stateRequestTranslationJson;
    _activeSkippableState = nullptr;
    _requestFile("", false, uri, _jsonTranslationFileName);
}

void RequestMetaDataTypeStateMachine::_requestTranslate()
{
    connect(_compMgr->translation(), &ComponentInformationTranslation::downloadComplete,
            this, &RequestMetaDataTypeStateMachine::_downloadAndTranslationComplete);

    if (!_compMgr->translation()->downloadAndTranslate(_jsonTranslationFileName,
                                                       _jsonMetadataFileName,
                                                       ComponentInformationManager::cachedFileMaxAgeSec)) {
        disconnect(_compMgr->translation(), &ComponentInformationTranslation::downloadComplete,
                   this, &RequestMetaDataTypeStateMachine::_downloadAndTranslationComplete);
        qCDebug(RequestMetaDataTypeStateMachineLog) << "downloadAndTranslate() failed";
        _stateRequestTranslate->complete();
    }
}

void RequestMetaDataTypeStateMachine::_downloadAndTranslationComplete(QString translatedJsonTempFile, QString errorMsg)
{
    disconnect(_compMgr->translation(), &ComponentInformationTranslation::downloadComplete,
               this, &RequestMetaDataTypeStateMachine::_downloadAndTranslationComplete);

    _jsonMetadataTranslatedFileName = translatedJsonTempFile;
    if (!errorMsg.isEmpty()) {
        qCWarning(RequestMetaDataTypeStateMachineLog) << "Metadata translation failed:" << errorMsg;
    }

    _stateRequestTranslate->complete();
}

void RequestMetaDataTypeStateMachine::_completeRequest()
{
    if (_jsonMetadataTranslatedFileName.isEmpty()) {
        _compInfo->setJson(_jsonMetadataFileName);
    } else {
        _compInfo->setJson(_jsonMetadataTranslatedFileName);
        QFile(_jsonMetadataTranslatedFileName).remove();
    }

    // If we don't have a CRC we didn't cache the file and need to delete it
    if (!_jsonMetadataCrcValid && !_jsonMetadataFileName.isEmpty()) {
        QFile(_jsonMetadataFileName).remove();
    }
    if (!_jsonMetadataCrcValid && !_jsonTranslationFileName.isEmpty()) {
        QFile(_jsonTranslationFileName).remove();
    }
}

void RequestMetaDataTypeStateMachine::_requestFile(const QString& cacheFileTag, bool crcValid, const QString& uri, QString& outputFileName)
{
    FTPManager* ftpManager = _compInfo->vehicle->ftpManager();
    _currentCacheFileTag = cacheFileTag;
    _currentFileName = &outputFileName;
    _currentFileValidCrc = crcValid;
    outputFileName.clear();

    auto completeCurrentState = [this]() {
        if (_activeAsyncState) {
            _activeAsyncState->complete();
        } else if (_activeSkippableState) {
            _activeSkippableState->complete();
        }
    };

    if (!_compInfo->available() || uri.isEmpty()) {
        qCDebug(RequestMetaDataTypeStateMachineLog) << "Skipping download. Component information not available for" << _currentCacheFileTag;
        completeCurrentState();
        return;
    }

    const QString cachedFile = crcValid ? _compMgr->fileCache().access(cacheFileTag) : "";

    if (!cachedFile.isEmpty()) {
        qCDebug(RequestMetaDataTypeStateMachineLog) << "Using cached file" << cachedFile;
        outputFileName = cachedFile;
        completeCurrentState();
        return;
    }

    qCDebug(RequestMetaDataTypeStateMachineLog) << "Downloading json" << uri;

    if (_uriIsMAVLinkFTP(uri)) {
        connect(ftpManager, &FTPManager::downloadComplete, this, &RequestMetaDataTypeStateMachine::_ftpDownloadComplete);
        if (ftpManager->download(MAV_COMP_ID_AUTOPILOT1, uri, QStandardPaths::writableLocation(QStandardPaths::TempLocation))) {
            _downloadStartTime.start();
            connect(ftpManager, &FTPManager::commandProgress, this, &RequestMetaDataTypeStateMachine::_ftpDownloadProgress);
        } else {
            qCWarning(RequestMetaDataTypeStateMachineLog) << "FTPManager::download returned failure";
            disconnect(ftpManager, &FTPManager::downloadComplete, this, &RequestMetaDataTypeStateMachine::_ftpDownloadComplete);
            completeCurrentState();
        }
    } else {
        connect(_compMgr->_cachedFileDownload, &QGCCachedFileDownload::finished,
                this, &RequestMetaDataTypeStateMachine::_httpDownloadComplete);
        if (_compMgr->_cachedFileDownload->download(uri, crcValid ? 0 : ComponentInformationManager::cachedFileMaxAgeSec)) {
            _downloadStartTime.start();
        } else {
            qCWarning(RequestMetaDataTypeStateMachineLog) << "QGCCachedFileDownload::download returned failure";
            disconnect(_compMgr->_cachedFileDownload, &QGCCachedFileDownload::finished,
                       this, &RequestMetaDataTypeStateMachine::_httpDownloadComplete);
            completeCurrentState();
        }
    }
}

QString RequestMetaDataTypeStateMachine::_downloadCompleteJsonWorker(const QString& fileName)
{
    const QString tempPath = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).absoluteFilePath(_currentCacheFileTag);
    QString outputFileName = QGCCompression::decompressIfNeeded(fileName, tempPath);
    if (outputFileName.isEmpty()) {
        qCWarning(RequestMetaDataTypeStateMachineLog) << "Inflate of compressed json failed" << _currentCacheFileTag;
    }

    if (_currentFileValidCrc) {
        // Cache the file (this will move/remove the temp file as well)
        outputFileName = _compMgr->fileCache().insert(_currentCacheFileTag, outputFileName);
    }
    return outputFileName;
}

void RequestMetaDataTypeStateMachine::_ftpDownloadComplete(const QString& fileName, const QString& errorMsg)
{
    qCDebug(RequestMetaDataTypeStateMachineLog) << "_ftpDownloadComplete fileName:errorMsg" << fileName << errorMsg;

    disconnect(_compInfo->vehicle->ftpManager(), &FTPManager::downloadComplete, this, &RequestMetaDataTypeStateMachine::_ftpDownloadComplete);
    disconnect(_compInfo->vehicle->ftpManager(), &FTPManager::commandProgress, this, &RequestMetaDataTypeStateMachine::_ftpDownloadProgress);

    if (errorMsg.isEmpty()) {
        if (_currentFileName) {
            *_currentFileName = _downloadCompleteJsonWorker(fileName);
        }
    } else if (qgcApp()->runningUnitTests()) {
        qCWarning(RequestMetaDataTypeStateMachineLog) << "_ftpDownloadComplete failed filename:errorMsg" << fileName << errorMsg;
    }

    if (_activeAsyncState) {
        _activeAsyncState->complete();
    } else if (_activeSkippableState) {
        _activeSkippableState->complete();
    }
}

void RequestMetaDataTypeStateMachine::_ftpDownloadProgress(float progress)
{
    int elapsedSec = _downloadStartTime.elapsed() / 1000;
    float totalDownloadTime = elapsedSec / progress;

    // Abort download if it's too slow (e.g. over telemetry link) and use the fallback
    const int maxDownloadTimeSec = 40;
    if (elapsedSec > 10 && progress < 0.5 && totalDownloadTime > maxDownloadTimeSec) {
        qCDebug(RequestMetaDataTypeStateMachineLog) << "Slow download, aborting. Total time (s):" << totalDownloadTime;
        _compInfo->vehicle->ftpManager()->cancelDownload();
    }
}

void RequestMetaDataTypeStateMachine::_httpDownloadComplete(bool success, const QString& localFile, const QString& errorMsg, bool fromCache)
{
    qCDebug(RequestMetaDataTypeStateMachineLog) << "_httpDownloadComplete success:localFile:errorMsg:fromCache"
                                                << success << localFile << errorMsg << fromCache;

    disconnect(_compMgr->_cachedFileDownload, &QGCCachedFileDownload::finished,
               this, &RequestMetaDataTypeStateMachine::_httpDownloadComplete);

    if (success && errorMsg.isEmpty()) {
        if (_currentFileName) {
            *_currentFileName = _downloadCompleteJsonWorker(localFile);
        }
    } else if (qgcApp()->runningUnitTests()) {
        qCWarning(RequestMetaDataTypeStateMachineLog) << "_httpDownloadComplete failed localFile:errorMsg:fromCache"
                                                      << localFile << errorMsg << fromCache;
    }

    if (_activeAsyncState) {
        _activeAsyncState->complete();
    } else if (_activeSkippableState) {
        _activeSkippableState->complete();
    }
}

bool RequestMetaDataTypeStateMachine::_uriIsMAVLinkFTP(const QString& uri)
{
    return uri.startsWith(QStringLiteral("%1://").arg(FTPManager::mavlinkFTPScheme), Qt::CaseInsensitive);
}
