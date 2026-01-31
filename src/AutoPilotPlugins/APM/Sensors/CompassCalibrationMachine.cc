#include "CompassCalibrationMachine.h"
#include "APMSensorsComponentController.h"
#include "ParameterManager.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

#include <QtCore/QTimer>

QGC_LOGGING_CATEGORY(CompassCalibrationMachineLog, "APMSensorCalibration.CompassCalibrationMachine")

CompassCalibrationMachine::CompassCalibrationMachine(APMSensorsComponentController* controller, QObject* parent)
    : QGCStateMachine("CompassCalibration", nullptr, parent)
    , _controller(controller)
{
    _buildStateMachine();
    _setupTransitions();
}

void CompassCalibrationMachine::_buildStateMachine()
{
    // Create all states
    _idleState = new QGCState("Idle", this);
    _checkingSupportState = new QGCState("CheckingSupport", this);
    _setupState = new QGCState("Setup", this);
    _rotatingState = new QGCState("Rotating", this);
    _successState = new QGCFinalState("Success", this);
    _failedState = new QGCFinalState("Failed", this);

    setInitialState(_idleState);

    // Configure state entry callbacks
    connect(_idleState, &QAbstractState::entered, this, [this]() {
        qCDebug(CompassCalibrationMachineLog) << "Entered Idle state";
        // Reset compass tracking
        _compassMask = 0;
        for (int i = 0; i < MaxCompasses; i++) {
            _rgProgress[i] = 0;
            _rgComplete[i] = false;
            _rgSucceeded[i] = false;
            _rgFitness[i] = 0.0f;
        }
    });

    connect(_checkingSupportState, &QAbstractState::entered, this, [this]() {
        qCDebug(CompassCalibrationMachineLog) << "Checking onboard compass cal support";
        // The support check is initiated by sending a cancel command and checking the result
        // This is handled externally by the main state machine
    });

    connect(_setupState, &QAbstractState::entered, this, [this]() {
        qCDebug(CompassCalibrationMachineLog) << "Setting up compass calibration";
        _determineCompassesToCalibrate();
        _startMagCal();
    });

    connect(_rotatingState, &QAbstractState::entered, this, [this]() {
        qCDebug(CompassCalibrationMachineLog) << "Entered Rotating state - user should rotate vehicle";
        _controller->appendStatusLog(tr("Rotate the vehicle randomly around all axes until the progress bar fills."));
    });

    connect(_successState, &QAbstractState::entered, this, [this]() {
        qCDebug(CompassCalibrationMachineLog) << "Compass calibration succeeded";
        _restoreCompassCalFitness();

        // Emit individual compass results
        for (int i = 0; i < MaxCompasses; i++) {
            if (_compassMask & (1 << i)) {
                _controller->_rgCompassCalSucceeded[i] = _rgSucceeded[i];
                _controller->_rgCompassCalFitness[i] = _rgFitness[i];
            }
        }

        emit _controller->compass1CalSucceededChanged(_controller->_rgCompassCalSucceeded[0]);
        emit _controller->compass2CalSucceededChanged(_controller->_rgCompassCalSucceeded[1]);
        emit _controller->compass3CalSucceededChanged(_controller->_rgCompassCalSucceeded[2]);
        emit _controller->compass1CalFitnessChanged(_controller->_rgCompassCalFitness[0]);
        emit _controller->compass2CalFitnessChanged(_controller->_rgCompassCalFitness[1]);
        emit _controller->compass3CalFitnessChanged(_controller->_rgCompassCalFitness[2]);

        emit calibrationComplete(true);
    });

    connect(_failedState, &QAbstractState::entered, this, [this]() {
        qCDebug(CompassCalibrationMachineLog) << "Compass calibration failed";
        _restoreCompassCalFitness();
        emit calibrationComplete(false);
    });
}

void CompassCalibrationMachine::_setupTransitions()
{
    // Idle -> CheckingSupport (on start)
    _idleState->addTransition(new MachineEventTransition("start", _checkingSupportState));

    // CheckingSupport -> Setup (on supported)
    _checkingSupportState->addTransition(new MachineEventTransition("supported", _setupState));

    // CheckingSupport -> Failed (on not_supported)
    _checkingSupportState->addTransition(new MachineEventTransition("not_supported", _failedState));

    // Setup -> Rotating (on setup_complete)
    _setupState->addTransition(new MachineEventTransition("setup_complete", _rotatingState));

    // Setup -> Failed (on setup_failed)
    _setupState->addTransition(new MachineEventTransition("setup_failed", _failedState));

    // Rotating -> Success (on all_complete_success)
    _rotatingState->addTransition(new MachineEventTransition("all_complete_success", _successState));

    // Rotating -> Failed (on all_complete_failed)
    _rotatingState->addTransition(new MachineEventTransition("all_complete_failed", _failedState));

    // Cancel transitions
    _checkingSupportState->addTransition(new MachineEventTransition("cancel", _idleState));
    _setupState->addTransition(new MachineEventTransition("cancel", _idleState));
    _rotatingState->addTransition(new MachineEventTransition("cancel", _idleState));
}

void CompassCalibrationMachine::startCalibration()
{
    qCDebug(CompassCalibrationMachineLog) << "Starting compass calibration";

    if (!isRunning()) {
        start();
    }

    QTimer::singleShot(0, this, [this]() {
        postEvent("start");
        emit calibrationStarted();
    });
}

void CompassCalibrationMachine::cancelCalibration()
{
    qCDebug(CompassCalibrationMachineLog) << "Cancelling compass calibration";
    _restoreCompassCalFitness();
    postEvent("cancel");
    emit calibrationCancelled();
}

void CompassCalibrationMachine::handleSupportCheckResult(bool supported)
{
    qCDebug(CompassCalibrationMachineLog) << "Support check result:" << supported;
    if (supported) {
        postEvent("supported");
    } else {
        postEvent("not_supported");
    }
}

void CompassCalibrationMachine::handleMagCalProgress(uint8_t compassId, uint8_t calMask, uint8_t completionPct)
{
    qCDebug(CompassCalibrationMachineLog) << "Compass" << compassId << "progress:" << completionPct << "%";

    if (compassId >= MaxCompasses) {
        return;
    }

    // Calculate how many compasses are being calibrated
    int compassCalCount = 0;
    for (int i = 0; i < MaxCompasses; i++) {
        if (calMask & (1 << i)) {
            compassCalCount++;
        }
    }

    if (compassCalCount > 0) {
        // Each compass contributes proportionally to overall progress
        _rgProgress[compassId] = completionPct / compassCalCount;
    }

    _updateProgressBar();
    emit progressChanged(compassId, completionPct);
}

void CompassCalibrationMachine::handleMagCalReport(uint8_t compassId, uint8_t calStatus, float fitness)
{
    qCDebug(CompassCalibrationMachineLog) << "Compass" << compassId << "report - status:" << calStatus << "fitness:" << fitness;

    if (compassId >= MaxCompasses) {
        return;
    }

    if (!_rgComplete[compassId]) {
        _rgComplete[compassId] = true;
        _rgSucceeded[compassId] = (calStatus == 4); // MAG_CAL_SUCCESS = 4
        _rgFitness[compassId] = fitness;

        if (_rgSucceeded[compassId]) {
            _controller->appendStatusLog(tr("Compass %1 calibration complete").arg(compassId + 1));
        } else {
            _controller->appendStatusLog(tr("Compass %1 calibration below quality threshold").arg(compassId + 1));
        }

        emit compassCalComplete(compassId, _rgSucceeded[compassId], fitness);
        _checkAllCompassesComplete();
    }
}

bool CompassCalibrationMachine::isCalibrating() const
{
    return isRunning() && (isStateActive(_checkingSupportState) || isStateActive(_setupState) || isStateActive(_rotatingState));
}

uint8_t CompassCalibrationMachine::compassProgress(int compassId) const
{
    if (compassId < 0 || compassId >= MaxCompasses) {
        return 0;
    }
    return _rgProgress[compassId];
}

bool CompassCalibrationMachine::compassComplete(int compassId) const
{
    if (compassId < 0 || compassId >= MaxCompasses) {
        return false;
    }
    return _rgComplete[compassId];
}

bool CompassCalibrationMachine::compassSucceeded(int compassId) const
{
    if (compassId < 0 || compassId >= MaxCompasses) {
        return false;
    }
    return _rgSucceeded[compassId];
}

float CompassCalibrationMachine::compassFitness(int compassId) const
{
    if (compassId < 0 || compassId >= MaxCompasses) {
        return 0.0f;
    }
    return _rgFitness[compassId];
}

void CompassCalibrationMachine::_determineCompassesToCalibrate()
{
    Vehicle* vehicle = _controller->_vehicle;
    ParameterManager* paramMgr = vehicle->parameterManager();

    _compassMask = 0;

    // Check each compass
    for (int i = 0; i < MaxCompasses; i++) {
        QString devIdParam = (i == 0) ? QStringLiteral("COMPASS_DEV_ID") :
                             QStringLiteral("COMPASS_DEV_ID%1").arg(i + 1);
        QString useParam = (i == 0) ? QStringLiteral("COMPASS_USE") :
                           QStringLiteral("COMPASS_USE%1").arg(i + 1);

        if (paramMgr->parameterExists(ParameterManager::defaultComponentId, devIdParam) &&
            paramMgr->parameterExists(ParameterManager::defaultComponentId, useParam)) {

            Fact* devIdFact = _controller->getParameterFact(ParameterManager::defaultComponentId, devIdParam);
            Fact* useFact = _controller->getParameterFact(ParameterManager::defaultComponentId, useParam);

            if (devIdFact->rawValue().toInt() > 0 && useFact->rawValue().toBool()) {
                _compassMask |= (1 << i);
                qCDebug(CompassCalibrationMachineLog) << "Will calibrate compass" << (i + 1);
            } else {
                // Mark as already complete/succeeded for compasses we're not calibrating
                _rgComplete[i] = true;
                _rgSucceeded[i] = true;
                _rgFitness[i] = 0.0f;
            }
        }
    }

    qCDebug(CompassCalibrationMachineLog) << "Compass mask:" << _compassMask;
}

void CompassCalibrationMachine::_startMagCal()
{
    Vehicle* vehicle = _controller->_vehicle;

    // Bump up the fitness value so calibration will always succeed
    static const char* compassCalFitnessParam = "COMPASS_CAL_FIT";
    if (_controller->parameterExists(ParameterManager::defaultComponentId, compassCalFitnessParam)) {
        Fact* fitnessFact = _controller->getParameterFact(ParameterManager::defaultComponentId, compassCalFitnessParam);
        _restoreFitness = true;
        _previousFitness = fitnessFact->rawValue().toFloat();
        fitnessFact->setRawValue(100.0);
    }

    // Start the mag cal command
    vehicle->sendMavCommand(
        vehicle->defaultComponentId(),
        MAV_CMD_DO_START_MAG_CAL,
        true,           // showError
        _compassMask,   // which compass(es) to calibrate
        0,              // no retry on failure
        1,              // save values after complete
        0,              // no delayed start
        0               // no auto-reboot
    );

    postEvent("setup_complete");
}

void CompassCalibrationMachine::_restoreCompassCalFitness()
{
    if (_restoreFitness) {
        _restoreFitness = false;
        static const char* compassCalFitnessParam = "COMPASS_CAL_FIT";
        if (_controller->parameterExists(ParameterManager::defaultComponentId, compassCalFitnessParam)) {
            _controller->getParameterFact(ParameterManager::defaultComponentId, compassCalFitnessParam)->setRawValue(_previousFitness);
        }
    }
}

void CompassCalibrationMachine::_checkAllCompassesComplete()
{
    // Check if all compasses we're calibrating are complete
    bool allComplete = true;
    bool allSucceeded = true;

    for (int i = 0; i < MaxCompasses; i++) {
        if (_compassMask & (1 << i)) {
            if (!_rgComplete[i]) {
                allComplete = false;
            }
            if (!_rgSucceeded[i]) {
                allSucceeded = false;
            }
        }
    }

    if (allComplete) {
        qCDebug(CompassCalibrationMachineLog) << "All compasses complete, allSucceeded:" << allSucceeded;

        if (allSucceeded) {
            _controller->appendStatusLog(tr("All compasses calibrated successfully"));
            _controller->appendStatusLog(tr("YOU MUST REBOOT YOUR VEHICLE NOW FOR NEW SETTINGS TO TAKE EFFECT"));
            postEvent("all_complete_success");
        } else {
            _controller->appendStatusLog(tr("Compass calibration failed"));
            _controller->appendStatusLog(tr("YOU MUST REBOOT YOUR VEHICLE NOW AND RETRY COMPASS CALIBRATION PRIOR TO FLIGHT"));
            postEvent("all_complete_failed");
        }
    } else {
        _controller->appendStatusLog(tr("Continue rotating..."));
    }
}

void CompassCalibrationMachine::_updateProgressBar()
{
    if (_controller->_progressBar) {
        float totalProgress = static_cast<float>(_rgProgress[0] + _rgProgress[1] + _rgProgress[2]) / 100.0f;
        (void) _controller->_progressBar->setProperty("value", totalProgress);
    }
}
