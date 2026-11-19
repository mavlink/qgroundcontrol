#include "ComponentInformationManager.h"
#include "ComponentInformationTranslation.h"
#include "ComponentInformationCache.h"
#include "Vehicle.h"
#include "CompInfoGeneral.h"
#include "CompInfoParam.h"
#include "CompInfoEvents.h"
#include "CompInfoActuators.h"
#include "QGCCachedFileDownload.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QStandardPaths>

QGC_LOGGING_CATEGORY(ComponentInformationManagerLog, "ComponentInformation.ComponentInformationManager")

ComponentInformationManager::ComponentInformationManager(Vehicle *vehicle, QObject *parent)
    : QGCStateMachine("ComponentInformationManager", vehicle, parent)
    , _requestTypeStateMachine(this, this)
    , _cachedFileDownload(new QGCCachedFileDownload(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QLatin1String("/QGCCompInfoFileDownloadCache"), this))
    , _fileCache(ComponentInformationCache::defaultInstance())
    , _translation(new ComponentInformationTranslation(this, _cachedFileDownload))
{
    qCDebug(ComponentInformationManagerLog) << this;
    qCDebug(ComponentInformationManagerLog) << "Cache location:" << QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QLatin1String("/QGCCompInfoCache");

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
    _stateUpdateUri = addFunctionState(
        QStringLiteral("UpdateUri"),
        [this]() { _updateAllUri(); }
    );

    // State 3: Request parameter metadata (skippable)
    _stateRequestParam = new SkippableAsyncState(
        QStringLiteral("RequestParam"),
        this,
        [this]() { return !_isCompTypeSupported(COMP_METADATA_TYPE_PARAMETER); },
        [this](SkippableAsyncState* state) { _requestCompInfoParam(state); },
        []() {
            qCDebug(ComponentInformationManagerLog) << "Skipping parameter metadata, not supported";
        }
    );

    // State 4: Request events metadata (skippable)
    _stateRequestEvents = new SkippableAsyncState(
        QStringLiteral("RequestEvents"),
        this,
        [this]() { return !_isCompTypeSupported(COMP_METADATA_TYPE_EVENTS); },
        [this](SkippableAsyncState* state) { _requestCompInfoEvents(state); },
        []() {
            qCDebug(ComponentInformationManagerLog) << "Skipping events metadata, not supported";
        }
    );

    // State 5: Request actuators metadata (skippable)
    _stateRequestActuators = new SkippableAsyncState(
        QStringLiteral("RequestActuators"),
        this,
        [this]() { return !_isCompTypeSupported(COMP_METADATA_TYPE_ACTUATORS); },
        [this](SkippableAsyncState* state) { _requestCompInfoActuators(state); },
        []() {
            qCDebug(ComponentInformationManagerLog) << "Skipping actuators metadata, not supported";
        }
    );

    // State 6: Signal completion
    _stateComplete = addFunctionState(
        QStringLiteral("Complete"),
        [this]() { _signalComplete(); }
    );

    // Final state
    _stateFinal = addFinalState(QStringLiteral("Final"));
}

void ComponentInformationManager::_wireTransitions()
{
    // RequestGeneral -> UpdateUri
    _stateRequestGeneral->addTransition(_stateRequestGeneral, &AsyncFunctionState::advance, _stateUpdateUri);

    // UpdateUri -> RequestParam
    _stateUpdateUri->addTransition(_stateUpdateUri, &FunctionState::advance, _stateRequestParam);

    // RequestParam -> RequestEvents
    _stateRequestParam->addTransition(_stateRequestParam, &SkippableAsyncState::advance, _stateRequestEvents);

    // RequestEvents -> RequestActuators
    _stateRequestEvents->addTransition(_stateRequestEvents, &SkippableAsyncState::advance, _stateRequestActuators);

    // RequestActuators -> Complete
    _stateRequestActuators->addTransition(_stateRequestActuators, &SkippableAsyncState::advance, _stateComplete);

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

void ComponentInformationManager::_requestCompInfoParam(SkippableAsyncState* state)
{
    qCDebug(ComponentInformationManagerLog) << Q_FUNC_INFO;

    state->connectToCompletion(&_requestTypeStateMachine, &RequestMetaDataTypeStateMachine::requestComplete);
    _requestTypeStateMachine.request(_compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_PARAMETER]);
}

void ComponentInformationManager::_requestCompInfoEvents(SkippableAsyncState* state)
{
    qCDebug(ComponentInformationManagerLog) << Q_FUNC_INFO;

    state->connectToCompletion(&_requestTypeStateMachine, &RequestMetaDataTypeStateMachine::requestComplete);
    _requestTypeStateMachine.request(_compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_EVENTS]);
}

void ComponentInformationManager::_requestCompInfoActuators(SkippableAsyncState* state)
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
