#pragma once

#include "QmlUITestBase.h"

class QString;

/// UI test that boots the full QML UI with a PX4 MockLink vehicle connected,
/// navigates to the Sensors setup page and runs complete magnetometer and
/// accelerometer calibrations by clicking the real UI buttons.
/// MockLinkPX4Calibration simulates the PX4 firmware [cal] protocol; the test
/// drives it by placing the simulated vehicle into each pose and verifies that
/// the pose indicator images and the progress bar update correctly.
class PX4SensorsCalibrationUITest : public QmlUITestBase
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

    /// Click a section sub-item in the vehicle config sidebar.
    void _clickSidebarSection(const QString &objectName);

    /// Verify all six pose indicators are visible and in the given SideCalState.
    void _verifyAllPosesState(int expectedState, const char *context);

    /// Click the given calibrate button and accept the pre-calibration dialog.
    void _startCalibration(const QString &calibrateButtonObjectName);

    /// Shared cancel-mid-calibration flow: select the sensor section, start the
    /// given calibration, put one side in progress, cancel and verify the UI
    /// returns to the idle state.
    void _runCalibrationCancelTest(const QString &sectionObjectName, const QString &calibrateButtonObjectName);
};
