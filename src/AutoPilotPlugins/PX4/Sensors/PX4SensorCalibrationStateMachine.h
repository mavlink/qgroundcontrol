#pragma once

#include "SensorCalibrationStateMachineBase.h"
#include "QGCMAVLink.h"

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(PX4SensorCalibrationStateMachineLog)

class SensorsComponentController;
class PX4OrientationCalibrationMachine;

/// Top-level state machine for PX4 sensor calibration workflows.
///
/// Orchestrates different calibration types:
/// - Accelerometer (6-side orientation)
/// - Magnetometer (6-side orientation with rotation)
/// - Gyroscope (single orientation)
/// - Level horizon
/// - Airspeed
///
/// PX4 uses text messages prefixed with "[cal]" to communicate calibration state.
/// This state machine parses those messages and routes them to the appropriate
/// sub-machine or handles them directly for simple calibrations.
///
/// States:
/// - Idle: No calibration in progress
/// - OrientationCalibration: Running accel/mag/gyro calibration (submachine)
/// - SimpleCalibration: Running level/airspeed calibration
/// - Cancelling: Waiting for cancel acknowledgment
class PX4SensorCalibrationStateMachine : public SensorCalibrationStateMachineBase
{
    Q_OBJECT

public:
    explicit PX4SensorCalibrationStateMachine(SensorsComponentController* controller, QObject* parent = nullptr);
    ~PX4SensorCalibrationStateMachine() override = default;

    /// Start compass/magnetometer calibration
    void calibrateCompass() override;

    /// Start gyroscope calibration
    void calibrateGyro() override;

    /// Cancel current calibration
    void cancelCalibration() override;

    /// Start accelerometer calibration
    void calibrateAccel();

    /// Start level horizon calibration
    void calibrateLevel();

    /// Start airspeed calibration
    void calibrateAirspeed();

    /// Handle text message from the vehicle
    /// @param text The raw message text
    void handleTextMessage(const QString& text);

private:
    void _buildStateMachine();
    void _setupTransitions();
    void _startLogCalibration();
    void _startVisualCalibration();
    void _stopCalibration(bool success);
    void _handleCalibrationStarted(const QString& text);
    void _handleProgress(const QString& text);

    SensorsComponentController* _px4Controller;

    // PX4-specific states
    QGCState* _orientationCalState = nullptr;

    // Sub-machines
    PX4OrientationCalibrationMachine* _orientationCalMachine = nullptr;

    // Firmware version tracking
    bool _unknownFirmwareVersion = false;
    static constexpr int _supportedFirmwareCalVersion = 2;
};
