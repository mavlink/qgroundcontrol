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
    registerState(_stateRequestGeneral);

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
    registerState(_stateRequestParam);

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
    registerState(_stateRequestEvents);

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
    registerState(_stateRequestActuators);

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

    // RequestParam -> RequestEvents (via advance or skipped)
    _stateRequestParam->addTransition(_stateRequestParam, &SkippableAsyncState::advance, _stateRequestEvents);
    _stateRequestParam->addTransition(_stateRequestParam, &SkippableAsyncState::skipped, _stateRequestEvents);

    // RequestEvents -> RequestActuators (via advance or skipped)
    _stateRequestEvents->addTransition(_stateRequestEvents, &SkippableAsyncState::advance, _stateRequestActuators);
    _stateRequestEvents->addTransition(_stateRequestEvents, &SkippableAsyncState::skipped, _stateRequestActuators);

    // RequestActuators -> Complete (via advance or skipped)
    _stateRequestActuators->addTransition(_stateRequestActuators, &SkippableAsyncState::advance, _stateComplete);
    _stateRequestActuators->addTransition(_stateRequestActuators, &SkippableAsyncState::skipped, _stateComplete);

    // Complete -> Final
    _stateComplete->addTransition(_stateComplete, &FunctionState::advance, _stateFinal);
}

void ComponentInformationManager::_wireProgressTracking()
{
    _stateRequestGeneral->setOnEntry([this]() {
        _currentStateIndex = 0;
        _updateProgress();
    });

    _stateUpdateUri->setOnEntry([this]() {
        _currentStateIndex = 1;
        _updateProgress();
    });

    _stateRequestParam->setOnEntry([this]() {
        _currentStateIndex = 2;
        _updateProgress();
    });

    _stateRequestEvents->setOnEntry([this]() {
        _currentStateIndex = 3;
        _updateProgress();
    });

    _stateRequestActuators->setOnEntry([this]() {
        _currentStateIndex = 4;
        _updateProgress();
    });

    _stateComplete->setOnEntry([this]() {
        _currentStateIndex = 5;
        _updateProgress();
    });
}

void ComponentInformationManager::_updateProgress()
{
    emit progressUpdate(progress());
}

float ComponentInformationManager::progress() const
{
    if (!isRunning()) {
        return 1.0f;
    }
    return static_cast<float>(_currentStateIndex) / static_cast<float>(_stateCount);
}

void ComponentInformationManager::requestAllComponentInformation(RequestAllCompleteFn requestAllCompletFn, void * requestAllCompleteFnData)
{
    qCDebug(ComponentInformationManagerLog) << Q_FUNC_INFO;

    _requestAllCompleteFn       = requestAllCompletFn;
    _requestAllCompleteFnData   = requestAllCompleteFnData;

    start();
    emit progressUpdate(progress());
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
