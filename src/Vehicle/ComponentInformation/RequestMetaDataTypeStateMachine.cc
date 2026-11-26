#include "RequestMetaDataTypeStateMachine.h"

#include "CompInfoGeneral.h"
#include "ComponentInformationCache.h"
#include "ComponentInformationManager.h"
#include "ComponentInformationTranslation.h"
#include "CompressionTypes.h"
#include "MAVLinkLib.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"
#include "VehicleFileFetch.h"
#include "VehicleLinkManager.h"
#include "WorkerJob.h"

// State types included via QGCStateMachine.h in header

#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>

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
    _stateRequestDeprecated = new AsyncFunctionState(
        "RequestCompInfoDeprecated", this, [this](AsyncFunctionState*) { _requestCompInfoDeprecated(); },
        timeouts.compInfoRequest);
    _stateRequestDeprecated->setSkipPredicate([this]() { return _shouldSkipDeprecatedRequest(); });

    // State 3: Download metadata JSON
    _stateRequestMetaDataJson = new AsyncFunctionState(
        "RequestMetaDataJson", this, [this](AsyncFunctionState*) { _requestMetaDataJson(); },
        timeouts.metaDataDownload);

    // State 4: Try fallback URI if primary download failed
    _stateRequestMetaDataJsonFallback = new AsyncFunctionState(
        "RequestMetaDataJsonFallback", this, [this](AsyncFunctionState*) { _requestMetaDataJsonFallback(); },
        timeouts.metaDataDownload);
    _stateRequestMetaDataJsonFallback->setSkipPredicate([this]() { return _shouldSkipFallback(); });

    // State 5: Download translation JSON
    _stateRequestTranslationJson = new AsyncFunctionState(
        "RequestTranslationJson", this, [this](AsyncFunctionState*) { _requestTranslationJson(); },
        timeouts.metaDataDownload);

    // State 6: Translate metadata
    _stateRequestTranslate = new AsyncFunctionState(
        "RequestTranslate", this, [this](AsyncFunctionState*) { _requestTranslate(); }, timeouts.translation);
    _stateRequestTranslate->setSkipPredicate([this]() { return _shouldSkipTranslation(); });

    // State 7: Read and parse the metadata
    _stateLoadMetaData = new AsyncFunctionState(
        "LoadMetaData", this, [this](AsyncFunctionState*) { _loadMetaData(); }, timeouts.metaDataLoad);

    // A state that times out is left while what it asked for is still under way. Nothing of that may reach a
    // later state, or the next request this machine makes.
    _stateRequestCompInfo->setOnExit([this]() { _cancelMessageRequest(); });
    _stateRequestDeprecated->setOnExit([this]() { _cancelMessageRequest(); });
    _stateRequestMetaDataJson->setOnExit([this]() { _cancelTransfer(); });
    _stateRequestMetaDataJsonFallback->setOnExit([this]() { _cancelTransfer(); });
    _stateRequestTranslationJson->setOnExit([this]() { _cancelTransfer(); });
    _stateRequestTranslate->setOnExit([this]() { _cancelTransfer(); });
    _stateLoadMetaData->setOnExit([this]() { _cancelLoad(); });

    // State 8: Complete request
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

    // RequestTranslate -> LoadMetaData
    _stateRequestTranslate->addTransition(_stateRequestTranslate, &QGCState::advance, _stateLoadMetaData);

    // LoadMetaData -> CompleteRequest
    _stateLoadMetaData->addTransition(_stateLoadMetaData, &QGCState::advance, _stateComplete);

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

    _stateRequestTranslate->addTransition(_stateRequestTranslate, &WaitStateBase::timedOut, _stateLoadMetaData);

    _stateLoadMetaData->addTransition(_stateLoadMetaData, &WaitStateBase::timedOut, _stateComplete);
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
    _loadEnded = QtFuture::makeReadyVoidFuture();

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
    _transferInFlight = Transfer::Translation;
    if (!_compMgr->translation()->downloadAndTranslate(_jsonTranslationFileName,
                                                       _jsonMetadataFileName,
                                                       ComponentInformationManager::cachedFileMaxAgeSec,
                                                       typeToString())) {
        _transferInFlight = Transfer::None;
        qCDebug(RequestMetaDataTypeStateMachineLog)
            << typeToString() << ": translation skipped for an English locale, using untranslated metadata";
        _stateRequestTranslate->complete();
    }
}

void RequestMetaDataTypeStateMachine::_loadMetaData()
{
    const QString fileName =
        _jsonMetadataTranslatedFileName.isEmpty() ? _jsonMetadataFileName : _jsonMetadataTranslatedFileName;
    if (fileName.isEmpty()) {
        _compInfo->setJson(fileName, {});
        _stateLoadMetaData->complete();
        return;
    }

    // The file comes from the vehicle, or from where it points to, and may be large: it is read and parsed on a
    // worker thread. The parse belongs to this state like a download: a result that comes after the state was left
    // is dropped.
    const auto load = [fileName, parse = _compInfo->fileParser()]() { return parse(fileName); };
    auto* const job = QGC::runOnWorker(this, load, [this, fileName](const CompInfo::ParsedJson& json) {
        _compInfo->setJson(fileName, json);
        _stateLoadMetaData->complete();
    });
    _loadJob = job;
    _loadEnded = job->ended();
}

void RequestMetaDataTypeStateMachine::_completeRequest()
{
    const bool success = !_jsonMetadataFileName.isEmpty();
    const bool translated = !_jsonMetadataTranslatedFileName.isEmpty();

    QStringList temporaryFiles;
    if (translated) {
        temporaryFiles.append(_jsonMetadataTranslatedFileName);
    }
    // If we don't have a CRC we didn't cache the file and need to delete it
    if (!_jsonMetadataCrcValid && !_jsonMetadataFileName.isEmpty()) {
        temporaryFiles.append(_jsonMetadataFileName);
    }
    if (!_jsonMetadataCrcValid && !_jsonTranslationFileName.isEmpty()) {
        temporaryFiles.append(_jsonTranslationFileName);
    }
    // A load that timed out may still be reading its file. The files go once it has ended, which is now for any other.
    (void) _loadEnded.then([temporaryFiles]() {
        for (const QString& file : temporaryFiles) {
            (void) QFile::remove(file);
        }
    });

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

    // The download belongs to the state that waits for it. A download that has not ended when the state is left is
    // cancelled (_cancelTransfer()): one that outlived its state completed whichever state was current by then.
    // The file is kept as it was sent, compressed or not: it is decompressed where it is read. It is held to the size
    // its reader takes, and written into the download directory of the manager, so that the files of another
    // vehicle, which often have the same names, never take their place.
    const bool ftp = VehicleFileFetch::isMavlinkFtp(uri);
    if (trackMetadataSource) {
        _metadataSource = ftp ? MetadataSource::FTP : MetadataSource::HTTP;
        _metadataUri = uri;
    }
    const VehicleFileFetch::Options options{
        .directory = _compMgr->downloadDirectory(),
        .maxBytes = Compression::kMaxSmallFileBytes,
        .httpCacheDirectory = ComponentInformationManager::httpCacheDirectory(),
        .httpCacheMaxAgeSec = crcValid ? 0 : ComponentInformationManager::cachedFileMaxAgeSec,
    };
    QString* const output = &outputFileName;
    _fetch = VehicleFileFetch::start(
        this, _compInfo->vehicle, uri, options,
        [this, state, output, cacheFileTag, crcValid](const QString& fileName, const QString& errorMsg) {
            qCDebug(RequestMetaDataTypeStateMachineLog)
                << "Download complete fileName:errorMsg" << fileName << errorMsg;
            _transferInFlight = Transfer::None;
            if (errorMsg.isEmpty()) {
                *output = _downloadCompleteJsonWorker(fileName, cacheFileTag, crcValid);
            } else {
                qCDebug(RequestMetaDataTypeStateMachineLog) << typeToString() << ": download failed:" << errorMsg;
            }
            state->complete();
        });
    if (!_fetch) {
        qCWarning(RequestMetaDataTypeStateMachineLog) << typeToString() << ": the download could not be started";
        state->complete();
        return;
    }

    _transferInFlight = Transfer::Download;
    // A download over MAVLink FTP may wait behind other transfers of the vehicle. The timeout of the state is for the
    // download, so it counts from when the download begins.
    state->holdTimeout();
    (void) connect(_fetch, &VehicleFileFetch::started, state, [state]() { state->restartTimeout(); });
    if (ftp) {
        // Throughput is measured from the first data packet (see _ftpDownloadProgress), not from here: the leading
        // session reset and open can take several seconds on a lossy link without saying anything about transfer
        // speed
        _downloadStartTime.invalidate();
        (void) connect(_fetch, &VehicleFileFetch::progressChanged, this,
                       [this, state](float progress) { _ftpDownloadProgress(state, progress); });
    }
}

QString RequestMetaDataTypeStateMachine::_downloadCompleteJsonWorker(const QString& fileName,
                                                                     const QString& cacheFileTag, bool crcValid)
{
    // Cache the file (this will move/remove the temp file as well)
    return crcValid ? _compMgr->fileCache().insert(cacheFileTag, fileName) : fileName;
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
    // Nothing is left to cancel once the completion has been handled. A cancelled download calls nothing; the
    // translation reports its end on the signal that the state, which is being left, has stopped listening to.
    switch (std::exchange(_transferInFlight, Transfer::None)) {
        case Transfer::None:
            break;
        case Transfer::Download:
            qCDebug(RequestMetaDataTypeStateMachineLog) << typeToString() << ": state left, cancelling download";
            // Over MAVLink FTP, gives the session of the vehicle back to the transfers that wait for it
            if (_fetch) {
                _fetch->cancel();
            }
            break;
        case Transfer::Translation:
            qCDebug(RequestMetaDataTypeStateMachineLog) << typeToString() << ": state left, cancelling translation";
            _compMgr->translation()->cancel();
            break;
    }
}

void RequestMetaDataTypeStateMachine::_cancelLoad()
{
    // Drops a result that has not been taken. The worker cannot be stopped: _loadEnded tells when it has ended.
    delete _loadJob.data();
}

void RequestMetaDataTypeStateMachine::_ftpDownloadProgress(CompletionWaitState* state, float progress)
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
        _cancelTransfer();
        state->complete();
    }
}
