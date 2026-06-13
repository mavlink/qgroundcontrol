#include "APMSensorsCalibrationUITest.h"

#include <QtCore/QElapsedTimer>
#include <QtQuick/QQuickItem>
#include <QtTest/QTest>

#include "MockLink.h"
#include "ParameterManager.h"
#include "Vehicle.h"

UT_REGISTER_TEST(APMSensorsCalibrationUITest, TestLabel::Integration)

namespace {

/// Wait for _refreshParams() traffic to settle so link teardown doesn't cut off
/// in-flight PARAM_REQUEST_READs (which can log warnings that fail strict mode).
void waitForParamRefreshQuiet(Vehicle *vehicle)
{
    ParameterManager *mgr = vehicle->parameterManager();
    QElapsedTimer sinceLastResponse;
    sinceLastResponse.start();

    QObject context;
    QObject::connect(mgr, &ParameterManager::_paramRequestReadSuccess, &context,
                     [&] { sinceLastResponse.restart(); });
    QObject::connect(mgr, &ParameterManager::_paramRequestReadFailure, &context,
                     [&] { sinceLastResponse.restart(); });

    (void) QTest::qWaitFor([&] { return sinceLastResponse.elapsed() > 500; }, 10000);
}

} // namespace

// ---------------------------------------------------------------------------

void APMSensorsCalibrationUITest::_navigateToSensorsPage()
{
    navigateToConfigureView();
    if (QTest::currentTestFailed()) return;

    QQuickItem *sensorsBtn = clickSidebarButton(QStringLiteral("vehicleConfig_comp_Sensors"));
    if (QTest::currentTestFailed()) return;

    // The Sensors component has no sub-sections on APM; clicking the button opens the page.
    // The Accelerometer indicator button must appear.
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_calibrateAccel"), 5000),
             "sensorsSetup_calibrateAccel not found after opening Sensors page");
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_calibrateCompass"), 3000),
             "sensorsSetup_calibrateCompass not found after opening Sensors page");
    Q_UNUSED(sensorsBtn);
}

void APMSensorsCalibrationUITest::_verifyCalIndicators(bool compassGreen, bool accelGreen, const char *context)
{
    struct Check {
        const char *name;
        bool expectedGreen;
    };
    const Check checks[] = {
        { "sensorsSetup_calibrateCompass", compassGreen },
        { "sensorsSetup_calibrateAccel",   accelGreen   },
    };

    for (const Check &c : checks) {
        QQuickItem *btn = findVisibleItem(_rootItem, QLatin1String(c.name), 3000);
        QVERIFY2(btn, qPrintable(QStringLiteral("Button not found (%1): %2")
                                     .arg(QLatin1String(context), QLatin1String(c.name))));
        (void) QTest::qWaitFor([&] {
            return btn->property("indicatorGreen").toBool() == c.expectedGreen;
        }, 3000);
        QVERIFY2(btn->property("indicatorGreen").toBool() == c.expectedGreen,
                 qPrintable(QStringLiteral("indicatorGreen is %1, expected %2 (%3): %4")
                                .arg(btn->property("indicatorGreen").toBool())
                                .arg(c.expectedGreen)
                                .arg(QLatin1String(context), QLatin1String(c.name))));
    }
}

void APMSensorsCalibrationUITest::_runFullAccelCal()
{
    // APM requires accel cal before compass can be run.
    // Click Accelerometer button → orientation dialog opens → accept (full cal, not simple).
    QVERIFY2(clickButton(QStringLiteral("sensorsSetup_calibrateAccel")),
             "Failed to click Accelerometer button");

    // Accept the orientation/pre-cal dialog (do NOT check the Simple checkbox)
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("popupDialog_acceptButton"), 5000),
             "Pre-accel-cal dialog accept button not found");
    QVERIFY2(clickButton(QStringLiteral("popupDialog_acceptButton")),
             "Failed to accept pre-accel-cal dialog");

    // MockLink will now drive the 6-pose ACCELCAL_VEHICLE_POS handshake.
    // For each pose the orientation cal area becomes visible with the relevant
    // side InProgress; the user (test) clicks Next to advance.

    // Poses in the order MockLink sends them:
    // LEVEL→downSide, LEFT→leftSide, RIGHT→rightSide,
    // NOSEDOWN→noseDownSide, NOSEUP→tailDownSide, BACK→upsideDownSide
    struct PoseInfo {
        const char *sideObjectName;
    };
    static constexpr PoseInfo kPoses[] = {
        { "sensorsCal_downSide"      },
        { "sensorsCal_leftSide"      },
        { "sensorsCal_rightSide"     },
        { "sensorsCal_noseDownSide"  },
        { "sensorsCal_tailDownSide"  },
        { "sensorsCal_upsideDownSide" },
    };

    for (const PoseInfo &pose : kPoses) {
        // Wait for the orientation cal area to show (controller.showOrientationCalArea = true)
        // and this side to be InProgress (calState == VehicleRotationCal.CalState.InProgress == 2)
        QQuickItem *sideItem = findVisibleItem(_rootItem, QLatin1String(pose.sideObjectName), 10000);
        QVERIFY2(sideItem, qPrintable(QStringLiteral("Side indicator not visible: %1")
                                          .arg(QLatin1String(pose.sideObjectName))));

        // calState InProgress == 2 (VehicleRotationCal.CalState enum)
        QVERIFY2(QTest::qWaitFor([&] { return sideItem->property("calState").toInt() == 2; }, 10000),
                 qPrintable(QStringLiteral("Side never went InProgress: %1")
                                .arg(QLatin1String(pose.sideObjectName))));

        // Click Next — sends COMMAND_ACK back to MockLink to advance to next pose.
        // The controller enables the Next button when it receives the ACCELCAL_VEHICLE_POS message,
        // which happens on the same tick as the side going InProgress, so wait briefly for it.
        QQuickItem *nextBtn = findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_nextButton"), 3000);
        QVERIFY2(nextBtn, "Next button not found");
        QVERIFY2(QTest::qWaitFor([&] { return nextBtn->property("enabled").toBool(); }, 3000),
                 qPrintable(QStringLiteral("Next button not enabled for side: %1")
                                .arg(QLatin1String(pose.sideObjectName))));
        QVERIFY2(clickButton(QStringLiteral("sensorsSetup_nextButton")), "Failed to click Next button");
    }

    // After the last pose MockLink sends ACCELCAL_VEHICLE_POS_SUCCESS → _stopCalibration(Success)
    // Progress bar goes to 1.0 and the cal area closes
    QQuickItem *progressBar = findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_progressBar"), 2000);
    QVERIFY2(progressBar, "Progress bar not found during accel cal");
    QVERIFY2(QTest::qWaitFor([&] { return qFuzzyCompare(progressBar->property("value").toDouble(), 1.0); }, 10000),
             "Progress bar never reached 1.0 after accel cal success");

    // APM's onCalibrationComplete handler opens postOnboardCompassCalibrationFactory for
    // both CalibrationAccel and CalibrationMag.  Dismiss the dialog so the sensors
    // page is unblocked before callers proceed.
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("popupDialog_acceptButton"), 5000),
             "Post-accel-cal dialog not shown");
    QVERIFY2(clickButton(QStringLiteral("popupDialog_acceptButton")),
             "Failed to dismiss post-accel-cal dialog");
}

// ---------------------------------------------------------------------------
// _testCompassCalibration
// ---------------------------------------------------------------------------
void APMSensorsCalibrationUITest::_testCompassCalibration()
{
    ignoreAPMMockLinkWarnings();
    runWithMockLink(
        [] { return MockLink::startAPMArduCopterMockLink(false, false, false); },
        [&](QPointer<MockLink> /*mockLink*/, Vehicle *vehicle) {

    resetAPMParamsToUncalibrated(vehicle);
    if (QTest::currentTestFailed()) return;

    _navigateToSensorsPage();
    if (QTest::currentTestFailed()) return;

    // Both indicators must be orange (not calibrated)
    _verifyCalIndicators(/*compassGreen=*/false, /*accelGreen=*/false, "after param reset");
    if (QTest::currentTestFailed()) return;

    // -------------------------------------------------------------------
    // 1. Accel calibration (required before compass on APM)
    // -------------------------------------------------------------------
    _runFullAccelCal();
    if (QTest::currentTestFailed()) return;

    waitForParamRefreshQuiet(vehicle);

    // Accel indicator should now be green; compass still orange
    _verifyCalIndicators(/*compassGreen=*/false, /*accelGreen=*/true, "after accel cal");
    if (QTest::currentTestFailed()) return;

    // -------------------------------------------------------------------
    // 2. Compass calibration
    // -------------------------------------------------------------------
    QVERIFY2(clickButton(QStringLiteral("sensorsSetup_calibrateCompass")),
             "Failed to click Compass button");

    // Orientation dialog opens → accept to start MAV_CMD_DO_START_MAG_CAL
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("popupDialog_acceptButton"), 5000),
             "Pre-compass-cal dialog accept button not found");
    QVERIFY2(clickButton(QStringLiteral("popupDialog_acceptButton")),
             "Failed to accept pre-compass-cal dialog");

    // MockLink sends MAG_CAL_PROGRESS 0→100 at ~10Hz
    QQuickItem *progressBar = findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_progressBar"), 2000);
    QVERIFY2(progressBar, "Progress bar not found during compass cal");

    // Wait for progress > 0 (calibration is running)
    QVERIFY2(QTest::qWaitFor([&] { return progressBar->property("value").toDouble() > 0.0; }, 5000),
             "Compass cal progress never advanced above 0");

    // Wait for completion: MAG_CAL_REPORT → StopCalibrationSuccessShowLog → dialog opens.
    // Note: StopCalibrationSuccessShowLog resets progress to 0 (not 1.0), so wait for
    // the post-cal dialog directly rather than waiting for progress == 1.0.
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("popupDialog_acceptButton"), 25000),
             "Post-compass-cal dialog not shown");
    QVERIFY2(clickButton(QStringLiteral("popupDialog_acceptButton")),
             "Failed to dismiss post-compass-cal dialog");

    waitForParamRefreshQuiet(vehicle);

    // Both indicators must be green
    _verifyCalIndicators(/*compassGreen=*/true, /*accelGreen=*/true, "after compass cal");

    });
}

// ---------------------------------------------------------------------------
// _testCompassCalibrationCancel
// ---------------------------------------------------------------------------
void APMSensorsCalibrationUITest::_testCompassCalibrationCancel()
{
    ignoreAPMMockLinkWarnings();
    runWithMockLink(
        [] { return MockLink::startAPMArduCopterMockLink(false, false, false); },
        [&](QPointer<MockLink> /*mockLink*/, Vehicle *vehicle) {

    resetAPMParamsToUncalibrated(vehicle);
    if (QTest::currentTestFailed()) return;

    _navigateToSensorsPage();
    if (QTest::currentTestFailed()) return;

    // Must do accel first
    _runFullAccelCal();
    if (QTest::currentTestFailed()) return;
    waitForParamRefreshQuiet(vehicle);

    // Start compass cal
    QVERIFY2(clickButton(QStringLiteral("sensorsSetup_calibrateCompass")),
             "Failed to click Compass button");
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("popupDialog_acceptButton"), 5000),
             "Pre-compass-cal dialog not found");
    QVERIFY2(clickButton(QStringLiteral("popupDialog_acceptButton")),
             "Failed to accept pre-compass-cal dialog");

    // Wait for some progress
    QQuickItem *progressBar = findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_progressBar"), 2000);
    QVERIFY2(progressBar, "Progress bar not found");
    QVERIFY2(QTest::qWaitFor([&] { return progressBar->property("value").toDouble() > 0.0; }, 5000),
             "Compass cal progress never started");

    // Cancel
    QVERIFY2(clickButton(QStringLiteral("sensorsSetup_cancelButton")),
             "Failed to click Cancel button");

    // After cancel the cancel button should become disabled (cal no longer active)
    QQuickItem *cancelBtn = findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_cancelButton"), 2000);
    QVERIFY2(cancelBtn, "Cancel button not found after cancel");
    QVERIFY2(QTest::qWaitFor([&] { return !cancelBtn->property("enabled").toBool(); }, 5000),
             "Cancel button still enabled after cancellation");

    // COMPASS_OFS_X must still be 0 (compass not calibrated)
    ParameterManager *mgr = vehicle->parameterManager();
    QVERIFY2(mgr->parameterExists(ParameterManager::defaultComponentId, QStringLiteral("COMPASS_OFS_X")),
             "COMPASS_OFS_X parameter not found");
    Fact *compassOfs = mgr->getParameter(ParameterManager::defaultComponentId, QStringLiteral("COMPASS_OFS_X"));
    QVERIFY2(qFuzzyIsNull(compassOfs->rawValue().toFloat()),
             "COMPASS_OFS_X is non-zero after compass cal cancel");

    // Compass indicator still orange
    _verifyCalIndicators(/*compassGreen=*/false, /*accelGreen=*/true, "after compass cal cancel");

    });
}

// ---------------------------------------------------------------------------
// _testAccelCalibration
// ---------------------------------------------------------------------------
void APMSensorsCalibrationUITest::_testAccelCalibration()
{
    ignoreAPMMockLinkWarnings();
    runWithMockLink(
        [] { return MockLink::startAPMArduCopterMockLink(false, false, false); },
        [&](QPointer<MockLink> /*mockLink*/, Vehicle *vehicle) {

    resetAPMParamsToUncalibrated(vehicle);
    if (QTest::currentTestFailed()) return;

    _navigateToSensorsPage();
    if (QTest::currentTestFailed()) return;

    _verifyCalIndicators(/*compassGreen=*/false, /*accelGreen=*/false, "after param reset");
    if (QTest::currentTestFailed()) return;

    _runFullAccelCal();
    if (QTest::currentTestFailed()) return;

    waitForParamRefreshQuiet(vehicle);

    // INS_ACCOFFS_X must be non-zero (MockLink writes 0.1 on SUCCESS)
    ParameterManager *mgr = vehicle->parameterManager();
    QVERIFY2(mgr->parameterExists(ParameterManager::defaultComponentId, QStringLiteral("INS_ACCOFFS_X")),
             "INS_ACCOFFS_X parameter not found");
    Fact *accelOfs = mgr->getParameter(ParameterManager::defaultComponentId, QStringLiteral("INS_ACCOFFS_X"));
    QVERIFY2(QTest::qWaitFor([&] { return !qFuzzyIsNull(accelOfs->rawValue().toFloat()); }, 10000),
             "INS_ACCOFFS_X still 0 after accel cal — param refresh did not bring back non-zero value");

    // Accel indicator green, compass still orange
    _verifyCalIndicators(/*compassGreen=*/false, /*accelGreen=*/true, "after accel cal");

    });
}
