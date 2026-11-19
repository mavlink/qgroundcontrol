#include "InitialConnectStateMachine.h"
#include "MAVLinkLib.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"
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
#include "SettingsManager.h"
#include "MavlinkSettings.h"

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

    setInitialState(_stateAutopilotVersion);
}

InitialConnectStateMachine::~InitialConnectStateMachine()
{
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
        _autopilotVersionMaxRetries,
        MAV_COMP_ID_AUTOPILOT1
    );
    _stateAutopilotVersion->setSkipPredicate([this]() {
        return _shouldSkipAutopilotVersionRequest();
    });
    _stateAutopilotVersion->setFailureHandler([this](auto, auto) {
        _handleAutopilotVersionFailure();
    });

    // State 1: Request standard modes
    // No timeout: download duration varies too much with link speed and mode count.
    // The standard modes protocol handles all timeouts internally and always signals completion.
    _stateStandardModes = new AsyncFunctionState(
        QStringLiteral("RequestStandardModes"),
        this,
        [this](AsyncFunctionState* state) { _requestStandardModes(state); }
    );

    // State 2: Request component information
    // No timeout: ComponentInformationManager's nested state machines have per-state
    // timeouts on every step and always signal completion.
    _stateCompInfo = new AsyncFunctionState(
        QStringLiteral("RequestCompInfo"),
        this,
        [this](AsyncFunctionState* state) { _requestCompInfo(state); }
    );

    // State 3: Request parameters (skippable)
    // No timeout: download duration varies too much with link speed and param count.
    // ParameterManager handles all timeouts internally and always terminates via
    // parametersReadyChanged or initialParametersRequestFailed.
    _stateParameters = new AsyncFunctionState(QStringLiteral("RequestParameters"), this,
                                              [this](AsyncFunctionState* state) { _requestParameters(state); });
    _stateParameters->setSkipPredicate(
        [this]() {
            if (_shouldSkipForArmed()) {
                // PX4 can try a lightweight hash-check cache load
                if (vehicle()->px4Firmware()) {
                    return false;
                }
                _lastSkipReason = QStringLiteral("(vehicle is armed)");
                return true;
            }
            return false;
        },
        [this]() {
            qCDebug(InitialConnectStateMachineLog) << "Skipping parameter download" << _lastSkipReason;
            vehicle()->_parameterManager->setParameterDownloadSkipped(true);
        });

    // State 4: Request mission (skippable)
    // No timeout: PlanManager handles all timeouts/retries internally and always signals completion.
    _stateMission = new AsyncFunctionState(QStringLiteral("RequestMission"), this,
                                           [this](AsyncFunctionState* state) { _requestMission(state); });
    _stateMission->setSkipPredicate(
        [this]() { return _shouldSkipForPlanLoad(); },
        [this]() { qCDebug(InitialConnectStateMachineLog) << "Skipping mission load" << _lastSkipReason; });

    // State 5: Request geofence (skippable)
    // No timeout: PlanManager handles all timeouts/retries internally and always signals completion.
    _stateGeoFence = new AsyncFunctionState(QStringLiteral("RequestGeoFence"), this,
                                            [this](AsyncFunctionState* state) { _requestGeoFence(state); });
    _stateGeoFence->setSkipPredicate(
        [this]() {
            if (_shouldSkipForPlanLoad()) {
                return true;
            }
            if (!vehicle()->_geoFenceManager->supported()) {
                _lastSkipReason = QStringLiteral("(not supported by vehicle)");
                return true;
            }
            return false;
        },
        [this]() { qCDebug(InitialConnectStateMachineLog) << "Skipping geofence load" << _lastSkipReason; });

    // State 6: Request rally points (skippable)
    // No timeout: PlanManager handles all timeouts/retries internally and always signals completion.
    _stateRallyPoints = new AsyncFunctionState(QStringLiteral("RequestRallyPoints"), this,
                                               [this](AsyncFunctionState* state) { _requestRallyPoints(state); });
    _stateRallyPoints->setSkipPredicate(
        [this]() {
            if (_shouldSkipForPlanLoad()) {
                return true;
            }
            if (!vehicle()->_rallyPointManager->supported()) {
                _lastSkipReason = QStringLiteral("(not supported by vehicle)");
                return true;
            }
            return false;
        },
        [this]() {
            qCDebug(InitialConnectStateMachineLog) << "Skipping rally points load" << _lastSkipReason;
            // Mark plan request complete when skipping
            vehicle()->_initialPlanRequestComplete = true;
            emit vehicle()->initialPlanRequestCompleteChanged(true);
        });

    // State 7: Signal completion
    _stateComplete = new FunctionState(QStringLiteral("SignalComplete"), this, [this]() { _signalComplete(); });

    // Final state
    _stateFinal = new QGCFinalState(QStringLiteral("Final"), this);
}

void InitialConnectStateMachine::_wireTransitions()
{
    // Linear progression
    _stateAutopilotVersion->addTransition(_stateAutopilotVersion, &QGCState::advance, _stateStandardModes);
    _stateStandardModes->addTransition(_stateStandardModes, &QGCState::advance, _stateCompInfo);
    _stateCompInfo->addTransition(_stateCompInfo, &QGCState::advance, _stateParameters);

    _stateParameters->addTransition(_stateParameters, &QGCState::advance, _stateMission);
    _stateMission->addTransition(_stateMission, &QGCState::advance, _stateGeoFence);
    _stateGeoFence->addTransition(_stateGeoFence, &QGCState::advance, _stateRallyPoints);
    _stateRallyPoints->addTransition(_stateRallyPoints, &QGCState::advance, _stateComplete);

    // Complete -> Final (FunctionState emits advance() after its function)
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

bool InitialConnectStateMachine::_shouldSkipForArmed() const
{
    if (!SettingsManager::instance()->mavlinkSettings()->noInitialDownloadWhenArmed()->rawValue().toBool()) {
        return false;
    }
    return vehicle()->armed();
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

bool InitialConnectStateMachine::_shouldSkipForPlanLoad()
{
    if (_shouldSkipForArmed()) {
        _lastSkipReason = QStringLiteral("(vehicle is armed)");
        return true;
    }
    if (!_hasPrimaryLink()) {
        _lastSkipReason = QStringLiteral("(no primary link)");
        return true;
    }
    if (_shouldSkipForLinkType()) {
        _lastSkipReason = QStringLiteral("(high latency or log replay link)");
        return true;
    }
    return false;
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

void InitialConnectStateMachine::_requestCompInfo(AsyncFunctionState* state)
{
    qCDebug(InitialConnectStateMachineLog) << "_stateRequestCompInfo";

    state->connectWhileWaiting(vehicle()->_componentInformationManager, &ComponentInformationManager::progressUpdate,
                               [this](float progress) { setSubProgress(progress); });
    state->connectToCompletion(vehicle()->_componentInformationManager, &ComponentInformationManager::requestAllComplete);
    vehicle()->_componentInformationManager->requestAllComponentInformation(nullptr, nullptr);
}

void InitialConnectStateMachine::_requestParameters(AsyncFunctionState* state)
{
    qCDebug(InitialConnectStateMachineLog) << "_stateRequestParameters";

    const bool cacheOnly = _shouldSkipForArmed();
    if (cacheOnly) {
        // If cache-only check fails (miss/timeout/non-PX4), complete the state without params
        state->connectToCompletion(vehicle()->_parameterManager, &ParameterManager::cacheCheckOnlyFailed,
                                   [state, this]() {
                                       qCDebug(InitialConnectStateMachineLog)
                                           << "Parameter cache check failed while armed, advancing without parameters";
                                       vehicle()->_parameterManager->setParameterDownloadSkipped(true);
                                       state->complete();
                                   });
    }

    state->connectWhileWaiting(vehicle()->_parameterManager, &ParameterManager::loadProgressChanged,
                               [this](float progress) { setSubProgress(progress); });

    // If the vehicle never answers PARAM_REQUEST_LIST, advance without parameters
    state->connectToCompletion(
        vehicle()->_parameterManager, &ParameterManager::initialParametersRequestFailed, [state]() {
            qCDebug(InitialConnectStateMachineLog) << "Initial parameter request failed, advancing without parameters";
            state->complete();
        });

    state->connectToCompletion(vehicle()->_parameterManager, &ParameterManager::parametersReadyChanged,
        [this](bool parametersReady) {
            _onParametersReady(parametersReady);
        });

    if (cacheOnly) {
        vehicle()->_parameterManager->tryHashCheckCacheLoad();
    } else {
        vehicle()->_parameterManager->refreshAllParameters(MAV_COMP_ID_ALL);
    }
}

void InitialConnectStateMachine::_onParametersReady(bool parametersReady)
{
    qCDebug(InitialConnectStateMachineLog) << "_onParametersReady" << parametersReady;

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

void InitialConnectStateMachine::_requestMission(AsyncFunctionState* state)
{
    qCDebug(InitialConnectStateMachineLog) << "_stateRequestMission";

    state->connectWhileWaiting(vehicle()->_missionManager, &MissionManager::progressPctChanged,
                               [this](double progress) { setSubProgress(static_cast<float>(progress)); });
    state->connectToCompletion(vehicle()->_missionManager, &MissionManager::newMissionItemsAvailable);

    vehicle()->_missionManager->loadFromVehicle();
}

void InitialConnectStateMachine::_requestGeoFence(AsyncFunctionState* state)
{
    qCDebug(InitialConnectStateMachineLog) << "_stateRequestGeoFence";

    state->connectWhileWaiting(vehicle()->_geoFenceManager, &GeoFenceManager::progressPctChanged,
                               [this](double progress) { setSubProgress(static_cast<float>(progress)); });
    state->connectToCompletion(vehicle()->_geoFenceManager, &GeoFenceManager::loadComplete);

    vehicle()->_geoFenceManager->loadFromVehicle();
}

void InitialConnectStateMachine::_requestRallyPoints(AsyncFunctionState* state)
{
    qCDebug(InitialConnectStateMachineLog) << "_stateRequestRallyPoints";

    state->connectWhileWaiting(vehicle()->_rallyPointManager, &RallyPointManager::progressPctChanged,
                               [this](double progress) { setSubProgress(static_cast<float>(progress)); });
    state->connectToCompletion(vehicle()->_rallyPointManager, &RallyPointManager::loadComplete,
        [this]() {
            // Mark initial plan request complete
            vehicle()->_initialPlanRequestComplete = true;
            emit vehicle()->initialPlanRequestCompleteChanged(true);

            if (_stateRallyPoints) {
                _stateRallyPoints->complete();
            }
        });

    vehicle()->_rallyPointManager->loadFromVehicle();
}

void InitialConnectStateMachine::_signalComplete()
{
    qCDebug(InitialConnectStateMachineLog) << "Signalling initialConnectComplete";
    connect(this, &QStateMachine::finished, vehicle(), [this]() {
        emit vehicle()->initialConnectComplete();
    }, Qt::SingleShotConnection);
}
