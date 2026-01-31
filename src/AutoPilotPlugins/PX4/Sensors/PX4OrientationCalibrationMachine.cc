#include "PX4OrientationCalibrationMachine.h"
#include "SensorsComponentController.h"
#include "SensorCalibrationSide.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QTimer>

QGC_LOGGING_CATEGORY(PX4OrientationCalibrationMachineLog, "PX4SensorCalibration.OrientationCalibrationMachine")

PX4OrientationCalibrationMachine::PX4OrientationCalibrationMachine(SensorsComponentController* controller, QObject* parent)
    : QGCStateMachine("PX4OrientationCalibration", nullptr, parent)
    , _controller(controller)
{
    _buildStateMachine();
    _setupTransitions();
}

void PX4OrientationCalibrationMachine::_buildStateMachine()
{
    // Create all states
    _idleState = new QGCState("Idle", this);
    _waitingForOrientationState = new QGCState("WaitingForOrientation", this);
    _orientationDetectedState = new QGCState("OrientationDetected", this);
    _successState = new QGCFinalState("Success", this);
    _failedState = new QGCFinalState("Failed", this);

    setInitialState(_idleState);

    // Configure state entry callbacks
    connect(_idleState, &QAbstractState::entered, this, [this]() {
        qCDebug(PX4OrientationCalibrationMachineLog) << "Entered Idle state";
    });

    connect(_waitingForOrientationState, &QAbstractState::entered, this, [this]() {
        qCDebug(PX4OrientationCalibrationMachineLog) << "Waiting for orientation";
        if (_calibrationType == CalibrationType::Mag) {
            _controller->setOrientationHelpText(
                tr("Place your vehicle into one of the Incomplete orientations shown below and hold it still"));
        } else {
            _controller->setOrientationHelpText(
                tr("Place your vehicle into one of the Incomplete orientations shown below and hold it still"));
        }
    });

    connect(_orientationDetectedState, &QAbstractState::entered, this, [this]() {
        qCDebug(PX4OrientationCalibrationMachineLog) << "Orientation detected:" << static_cast<int>(_currentSide);
        OrientationCalibrationState& state = _controller->orientationState();
        state.setSideInProgress(_currentSide, true);
        if (_calibrationType == CalibrationType::Mag) {
            state.setSideRotate(_currentSide, true);
            _controller->setOrientationHelpText(
                tr("Rotate the vehicle continuously as shown in the diagram until marked as Completed"));
        } else {
            _controller->setOrientationHelpText(
                tr("Hold still in the current orientation"));
        }
        emit sideStarted(_currentSide);
    });

    connect(_successState, &QAbstractState::entered, this, [this]() {
        qCDebug(PX4OrientationCalibrationMachineLog) << "Calibration succeeded";
        emit calibrationComplete(true);
    });

    connect(_failedState, &QAbstractState::entered, this, [this]() {
        qCDebug(PX4OrientationCalibrationMachineLog) << "Calibration failed";
        emit calibrationComplete(false);
    });
}

void PX4OrientationCalibrationMachine::_setupTransitions()
{
    // Idle -> WaitingForOrientation (on start)
    _idleState->addTransition(new MachineEventTransition("start", _waitingForOrientationState));

    // WaitingForOrientation -> OrientationDetected (on orientation_detected)
    _waitingForOrientationState->addTransition(new MachineEventTransition("orientation_detected", _orientationDetectedState));

    // OrientationDetected -> WaitingForOrientation (on side_done)
    _orientationDetectedState->addTransition(new MachineEventTransition("side_done", _waitingForOrientationState));

    // WaitingForOrientation -> Success (on calibration_done)
    _waitingForOrientationState->addTransition(new MachineEventTransition("calibration_done", _successState));
    _orientationDetectedState->addTransition(new MachineEventTransition("calibration_done", _successState));

    // Any -> Failed (on calibration_failed)
    _idleState->addTransition(new MachineEventTransition("calibration_failed", _failedState));
    _waitingForOrientationState->addTransition(new MachineEventTransition("calibration_failed", _failedState));
    _orientationDetectedState->addTransition(new MachineEventTransition("calibration_failed", _failedState));

    // Cancel transitions
    _waitingForOrientationState->addTransition(new MachineEventTransition("cancel", _idleState));
    _orientationDetectedState->addTransition(new MachineEventTransition("cancel", _idleState));
}

void PX4OrientationCalibrationMachine::startCalibration(CalibrationType type, int visibleSides)
{
    qCDebug(PX4OrientationCalibrationMachineLog) << "Starting orientation calibration, type:" << static_cast<int>(type);

    _calibrationType = type;
    _visibleSides = visibleSides;

    // Reset all orientation flags and set visible sides
    OrientationCalibrationState& state = _controller->orientationState();
    state.reset();
    state.setVisibleSides(visibleSides);

    if (!isRunning()) {
        start();
    }

    QTimer::singleShot(0, this, [this]() {
        postEvent("start");
        emit calibrationStarted(_calibrationType);
    });
}

void PX4OrientationCalibrationMachine::cancelCalibration()
{
    qCDebug(PX4OrientationCalibrationMachineLog) << "Cancelling calibration";
    postEvent("cancel");
    emit calibrationCancelled();
}

bool PX4OrientationCalibrationMachine::handleCalibrationMessage(const QString& text)
{
    OrientationCalibrationState& state = _controller->orientationState();

    // Handle "orientation detected"
    if (text.endsWith("orientation detected")) {
        QString sideText = text.section(" ", 0, 0);
        _currentSide = SensorCalibrationSide::parseSideText(sideText);
        qCDebug(PX4OrientationCalibrationMachineLog) << "Side detected:" << sideText;
        postEvent("orientation_detected");
        return true;
    }

    // Handle "side done"
    if (text.endsWith("side done, rotate to a different side")) {
        QString sideText = text.section(" ", 0, 0);
        CalibrationSide side = SensorCalibrationSide::parseSideText(sideText);
        qCDebug(PX4OrientationCalibrationMachineLog) << "Side done:" << sideText;

        state.setSideInProgress(side, false);
        state.setSideDone(side);
        state.setSideRotate(side, false);

        _controller->setOrientationHelpText(
            tr("Place your vehicle into one of the orientations shown below and hold it still"));

        emit sideCompleted(side);
        postEvent("side_done");
        return true;
    }

    // Handle "side already completed"
    if (text.endsWith("side already completed")) {
        _controller->setOrientationHelpText(
            tr("Orientation already completed, place your vehicle into one of the incomplete orientations shown below and hold it still"));
        return true;
    }

    // Handle "calibration done"
    if (text.startsWith("calibration done:")) {
        qCDebug(PX4OrientationCalibrationMachineLog) << "Calibration complete";
        postEvent("calibration_done");
        return true;
    }

    // Handle "calibration cancelled" or "calibration failed"
    if (text.startsWith("calibration cancelled") || text.startsWith("calibration failed")) {
        qCDebug(PX4OrientationCalibrationMachineLog) << "Calibration ended:" << text;
        postEvent("calibration_failed");
        return true;
    }

    return false;
}

bool PX4OrientationCalibrationMachine::isCalibrating() const
{
    return isRunning() && !isStateActive(_idleState) && !isStateActive(_successState) && !isStateActive(_failedState);
}
