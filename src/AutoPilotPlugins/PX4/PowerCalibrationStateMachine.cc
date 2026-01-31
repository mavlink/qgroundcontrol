#include "PowerCalibrationStateMachine.h"
#include "PowerComponentController.h"
#include "Vehicle.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(PowerCalibrationStateMachineLog, "AutoPilotPlugin.PX4.PowerCalibrationStateMachine")

PowerCalibrationStateMachine::PowerCalibrationStateMachine(PowerComponentController* controller, Vehicle* vehicle, QObject* parent)
    : QGCStateMachine("PowerCalibration", vehicle, parent)
    , _controller(controller)
    , _vehicle(vehicle)
{
    _buildStateMachine();
    _wireTransitions();
}

PowerCalibrationStateMachine::~PowerCalibrationStateMachine()
{
    qCDebug(PowerCalibrationStateMachineLog) << "Destroying power calibration state machine";
}

void PowerCalibrationStateMachine::_buildStateMachine()
{
    // Message-driven states
    _idleState = new QGCState("Idle", this);
    connect(_idleState, &QAbstractState::entered, this, &PowerCalibrationStateMachine::_onIdleEntered);

    _startedState = new QGCState("Started", this);
    connect(_startedState, &QAbstractState::entered, this, &PowerCalibrationStateMachine::_onStartedEntered);

    _waitingBatteryState = new QGCState("WaitingBattery", this);
    connect(_waitingBatteryState, &QAbstractState::entered, this, &PowerCalibrationStateMachine::_onWaitingBatteryEntered);

    _batteryConnectedState = new QGCState("BatteryConnected", this);
    connect(_batteryConnectedState, &QAbstractState::entered, this, &PowerCalibrationStateMachine::_onBatteryConnectedEntered);

    // Terminal states using semantic FunctionState
    _successState = new FunctionState("Success", this, [this]() { _onSuccess(); });
    _failedState = new FunctionState("Failed", this, [this]() { _onFailed(); });
    _finalState = new QGCFinalState("Final", this);

    setInitialState(_idleState);

    // Set progress weights
    setProgressWeights({
        {_startedState, 20},
        {_waitingBatteryState, 30},
        {_batteryConnectedState, 50}
    });
}

void PowerCalibrationStateMachine::_wireTransitions()
{
    // Idle -> Started (on start)
    _idleState->addTransition(new MachineEventTransition("start", _startedState));

    // Started -> WaitingBattery (on connect_battery message)
    _startedState->addTransition(new MachineEventTransition("connect_battery", _waitingBatteryState));

    // Started -> Success (for bus config which may not need battery)
    _startedState->addTransition(new MachineEventTransition("success", _successState));

    // WaitingBattery -> BatteryConnected
    _waitingBatteryState->addTransition(new MachineEventTransition("battery_connected", _batteryConnectedState));

    // BatteryConnected -> Success
    _batteryConnectedState->addTransition(new MachineEventTransition("success", _successState));

    // Any state -> Failed
    for (auto* state : {_startedState, _waitingBatteryState, _batteryConnectedState}) {
        state->addTransition(new MachineEventTransition("failed", _failedState));
        state->addTransition(new MachineEventTransition("disconnect_battery", _failedState));
    }

    // Timeout transitions
    for (auto* state : {_startedState, _waitingBatteryState, _batteryConnectedState}) {
        addTimeoutTransition(state, _timeoutMs, _failedState);
    }

    // Terminal states -> Final
    _successState->addTransition(_successState, &QGCState::advance, _finalState);
    _failedState->addTransition(_failedState, &QGCState::advance, _finalState);
}

void PowerCalibrationStateMachine::_onIdleEntered()
{
    qCDebug(PowerCalibrationStateMachineLog) << "Idle state";
    _calibrating = false;
    _calibrationType = CalibrationNone;
    _warningMessages.clear();
    _failureMessage.clear();
}

void PowerCalibrationStateMachine::_onStartedEntered()
{
    qCDebug(PowerCalibrationStateMachineLog) << "Started state - calibration type:" << _calibrationType;
    _calibrating = true;
}

void PowerCalibrationStateMachine::_onWaitingBatteryEntered()
{
    qCDebug(PowerCalibrationStateMachineLog) << "Waiting for battery connection";
    emit connectBattery();
}

void PowerCalibrationStateMachine::_onBatteryConnectedEntered()
{
    qCDebug(PowerCalibrationStateMachineLog) << "Battery connected, calibrating";
    emit batteryConnected();
}

void PowerCalibrationStateMachine::_onSuccess()
{
    qCDebug(PowerCalibrationStateMachineLog) << "Calibration success";
    _calibrating = false;
    emit calibrationSuccess(_warningMessages);
}

void PowerCalibrationStateMachine::_onFailed()
{
    qCDebug(PowerCalibrationStateMachineLog) << "Calibration failed:" << _failureMessage;
    _calibrating = false;
    emit calibrationFailed(_failureMessage);
}

void PowerCalibrationStateMachine::startEscCalibration()
{
    qCDebug(PowerCalibrationStateMachineLog) << "Starting ESC calibration";

    _warningMessages.clear();
    _failureMessage.clear();
    _calibrationType = CalibrationEsc;

    if (!isRunning()) {
        start();
    }

    _vehicle->startCalibration(QGCMAVLink::CalibrationEsc);
    postEvent("start");
}

void PowerCalibrationStateMachine::startBusConfig()
{
    qCDebug(PowerCalibrationStateMachineLog) << "Starting UAVCAN bus configuration";

    _warningMessages.clear();
    _failureMessage.clear();
    _calibrationType = CalibrationBusConfig;

    if (!isRunning()) {
        start();
    }

    _vehicle->startUAVCANBusConfig();
    postEvent("start");
}

void PowerCalibrationStateMachine::stopCalibration()
{
    qCDebug(PowerCalibrationStateMachineLog) << "Stopping calibration";

    if (_calibrationType == CalibrationBusConfig) {
        _vehicle->stopUAVCANBusConfig();
    }

    _calibrating = false;
    _calibrationType = CalibrationNone;

    if (isRunning()) {
        QGCStateMachine::stop();
    }
}

void PowerCalibrationStateMachine::handleTextMessage(const QString& text)
{
    // All calibration messages start with [cal]
    const QString calPrefix("[cal] ");
    if (!text.startsWith(calPrefix)) {
        return;
    }

    QString message = text.mid(calPrefix.length());
    _parseCalibrationMessage(message);
}

void PowerCalibrationStateMachine::_parseCalibrationMessage(const QString& text)
{
    qCDebug(PowerCalibrationStateMachineLog) << "Parsing calibration message:" << text;

    // Check for calibration started message
    const QString calStartPrefix("calibration started: ");
    if (text.startsWith(calStartPrefix)) {
        QString versionInfo = text.mid(calStartPrefix.length());
        QStringList parts = versionInfo.split(" ");

        if (parts.count() != 2) {
            emit incorrectFirmwareRevReporting();
            return;
        }

        // Firmware version check (currently disabled in original code)
#if 0
        int firmwareRev = parts[0].toInt();
        if (firmwareRev < _neededFirmwareRev) {
            emit oldFirmware();
            return;
        }
        if (firmwareRev > _neededFirmwareRev) {
            emit newerFirmware();
            return;
        }
#endif
        return;
    }

    // Connect battery request
    if (text == "Connect battery now") {
        postEvent("connect_battery");
        return;
    }

    // Battery connected
    if (text == "Battery connected") {
        postEvent("battery_connected");
        return;
    }

    // Calibration failed
    const QString failedPrefix("calibration failed: ");
    if (text.startsWith(failedPrefix)) {
        QString failureText = text.mid(failedPrefix.length());

        if (failureText.startsWith("Disconnect battery")) {
            _failureMessage = failureText;
            emit disconnectBattery();
            postEvent("disconnect_battery");
        } else {
            _failureMessage = failureText;
            postEvent("failed");
        }
        return;
    }

    // Calibration done
    const QString calCompletePrefix("calibration done:");
    if (text.startsWith(calCompletePrefix)) {
        postEvent("success");
        return;
    }

    // Config warning
    const QString warningPrefix("config warning: ");
    if (text.startsWith(warningPrefix)) {
        _warningMessages << text.mid(warningPrefix.length());
        return;
    }

    // Bus config failed
    const QString busFailedPrefix("bus conf fail:");
    if (text.startsWith(busFailedPrefix)) {
        _failureMessage = text.mid(busFailedPrefix.length());
        postEvent("failed");
        return;
    }

    // Bus config warning
    const QString busWarningPrefix("bus conf warn: ");
    if (text.startsWith(busWarningPrefix)) {
        _warningMessages << text.mid(busWarningPrefix.length());
        return;
    }
}
