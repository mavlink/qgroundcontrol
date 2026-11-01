#pragma once

#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtCore/QVariantList>
#include <QtQmlIntegration/QtQmlIntegration>

#include "FactPanelController.h"

Q_DECLARE_LOGGING_CATEGORY(MotorIdentificationControllerLog)

/// Pulses one motor at a time and records which frame position the user saw spinning, so the motor numbering can be
/// checked against the frame layout.
///
/// By default the positions are spaced evenly around the frame, numbered clockwise (viewed from above) from the first
/// position on or clockwise of the nose, and nothing is known about which motor belongs where. Firmware specific
/// subclasses provide the real layout with _setLayout().
class MotorIdentificationController : public FactPanelController
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool supported READ supported CONSTANT)
    Q_PROPERTY(int motorCount READ motorCount CONSTANT)
    /// Angle of each position in degrees, clockwise from the nose
    Q_PROPERTY(QVariantList positionAngles READ positionAngles CONSTANT)
    /// true: position N is where motor N belongs, so selections can be checked
    Q_PROPERTY(bool layoutKnown READ layoutKnown CONSTANT)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    /// Motor being pulsed, -1 when not running
    Q_PROPERTY(int currentMotor READ currentMotor NOTIFY currentMotorChanged)
    /// Position the user selected for each motor, in motor order
    Q_PROPERTY(QVariantList selectedPositions READ selectedPositions NOTIFY selectedPositionsChanged)
    Q_PROPERTY(bool complete READ complete NOTIFY selectedPositionsChanged)
    Q_PROPERTY(bool orderCorrect READ orderCorrect NOTIFY selectedPositionsChanged)
    /// Settings changes which would fix a wrong motor order, one line each. Empty if the firmware can't fix it.
    Q_PROPERTY(QStringList remapChanges READ remapChanges NOTIFY selectedPositionsChanged)

public:
    explicit MotorIdentificationController(QObject* parent = nullptr);
    ~MotorIdentificationController() override;

    /// Starts pulsing the first motor at the specified throttle
    Q_INVOKABLE void start(int throttlePercent);

    /// Records the position of the motor being pulsed and moves on to the next motor
    Q_INVOKABLE void selectPosition(int position);

    /// Stops the motor being pulsed and discards the selections
    Q_INVOKABLE void cancel();

    /// Applies remapChanges, then discards the selections so identification can be run again to confirm
    Q_INVOKABLE virtual void remapMotors() {}

    /// @return Motor number sent in MAV_CMD_DO_MOTOR_TEST for the motor, numbered from 1
    Q_INVOKABLE int motorTestNumber(int motor) const;

    [[nodiscard]] static QList<double> layoutAngles(int motorCount, bool xLayout);

    bool supported() const { return motorCount() >= _minimumMotorCount; }

    int motorCount() const { return static_cast<int>(_motorTestNumbers.count()); }

    QVariantList positionAngles() const;

    bool layoutKnown() const { return _layoutKnown; }

    bool running() const { return _currentMotor != -1; }

    int currentMotor() const { return _currentMotor; }

    QVariantList selectedPositions() const;

    bool complete() const { return supported() && (_selectedPositions.count() == motorCount()); }

    bool orderCorrect() const;

    virtual QStringList remapChanges() const { return {}; }

    void setPulseIntervalMs(int intervalMs) { _pulseTimer.setInterval(intervalMs); }

signals:
    void runningChanged();
    void currentMotorChanged();
    void selectedPositionsChanged();

protected:
    /// Replaces the layout. Only call from a subclass constructor, before QML reads the constant properties.
    ///     @param motorTestNumbers Motor test number of each motor, in the order they are pulsed
    ///     @param positionAngles Angle of each position, clockwise from the nose
    ///     @param layoutKnown true: position N is where motor N belongs
    void _setLayout(const QList<int>& motorTestNumbers, const QList<double>& positionAngles, bool layoutKnown);

    const QList<int>& _selections() const { return _selectedPositions; }

private slots:
    void _pulseCurrentMotor();

private:
    void _stopCurrentMotor();
    void _setCurrentMotor(int motor);

    static constexpr int _minimumMotorCount = 3;
    static constexpr int _pulseDurationSecs = 1;
    static constexpr int _pulseIntervalMsDefault = 1500;

    QList<int> _motorTestNumbers;
    QList<double> _positionAngles;
    bool _layoutKnown = false;
    int _throttlePercent = 0;
    int _currentMotor = -1;
    QList<int> _selectedPositions;
    QTimer _pulseTimer;
};
