#include "SensorCalibrationStateMachineBase.h"
#include "SensorCalibrationControllerBase.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(SensorCalibrationStateMachineBaseLog, "Sensors.SensorCalibrationStateMachineBase")

SensorCalibrationStateMachineBase::SensorCalibrationStateMachineBase(
    const QString& name,
    SensorCalibrationControllerBase* controller,
    QObject* parent)
    : QGCStateMachine(name, nullptr, parent)
    , _controller(controller)
{
}

bool SensorCalibrationStateMachineBase::isCalibrating() const
{
    return _calType != QGCMAVLink::CalibrationNone;
}

void SensorCalibrationStateMachineBase::buildBaseStates()
{
    // Create common states
    _idleState = new QGCState("Idle", this);
    _simpleCalState = new QGCState("SimpleCalibration", this);
    _cancellingState = new QGCState("Cancelling", this);

    setInitialState(_idleState);

    // Configure idle state entry
    connect(_idleState, &QAbstractState::entered, this, [this]() {
        qCDebug(SensorCalibrationStateMachineBaseLog) << "Entered Idle state";
        _calType = QGCMAVLink::CalibrationNone;
        setButtonsEnabled(true);
    });

    // Configure simple calibration state entry
    connect(_simpleCalState, &QAbstractState::entered, this, [this]() {
        qCDebug(SensorCalibrationStateMachineBaseLog) << "Entered SimpleCalibration state";
    });

    // Configure cancelling state entry
    connect(_cancellingState, &QAbstractState::entered, this, [this]() {
        qCDebug(SensorCalibrationStateMachineBaseLog) << "Entered Cancelling state";
    });
}

void SensorCalibrationStateMachineBase::startLogCalibration()
{
    hideAllCalAreas();
    setButtonsEnabled(false);
    _controller->setProgress(0);
}

void SensorCalibrationStateMachineBase::startVisualCalibration()
{
    setButtonsEnabled(false);
    _controller->setProgress(0);
    setShowOrientationCalArea(true);
}

void SensorCalibrationStateMachineBase::stopCalibration(bool success)
{
    Q_UNUSED(success);

    hideAllCalAreas();
    setButtonsEnabled(true);

    QGCMAVLink::CalibrationType completedType = _calType;
    _calType = QGCMAVLink::CalibrationNone;

    emit calibrationComplete(completedType, success);
}

void SensorCalibrationStateMachineBase::setButtonsEnabled(bool enabled)
{
    if (_controller) {
        _controller->setButtonsEnabled(enabled);
    }
}

void SensorCalibrationStateMachineBase::appendStatusLog(const QString& text)
{
    if (_controller) {
        _controller->appendStatusLog(text);
    }
}

void SensorCalibrationStateMachineBase::hideAllCalAreas()
{
    if (_controller) {
        _controller->hideAllCalAreas();
    }
}

void SensorCalibrationStateMachineBase::setShowOrientationCalArea(bool show)
{
    if (_controller) {
        _controller->setShowOrientationCalArea(show);
    }
}
