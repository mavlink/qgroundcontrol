#include "InitialConnectStateMachine.h"
#include "Vehicle.h"
#include "QGCCorePlugin.h"
#include "QGCOptions.h"
#include "FirmwarePlugin.h"
#include "ParameterManager.h"
#include "ComponentInformationManager.h"
#include "MissionManager.h"
#include "StandardModes.h"
#include "GeoFenceManager.h"
#include "RallyPointManager.h"
#include "QGCLoggingCategory.h"

#include <cstring>

QGC_LOGGING_CATEGORY(InitialConnectStateMachineLog, "Vehicle.InitialConnectStateMachine")

// ============================================================================
// InitialConnectStateMachine Implementation
// ============================================================================

InitialConnectStateMachine::InitialConnectStateMachine(Vehicle* vehicle, QObject* parent)
    : QGCStateMachine(QStringLiteral("InitialConnectStateMachine"), vehicle, parent)
{
    _createStates();
    _wireTransitions();
    _wireProgressTracking();
    _wireTimeoutHandling();

    setInitialState(_stateAutopilotVersion);
}

InitialConnectStateMachine::~InitialConnectStateMachine()
{
}

void InitialConnectStateMachine::start()
{
    resetProgress();
    QGCStateMachine::start();
}

void InitialConnectStateMachine::_createStates()
{
    // State 0: Request autopilot version
    _stateAutopilotVersion = new RetryableRequestMessageState(
        QStringLiteral("RequestAutopilotVersion"),
        this,
        MAVLINK_MSG_ID_AUTOPILOT_VERSION,
        [this](Vehicle*, const mavlink_message_t& message) {
            _handleAutopilotVersionSuccess(message);
        },
        _maxRetries,
        MAV_COMP_ID_AUTOPILOT1,
        _timeoutAutopilotVersion
    );
    _stateAutopilotVersion->setSkipPredicate([this]() {
        return _shouldSkipAutopilotVersionRequest();
    });
    _stateAutopilotVersion->setFailureHandler([this](auto, auto) {
        _handleAutopilotVersionFailure();
    });

    // State 1: Request standard modes
    _stateStandardModes = new AsyncFunctionState(
        QStringLiteral("RequestStandardModes"),
        this,
        [this](AsyncFunctionState* state) { _requestStandardModes(state); },
        _timeoutStandardModes
    );

    // State 2: Request component information
    _stateCompInfo = new AsyncFunctionState(
        QStringLiteral("RequestCompInfo"),
        this,
        [this](AsyncFunctionState* state) { _requestCompInfo(state); },
        _timeoutCompInfo
    );

    // State 3: Request parameters
    _stateParameters = new AsyncFunctionState(
        QStringLiteral("RequestParameters"),
        this,
        [this](AsyncFunctionState* state) { _requestParameters(state); },
        _timeoutParameters
    );

    // State 4: Request mission (skippable)
    _stateMission = new SkippableAsyncState(
        QStringLiteral("RequestMission"),
        this,
        [this]() { return _shouldSkipForLinkType() || !_hasPrimaryLink(); },
        [this](SkippableAsyncState* state) { _requestMission(state); },
        []() {
            qCDebug(InitialConnectStateMachineLog) << "Skipping mission load";
        },
        _timeoutMission
    );

    // State 5: Request geofence (skippable)
    _stateGeoFence = new SkippableAsyncState(
        QStringLiteral("RequestGeoFence"),
        this,
        [this]() {
            return _shouldSkipForLinkType() || !_hasPrimaryLink() ||
                   !vehicle()->_geoFenceManager->supported();
        },
        [this](SkippableAsyncState* state) { _requestGeoFence(state); },
        []() {
            qCDebug(InitialConnectStateMachineLog) << "Skipping geofence load";
        },
        _timeoutGeoFence
    );

    // State 6: Request rally points (skippable)
    _stateRallyPoints = new SkippableAsyncState(
        QStringLiteral("RequestRallyPoints"),
        this,
        [this]() {
            return _shouldSkipForLinkType() || !_hasPrimaryLink() ||
                   !vehicle()->_rallyPointManager->supported();
        },
        [this](SkippableAsyncState* state) { _requestRallyPoints(state); },
        [this]() {
            qCDebug(InitialConnectStateMachineLog) << "Skipping rally points load";
            // Mark plan request complete when skipping
            vehicle()->_initialPlanRequestComplete = true;
            emit vehicle()->initialPlanRequestCompleteChanged(true);
        },
        _timeoutRallyPoints
    );

    // State 7: Signal completion
    // Use RetryState with zero retries so completion participates in the unified
    // retry/error state family while preserving immediate success behavior.
    _stateComplete = new RetryState(
        QStringLiteral("SignalComplete"),
        this,
        [this]() {
            _signalComplete();
            return true;
        },
        0,
        0,
        RetryState::EmitError
    );

    // Final state
    _stateFinal = new QGCFinalState(QStringLiteral("Final"), this);
}

void InitialConnectStateMachine::_wireTransitions()
{
    // Linear progression - use completed() for WaitStateBase-derived states (more semantic)
    _stateAutopilotVersion->addTransition(_stateAutopilotVersion, &WaitStateBase::completed, _stateStandardModes);
    _stateStandardModes->addTransition(_stateStandardModes, &WaitStateBase::completed, _stateCompInfo);
    _stateCompInfo->addTransition(_stateCompInfo, &WaitStateBase::completed, _stateParameters);
    _stateParameters->addTransition(_stateParameters, &WaitStateBase::completed, _stateMission);

    // SkippableAsyncStates: both completed and skipped go to next state
    _stateMission->addTransition(_stateMission, &WaitStateBase::completed, _stateGeoFence);
    _stateMission->addTransition(_stateMission, &SkippableAsyncState::skipped, _stateGeoFence);

    _stateGeoFence->addTransition(_stateGeoFence, &WaitStateBase::completed, _stateRallyPoints);
    _stateGeoFence->addTransition(_stateGeoFence, &SkippableAsyncState::skipped, _stateRallyPoints);

    _stateRallyPoints->addTransition(_stateRallyPoints, &WaitStateBase::completed, _stateComplete);
    _stateRallyPoints->addTransition(_stateRallyPoints, &SkippableAsyncState::skipped, _stateComplete);

    // Complete -> Final (RetryState emits advance() on success)
    _stateComplete->addTransition(_stateComplete, &QGCState::advance, _stateFinal);
}

void InitialConnectStateMachine::_wireProgressTracking()
{
    // Use QGCStateMachine's built-in weighted progress tracking
    // Progress is automatically updated when states are entered
    setProgressWeights({
        {_stateAutopilotVersion, 1},
        {_stateStandardModes, 1},
        {_stateCompInfo, 5},
        {_stateParameters, 5},
        {_stateMission, 2},
        {_stateGeoFence, 1},
        {_stateRallyPoints, 1},
        {_stateComplete, 1}
    });
}

void InitialConnectStateMachine::_onSubProgressUpdate(double progressValue)
{
    setSubProgress(static_cast<float>(progressValue));
}

// ============================================================================
// Timeout Handling
// ============================================================================

void InitialConnectStateMachine::_wireTimeoutHandling()
{
    // Note: _stateAutopilotVersion is RetryableRequestMessageState which handles its own retry

    // Use addRetryTransition builder for cleaner timeout handling
    addRetryTransition(_stateStandardModes, &WaitStateBase::timedOut, _stateCompInfo,
                       [this]() { _requestStandardModes(_stateStandardModes); }, _maxRetries);

    addRetryTransition(_stateCompInfo, &WaitStateBase::timedOut, _stateParameters,
                       [this]() { _requestCompInfo(_stateCompInfo); }, _maxRetries);

    addRetryTransition(_stateParameters, &WaitStateBase::timedOut, _stateMission,
                       [this]() { _requestParameters(_stateParameters); }, _maxRetries);

    addRetryTransition(_stateMission, &WaitStateBase::timedOut, _stateGeoFence,
                       [this]() { _requestMission(_stateMission); }, _maxRetries);

    addRetryTransition(_stateGeoFence, &WaitStateBase::timedOut, _stateRallyPoints,
                       [this]() { _requestGeoFence(_stateGeoFence); }, _maxRetries);

    addRetryTransition(_stateRallyPoints, &WaitStateBase::timedOut, _stateComplete,
                       [this]() { _requestRallyPoints(_stateRallyPoints); }, _maxRetries);
}

// ============================================================================
// Skip Predicates
// ============================================================================

bool InitialConnectStateMachine::_shouldSkipAutopilotVersionRequest() const
{
    SharedLinkInterfacePtr sharedLink = vehicle()->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCDebug(InitialConnectStateMachineLog) << "Skipping AUTOPILOT_VERSION: no primary link";
        return true;
    }
    if (sharedLink->linkConfiguration()->isHighLatency() || sharedLink->isLogReplay()) {
        qCDebug(InitialConnectStateMachineLog) << "Skipping AUTOPILOT_VERSION: high latency or log replay";
        return true;
    }
    return false;
}

bool InitialConnectStateMachine::_shouldSkipForLinkType() const
{
    SharedLinkInterfacePtr sharedLink = vehicle()->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        return true;
    }
    return sharedLink->linkConfiguration()->isHighLatency() || sharedLink->isLogReplay();
}

bool InitialConnectStateMachine::_hasPrimaryLink() const
{
    SharedLinkInterfacePtr sharedLink = vehicle()->vehicleLinkManager()->primaryLink().lock();
    return sharedLink != nullptr;
}

// ============================================================================
// State Callbacks
// ============================================================================

void InitialConnectStateMachine::_handleAutopilotVersionSuccess(const mavlink_message_t& message)
{
    qCDebug(InitialConnectStateMachineLog) << "AUTOPILOT_VERSION received";

    mavlink_autopilot_version_t autopilotVersion;
    mavlink_msg_autopilot_version_decode(&message, &autopilotVersion);

    vehicle()->_uid = (quint64)autopilotVersion.uid;
    vehicle()->_firmwareBoardVendorId = autopilotVersion.vendor_id;
    vehicle()->_firmwareBoardProductId = autopilotVersion.product_id;
    emit vehicle()->vehicleUIDChanged();

    if (autopilotVersion.flight_sw_version != 0) {
        int majorVersion, minorVersion, patchVersion;
        FIRMWARE_VERSION_TYPE versionType;

        majorVersion = (autopilotVersion.flight_sw_version >> (8*3)) & 0xFF;
        minorVersion = (autopilotVersion.flight_sw_version >> (8*2)) & 0xFF;
        patchVersion = (autopilotVersion.flight_sw_version >> (8*1)) & 0xFF;
        versionType = (FIRMWARE_VERSION_TYPE)((autopilotVersion.flight_sw_version >> (8*0)) & 0xFF);
        vehicle()->setFirmwareVersion(majorVersion, minorVersion, patchVersion, versionType);
    }

    if (vehicle()->px4Firmware()) {
        // Lower 3 bytes is custom version
        int majorVersion, minorVersion, patchVersion;
        majorVersion = autopilotVersion.flight_custom_version[2];
        minorVersion = autopilotVersion.flight_custom_version[1];
        patchVersion = autopilotVersion.flight_custom_version[0];
        vehicle()->setFirmwareCustomVersion(majorVersion, minorVersion, patchVersion);

        // PX4 Firmware stores the first 16 characters of the git hash as binary, with the individual bytes in reverse order
        vehicle()->_gitHash = "";
        for (int i = 7; i >= 0; i--) {
            vehicle()->_gitHash.append(QString("%1").arg(autopilotVersion.flight_custom_version[i], 2, 16, QChar('0')));
        }
    } else {
        // APM Firmware stores the first 8 characters of the git hash as an ASCII character string
        char nullStr[9];
        strncpy(nullStr, (char*)autopilotVersion.flight_custom_version, 8);
        nullStr[8] = 0;
        vehicle()->_gitHash = nullStr;
    }

    if (QGCCorePlugin::instance()->options()->checkFirmwareVersion() && !vehicle()->_checkLatestStableFWDone) {
        vehicle()->_checkLatestStableFWDone = true;
        vehicle()->_firmwarePlugin->checkIfIsLatestStable(vehicle());
    }
    emit vehicle()->gitHashChanged(vehicle()->_gitHash);

    vehicle()->_setCapabilities(autopilotVersion.capabilities);
}

void InitialConnectStateMachine::_handleAutopilotVersionFailure()
{
    qCDebug(InitialConnectStateMachineLog) << "AUTOPILOT_VERSION request failed, setting assumed capabilities";

    uint64_t assumedCapabilities = MAV_PROTOCOL_CAPABILITY_MAVLINK2;
    if (vehicle()->px4Firmware() || vehicle()->apmFirmware()) {
        // We make some assumptions for known firmware
        assumedCapabilities |= MAV_PROTOCOL_CAPABILITY_MISSION_INT | MAV_PROTOCOL_CAPABILITY_COMMAND_INT |
                               MAV_PROTOCOL_CAPABILITY_MISSION_FENCE | MAV_PROTOCOL_CAPABILITY_MISSION_RALLY;
    }
    vehicle()->_setCapabilities(assumedCapabilities);
}

void InitialConnectStateMachine::_requestStandardModes(AsyncFunctionState* state)
{
    qCDebug(InitialConnectStateMachineLog) << "_stateRequestStandardModes";

    state->connectToCompletion(vehicle()->_standardModes, &StandardModes::requestCompleted);
    vehicle()->_standardModes->request();
}

void InitialConnectStateMachine::_requestCompInfo(AsyncFunctionState* /*state*/)
{
    qCDebug(InitialConnectStateMachineLog) << "_stateRequestCompInfo";

    connect(vehicle()->_componentInformationManager, &ComponentInformationManager::progressUpdate,
            this, &InitialConnectStateMachine::_onSubProgressUpdate, Qt::UniqueConnection);

    vehicle()->_componentInformationManager->requestAllComponentInformation(
        [](void* requestAllCompleteFnData) {
            auto* self = static_cast<InitialConnectStateMachine*>(requestAllCompleteFnData);
            disconnect(self->vehicle()->_componentInformationManager, &ComponentInformationManager::progressUpdate,
                       self, &InitialConnectStateMachine::_onSubProgressUpdate);
            if (self->_stateCompInfo) {
                self->_stateCompInfo->complete();
            }
        },
        this
    );
}

void InitialConnectStateMachine::_requestParameters(AsyncFunctionState* state)
{
    qCDebug(InitialConnectStateMachineLog) << "_stateRequestParameters";

    connect(vehicle()->_parameterManager, &ParameterManager::loadProgressChanged,
            this, &InitialConnectStateMachine::_onSubProgressUpdate, Qt::UniqueConnection);

    state->connectToCompletion(vehicle()->_parameterManager, &ParameterManager::parametersReadyChanged,
        [this](bool parametersReady) {
            _onParametersReady(parametersReady);
        });

    // Ensure progress tracking is always cleaned up, including timeout/skip paths.
    state->setOnExit([this]() {
        disconnect(vehicle()->_parameterManager, &ParameterManager::loadProgressChanged,
                   this, &InitialConnectStateMachine::_onSubProgressUpdate);
    });

    vehicle()->_parameterManager->refreshAllParameters();
}

void InitialConnectStateMachine::_onParametersReady(bool parametersReady)
{
    qCDebug(InitialConnectStateMachineLog) << "_onParametersReady" << parametersReady;

    // Disconnect progress tracking from parameter manager
    disconnect(vehicle()->_parameterManager, &ParameterManager::loadProgressChanged,
               this, &InitialConnectStateMachine::_onSubProgressUpdate);

    if (parametersReady) {
        // Send time to vehicle (twice for reliability on noisy links)
        vehicle()->_sendQGCTimeToVehicle();
        vehicle()->_sendQGCTimeToVehicle();

        // Set up auto-disarm signalling
        vehicle()->_setupAutoDisarmSignalling();

        // Note: Speed limits are handled by Vehicle::_parametersReady

        if (_stateParameters) {
            _stateParameters->complete();
        }
    }
}

void InitialConnectStateMachine::_requestMission(SkippableAsyncState* state)
{
    qCDebug(InitialConnectStateMachineLog) << "_stateRequestMission";

    connect(vehicle()->_missionManager, &MissionManager::progressPctChanged,
            this, &InitialConnectStateMachine::_onSubProgressUpdate, Qt::UniqueConnection);

    state->connectToCompletion(vehicle()->_missionManager, &MissionManager::newMissionItemsAvailable);

    // Disconnect progress tracking on exit
    state->setOnExit([this]() {
        disconnect(vehicle()->_missionManager, &MissionManager::progressPctChanged,
                   this, &InitialConnectStateMachine::_onSubProgressUpdate);
    });

    vehicle()->_missionManager->loadFromVehicle();
}

void InitialConnectStateMachine::_requestGeoFence(SkippableAsyncState* state)
{
    qCDebug(InitialConnectStateMachineLog) << "_stateRequestGeoFence";

    connect(vehicle()->_geoFenceManager, &GeoFenceManager::progressPctChanged,
            this, &InitialConnectStateMachine::_onSubProgressUpdate, Qt::UniqueConnection);

    state->connectToCompletion(vehicle()->_geoFenceManager, &GeoFenceManager::loadComplete);

    // Disconnect progress tracking on exit
    state->setOnExit([this]() {
        disconnect(vehicle()->_geoFenceManager, &GeoFenceManager::progressPctChanged,
                   this, &InitialConnectStateMachine::_onSubProgressUpdate);
    });

    vehicle()->_geoFenceManager->loadFromVehicle();
}

void InitialConnectStateMachine::_requestRallyPoints(SkippableAsyncState* state)
{
    qCDebug(InitialConnectStateMachineLog) << "_stateRequestRallyPoints";

    connect(vehicle()->_rallyPointManager, &RallyPointManager::progressPctChanged,
            this, &InitialConnectStateMachine::_onSubProgressUpdate, Qt::UniqueConnection);

    state->connectToCompletion(vehicle()->_rallyPointManager, &RallyPointManager::loadComplete,
        [this]() {
            // Mark initial plan request complete
            vehicle()->_initialPlanRequestComplete = true;
            emit vehicle()->initialPlanRequestCompleteChanged(true);

            if (_stateRallyPoints) {
                _stateRallyPoints->complete();
            }
        });

    // Always clean up progress tracking when leaving this state.
    state->setOnExit([this]() {
        disconnect(vehicle()->_rallyPointManager, &RallyPointManager::progressPctChanged,
                   this, &InitialConnectStateMachine::_onSubProgressUpdate);
    });

    vehicle()->_rallyPointManager->loadFromVehicle();
}

void InitialConnectStateMachine::_signalComplete()
{
    qCDebug(InitialConnectStateMachineLog) << "Signalling initialConnectComplete";
    emit vehicle()->initialConnectComplete();
}
