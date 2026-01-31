#pragma once

#include "QGCStateMachine.h"
#include "SensorCalibrationSide.h"

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(PX4OrientationCalibrationMachineLog)

class SensorsComponentController;

/// State machine for PX4 6-side orientation calibration workflows.
/// Used for accelerometer, magnetometer, and gyroscope calibrations.
///
/// PX4 uses text messages with "[cal]" prefix to communicate calibration state:
/// - "calibration started: <version> <type>" - calibration begins
/// - "<side> orientation detected" - side is being calibrated
/// - "<side> side done, rotate to a different side" - side complete
/// - "calibration done:" - all sides complete
/// - "calibration cancelled" / "calibration failed" - calibration ended
///
/// States:
/// - Idle: Waiting for calibration to start
/// - WaitingForOrientation: Waiting for user to position vehicle
/// - OrientationDetected: Side detected, calibrating
/// - Success: Calibration completed successfully (final)
/// - Failed: Calibration failed (final)
class PX4OrientationCalibrationMachine : public QGCStateMachine
{
    Q_OBJECT

public:
    enum class CalibrationType {
        Accel,
        Mag,
        Gyro
    };
    Q_ENUM(CalibrationType)

    explicit PX4OrientationCalibrationMachine(SensorsComponentController* controller, QObject* parent = nullptr);
    ~PX4OrientationCalibrationMachine() override = default;

    /// Start the orientation calibration
    /// @param type The type of calibration (accel, mag, or gyro)
    /// @param visibleSides Bitmask of which sides are visible (for mag cal)
    void startCalibration(CalibrationType type, int visibleSides = CalibrationSideMask::MaskAll);

    /// Cancel the calibration workflow
    void cancelCalibration();

    /// Handle text message from the vehicle
    /// @param text The calibration message text (with [cal] prefix stripped)
    /// @return true if the message was handled
    bool handleCalibrationMessage(const QString& text);

    /// @return true if calibration is in progress
    bool isCalibrating() const;

    /// @return The current calibration type
    CalibrationType calibrationType() const { return _calibrationType; }

signals:
    void calibrationStarted(CalibrationType type);
    void calibrationCancelled();
    void calibrationComplete(bool success);
    void sideStarted(CalibrationSide side);
    void sideCompleted(CalibrationSide side);
    void progressChanged(int percent);

private:
    void _buildStateMachine();
    void _setupTransitions();

    SensorsComponentController* _controller;
    CalibrationType _calibrationType = CalibrationType::Accel;
    int _visibleSides = CalibrationSideMask::MaskAll;

    // States
    QGCState* _idleState = nullptr;
    QGCState* _waitingForOrientationState = nullptr;
    QGCState* _orientationDetectedState = nullptr;
    QGCFinalState* _successState = nullptr;
    QGCFinalState* _failedState = nullptr;

    CalibrationSide _currentSide = CalibrationSide::Down;
};
