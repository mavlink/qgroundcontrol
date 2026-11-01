#pragma once

#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QTimer>
#include <QtCore/QVariantList>
#include <QtQmlIntegration/QtQmlIntegration>

#include "FactPanelController.h"

Q_DECLARE_LOGGING_CATEGORY(MotorIdentificationControllerLog)

/// Pulses one motor at a time and records which frame position the user saw spinning, so the motor numbering can be
/// checked against the frame layout.
///
/// Positions are numbered clockwise (viewed from above) starting at the first motor on or clockwise of the nose.
class MotorIdentificationController : public FactPanelController
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool supported READ supported CONSTANT)
    Q_PROPERTY(int motorCount READ motorCount CONSTANT)
    /// Angle of each position in degrees, clockwise from the nose
    Q_PROPERTY(QVariantList positionAngles READ positionAngles CONSTANT)
    /// true: motor N is expected at position N (ArduPilot motor test order)
    Q_PROPERTY(bool expectClockwiseOrder READ expectClockwiseOrder WRITE setExpectClockwiseOrder NOTIFY
                   expectClockwiseOrderChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    /// Motor being pulsed, -1 when not running
    Q_PROPERTY(int currentMotor READ currentMotor NOTIFY currentMotorChanged)
    /// Position the user selected for each motor, in motor order
    Q_PROPERTY(QVariantList selectedPositions READ selectedPositions NOTIFY selectedPositionsChanged)
    Q_PROPERTY(bool complete READ complete NOTIFY selectedPositionsChanged)
    Q_PROPERTY(bool orderCorrect READ orderCorrect NOTIFY selectedPositionsChanged)

public:
    explicit MotorIdentificationController(QObject* parent = nullptr);
    ~MotorIdentificationController() override;

    /// Starts pulsing the first motor at the specified throttle
    Q_INVOKABLE void start(int throttlePercent);

    /// Records the position of the motor being pulsed and moves on to the next motor
    Q_INVOKABLE void selectPosition(int position);

    /// Stops the motor being pulsed and discards the selections
    Q_INVOKABLE void cancel();

    [[nodiscard]] static QList<double> layoutAngles(int motorCount, bool xLayout);

    bool supported() const { return _motorCount >= _minimumMotorCount; }

    int motorCount() const { return _motorCount; }

    QVariantList positionAngles() const;

    bool expectClockwiseOrder() const { return _expectClockwiseOrder; }

    void setExpectClockwiseOrder(bool expectClockwiseOrder);

    bool running() const { return _currentMotor != -1; }

    int currentMotor() const { return _currentMotor; }

    QVariantList selectedPositions() const;

    bool complete() const { return supported() && (_selectedPositions.count() == _motorCount); }

    bool orderCorrect() const;

    void setPulseIntervalMs(int intervalMs) { _pulseTimer.setInterval(intervalMs); }

signals:
    void expectClockwiseOrderChanged();
    void runningChanged();
    void currentMotorChanged();
    void selectedPositionsChanged();

private slots:
    void _pulseCurrentMotor();

private:
    void _stopCurrentMotor();
    void _setCurrentMotor(int motor);

    static constexpr int _minimumMotorCount = 3;
    static constexpr int _pulseDurationSecs = 1;
    static constexpr int _pulseIntervalMsDefault = 1500;

    int _motorCount = 0;
    bool _xLayout = true;
    bool _expectClockwiseOrder = false;
    int _throttlePercent = 0;
    int _currentMotor = -1;
    QList<int> _selectedPositions;
    QTimer _pulseTimer;
};
