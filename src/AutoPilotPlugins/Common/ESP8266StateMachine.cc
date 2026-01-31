#include "ESP8266StateMachine.h"
#include "ESP8266ComponentController.h"
#include "ParameterManager.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(ESP8266StateMachineLog, "AutoPilotPlugins.ESP8266StateMachine")

ESP8266StateMachine::ESP8266StateMachine(ESP8266ComponentController* controller, QObject* parent)
    : QGCStateMachine("ESP8266", nullptr, parent)
    , _controller(controller)
{
    _buildStateMachine();
    _setupTransitions();
}

void ESP8266StateMachine::_buildStateMachine()
{
    // Create states
    _idleState = new QGCState("Idle", this);
    _rebootingState = new QGCState("Rebooting", this);
    _restoringState = new QGCState("Restoring", this);
    _completeState = new QGCFinalState("Complete", this);

    setInitialState(_idleState);

    // Idle state entry
    connect(_idleState, &QAbstractState::entered, this, [this]() {
        qCDebug(ESP8266StateMachineLog) << "Entered Idle state";
        _currentOperation = Operation::None;
        _retryCount = 0;
    });

    // Rebooting state entry - send the reboot command
    connect(_rebootingState, &QAbstractState::entered, this, [this]() {
        qCDebug(ESP8266StateMachineLog) << "Entered Rebooting state";
        _currentOperation = Operation::Reboot;
        _sendRebootCommand();
    });

    // Restoring state entry - send the restore command
    connect(_restoringState, &QAbstractState::entered, this, [this]() {
        qCDebug(ESP8266StateMachineLog) << "Entered Restoring state";
        _currentOperation = Operation::Restore;
        _sendRestoreCommand();
    });

    // Complete state entry - notify and restart
    connect(_completeState, &QAbstractState::entered, this, [this]() {
        qCDebug(ESP8266StateMachineLog) << "Operation complete";

        // If restore operation, refresh parameters
        if (_currentOperation == Operation::Restore) {
            _controller->_vehicle->parameterManager()->refreshAllParameters(ESP8266ComponentController::componentID());
        }

        emit operationComplete(true);
        emit busyChanged();

        // Restart machine to return to idle
        stop();
        start();
    });
}

void ESP8266StateMachine::_setupTransitions()
{
    // Idle -> Rebooting
    _idleState->addTransition(new MachineEventTransition("start_reboot", _rebootingState));

    // Idle -> Restoring
    _idleState->addTransition(new MachineEventTransition("start_restore", _restoringState));

    // Rebooting -> Complete (on success)
    _rebootingState->addTransition(new MachineEventTransition("ack_received", _completeState));

    // Restoring -> Complete (on success)
    _restoringState->addTransition(new MachineEventTransition("ack_received", _completeState));

    // Timeout transitions with retry
    addTimeoutTransition(_rebootingState, _commandTimeoutMs, _rebootingState);
    addTimeoutTransition(_restoringState, _commandTimeoutMs, _restoringState);

    // Failed transitions back to idle
    _rebootingState->addTransition(new MachineEventTransition("failed", _idleState));
    _restoringState->addTransition(new MachineEventTransition("failed", _idleState));
}

void ESP8266StateMachine::startReboot()
{
    qCDebug(ESP8266StateMachineLog) << "Starting reboot operation";

    if (!isRunning()) {
        start();
    }

    _retryCount = 0;
    emit busyChanged();
    postEvent("start_reboot");
}

void ESP8266StateMachine::startRestore()
{
    qCDebug(ESP8266StateMachineLog) << "Starting restore defaults operation";

    if (!isRunning()) {
        start();
    }

    _retryCount = 0;
    emit busyChanged();
    postEvent("start_restore");
}

void ESP8266StateMachine::handleCommandResult(int command, int result)
{
    qCDebug(ESP8266StateMachineLog) << "Command result:" << command << "result:" << result;

    // Check if this is the command we're waiting for
    bool isRebootCommand = (command == MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN);
    bool isRestoreCommand = (command == MAV_CMD_PREFLIGHT_STORAGE);

    if (_currentOperation == Operation::Reboot && isRebootCommand) {
        if (result == MAV_RESULT_ACCEPTED) {
            postEvent("ack_received");
        } else {
            qCWarning(ESP8266StateMachineLog) << "Reboot command rejected, result:" << result;
            _retryCount++;
            if (_retryCount >= _maxRetries) {
                emit operationComplete(false);
                emit busyChanged();
                postEvent("failed");
            }
            // Otherwise, timeout will trigger retry
        }
    } else if (_currentOperation == Operation::Restore && isRestoreCommand) {
        if (result == MAV_RESULT_ACCEPTED) {
            postEvent("ack_received");
        } else {
            qCWarning(ESP8266StateMachineLog) << "Restore command rejected, result:" << result;
            _retryCount++;
            if (_retryCount >= _maxRetries) {
                emit operationComplete(false);
                emit busyChanged();
                postEvent("failed");
            }
            // Otherwise, timeout will trigger retry
        }
    }
}

bool ESP8266StateMachine::isBusy() const
{
    return isRunning() && (isStateActive(_rebootingState) || isStateActive(_restoringState));
}

void ESP8266StateMachine::_sendRebootCommand()
{
    qCDebug(ESP8266StateMachineLog) << "Sending reboot command, attempt:" << (_retryCount + 1);

    if (_retryCount >= _maxRetries) {
        qCWarning(ESP8266StateMachineLog) << "Max retries reached for reboot";
        emit operationComplete(false);
        emit busyChanged();
        postEvent("failed");
        return;
    }

    _controller->_vehicle->sendMavCommand(
        ESP8266ComponentController::componentID(),
        MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN,
        true,  // showError
        0.0f,  // param1
        1.0f   // param2 - reboot companion computer
    );
}

void ESP8266StateMachine::_sendRestoreCommand()
{
    qCDebug(ESP8266StateMachineLog) << "Sending restore defaults command, attempt:" << (_retryCount + 1);

    if (_retryCount >= _maxRetries) {
        qCWarning(ESP8266StateMachineLog) << "Max retries reached for restore";
        emit operationComplete(false);
        emit busyChanged();
        postEvent("failed");
        return;
    }

    _controller->_vehicle->sendMavCommand(
        ESP8266ComponentController::componentID(),
        MAV_CMD_PREFLIGHT_STORAGE,
        true,  // showError
        2.0f   // param1 - reset to defaults
    );
}
