#include "PowerComponentController.h"
#include "PowerCalibrationStateMachine.h"
#include "Vehicle.h"

PowerComponentController::PowerComponentController()
    : FactPanelController()
{
}

void PowerComponentController::calibrateEsc()
{
    if (!_stateMachine) {
        _stateMachine = new PowerCalibrationStateMachine(this, _vehicle, this);

        // Forward signals from state machine to controller
        connect(_stateMachine, &PowerCalibrationStateMachine::connectBattery, this, &PowerComponentController::connectBattery);
        connect(_stateMachine, &PowerCalibrationStateMachine::batteryConnected, this, &PowerComponentController::batteryConnected);
        connect(_stateMachine, &PowerCalibrationStateMachine::disconnectBattery, this, &PowerComponentController::disconnectBattery);
        connect(_stateMachine, &PowerCalibrationStateMachine::calibrationFailed, this, &PowerComponentController::calibrationFailed);
        connect(_stateMachine, &PowerCalibrationStateMachine::calibrationSuccess, this, &PowerComponentController::calibrationSuccess);
        connect(_stateMachine, &PowerCalibrationStateMachine::oldFirmware, this, &PowerComponentController::oldFirmware);
        connect(_stateMachine, &PowerCalibrationStateMachine::newerFirmware, this, &PowerComponentController::newerFirmware);
        connect(_stateMachine, &PowerCalibrationStateMachine::incorrectFirmwareRevReporting, this, &PowerComponentController::incorrectFirmwareRevReporting);

        connect(_vehicle, &Vehicle::textMessageReceived, this, &PowerComponentController::_handleVehicleTextMessage);
    }

    _stateMachine->startEscCalibration();
}

void PowerComponentController::startBusConfigureActuators()
{
    if (!_stateMachine) {
        _stateMachine = new PowerCalibrationStateMachine(this, _vehicle, this);

        // Forward signals from state machine to controller
        connect(_stateMachine, &PowerCalibrationStateMachine::connectBattery, this, &PowerComponentController::connectBattery);
        connect(_stateMachine, &PowerCalibrationStateMachine::batteryConnected, this, &PowerComponentController::batteryConnected);
        connect(_stateMachine, &PowerCalibrationStateMachine::disconnectBattery, this, &PowerComponentController::disconnectBattery);
        connect(_stateMachine, &PowerCalibrationStateMachine::calibrationFailed, this, &PowerComponentController::calibrationFailed);
        connect(_stateMachine, &PowerCalibrationStateMachine::calibrationSuccess, this, &PowerComponentController::calibrationSuccess);
        connect(_stateMachine, &PowerCalibrationStateMachine::oldFirmware, this, &PowerComponentController::oldFirmware);
        connect(_stateMachine, &PowerCalibrationStateMachine::newerFirmware, this, &PowerComponentController::newerFirmware);
        connect(_stateMachine, &PowerCalibrationStateMachine::incorrectFirmwareRevReporting, this, &PowerComponentController::incorrectFirmwareRevReporting);

        connect(_vehicle, &Vehicle::textMessageReceived, this, &PowerComponentController::_handleVehicleTextMessage);
    }

    _stateMachine->startBusConfig();
}

void PowerComponentController::stopBusConfigureActuators()
{
    if (_stateMachine) {
        _stateMachine->stop();
    }
}

void PowerComponentController::_handleVehicleTextMessage(int vehicleId, int compId, int severity, QString text, const QString& description)
{
    Q_UNUSED(compId);
    Q_UNUSED(severity);
    Q_UNUSED(description);

    if (vehicleId != _vehicle->id()) {
        return;
    }

    if (_stateMachine && _stateMachine->isCalibrating()) {
        _stateMachine->handleTextMessage(text);
    }
}
