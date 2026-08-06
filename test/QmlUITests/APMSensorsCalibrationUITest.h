#pragma once

#include "VehicleConfigUITestBase.h"

class Vehicle;

/// UI test that boots the full QML UI with an ArduCopter MockLink vehicle connected,
/// navigates to the Sensors setup page and runs magnetometer and accelerometer
/// calibrations by clicking the real UI buttons.
///
/// MockLink simulates the ArduPilot MAV_CMD_DO_START_MAG_CAL / MAG_CAL_PROGRESS /
/// MAG_CAL_REPORT protocol for compass and the ACCELCAL_VEHICLE_POS COMMAND_LONG
/// handshake for full accelerometer calibration.
///
/// Tests start from an uncalibrated state produced by resetAPMParamsToUncalibrated(),
/// which zeroes the compass and accel offset parameters via MockLink's APM
/// MAV_CMD_PREFLIGHT_STORAGE param1=2 handler.
class APMSensorsCalibrationUITest : public VehicleConfigUITestBase
{
    Q_OBJECT

public:
    APMSensorsCalibrationUITest() = default;

private slots:
    void _testCompassCalibration();
    void _testCompassCalibrationCancel();
    void _testCompassCalibrationIgnoresStaleFailedReports();
    void _testCompassCalibrationStartRejected();
    void _testAccelCalibration();
};
