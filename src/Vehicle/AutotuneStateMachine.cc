#include "AutotuneStateMachine.h"
#include "Autotune.h"
#include "Vehicle.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(AutotuneStateMachineLog, "Vehicle.AutotuneStateMachine")

AutotuneStateMachine::AutotuneStateMachine(Autotune* autotune, Vehicle* vehicle, QObject* parent)
    : QGCStateMachine("Autotune", vehicle, parent)
    , _autotune(autotune)
    , _vehicle(vehicle)
    , _statusString(tr("Autotune: Not performed"))
{
    _buildStateMachine();
    _wireTransitions();
}

AutotuneStateMachine::~AutotuneStateMachine()
{
    qCDebug(AutotuneStateMachineLog) << "Destroying autotune state machine";
}

void AutotuneStateMachine::_buildStateMachine()
{
    // Idle state - waiting to start
    _idleState = new QGCState("Idle", this);
    connect(_idleState, &QAbstractState::entered, this, [this]() {
        qCDebug(AutotuneStateMachineLog) << "Idle state";
        _inProgress = false;
        _progress = 0.0f;
        _disarmMessageDisplayed = false;
    });

    // Progress monitoring states - poll and wait for external progress events
    _initializingState = new QGCState("Initializing", this);
    connect(_initializingState, &QAbstractState::entered, this, &AutotuneStateMachine::_onInitializingEntered);

    _rollState = new QGCState("Roll", this);
    connect(_rollState, &QAbstractState::entered, this, &AutotuneStateMachine::_onRollEntered);

    _pitchState = new QGCState("Pitch", this);
    connect(_pitchState, &QAbstractState::entered, this, &AutotuneStateMachine::_onPitchEntered);

    _yawState = new QGCState("Yaw", this);
    connect(_yawState, &QAbstractState::entered, this, &AutotuneStateMachine::_onYawEntered);

    _waitForDisarmState = new QGCState("WaitForDisarm", this);
    connect(_waitForDisarmState, &QAbstractState::entered, this, &AutotuneStateMachine::_onWaitForDisarmEntered);

    // Terminal states using semantic FunctionState
    _successState = new FunctionState("Success", this, [this]() { _onSuccess(); });
    _failedState = new FunctionState("Failed", this, [this]() { _onFailed(); });
    _finalState = new QGCFinalState("Final", this);

    setInitialState(_idleState);

    // Set progress weights for automatic progress calculation
    setProgressWeights({
        {_initializingState, 20},
        {_rollState, 20},
        {_pitchState, 20},
        {_yawState, 20},
        {_waitForDisarmState, 20}
    });
}

void AutotuneStateMachine::_wireTransitions()
{
    // Idle -> Initializing (on start)
    _idleState->addTransition(new MachineEventTransition("start", _initializingState));

    // Progress-based transitions
    _initializingState->addTransition(new MachineEventTransition("roll", _rollState));
    _rollState->addTransition(new MachineEventTransition("pitch", _pitchState));
    _pitchState->addTransition(new MachineEventTransition("yaw", _yawState));
    _yawState->addTransition(new MachineEventTransition("wait_disarm", _waitForDisarmState));
    _waitForDisarmState->addTransition(new MachineEventTransition("success", _successState));

    // Any active state -> Failed (on failure)
    for (auto* state : {_initializingState, _rollState, _pitchState, _yawState, _waitForDisarmState}) {
        state->addTransition(new MachineEventTransition("failed", _failedState));
    }

    // Polling timeouts - send command again (loops back to self)
    for (auto* state : {_initializingState, _rollState, _pitchState, _yawState, _waitForDisarmState}) {
        addTimeoutTransition(state, _pollIntervalMs, state);
    }

    // Terminal states -> Final
    _successState->addTransition(_successState, &QGCState::advance, _finalState);
    _failedState->addTransition(_failedState, &QGCState::advance, _finalState);
}

void AutotuneStateMachine::_onInitializingEntered()
{
    qCDebug(AutotuneStateMachineLog) << "Initializing state";
    _inProgress = true;
    _statusString = tr("Autotune: initializing");
    emit autotuneChanged();
    _sendAutotuneCommand();
}

void AutotuneStateMachine::_onRollEntered()
{
    qCDebug(AutotuneStateMachineLog) << "Roll tuning state";
    _statusString = tr("Autotune: roll");
    emit autotuneChanged();
}

void AutotuneStateMachine::_onPitchEntered()
{
    qCDebug(AutotuneStateMachineLog) << "Pitch tuning state";
    _statusString = tr("Autotune: pitch");
    emit autotuneChanged();
}

void AutotuneStateMachine::_onYawEntered()
{
    qCDebug(AutotuneStateMachineLog) << "Yaw tuning state";
    _statusString = tr("Autotune: yaw");
    emit autotuneChanged();
}

void AutotuneStateMachine::_onWaitForDisarmEntered()
{
    qCDebug(AutotuneStateMachineLog) << "Wait for disarm state";
    _statusString = tr("Wait for disarm");

    if (!_disarmMessageDisplayed) {
        qgcApp()->showAppMessage(tr("Land and disarm the vehicle in order to apply the parameters."));
        _disarmMessageDisplayed = true;
        emit disarmRequired();
    }
    emit autotuneChanged();
}

void AutotuneStateMachine::_onSuccess()
{
    qCDebug(AutotuneStateMachineLog) << "Success state";
    _inProgress = false;
    _progress = 1.0f;
    _statusString = tr("Autotune: Success");
    qgcApp()->showAppMessage(tr("Autotune successful."));
    emit autotuneChanged();
    emit autotuneComplete(true);
}

void AutotuneStateMachine::_onFailed()
{
    qCDebug(AutotuneStateMachineLog) << "Failed state";
    _inProgress = false;
    _disarmMessageDisplayed = false;
    emit autotuneChanged();
    emit autotuneComplete(false);
}

void AutotuneStateMachine::startAutotune()
{
    qCDebug(AutotuneStateMachineLog) << "Starting autotune";

    _progress = 0.0f;
    _disarmMessageDisplayed = false;
    _statusString = tr("Autotune: In progress");

    if (!isRunning()) {
        start();
    }

    postEvent("start");
}

void AutotuneStateMachine::handleProgress(uint8_t progress)
{
    qCDebug(AutotuneStateMachineLog) << "Progress update:" << progress;

    _progress = progress / 100.0f;
    _updateStatusForProgress(progress);

    // Determine which state transition to trigger based on progress
    if (progress >= 100) {
        postEvent("success");
    } else if (progress >= 80) {
        if (!isStateActive(_waitForDisarmState)) {
            postEvent("wait_disarm");
        }
    } else if (progress >= 60) {
        if (!isStateActive(_yawState) && !isStateActive(_waitForDisarmState)) {
            postEvent("yaw");
        }
    } else if (progress >= 40) {
        if (!isStateActive(_pitchState) && !isStateActive(_yawState) && !isStateActive(_waitForDisarmState)) {
            postEvent("pitch");
        }
    } else if (progress >= 20) {
        if (!isStateActive(_rollState) && !isStateActive(_pitchState) &&
            !isStateActive(_yawState) && !isStateActive(_waitForDisarmState)) {
            postEvent("roll");
        }
    }

    emit autotuneChanged();
}

void AutotuneStateMachine::handleFailure()
{
    qCDebug(AutotuneStateMachineLog) << "Autotune failed";
    _statusString = tr("Autotune: Failed");
    postEvent("failed");
}

void AutotuneStateMachine::handleError(uint8_t errorCode)
{
    qCDebug(AutotuneStateMachineLog) << "Autotune error:" << errorCode;
    _statusString = tr("Autotune: Ack error %1").arg(errorCode);
    postEvent("failed");
}

void AutotuneStateMachine::_updateStatusForProgress(uint8_t progress)
{
    if (progress < 20) {
        _statusString = tr("Autotune: initializing");
    } else if (progress < 40) {
        _statusString = tr("Autotune: roll");
    } else if (progress < 60) {
        _statusString = tr("Autotune: pitch");
    } else if (progress < 80) {
        _statusString = tr("Autotune: yaw");
    } else if (progress == 95) {
        _statusString = tr("Wait for disarm");
    } else if (progress < 100) {
        _statusString = tr("Autotune: in progress");
    } else if (progress == 100) {
        _statusString = tr("Autotune: Success");
    } else {
        _statusString = tr("Autotune: Unknown error");
    }
}

void AutotuneStateMachine::_sendAutotuneCommand()
{
    qCDebug(AutotuneStateMachineLog) << "Sending autotune command";

    // Use the Autotune class's static handlers
    Vehicle::MavCmdAckHandlerInfo_t handlerInfo = {};
    handlerInfo.resultHandler = Autotune::ackHandler;
    handlerInfo.resultHandlerData = _autotune;
    handlerInfo.progressHandler = Autotune::progressHandler;
    handlerInfo.progressHandlerData = _autotune;

    _vehicle->sendMavCommandWithHandler(
        &handlerInfo,
        MAV_COMP_ID_AUTOPILOT1,
        MAV_CMD_DO_AUTOTUNE_ENABLE,
        1,  // request autotune
        0, 0, 0, 0, 0, 0);
}
