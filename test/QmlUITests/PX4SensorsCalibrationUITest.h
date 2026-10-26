#pragma once

#include "VehicleConfigUITestBase.h"

class QString;
class Vehicle;

/// UI test that boots the full QML UI with a PX4 MockLink vehicle connected,
/// navigates to the Sensors setup page and runs complete magnetometer and
/// accelerometer calibrations by clicking the real UI buttons.
/// MockLinkPX4Calibration simulates the PX4 firmware [cal] protocol; the test
/// drives it by placing the simulated vehicle into each pose and verifies that
/// the pose indicator images and the progress bar update correctly.
class PX4SensorsCalibrationUITest : public VehicleConfigUITestBase
{
    Q_OBJECT

public:
    PX4SensorsCalibrationUITest() = default;

private slots:
    void _testMagCalibration();
    void _testMagCalibrationCancel();
    void _testAccelCalibration();
    void _testAccelCalibrationCancel();
    void _testInlineCalibration_data();
    void _testInlineCalibration();
    void _testInlineCalibrationCancel();
    void _testPageTabs();
    void _testOrientationCalibrationFailure_data();
    void _testOrientationCalibrationFailure();
    void _testLargeVehicleCalibration();

private:
    /// Reset the vehicle's parameters, open the Sensors page and check every sensor needs calibration.
    void _openSensorsPage(Vehicle* vehicle);

    /// Navigate from the Fly view to the Sensors page of the Configure view.
    void _navigateToSensorsPanel();

    /// Verify all six pose indicators are visible and in the given SideCalState.
    void _verifyAllPosesState(int expectedState, const char *context);

    /// Expected setup-complete states for the Sensors button in the config sidebar and the calibration rows
    struct SensorsSetupStates {
        bool sensorsComplete;       ///< Top-level Sensors component button
        bool compassComplete;
        bool gyroscopeComplete;
        bool accelerometerComplete;
    };

    /// Verify the setup-complete state shown by the top-level Sensors component
    /// button and the Compass, Gyroscope and Accelerometer calibration rows.
    /// A row which needs calibration shows its needs state, a calibrated one shows ok.
    void _verifySensorsSetupStates(const SensorsSetupStates &expected, const char *context);

    /// Click the given calibrate button and accept the pre-calibration dialog.
    void _startCalibration(const QString &calibrateButtonObjectName);

    /// Shared cancel-mid-calibration flow: start the given calibration, put one
    /// side in progress, cancel and verify the page returns to the idle state.
    void _runCalibrationCancelTest(const QString& calibrateButtonObjectName);
};
