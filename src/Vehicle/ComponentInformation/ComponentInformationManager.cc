#include "ComponentInformationManager.h"

#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtCore/QTemporaryDir>

#include "CompInfoActuators.h"
#include "CompInfoEvents.h"
#include "CompInfoGeneral.h"
#include "CompInfoParam.h"
#include "ComponentInformationCache.h"
#include "ComponentInformationTranslation.h"
#include "QGCCachedFileDownload.h"
#include "QGCFileDownload.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"
#include "VehicleFileFetch.h"

QGC_LOGGING_CATEGORY(ComponentInformationManagerLog, "ComponentInformation.ComponentInformationManager")

ComponentInformationManager::ComponentInformationManager(Vehicle* vehicle, QObject* parent)
    : QGCStateMachine("ComponentInformationManager", vehicle, parent)
    // Without a QObject parent: a machine whose parent is this machine is one of its states to Qt
    , _requestTypeStateMachine(this)
    , _cachedFileDownload(new QGCCachedFileDownload(httpCacheDirectory(), this))
    , _fileCache(ComponentInformationCache::defaultInstance())
    , _translation(new ComponentInformationTranslation(this, _cachedFileDownload))
{
    qCDebug(ComponentInformationManagerLog) << this;
    qCDebug(ComponentInformationManagerLog) << "Cache location:" << ComponentInformationCache::defaultDirectory();

    // The translation summary and the translation file come from where the vehicle points to, and are read whole
    _cachedFileDownload->fileDownloader()->setMaxBytes(VehicleFileFetch::kMaxSmallFileBytes);

    _compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_GENERAL]    = new CompInfoGeneral   (MAV_COMP_ID_AUTOPILOT1, vehicle, this);
    _compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_PARAMETER]  = new CompInfoParam     (MAV_COMP_ID_AUTOPILOT1, vehicle, this);
    _compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_EVENTS]     = new CompInfoEvents    (MAV_COMP_ID_AUTOPILOT1, vehicle, this);
    _compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_ACTUATORS]  = new CompInfoActuators (MAV_COMP_ID_AUTOPILOT1, vehicle, this);

    _createStates();
    _wireTransitions();
    _wireProgressTracking();

    setInitialState(_stateRequestGeneral);
}

ComponentInformationManager::~ComponentInformationManager()
{
    qCDebug(ComponentInformationManagerLog) << this;

    // Before the download directory goes. Deleting the downloader closes the file it writes into it; the translation,
    // which uses the downloader, goes first.
    delete _translation;
    delete _cachedFileDownload;
}

QString ComponentInformationManager::httpCacheDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
           QLatin1String("/QGCCompInfoFileDownloadCache");
}

QString ComponentInformationManager::downloadDirectory()
{
    // Made once a download needs it: a vehicle that downloads nothing, such as an offline one, makes none
    if (!_downloadDir) {
        const QDir tempDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
        _downloadDir = std::make_unique<QTemporaryDir>(tempDir.filePath(QStringLiteral("qgc_compinfo_XXXXXX")));
        if (_downloadDir->isValid()) {
            _cachedFileDownload->fileDownloader()->setOutputDirectory(_downloadDir->path());
        } else {
            qCWarning(ComponentInformationManagerLog)
                << "Cannot create a download directory, downloading to the shared temp location:"
                << _downloadDir->errorString();
        }
    }

    return _cachedFileDownload->fileDownloader()->outputDirectory();
}

void ComponentInformationManager::_createStates()
{
    // State 1: Request general component information
    _stateRequestGeneral = new AsyncFunctionState(
        QStringLiteral("RequestGeneral"),
        this,
        [this](AsyncFunctionState* state) { _requestCompInfoGeneral(state); }
    );

    // State 2: Update URIs after general info received
    _stateUpdateUri = new FunctionState(QStringLiteral("UpdateUri"), this, [this]() { _updateAllUri(); });

    // State 3: Request parameter metadata (skippable)
    _stateRequestParam = new AsyncFunctionState(QStringLiteral("RequestParam"), this,
                                                [this](AsyncFunctionState* state) { _requestCompInfoParam(state); });
    _stateRequestParam->setSkipPredicate(
        [this]() { return !_isCompTypeSupported(COMP_METADATA_TYPE_PARAMETER); },
        []() { qCDebug(ComponentInformationManagerLog) << "Skipping parameter metadata, not supported"; });

    // State 4: Request events metadata (skippable)
    _stateRequestEvents = new AsyncFunctionState(QStringLiteral("RequestEvents"), this,
                                                 [this](AsyncFunctionState* state) { _requestCompInfoEvents(state); });
    _stateRequestEvents->setSkipPredicate(
        [this]() { return !_isCompTypeSupported(COMP_METADATA_TYPE_EVENTS); },
        []() { qCDebug(ComponentInformationManagerLog) << "Skipping events metadata, not supported"; });

    // State 5: Request actuators metadata (skippable)
    _stateRequestActuators =
        new AsyncFunctionState(QStringLiteral("RequestActuators"), this,
                               [this](AsyncFunctionState* state) { _requestCompInfoActuators(state); });
    _stateRequestActuators->setSkipPredicate(
        [this]() { return !_isCompTypeSupported(COMP_METADATA_TYPE_ACTUATORS); },
        []() { qCDebug(ComponentInformationManagerLog) << "Skipping actuators metadata, not supported"; });

    // State 6: Signal completion
    _stateComplete = new FunctionState(QStringLiteral("Complete"), this, [this]() { _signalComplete(); });

    // Final state
    _stateFinal = new QGCFinalState(QStringLiteral("Final"), this);
}

void ComponentInformationManager::_wireTransitions()
{
    // RequestGeneral -> UpdateUri
    _stateRequestGeneral->addTransition(_stateRequestGeneral, &AsyncFunctionState::advance, _stateUpdateUri);

    // UpdateUri -> RequestParam
    _stateUpdateUri->addTransition(_stateUpdateUri, &FunctionState::advance, _stateRequestParam);

    // RequestParam -> RequestEvents
    _stateRequestParam->addTransition(_stateRequestParam, &QGCState::advance, _stateRequestEvents);

    // RequestEvents -> RequestActuators
    _stateRequestEvents->addTransition(_stateRequestEvents, &QGCState::advance, _stateRequestActuators);

    // RequestActuators -> Complete
    _stateRequestActuators->addTransition(_stateRequestActuators, &QGCState::advance, _stateComplete);

    // Complete -> Final
    _stateComplete->addTransition(_stateComplete, &FunctionState::advance, _stateFinal);
}

void ComponentInformationManager::_wireProgressTracking()
{
    setProgressWeights({
        {_stateRequestGeneral, 1},
        {_stateUpdateUri, 1},
        {_stateRequestParam, 1},
        {_stateRequestEvents, 1},
        {_stateRequestActuators, 1},
        {_stateComplete, 1},
    });
}

void ComponentInformationManager::requestAllComponentInformation(RequestAllCompleteFn requestAllCompletFn, void * requestAllCompleteFnData)
{
    qCDebug(ComponentInformationManagerLog) << Q_FUNC_INFO;

    _requestAllCompleteFn       = requestAllCompletFn;
    _requestAllCompleteFnData   = requestAllCompleteFnData;

    // Guard against double-start: a request while already running just updates the
    // callback pointers; the running machine still emits requestAllComplete at the end.
    if (!inProgress()) {
        start();
    }
}

void ComponentInformationManager::_requestCompInfoGeneral(AsyncFunctionState* state)
{
    qCDebug(ComponentInformationManagerLog) << Q_FUNC_INFO;

    // Connect to requestComplete signal to advance when done
    state->connectToCompletion(&_requestTypeStateMachine, &RequestMetaDataTypeStateMachine::requestComplete);

    _requestTypeStateMachine.request(_compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_GENERAL]);
}

void ComponentInformationManager::_updateAllUri()
{
    qCDebug(ComponentInformationManagerLog) << Q_FUNC_INFO;

    CompInfoGeneral* general = qobject_cast<CompInfoGeneral*>(_compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_GENERAL]);
    for (auto& compInfo : _compInfoMap[MAV_COMP_ID_AUTOPILOT1]) {
        general->setUris(*compInfo);
    }
}

void ComponentInformationManager::_requestCompInfoParam(AsyncFunctionState* state)
{
    qCDebug(ComponentInformationManagerLog) << Q_FUNC_INFO;

    state->connectToCompletion(&_requestTypeStateMachine, &RequestMetaDataTypeStateMachine::requestComplete);
    _requestTypeStateMachine.request(_compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_PARAMETER]);
}

void ComponentInformationManager::_requestCompInfoEvents(AsyncFunctionState* state)
{
    qCDebug(ComponentInformationManagerLog) << Q_FUNC_INFO;

    state->connectToCompletion(&_requestTypeStateMachine, &RequestMetaDataTypeStateMachine::requestComplete);
    _requestTypeStateMachine.request(_compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_EVENTS]);
}

void ComponentInformationManager::_requestCompInfoActuators(AsyncFunctionState* state)
{
    qCDebug(ComponentInformationManagerLog) << Q_FUNC_INFO;

    state->connectToCompletion(&_requestTypeStateMachine, &RequestMetaDataTypeStateMachine::requestComplete);
    _requestTypeStateMachine.request(_compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_ACTUATORS]);
}

void ComponentInformationManager::_signalComplete()
{
    qCDebug(ComponentInformationManagerLog) << Q_FUNC_INFO;

    if (_requestAllCompleteFn) {
        (*_requestAllCompleteFn)(_requestAllCompleteFnData);
        _requestAllCompleteFn      = nullptr;
        _requestAllCompleteFnData  = nullptr;
    }
    emit requestAllComplete();
}

bool ComponentInformationManager::_isCompTypeSupported(COMP_METADATA_TYPE type) const
{
    CompInfoGeneral* general = qobject_cast<CompInfoGeneral*>(_compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_GENERAL]);
    return general ? general->isMetaDataTypeSupported(type) : false;
}

CompInfoParam* ComponentInformationManager::compInfoParam(uint8_t compId)
{
    if (!_compInfoMap.contains(compId)) {
        // Create default info
        _compInfoMap[compId][COMP_METADATA_TYPE_PARAMETER] = new CompInfoParam(compId, vehicle(), this);
    }
    return qobject_cast<CompInfoParam*>(_compInfoMap[compId][COMP_METADATA_TYPE_PARAMETER]);
}

CompInfoGeneral* ComponentInformationManager::compInfoGeneral(uint8_t compId)
{
    return _compInfoMap.contains(compId) && _compInfoMap[compId].contains(COMP_METADATA_TYPE_GENERAL) ? qobject_cast<CompInfoGeneral*>(_compInfoMap[compId][COMP_METADATA_TYPE_GENERAL]) : nullptr;
}

QString ComponentInformationManager::_getFileCacheTag(int compInfoType, uint32_t crc, bool isTranslation)
{
    return QString::asprintf("%08x_%02i_%i", crc, compInfoType, (int)isTranslation);
}
