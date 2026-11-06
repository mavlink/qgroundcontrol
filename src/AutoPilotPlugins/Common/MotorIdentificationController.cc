#include "MotorIdentificationController.h"

#include "QGCLoggingCategory.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(MotorIdentificationControllerLog, "AutoPilotPlugins.MotorIdentificationController")

MotorIdentificationController::MotorIdentificationController(QObject* parent)
    : FactPanelController(parent)
{
    // Stacked coaxial motors share a position, so they can't be told apart from above
    if (_vehicle && !_vehicle->coaxialMotors()) {
        const int motorCount = _vehicle->motorCount();
        // A tricopter has no plus variant: its motors are always front-right, rear and front-left
        const bool xLayout = (motorCount == 3) || _vehicle->xConfigMotors();
        for (int motor = 0; motor < motorCount; motor++) {
            _motorTestNumbers.append(motor + 1);
        }
        _positionAngles = layoutAngles(motorCount, xLayout);
    }

    _pulseTimer.setInterval(_pulseIntervalMsDefault);
    (void) connect(&_pulseTimer, &QTimer::timeout, this, &MotorIdentificationController::_pulseCurrentMotor);
}

MotorIdentificationController::~MotorIdentificationController()
{
    _stopCurrentMotor();
}

QList<double> MotorIdentificationController::layoutAngles(int motorCount, bool xLayout)
{
    QList<double> angles;
    if (motorCount <= 0) {
        return angles;
    }

    const double spacing = 360.0 / motorCount;
    const double offset = xLayout ? spacing / 2.0 : 0.0;
    for (int i = 0; i < motorCount; i++) {
        angles.append(offset + (i * spacing));
    }
    return angles;
}

QVariantList MotorIdentificationController::positionAngles() const
{
    QVariantList angles;
    if (!supported()) {
        return angles;
    }

    for (const double angle : _positionAngles) {
        angles.append(angle);
    }
    return angles;
}

int MotorIdentificationController::motorTestNumber(int motor) const
{
    return _motorTestNumbers.value(motor, -1);
}

void MotorIdentificationController::_setLayout(const QList<int>& motorTestNumbers, const QList<double>& positionAngles,
                                               bool layoutKnown)
{
    if (motorTestNumbers.count() != positionAngles.count()) {
        qCWarning(MotorIdentificationControllerLog)
            << "Mismatched layout"
            << "motors:" << motorTestNumbers.count() << "positions:" << positionAngles.count();
        return;
    }

    _motorTestNumbers = motorTestNumbers;
    _positionAngles = positionAngles;
    _layoutKnown = layoutKnown;
}

QVariantList MotorIdentificationController::selectedPositions() const
{
    QVariantList positions;
    for (const int position : _selectedPositions) {
        positions.append(position);
    }
    return positions;
}

bool MotorIdentificationController::orderCorrect() const
{
    if (!complete() || !_layoutKnown) {
        return false;
    }

    for (int motor = 0; motor < _selectedPositions.count(); motor++) {
        if (_selectedPositions[motor] != motor) {
            return false;
        }
    }
    return true;
}

void MotorIdentificationController::start(int throttlePercent)
{
    if (!supported() || running()) {
        qCWarning(MotorIdentificationControllerLog) << "Unable to start identification"
                                                    << "supported:" << supported() << "running:" << running();
        return;
    }
    if ((throttlePercent <= 0) || (throttlePercent > 100)) {
        qCWarning(MotorIdentificationControllerLog) << "Invalid throttle:" << throttlePercent;
        return;
    }

    _throttlePercent = throttlePercent;
    _selectedPositions.clear();
    emit selectedPositionsChanged();

    _setCurrentMotor(0);
    _pulseCurrentMotor();
    _pulseTimer.start();
}

void MotorIdentificationController::selectPosition(int position)
{
    if (!running()) {
        qCDebug(MotorIdentificationControllerLog) << "Not running";
        return;
    }
    if ((position < 0) || (position >= motorCount()) || _selectedPositions.contains(position)) {
        qCDebug(MotorIdentificationControllerLog) << "Ignoring unavailable position:" << position;
        return;
    }

    _stopCurrentMotor();
    _selectedPositions.append(position);

    if (_selectedPositions.count() == motorCount()) {
        _pulseTimer.stop();
        _setCurrentMotor(-1);
    } else {
        // Leave a gap before the next motor so the two pulses can't be confused
        _setCurrentMotor(_currentMotor + 1);
        _pulseTimer.start();
    }
    emit selectedPositionsChanged();
}

void MotorIdentificationController::cancel()
{
    _pulseTimer.stop();
    _stopCurrentMotor();
    _setCurrentMotor(-1);
    if (!_selectedPositions.isEmpty()) {
        _selectedPositions.clear();
        emit selectedPositionsChanged();
    }
}

void MotorIdentificationController::_pulseCurrentMotor()
{
    if (!running() || !_vehicle) {
        return;
    }

    // Only the first pulse reports errors, so a vehicle which rejects the test doesn't raise one per pulse
    const bool firstPulse = !_pulseTimer.isActive();
    qCDebug(MotorIdentificationControllerLog)
        << "Pulsing motor:" << motorTestNumber(_currentMotor) << "throttle:" << _throttlePercent;
    _vehicle->motorTest(motorTestNumber(_currentMotor), _throttlePercent, _pulseDurationSecs, firstPulse);
}

void MotorIdentificationController::_stopCurrentMotor()
{
    if (running() && _vehicle) {
        _vehicle->motorTest(motorTestNumber(_currentMotor), 0, 0, false);
    }
}

void MotorIdentificationController::_setCurrentMotor(int motor)
{
    if (motor != _currentMotor) {
        const bool wasRunning = running();
        _currentMotor = motor;
        emit currentMotorChanged();
        if (running() != wasRunning) {
            emit runningChanged();
        }
    }
}
