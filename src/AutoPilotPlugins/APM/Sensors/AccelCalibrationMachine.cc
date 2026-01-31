#include "AccelCalibrationMachine.h"
#include "APMSensorsComponentController.h"
#include "SensorCalibrationSide.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QTimer>

QGC_LOGGING_CATEGORY(AccelCalibrationMachineLog, "APMSensorCalibration.AccelCalibrationMachine")

AccelCalibrationMachine::AccelCalibrationMachine(APMSensorsComponentController* controller, QObject* parent)
    : QGCStateMachine("AccelCalibration", nullptr, parent)
    , _controller(controller)
{
    _buildStateMachine();
    _setupTransitions();
}

void AccelCalibrationMachine::_buildStateMachine()
{
    // Create all states
    _initState = new QGCState("Init", this);
    _levelState = new QGCState("Level", this);
    _leftState = new QGCState("Left", this);
    _rightState = new QGCState("Right", this);
    _noseDownState = new QGCState("NoseDown", this);
    _tailDownState = new QGCState("TailDown", this);
    _upsideDownState = new QGCState("UpsideDown", this);
    _successState = new QGCFinalState("Success", this);
    _failedState = new QGCFinalState("Failed", this);

    setInitialState(_initState);

    // Set up progress weights for 6 positions
    setProgressWeights({
        {_levelState, 1},
        {_leftState, 1},
        {_rightState, 1},
        {_noseDownState, 1},
        {_tailDownState, 1},
        {_upsideDownState, 1}
    });

    // Configure state entry callbacks
    connect(_initState, &QAbstractState::entered, this, [this]() {
        qCDebug(AccelCalibrationMachineLog) << "Entered Init state";
        _currentPosition = Position::Level;
    });

    connect(_levelState, &QAbstractState::entered, this, [this]() {
        qCDebug(AccelCalibrationMachineLog) << "Entered Level state";
        _currentPosition = Position::Level;
        _updateControllerUI(Position::Level);
        _controller->_nextButton->setEnabled(true);
        emit positionChanged(Position::Level);
    });

    connect(_leftState, &QAbstractState::entered, this, [this]() {
        qCDebug(AccelCalibrationMachineLog) << "Entered Left state";
        _currentPosition = Position::Left;
        _updateControllerUI(Position::Left);
        emit positionChanged(Position::Left);
    });

    connect(_rightState, &QAbstractState::entered, this, [this]() {
        qCDebug(AccelCalibrationMachineLog) << "Entered Right state";
        _currentPosition = Position::Right;
        _updateControllerUI(Position::Right);
        emit positionChanged(Position::Right);
    });

    connect(_noseDownState, &QAbstractState::entered, this, [this]() {
        qCDebug(AccelCalibrationMachineLog) << "Entered NoseDown state";
        _currentPosition = Position::NoseDown;
        _updateControllerUI(Position::NoseDown);
        emit positionChanged(Position::NoseDown);
    });

    connect(_tailDownState, &QAbstractState::entered, this, [this]() {
        qCDebug(AccelCalibrationMachineLog) << "Entered TailDown state";
        _currentPosition = Position::TailDown;
        _updateControllerUI(Position::TailDown);
        emit positionChanged(Position::TailDown);
    });

    connect(_upsideDownState, &QAbstractState::entered, this, [this]() {
        qCDebug(AccelCalibrationMachineLog) << "Entered UpsideDown state";
        _currentPosition = Position::UpsideDown;
        _updateControllerUI(Position::UpsideDown);
        emit positionChanged(Position::UpsideDown);
    });

    connect(_successState, &QAbstractState::entered, this, [this]() {
        qCDebug(AccelCalibrationMachineLog) << "Accel calibration succeeded";
        emit calibrationComplete(true);
    });

    connect(_failedState, &QAbstractState::entered, this, [this]() {
        qCDebug(AccelCalibrationMachineLog) << "Accel calibration failed";
        emit calibrationComplete(false);
    });
}

void AccelCalibrationMachine::_setupTransitions()
{
    // Init -> Level (on start event from startCalibration)
    _initState->addTransition(new MachineEventTransition("start", _levelState));

    // Level -> Left (on position command)
    _levelState->addTransition(new MachineEventTransition("pos_left", _leftState));

    // Left -> Right (on position command)
    _leftState->addTransition(new MachineEventTransition("pos_right", _rightState));

    // Right -> NoseDown (on position command)
    _rightState->addTransition(new MachineEventTransition("pos_nosedown", _noseDownState));

    // NoseDown -> TailDown (on position command)
    _noseDownState->addTransition(new MachineEventTransition("pos_taildown", _tailDownState));

    // TailDown -> UpsideDown (on position command)
    _tailDownState->addTransition(new MachineEventTransition("pos_upsidedown", _upsideDownState));

    // UpsideDown -> Success (on success command)
    _upsideDownState->addTransition(new MachineEventTransition("pos_success", _successState));

    // Any state -> Failed (on failed command)
    _initState->addTransition(new MachineEventTransition("pos_failed", _failedState));
    _levelState->addTransition(new MachineEventTransition("pos_failed", _failedState));
    _leftState->addTransition(new MachineEventTransition("pos_failed", _failedState));
    _rightState->addTransition(new MachineEventTransition("pos_failed", _failedState));
    _noseDownState->addTransition(new MachineEventTransition("pos_failed", _failedState));
    _tailDownState->addTransition(new MachineEventTransition("pos_failed", _failedState));
    _upsideDownState->addTransition(new MachineEventTransition("pos_failed", _failedState));

    // Any state -> Init (on cancel)
    _levelState->addTransition(new MachineEventTransition("cancel", _initState));
    _leftState->addTransition(new MachineEventTransition("cancel", _initState));
    _rightState->addTransition(new MachineEventTransition("cancel", _initState));
    _noseDownState->addTransition(new MachineEventTransition("cancel", _initState));
    _tailDownState->addTransition(new MachineEventTransition("cancel", _initState));
    _upsideDownState->addTransition(new MachineEventTransition("cancel", _initState));
}

void AccelCalibrationMachine::startCalibration()
{
    qCDebug(AccelCalibrationMachineLog) << "Starting accel calibration";

    // Reset all side states and make all visible
    _controller->orientationState().reset();
    _controller->orientationState().setVisibleSides(CalibrationSideMask::MaskAll);

    if (!isRunning()) {
        start();
    }

    // Post start event to transition from Init to Level
    QTimer::singleShot(0, this, [this]() {
        postEvent("start");
        emit calibrationStarted();
    });
}

void AccelCalibrationMachine::cancelCalibration()
{
    qCDebug(AccelCalibrationMachineLog) << "Cancelling accel calibration";
    postEvent("cancel");
    emit calibrationCancelled();
}

void AccelCalibrationMachine::handlePositionCommand(Position position)
{
    qCDebug(AccelCalibrationMachineLog) << "Received position command:" << static_cast<int>(position);

    QString eventName;
    switch (position) {
    case Position::Level:
        eventName = "pos_level";
        break;
    case Position::Left:
        eventName = "pos_left";
        break;
    case Position::Right:
        eventName = "pos_right";
        break;
    case Position::NoseDown:
        eventName = "pos_nosedown";
        break;
    case Position::TailDown:
        eventName = "pos_taildown";
        break;
    case Position::UpsideDown:
        eventName = "pos_upsidedown";
        break;
    case Position::Success:
        eventName = "pos_success";
        break;
    case Position::Failed:
        eventName = "pos_failed";
        break;
    default:
        qCWarning(AccelCalibrationMachineLog) << "Unknown position command:" << static_cast<int>(position);
        return;
    }

    postEvent(eventName);
}

void AccelCalibrationMachine::nextButtonPressed()
{
    qCDebug(AccelCalibrationMachineLog) << "Next button pressed";
    // The next button sends a MAVLink ACK to the vehicle which triggers the position change
    // The actual state transition happens when we receive the position command from vehicle
}

bool AccelCalibrationMachine::isCalibrating() const
{
    return isRunning() && !isStateActive(_initState) && !isStateActive(_successState) && !isStateActive(_failedState);
}

int AccelCalibrationMachine::currentPositionIndex() const
{
    switch (_currentPosition) {
    case Position::Level:
        return 0;
    case Position::Left:
        return 1;
    case Position::Right:
        return 2;
    case Position::NoseDown:
        return 3;
    case Position::TailDown:
        return 4;
    case Position::UpsideDown:
        return 5;
    default:
        return 0;
    }
}

void AccelCalibrationMachine::_updateControllerUI(Position position)
{
    // Update progress bar
    static constexpr float progressPerPosition = 1.0f / 6.0f;
    int posIndex = currentPositionIndex();
    _controller->setProgress(static_cast<float>(posIndex) * progressPerPosition);

    OrientationCalibrationState& state = _controller->orientationState();

    // Clear all in-progress flags
    state.setSideInProgress(CalibrationSide::Down, false);
    state.setSideInProgress(CalibrationSide::Left, false);
    state.setSideInProgress(CalibrationSide::Right, false);
    state.setSideInProgress(CalibrationSide::Front, false);
    state.setSideInProgress(CalibrationSide::Back, false);
    state.setSideInProgress(CalibrationSide::Up, false);

    switch (position) {
    case Position::Level:
        state.setSideInProgress(CalibrationSide::Down, true);
        break;
    case Position::Left:
        state.setSideDone(CalibrationSide::Down);
        state.setSideInProgress(CalibrationSide::Left, true);
        break;
    case Position::Right:
        state.setSideDone(CalibrationSide::Left);
        state.setSideInProgress(CalibrationSide::Right, true);
        break;
    case Position::NoseDown:
        state.setSideDone(CalibrationSide::Right);
        state.setSideInProgress(CalibrationSide::Front, true);
        break;
    case Position::TailDown:
        state.setSideDone(CalibrationSide::Front);
        state.setSideInProgress(CalibrationSide::Back, true);
        break;
    case Position::UpsideDown:
        state.setSideDone(CalibrationSide::Back);
        state.setSideInProgress(CalibrationSide::Up, true);
        break;
    default:
        break;
    }
}
