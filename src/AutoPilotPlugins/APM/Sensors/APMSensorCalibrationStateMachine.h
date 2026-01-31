#pragma once

#include "SensorCalibrationStateMachineBase.h"
#include "QGCMAVLink.h"

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(APMSensorCalibrationStateMachineLog)

class APMSensorsComponentController;
class AccelCalibrationMachine;
class CompassCalibrationMachine;
class LinkInterface;

/// Top-level state machine for APM sensor calibration workflows.
///
/// Orchestrates different calibration types:
/// - Accelerometer (6-side and simple)
/// - Compass (onboard calibration)
/// - Gyro
/// - Level horizon
/// - Pressure/Airspeed
/// - CompassMot (motor interference)
///
/// States:
/// - Idle: No calibration in progress
/// - AccelCalibration: Running 6-side accel calibration (submachine)
/// - CompassCalibration: Running onboard compass calibration (submachine)
/// - SimpleCalibration: Running gyro/level/pressure/simple accel calibration
/// - CompassMotCalibration: Running motor interference calibration
/// - Cancelling: Waiting for cancel acknowledgment
class APMSensorCalibrationStateMachine : public SensorCalibrationStateMachineBase
{
    Q_OBJECT

public:
    explicit APMSensorCalibrationStateMachine(APMSensorsComponentController* controller, QObject* parent = nullptr);
    ~APMSensorCalibrationStateMachine() override = default;

    /// Start compass calibration
    void calibrateCompass() override;

    /// Start gyroscope calibration
    void calibrateGyro() override;

    /// Cancel current calibration
    void cancelCalibration() override;

    /// Start accelerometer calibration
    /// @param simple If true, uses simple single-position calibration
    void calibrateAccel(bool simple);

    /// Start compass calibration with known north direction
    /// @param lat Latitude
    /// @param lon Longitude
    /// @param mask Compass mask
    void calibrateCompassNorth(float lat, float lon, int mask);

    /// Start motor interference calibration
    void calibrateMotorInterference();

    /// Start level horizon calibration
    void levelHorizon();

    /// Start pressure/airspeed calibration
    void calibratePressure();

    /// Handle next button press from UI
    void nextClicked();

    /// Handle incoming MAVLink messages for calibration
    void handleMavlinkMessage(LinkInterface* link, const mavlink_message_t& message);

    /// Handle MAV command results
    void handleMavCommandResult(int vehicleId, int component, int command, int result, int failureCode);

private:
    void _buildStateMachine();
    void _setupTransitions();
    void _startLogCalibration();
    void _startVisualCalibration();
    void _stopCalibration(bool success, bool showLog = false);
    void _handleCommandAck(const mavlink_message_t& message);
    void _handleMagCalProgress(const mavlink_message_t& message);
    void _handleMagCalReport(const mavlink_message_t& message);
    void _handleCommandLong(const mavlink_message_t& message);

    APMSensorsComponentController* _apmController;

    // APM-specific states
    QGCState* _accelCalState = nullptr;
    QGCState* _compassCalState = nullptr;
    QGCState* _compassMotState = nullptr;

    // Sub-machines
    AccelCalibrationMachine* _accelCalMachine = nullptr;
    CompassCalibrationMachine* _compassCalMachine = nullptr;
};
