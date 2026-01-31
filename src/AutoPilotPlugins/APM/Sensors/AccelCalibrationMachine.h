#pragma once

#include "QGCStateMachine.h"

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(AccelCalibrationMachineLog)

class APMSensorsComponentController;

/// State machine for APM 6-side accelerometer calibration workflow.
///
/// States:
/// - Init: Initial state, waiting for start command
/// - Level: Vehicle level position (Down facing up)
/// - Left: Vehicle on left side
/// - Right: Vehicle on right side
/// - NoseDown: Vehicle nose down
/// - TailDown: Vehicle tail down (nose up)
/// - UpsideDown: Vehicle upside down
/// - Success: Calibration completed successfully (final)
/// - Failed: Calibration failed (final)
///
/// Transitions are triggered by ACCELCAL_VEHICLE_POS commands from the vehicle.
class AccelCalibrationMachine : public QGCStateMachine
{
    Q_OBJECT

public:
    /// Position enumeration matching ACCELCAL_VEHICLE_POS
    enum class Position {
        Level = 1,      // ACCELCAL_VEHICLE_POS_LEVEL
        Left = 2,       // ACCELCAL_VEHICLE_POS_LEFT
        Right = 3,      // ACCELCAL_VEHICLE_POS_RIGHT
        NoseDown = 4,   // ACCELCAL_VEHICLE_POS_NOSEDOWN
        TailDown = 5,   // ACCELCAL_VEHICLE_POS_NOSEUP (tail down = nose up)
        UpsideDown = 6, // ACCELCAL_VEHICLE_POS_BACK
        Success = 16777215, // ACCELCAL_VEHICLE_POS_SUCCESS
        Failed = 16777216   // ACCELCAL_VEHICLE_POS_FAILED
    };
    Q_ENUM(Position)

    explicit AccelCalibrationMachine(APMSensorsComponentController* controller, QObject* parent = nullptr);
    ~AccelCalibrationMachine() override = default;

    /// Start the 6-side accelerometer calibration
    void startCalibration();

    /// Cancel the calibration workflow
    void cancelCalibration();

    /// Handle position command from the vehicle (ACCELCAL_VEHICLE_POS)
    /// @param position The position command from MAVLink
    void handlePositionCommand(Position position);

    /// Handle next button press from UI
    void nextButtonPressed();

    /// @return true if calibration is in progress
    bool isCalibrating() const;

    /// @return Current position index (0-5) for progress calculation
    int currentPositionIndex() const;

signals:
    void calibrationStarted();
    void calibrationCancelled();
    void calibrationComplete(bool success);
    void positionChanged(Position position);

private:
    void _buildStateMachine();
    void _setupTransitions();
    void _updateControllerUI(Position position);
    void _clearAllSideProgress();
    void _setAllSidesVisible();

    APMSensorsComponentController* _controller;

    // States
    QGCState* _initState = nullptr;
    QGCState* _levelState = nullptr;
    QGCState* _leftState = nullptr;
    QGCState* _rightState = nullptr;
    QGCState* _noseDownState = nullptr;
    QGCState* _tailDownState = nullptr;
    QGCState* _upsideDownState = nullptr;
    QGCFinalState* _successState = nullptr;
    QGCFinalState* _failedState = nullptr;

    Position _currentPosition = Position::Level;
};
