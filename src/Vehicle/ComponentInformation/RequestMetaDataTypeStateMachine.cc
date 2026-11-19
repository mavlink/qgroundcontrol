#include "RequestMetaDataTypeStateMachine.h"
#include "ComponentInformationManager.h"
#include "ComponentInformationTranslation.h"
#include "ComponentInformationCache.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"
#include "FTPManager.h"
#include "QGCCompression.h"
#include "CompInfoGeneral.h"
#include "QGCCachedFileDownload.h"
#include "QGCLoggingCategory.h"

// State types included via QGCStateMachine.h in header

#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QStandardPaths>

QGC_LOGGING_CATEGORY(RequestMetaDataTypeStateMachineLog, "ComponentInformation.RequestMetaDataTypeStateMachine")

namespace {

/// @return The URI in a MAVLink char field. A URI that fills its field has no terminator.
template <size_t Length>
QString metadataUriFromField(const char (&field)[Length])
{
    return QString::fromUtf8(field, static_cast<qsizetype>(qstrnlen(field, Length)));
}

}  // namespace

RequestMetaDataTypeStateMachine::RequestMetaDataTypeStateMachine(ComponentInformationManager* compMgr, QObject* parent,
                                                                 const Timeouts& timeouts)
    : QGCStateMachine("RequestMetaDataType", compMgr->vehicle(), parent)
    , _compMgr(compMgr)
{
    qCDebug(RequestMetaDataTypeStateMachineLog) << Q_FUNC_INFO << this;

    _createStates(timeouts);
    _wireTransitions();
    _wireTimeoutHandling();
}

RequestMetaDataTypeStateMachine::~RequestMetaDataTypeStateMachine()
{
    qCDebug(RequestMetaDataTypeStateMachineLog) << this;
}

void RequestMetaDataTypeStateMachine::_createStates(const Timeouts& timeouts)
{
    // State 1: Request COMPONENT_METADATA message
    _stateRequestCompInfo = new AsyncFunctionState(
        "RequestCompMetadata", this, [this](AsyncFunctionState*) { _requestCompInfo(); }, timeouts.compInfoRequest);

    // State 2: Fallback to deprecated COMPONENT_INFORMATION if needed
    _stateRequestDeprecated = new SkippableAsyncState(
        "RequestCompInfoDeprecated", this, [this]() { return _shouldSkipDeprecatedRequest(); },
        [this](SkippableAsyncState*) { _requestCompInfoDeprecated(); }, nullptr, timeouts.compInfoRequest);

    // State 3: Download metadata JSON
    _stateRequestMetaDataJson = new AsyncFunctionState(
        "RequestMetaDataJson", this, [this](AsyncFunctionState*) { _requestMetaDataJson(); },
        timeouts.metaDataDownload);

    // State 4: Try fallback URI if primary download failed
    _stateRequestMetaDataJsonFallback = new SkippableAsyncState(
        "RequestMetaDataJsonFallback", this, [this]() { return _shouldSkipFallback(); },
        [this](SkippableAsyncState*) { _requestMetaDataJsonFallback(); }, nullptr, timeouts.metaDataDownload);

    // State 5: Download translation JSON
    _stateRequestTranslationJson = new AsyncFunctionState(
        "RequestTranslationJson", this, [this](AsyncFunctionState*) { _requestTranslationJson(); },
        timeouts.metaDataDownload);

    // State 6: Translate metadata
    _stateRequestTranslate = new SkippableAsyncState(
        "RequestTranslate", this, [this]() { return _shouldSkipTranslation(); },
        [this](SkippableAsyncState*) { _requestTranslate(); }, nullptr, timeouts.translation);

    // A state that times out is left while what it asked for is still under way. Nothing of that may reach a
    // later state, or the next request this machine makes.
    _stateRequestCompInfo->setOnExit([this]() { _cancelMessageRequest(); });
    _stateRequestDeprecated->setOnExit([this]() { _cancelMessageRequest(); });
    _stateRequestMetaDataJson->setOnExit([this]() { _cancelTransfer(); });
    _stateRequestMetaDataJsonFallback->setOnExit([this]() { _cancelTransfer(); });
    _stateRequestTranslationJson->setOnExit([this]() { _cancelTransfer(); });
    _stateRequestTranslate->setOnExit([this]() { _cancelTransfer(); });

    // State 7: Complete request
    _stateComplete = new FunctionState("CompleteRequest", this, [this]() { _completeRequest(); });

    _stateFinal = new QGCFinalState("Final", this);

    setInitialState(_stateRequestCompInfo);
}

void RequestMetaDataTypeStateMachine::_wireTransitions()
{
    // RequestCompMetadata -> RequestDeprecated
    _stateRequestCompInfo->addTransition(_stateRequestCompInfo, &QGCState::advance, _stateRequestDeprecated);

    // RequestDeprecated -> RequestMetaDataJson
    _stateRequestDeprecated->addTransition(_stateRequestDeprecated, &QGCState::advance, _stateRequestMetaDataJson);

    // RequestMetaDataJson -> RequestMetaDataJsonFallback
    _stateRequestMetaDataJson->addTransition(_stateRequestMetaDataJson, &QGCState::advance,
                                             _stateRequestMetaDataJsonFallback);

    // RequestMetaDataJsonFallback -> RequestTranslationJson
    _stateRequestMetaDataJsonFallback->addTransition(_stateRequestMetaDataJsonFallback, &QGCState::advance,
                                                     _stateRequestTranslationJson);

    // RequestTranslationJson -> RequestTranslate
    _stateRequestTranslationJson->addTransition(_stateRequestTranslationJson, &QGCState::advance,
                                                _stateRequestTranslate);

    // RequestTranslate -> CompleteRequest
    _stateRequestTranslate->addTransition(_stateRequestTranslate, &QGCState::advance, _stateComplete);

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
    _compInfo = compInfo;
    qCDebug(RequestMetaDataTypeStateMachineLog) << Q_FUNC_INFO << typeToString();

    _jsonMetadataFileName.clear();
    _jsonMetadataTranslatedFileName.clear();
    _jsonTranslationFileName.clear();
    _metadataSource = MetadataSource::None;
    _metadataUri.clear();
    _metadataIsFallback = false;

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
        _compInfo->setUriMetaData(metadataUriFromField(componentMetadata.uri), componentMetadata.file_crc);
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

void RequestMetaDataTypeStateMachine::_handleCompInfoResult(MAV_RESULT result, VehicleTypes::RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t& message)
{
    if (result == MAV_RESULT_ACCEPTED) {
        mavlink_component_information_t componentInformation;
        mavlink_msg_component_information_decode(&message, &componentInformation);
        _compInfo->setUriMetaData(metadataUriFromField(componentInformation.general_metadata_uri),
                                  componentInformation.general_metadata_file_crc);
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

    qCDebug(RequestMetaDataTypeStateMachineLog) << typeToString() << ": requesting metadata (primary) from" << uri;

    _requestFile(_stateRequestMetaDataJson, fileTag, compInfo->crcMetaDataValid(), uri, _jsonMetadataFileName, true);
}

void RequestMetaDataTypeStateMachine::_requestMetaDataJsonFallback()
{
    qCDebug(RequestMetaDataTypeStateMachineLog) << typeToString() << ": primary failed, requesting metadata (fallback) from" << _compInfo->uriMetaDataFallback();
    _metadataIsFallback = true;

    CompInfo* compInfo = _compInfo;
    const QString fileTag = ComponentInformationManager::_getFileCacheTag(compInfo->type, compInfo->crcMetaDataFallback(), false);
    const QString uri = compInfo->uriMetaDataFallback();
    _jsonMetadataCrcValid = compInfo->crcMetaDataFallbackValid();

    _requestFile(_stateRequestMetaDataJsonFallback, fileTag, compInfo->crcMetaDataFallbackValid(), uri,
                 _jsonMetadataFileName, true);
}

void RequestMetaDataTypeStateMachine::_requestTranslationJson()
{
    CompInfo* compInfo = _compInfo;
    const QString uri = compInfo->uriTranslation();

    if (uri.isEmpty()) {
        qCDebug(RequestMetaDataTypeStateMachineLog) << "No translation URI for" << typeToString();
        _stateRequestTranslationJson->complete();
        return;
    }

    _requestFile(_stateRequestTranslationJson, "", false, uri, _jsonTranslationFileName, false);
}

void RequestMetaDataTypeStateMachine::_requestTranslate()
{
    _stateRequestTranslate->connectToCompletion(
        _compMgr->translation(), &ComponentInformationTranslation::downloadComplete,
        [this](const QString& translatedJsonTempFile, const QString& errorMsg) {
            _transferInFlight = Transfer::None;
            _jsonMetadataTranslatedFileName = translatedJsonTempFile;
            if (!errorMsg.isEmpty()) {
                qCWarning(RequestMetaDataTypeStateMachineLog) << "Metadata translation failed:" << errorMsg;
            }

            _stateRequestTranslate->complete();
        });

    // The translation file comes through the downloader that the metadata files use
    _transferInFlight = Transfer::Http;
    if (!_compMgr->translation()->downloadAndTranslate(_jsonTranslationFileName,
                                                       _jsonMetadataFileName,
                                                       ComponentInformationManager::cachedFileMaxAgeSec,
                                                       typeToString())) {
        _transferInFlight = Transfer::None;
        qCDebug(RequestMetaDataTypeStateMachineLog) << typeToString() << ": translation skipped (English locale, locale unavailable, or download failure), using untranslated metadata";
        _stateRequestTranslate->complete();
    }
}

void RequestMetaDataTypeStateMachine::_completeRequest()
{
    const bool success = !_jsonMetadataFileName.isEmpty();
    const bool translated = !_jsonMetadataTranslatedFileName.isEmpty();

    if (translated) {
        _compInfo->setJson(_jsonMetadataTranslatedFileName);
        QFile(_jsonMetadataTranslatedFileName).remove();
    } else {
        _compInfo->setJson(_jsonMetadataFileName);
    }

    // If we don't have a CRC we didn't cache the file and need to delete it
    if (!_jsonMetadataCrcValid && !_jsonMetadataFileName.isEmpty()) {
        QFile(_jsonMetadataFileName).remove();
    }
    if (!_jsonMetadataCrcValid && !_jsonTranslationFileName.isEmpty()) {
        QFile(_jsonTranslationFileName).remove();
    }

    // Summary log for easy filtering
    const char* sourceLabel = _metadataIsFallback ? "(fallback)" : "(primary)";
    if (success) {
        if (translated) {
            qCDebug(RequestMetaDataTypeStateMachineLog) << typeToString() << ":" << _metadataSourceToString(_metadataSource)
                                                    << sourceLabel << "(translated)" << _metadataUri;
        } else {
            qCDebug(RequestMetaDataTypeStateMachineLog) << typeToString() << ":" << _metadataSourceToString(_metadataSource)
                                                    << sourceLabel << _metadataUri;
        }
    } else {
        const QString failureMsg = QStringLiteral("%1 : failed to load metadata (primary and fallback) %2")
                .arg(typeToString(), _metadataUri.isEmpty() ? _compInfo->uriMetaData() : _metadataUri);
        if (_compInfo->vehicle->apmFirmware()) {
            // ArduPilot doesn't support the component metadata protocol, so failure is expected
            qCDebug(RequestMetaDataTypeStateMachineLog).noquote() << failureMsg;
        } else {
            qCWarning(RequestMetaDataTypeStateMachineLog).noquote() << failureMsg;
        }
    }
}

const char* RequestMetaDataTypeStateMachine::_metadataSourceToString(MetadataSource source)
{
    switch (source) {
    case MetadataSource::Cache: return "loaded from cache";
    case MetadataSource::FTP:   return "downloaded via FTP";
    case MetadataSource::HTTP:  return "downloaded via HTTP";
    case MetadataSource::None:  return "not available";
    }
    return "unknown";
}

void RequestMetaDataTypeStateMachine::_requestFile(CompletionWaitState* state, const QString& cacheFileTag,
                                                   bool crcValid, const QString& uri, QString& outputFileName,
                                                   bool trackMetadataSource)
{
    outputFileName.clear();

    if (!_compInfo->available() || uri.isEmpty()) {
        qCDebug(RequestMetaDataTypeStateMachineLog) << typeToString() << ": metadata not available, skipping download";
        state->complete();
        return;
    }

    const QString cachedFile = crcValid ? _compMgr->fileCache().access(cacheFileTag) : "";

    if (!cachedFile.isEmpty()) {
        qCDebug(RequestMetaDataTypeStateMachineLog) << "Using cached file" << cachedFile;
        if (trackMetadataSource) {
            _metadataSource = MetadataSource::Cache;
            _metadataUri = uri;
        }
        outputFileName = cachedFile;
        state->complete();
        return;
    }

    if (!crcValid) {
        qCDebug(RequestMetaDataTypeStateMachineLog) << typeToString() << ": CRC not available, cache bypassed";
    } else {
        qCDebug(RequestMetaDataTypeStateMachineLog) << typeToString() << ": not found in cache, downloading";
    }

    qCDebug(RequestMetaDataTypeStateMachineLog) << typeToString() << ": downloading json" << uri;

    // The download belongs to the state that waits for it. Its completion is connected through the state, so the
    // connection ends with the wait, and a download that is still running when the state is left is cancelled
    // (_cancelTransfer()): one that outlived its state completed whichever state was current by then.
    QString* const output = &outputFileName;
    if (_uriIsMAVLinkFTP(uri)) {
        if (trackMetadataSource) {
            _metadataSource = MetadataSource::FTP;
            _metadataUri = uri;
        }
        FTPManager* const ftpManager = _compInfo->vehicle->ftpManager();
        state->connectToCompletion(
            ftpManager, &FTPManager::downloadComplete,
            [this, state, output, cacheFileTag, crcValid](const QString& fileName, const QString& errorMsg) {
                qCDebug(RequestMetaDataTypeStateMachineLog)
                    << "FTP download complete fileName:errorMsg" << fileName << errorMsg;
                _transferInFlight = Transfer::None;
                if (errorMsg.isEmpty()) {
                    *output = _downloadCompleteJsonWorker(fileName, cacheFileTag, crcValid);
                } else {
                    qCDebug(RequestMetaDataTypeStateMachineLog)
                        << typeToString() << ": FTP download failed:" << errorMsg;
                }
                state->complete();
            });
        _transferInFlight = Transfer::Ftp;
        if (ftpManager->download(MAV_COMP_ID_AUTOPILOT1, uri, QStandardPaths::writableLocation(QStandardPaths::TempLocation))) {
            // Throughput is measured from the first data packet (see _ftpDownloadProgress), not from here: the
            // leading session reset and open can take several seconds on a lossy link without saying anything
            // about transfer speed
            _downloadStartTime.invalidate();
            state->connectWhileWaiting(ftpManager, &FTPManager::commandProgress,
                                       [this](float progress) { _ftpDownloadProgress(progress); });
        } else {
            _transferInFlight = Transfer::None;
            qCWarning(RequestMetaDataTypeStateMachineLog) << "FTPManager::download returned failure";
            state->complete();
        }
    } else {
        if (trackMetadataSource) {
            _metadataSource = MetadataSource::HTTP;
            _metadataUri = uri;
        }
        QGCCachedFileDownload* const cachedFileDownload = _compMgr->_cachedFileDownload;
        state->connectToCompletion(
            cachedFileDownload, &QGCCachedFileDownload::finished,
            [this, state, output, cacheFileTag, crcValid](bool success, const QString& localFile,
                                                          const QString& errorMsg, bool fromCache) {
                qCDebug(RequestMetaDataTypeStateMachineLog)
                    << "HTTP download complete success:localFile:errorMsg:fromCache" << success << localFile << errorMsg
                    << fromCache;
                _transferInFlight = Transfer::None;
                if (success && errorMsg.isEmpty()) {
                    *output = _downloadCompleteJsonWorker(localFile, cacheFileTag, crcValid);
                } else {
                    qCDebug(RequestMetaDataTypeStateMachineLog)
                        << typeToString() << ": HTTP download failed:" << errorMsg;
                }
                state->complete();
            });
        _transferInFlight = Transfer::Http;
        if (cachedFileDownload->download(uri, crcValid ? 0 : ComponentInformationManager::cachedFileMaxAgeSec)) {
            _downloadStartTime.start();
        } else {
            _transferInFlight = Transfer::None;
            qCWarning(RequestMetaDataTypeStateMachineLog) << "QGCCachedFileDownload::download returned failure";
            state->complete();
        }
    }
}

QString RequestMetaDataTypeStateMachine::_downloadCompleteJsonWorker(const QString& fileName,
                                                                     const QString& cacheFileTag, bool crcValid)
{
    const QString tempPath =
        QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).absoluteFilePath(cacheFileTag);
    QString outputFileName = QGCCompression::decompressIfNeeded(fileName, tempPath);
    if (outputFileName.isEmpty()) {
        qCWarning(RequestMetaDataTypeStateMachineLog) << "Inflate of compressed json failed" << cacheFileTag;
    }

    if (crcValid) {
        // Cache the file (this will move/remove the temp file as well)
        outputFileName = _compMgr->fileCache().insert(cacheFileTag, outputFileName);
    }
    return outputFileName;
}

void RequestMetaDataTypeStateMachine::_cancelMessageRequest()
{
    // The answer to a request that its state no longer waits for would set the URI of whatever is requested then
    if (Vehicle* const requestVehicle = vehicle()) {
        requestVehicle->cancelRequestMessage(this);
    }
}

void RequestMetaDataTypeStateMachine::_cancelTransfer()
{
    // Nothing is left to cancel once the completion has been handled. The two calls report the end of the
    // download on the signals that the state, which is being left, has stopped listening to.
    switch (std::exchange(_transferInFlight, Transfer::None)) {
        case Transfer::None:
            break;
        case Transfer::Ftp:
            qCDebug(RequestMetaDataTypeStateMachineLog) << typeToString() << ": state left, cancelling FTP download";
            // Gives the FTP session of the vehicle back, which a fallback over FTP needs
            _compInfo->vehicle->ftpManager()->cancelDownload();
            break;
        case Transfer::Http:
            qCDebug(RequestMetaDataTypeStateMachineLog) << typeToString() << ": state left, cancelling HTTP download";
            _compMgr->_cachedFileDownload->cancel();
            break;
    }
}

void RequestMetaDataTypeStateMachine::_ftpDownloadProgress(float progress)
{
    if (!_downloadStartTime.isValid()) {
        _downloadStartTime.start();
        return;
    }

    int elapsedSec = _downloadStartTime.elapsed() / 1000;
    float totalDownloadTime = elapsedSec / progress;

    // Abort download if it's too slow (e.g. over telemetry link) and use the fallback. This sits in front of
    // parameter load, so decide as soon as the projection is trustworthy rather than after a fixed 10s.
    const int minElapsedSec = 5;
    const int maxDownloadTimeSec = 40;
    if (elapsedSec > minElapsedSec && progress < 0.5 && totalDownloadTime > maxDownloadTimeSec) {
        qCDebug(RequestMetaDataTypeStateMachineLog) << "Slow download, aborting. Total time (s):" << totalDownloadTime;
        _compInfo->vehicle->ftpManager()->cancelDownload();
    }
}

bool RequestMetaDataTypeStateMachine::_uriIsMAVLinkFTP(const QString& uri)
{
    return uri.startsWith(QStringLiteral("%1://").arg(FTPManager::mavlinkFTPScheme), Qt::CaseInsensitive);
}
