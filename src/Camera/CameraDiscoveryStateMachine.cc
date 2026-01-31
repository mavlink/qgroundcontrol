#include "CameraDiscoveryStateMachine.h"
#include "QGCCameraManager.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(CameraDiscoveryStateMachineLog, "Camera.CameraDiscoveryStateMachine")

// Forward declarations for request handlers
static void _requestCameraInfoMessageResultHandler(void* resultHandlerData, MAV_RESULT result,
    Vehicle::RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t& message);
static void _requestCameraInfoCommandResultHandler(void* resultHandlerData, int compId,
    const mavlink_command_ack_t& ack, Vehicle::MavCmdResultFailureCode_t failureCode);

CameraDiscoveryStateMachine::CameraDiscoveryStateMachine(QGCCameraManager* manager, uint8_t compId, Vehicle* vehicle, QObject* parent)
    : QGCStateMachine("CameraDiscovery", vehicle, parent)
    , _manager(manager)
    , _vehicle(vehicle)
    , _compId(compId)
{
    _lastHeartbeat.start();
    _buildStateMachine();
    _wireTransitions();
}

CameraDiscoveryStateMachine::~CameraDiscoveryStateMachine()
{
    qCDebug(CameraDiscoveryStateMachineLog) << "Destroying discovery state machine for compId" << _compId;
}

void CameraDiscoveryStateMachine::_buildStateMachine()
{
    // Idle state - simple starting point
    _idleState = new QGCState("Idle", this);
    connect(_idleState, &QAbstractState::entered, this, [this]() {
        qCDebug(CameraDiscoveryStateMachineLog) << "Idle state for compId" << _compId;
    });

    // RequestingInfo - async state that sends request and waits for callback
    _requestingInfoState = new AsyncFunctionState(
        "RequestingInfo", this,
        [this](AsyncFunctionState* state) { _setupRequestingInfo(state); },
        _requestTimeoutMs
    );

    // WaitingBackoff - uses timer for exponential backoff
    _waitingBackoffState = new QGCState("WaitingBackoff", this);
    connect(_waitingBackoffState, &QAbstractState::entered, this, [this]() {
        int backoffMs = _calculateBackoffMs();
        qCDebug(CameraDiscoveryStateMachineLog) << "WaitingBackoff state for compId" << _compId
                                                 << "waiting" << backoffMs << "ms";
        _backoffTimer.start(backoffMs);
    });
    connect(_waitingBackoffState, &QAbstractState::exited, this, [this]() {
        _backoffTimer.stop();
    });

    // Configure backoff timer
    _backoffTimer.setSingleShot(true);
    connect(&_backoffTimer, &QTimer::timeout, this, [this]() {
        postEvent("backoff_complete");
    });

    // Complete - function state that signals success
    _completeState = new FunctionState("Complete", this, [this]() { _onComplete(); });

    // Failed - function state that signals failure
    _failedState = new FunctionState("Failed", this, [this]() { _onFailed(); });

    // Final state
    _finalState = new QGCFinalState("Final", this);

    setInitialState(_idleState);
}

void CameraDiscoveryStateMachine::_wireTransitions()
{
    // Idle -> RequestingInfo (on start)
    _idleState->addTransition(new MachineEventTransition("start", _requestingInfoState));

    // RequestingInfo -> Complete (on completed - info received)
    _requestingInfoState->addTransition(_requestingInfoState, &WaitStateBase::completed, _completeState);

    // RequestingInfo -> WaitingBackoff (on failed/timeout, if retries remain)
    _requestingInfoState->addTransition(_requestingInfoState, &WaitStateBase::timedOut, _waitingBackoffState);
    _requestingInfoState->addTransition(new MachineEventTransition("retry", _waitingBackoffState));

    // RequestingInfo -> Failed (on max_retries)
    _requestingInfoState->addTransition(new MachineEventTransition("max_retries", _failedState));

    // WaitingBackoff -> RequestingInfo (on backoff timer complete)
    _waitingBackoffState->addTransition(new MachineEventTransition("backoff_complete", _requestingInfoState));

    // Complete -> Final
    _completeState->addTransition(_completeState, &QGCState::advance, _finalState);

    // Failed -> Final
    _failedState->addTransition(_failedState, &QGCState::advance, _finalState);
}

void CameraDiscoveryStateMachine::_setupRequestingInfo(AsyncFunctionState* state)
{
    Q_UNUSED(state)
    qCDebug(CameraDiscoveryStateMachineLog) << "RequestingInfo state for compId" << _compId
                                             << "attempt" << (_retryCount + 1);
    _sendInfoRequest();
}

void CameraDiscoveryStateMachine::_onComplete()
{
    qCDebug(CameraDiscoveryStateMachineLog) << "Discovery complete for compId" << _compId;
    _infoReceived = true;
    emit discoveryComplete(_compId);
}

void CameraDiscoveryStateMachine::_onFailed()
{
    qCDebug(CameraDiscoveryStateMachineLog) << "Discovery failed for compId" << _compId
                                             << "after" << _retryCount << "attempts";
    emit discoveryFailed(_compId);
}

void CameraDiscoveryStateMachine::startDiscovery()
{
    qCDebug(CameraDiscoveryStateMachineLog) << "Starting discovery for compId" << _compId;

    _retryCount = 0;
    _infoReceived = false;
    _lastHeartbeat.start();

    if (!isRunning()) {
        start();
    }

    postEvent("start");
}

void CameraDiscoveryStateMachine::handleInfoReceived()
{
    qCDebug(CameraDiscoveryStateMachineLog) << "Info received for compId" << _compId;

    if (isStateActive(_requestingInfoState)) {
        _requestingInfoState->complete();
    }
}

void CameraDiscoveryStateMachine::handleRequestFailed()
{
    qCDebug(CameraDiscoveryStateMachineLog) << "Request failed for compId" << _compId
                                             << "retryCount" << _retryCount;

    _retryCount++;

    if (_retryCount >= _maxRetryCount) {
        postEvent("max_retries");
    } else {
        if (isStateActive(_requestingInfoState)) {
            _requestingInfoState->fail();
        }
        postEvent("retry");
    }
}

void CameraDiscoveryStateMachine::handleHeartbeat()
{
    _lastHeartbeat.start();

    // If we timed out but now have heartbeat again, restart discovery
    if (_infoReceived) {
        return;  // Already discovered
    }

    if (!isRunning() || isStateActive(_idleState)) {
        qCDebug(CameraDiscoveryStateMachineLog) << "Heartbeat restarting discovery for compId" << _compId;
        _retryCount = 0;
        startDiscovery();
    }
}

bool CameraDiscoveryStateMachine::isTimedOut() const
{
    return _lastHeartbeat.elapsed() > _silentTimeoutMs;
}

void CameraDiscoveryStateMachine::_sendInfoRequest()
{
    if (_retryCount >= _maxRetryCount) {
        postEvent("max_retries");
        return;
    }

    // Alternate between REQUEST_MESSAGE and REQUEST_CAMERA_INFORMATION
    if ((_retryCount % 2) == 0) {
        qCDebug(CameraDiscoveryStateMachineLog) << "Using MAV_CMD_REQUEST_MESSAGE for compId" << _compId;
        _vehicle->requestMessage(_requestCameraInfoMessageResultHandler, this, _compId, MAVLINK_MSG_ID_CAMERA_INFORMATION);
    } else {
        qCDebug(CameraDiscoveryStateMachineLog) << "Using MAV_CMD_REQUEST_CAMERA_INFORMATION for compId" << _compId;

        Vehicle::MavCmdAckHandlerInfo_t ackHandlerInfo{};
        ackHandlerInfo.resultHandler = _requestCameraInfoCommandResultHandler;
        ackHandlerInfo.resultHandlerData = this;
        ackHandlerInfo.progressHandler = nullptr;
        ackHandlerInfo.progressHandlerData = nullptr;

        _vehicle->sendMavCommandWithHandler(&ackHandlerInfo, _compId, MAV_CMD_REQUEST_CAMERA_INFORMATION, 1);
    }
}

int CameraDiscoveryStateMachine::_calculateBackoffMs() const
{
    // Exponential backoff: 2^(retryCount/2) seconds for even attempts >= 2
    if (_retryCount >= 2 && (_retryCount % 2) == 0) {
        int delaySeconds = 1 << (_retryCount / 2);
        return delaySeconds * 1000;
    }
    return 100;  // Short delay for odd attempts
}

// Static callback handlers
static void _requestCameraInfoMessageResultHandler(void* resultHandlerData, MAV_RESULT result,
    Vehicle::RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t& message)
{
    Q_UNUSED(message);

    auto* stateMachine = static_cast<CameraDiscoveryStateMachine*>(resultHandlerData);
    if (!stateMachine) {
        return;
    }

    if (result != MAV_RESULT_ACCEPTED) {
        qCDebug(CameraDiscoveryStateMachineLog) << "MAV_CMD_REQUEST_MESSAGE failed for compId"
                                                 << stateMachine->compId()
                                                 << "result:" << result
                                                 << "failureCode:" << failureCode;
        stateMachine->handleRequestFailed();
    }
    // Note: Success is handled when CAMERA_INFORMATION message is received
}

static void _requestCameraInfoCommandResultHandler(void* resultHandlerData, int compId,
    const mavlink_command_ack_t& ack, Vehicle::MavCmdResultFailureCode_t failureCode)
{
    Q_UNUSED(compId);

    auto* stateMachine = static_cast<CameraDiscoveryStateMachine*>(resultHandlerData);
    if (!stateMachine) {
        return;
    }

    if (ack.result != MAV_RESULT_ACCEPTED) {
        qCDebug(CameraDiscoveryStateMachineLog) << "MAV_CMD_REQUEST_CAMERA_INFORMATION failed for compId"
                                                 << stateMachine->compId()
                                                 << "result:" << ack.result
                                                 << "failureCode:" << failureCode;
        stateMachine->handleRequestFailed();
    }
    // Note: Success is handled when CAMERA_INFORMATION message is received
}
