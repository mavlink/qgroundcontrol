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

private:
    /// Navigate from the Fly view to the Sensors page of the Configure view.
    void _navigateToSensorsPanel();

    /// Verify all six pose indicators are visible and in the given SideCalState.
    void _verifyAllPosesState(int expectedState, const char *context);

    /// Expected setup-complete states for the Sensors buttons in the config sidebar
    struct SensorsSetupStates {
        bool sensorsComplete;       ///< Top-level Sensors component button
        bool compassComplete;
        bool gyroscopeComplete;
        bool accelerometerComplete;
        bool levelHorizonComplete;
        bool orientationsComplete;
    };

    /// Verify the setup-complete state shown by the top-level Sensors component
    /// button and the Compass, Accelerometer and Level Horizon section buttons.
    /// Incomplete buttons show the orange config-required indicator.
    void _verifySensorsSetupStates(const SensorsSetupStates &expected, const char *context);

    /// Click the given calibrate button and accept the pre-calibration dialog.
    void _startCalibration(const QString &calibrateButtonObjectName);

    /// Shared cancel-mid-calibration flow: select the sensor section, start the
    /// given calibration, put one side in progress, cancel and verify the UI
    /// returns to the idle state.
    void _runCalibrationCancelTest(const QString &sectionObjectName, const QString &calibrateButtonObjectName);
};
