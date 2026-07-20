#include "APMSensorsCalibrationUITest.h"

#include <QtQuick/QQuickItem>
#include <QtTest/QTest>

#include "MockLink.h"
#include "ParameterManager.h"
#include "Vehicle.h"

UT_REGISTER_TEST(APMSensorsCalibrationUITest, TestLabel::Integration)

// ---------------------------------------------------------------------------
// _testCompassCalibration
// ---------------------------------------------------------------------------
void APMSensorsCalibrationUITest::_testCompassCalibration()
{
    ignoreAPMMockLinkWarnings();
    runWithMockLink(
        [] { return MockLink::startAPMArduCopterMockLink(); },
        [&](QPointer<MockLink> /*mockLink*/, Vehicle *vehicle) {

    resetAPMParamsToUncalibrated(vehicle);
    if (QTest::currentTestFailed()) return;

    navigateToAPMSensorsPage();
    if (QTest::currentTestFailed()) return;

    // Both indicators must be orange (not calibrated)
    verifyAPMCalIndicators(/*compassGreen=*/false, /*accelGreen=*/false, "after param reset");
    if (QTest::currentTestFailed()) return;

    // -------------------------------------------------------------------
    // 1. Accel calibration (required before compass on APM)
    // -------------------------------------------------------------------
    runAPMFullAccelCal();
    if (QTest::currentTestFailed()) return;

    waitForParamRefreshQuiet(vehicle);
    if (QTest::currentTestFailed()) return;

    // Accel indicator should now be green; compass still orange
    verifyAPMCalIndicators(/*compassGreen=*/false, /*accelGreen=*/true, "after accel cal");
    if (QTest::currentTestFailed()) return;

    // -------------------------------------------------------------------
    // 2. Compass calibration
    // -------------------------------------------------------------------
    runAPMCompassCal();
    if (QTest::currentTestFailed()) return;

    waitForParamRefreshQuiet(vehicle);
    if (QTest::currentTestFailed()) return;

    // Both indicators must be green
    verifyAPMCalIndicators(/*compassGreen=*/true, /*accelGreen=*/true, "after compass cal");

    });
}

// ---------------------------------------------------------------------------
// _testCompassCalibrationCancel
// ---------------------------------------------------------------------------
void APMSensorsCalibrationUITest::_testCompassCalibrationCancel()
{
    ignoreAPMMockLinkWarnings();
    runWithMockLink(
        [] { return MockLink::startAPMArduCopterMockLink(); },
        [&](QPointer<MockLink> /*mockLink*/, Vehicle *vehicle) {

    resetAPMParamsToUncalibrated(vehicle);
    if (QTest::currentTestFailed()) return;

    navigateToAPMSensorsPage();
    if (QTest::currentTestFailed()) return;

    // Must do accel first
    runAPMFullAccelCal();
    if (QTest::currentTestFailed()) return;
    waitForParamRefreshQuiet(vehicle);
    if (QTest::currentTestFailed()) return;

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
    verifyAPMCalIndicators(/*compassGreen=*/false, /*accelGreen=*/true, "after compass cal cancel");

    });
}

// ---------------------------------------------------------------------------
// _testCompassCalibrationIgnoresStaleFailedReports
//
// ArduPilot keeps streaming MAG_CAL_REPORT(MAG_CAL_FAILED) after a failed
// onboard compass cal until it is cancelled. Starting a new calibration while
// that stream is active must not instantly complete/fail the new cal.
// ---------------------------------------------------------------------------
void APMSensorsCalibrationUITest::_testCompassCalibrationIgnoresStaleFailedReports()
{
    ignoreAPMMockLinkWarnings();
    runWithMockLink(
        [] { return MockLink::startAPMArduCopterMockLink(); },
        [&](QPointer<MockLink> mockLink, Vehicle *vehicle) {

    resetAPMParamsToUncalibrated(vehicle);
    if (QTest::currentTestFailed()) return;

    navigateToAPMSensorsPage();
    if (QTest::currentTestFailed()) return;

    // Must do accel first
    runAPMFullAccelCal();
    if (QTest::currentTestFailed()) return;
    waitForParamRefreshQuiet(vehicle);
    if (QTest::currentTestFailed()) return;

    // Simulate leftover MAG_CAL_REPORT(FAILED) stream from a previously failed cal
    mockLink->startAPMStaleFailedMagCalReportStreaming();

    // Start compass cal while the stale stream is active
    QVERIFY2(clickButton(QStringLiteral("sensorsSetup_calibrateCompass")),
             "Failed to click Compass button");
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("popupDialog_acceptButton"), 5000),
             "Pre-compass-cal dialog not found");
    QVERIFY2(clickButton(QStringLiteral("popupDialog_acceptButton")),
             "Failed to accept pre-compass-cal dialog");

    // The stale failed reports must not terminate the new calibration: the cancel
    // button stays enabled while a compass cal is active, so it must remain enabled.
    QQuickItem *cancelBtn = findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_cancelButton"), 2000);
    QVERIFY2(cancelBtn, "Cancel button not found");
    QVERIFY2(QTest::qWaitFor([&] { return cancelBtn->property("enabled").toBool(); }, 5000),
             "Compass cal never became active");

    // Starting the new cal must have cancelled the stale stream on the vehicle
    QVERIFY2(QTest::qWaitFor([&] { return !mockLink->apmStaleFailedMagCalReportStreamingActive(); }, 5000),
             "Stale failed MAG_CAL_REPORT stream was never cancelled");

    // Calibration must still be running (not instantly completed/failed by a stale report):
    // give the stale stream time to have been (mis)processed, then check cancel still enabled.
    QVERIFY2(!QTest::qWaitFor([&] { return !cancelBtn->property("enabled").toBool(); }, 2000),
             "Compass cal terminated prematurely - stale MAG_CAL_REPORT was processed");

    // Now let the normal MockLink progress simulation complete the calibration
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("popupDialog_acceptButton"), 25000),
             "Post-compass-cal dialog not shown");
    QVERIFY2(clickButton(QStringLiteral("popupDialog_acceptButton")),
             "Failed to dismiss post-compass-cal dialog");

    waitForParamRefreshQuiet(vehicle);
    if (QTest::currentTestFailed()) return;

    // Both indicators must be green - the cal completed successfully
    verifyAPMCalIndicators(/*compassGreen=*/true, /*accelGreen=*/true, "after compass cal with stale reports");

    });
}

// ---------------------------------------------------------------------------
// _testCompassCalibrationStartRejected
//
// MAG_CAL_PROGRESS/REPORT are ignored until MAV_CMD_DO_START_MAG_CAL is
// accepted, so a rejected START is the only exit path for a cal that never
// starts. Verify the UI exits cleanly instead of staying in a running state.
// ---------------------------------------------------------------------------
void APMSensorsCalibrationUITest::_testCompassCalibrationStartRejected()
{
    ignoreAPMMockLinkWarnings();
    runWithMockLink(
        [] { return MockLink::startAPMArduCopterMockLink(); },
        [&](QPointer<MockLink> mockLink, Vehicle *vehicle) {

    resetAPMParamsToUncalibrated(vehicle);
    if (QTest::currentTestFailed()) return;

    navigateToAPMSensorsPage();
    if (QTest::currentTestFailed()) return;

    // Must do accel first
    runAPMFullAccelCal();
    if (QTest::currentTestFailed()) return;
    waitForParamRefreshQuiet(vehicle);
    if (QTest::currentTestFailed()) return;

    // Vehicle rejects MAV_CMD_DO_START_MAG_CAL
    mockLink->setAPMMagCalStartFailureMode(true);

    // START is sent with showError=true so the rejection first shows the command
    // error, then _stopCalibration(StopCalibrationFailed) shows the cal failed message.
    expectAppMessage(QRegularExpression(QStringLiteral("command failed")));
    expectAppMessage(QRegularExpression(QStringLiteral("Calibration failed")));

    QVERIFY2(clickButton(QStringLiteral("sensorsSetup_calibrateCompass")),
             "Failed to click Compass button");
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("popupDialog_acceptButton"), 5000),
             "Pre-compass-cal dialog not found");
    QVERIFY2(clickButton(QStringLiteral("popupDialog_acceptButton")),
             "Failed to accept pre-compass-cal dialog");

    // The rejected START must cleanly stop the calibration: the cancel button is only
    // enabled while a cal is active, so it must end up disabled.
    QQuickItem *cancelBtn = findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_cancelButton"), 2000);
    QVERIFY2(cancelBtn, "Cancel button not found");
    QVERIFY2(QTest::qWaitFor([&] { return !cancelBtn->property("enabled").toBool(); }, 10000),
             "Compass cal still active after START was rejected");

    verifyExpectedLogMessage();  // command failed
    verifyExpectedLogMessage();  // Calibration failed

    waitForParamRefreshQuiet(vehicle);
    if (QTest::currentTestFailed()) return;

    // Compass indicator still orange, accel green
    verifyAPMCalIndicators(/*compassGreen=*/false, /*accelGreen=*/true, "after rejected compass cal start");

    });
}

// ---------------------------------------------------------------------------
// _testAccelCalibration
// ---------------------------------------------------------------------------
void APMSensorsCalibrationUITest::_testAccelCalibration()
{
    ignoreAPMMockLinkWarnings();
    runWithMockLink(
        [] { return MockLink::startAPMArduCopterMockLink(); },
        [&](QPointer<MockLink> /*mockLink*/, Vehicle *vehicle) {

    resetAPMParamsToUncalibrated(vehicle);
    if (QTest::currentTestFailed()) return;

    navigateToAPMSensorsPage();
    if (QTest::currentTestFailed()) return;

    verifyAPMCalIndicators(/*compassGreen=*/false, /*accelGreen=*/false, "after param reset");
    if (QTest::currentTestFailed()) return;

    runAPMFullAccelCal();
    if (QTest::currentTestFailed()) return;

    waitForParamRefreshQuiet(vehicle);
    if (QTest::currentTestFailed()) return;

    // INS_ACCOFFS_X must be non-zero (MockLink writes 0.1 on SUCCESS)
    ParameterManager *mgr = vehicle->parameterManager();
    QVERIFY2(mgr->parameterExists(ParameterManager::defaultComponentId, QStringLiteral("INS_ACCOFFS_X")),
             "INS_ACCOFFS_X parameter not found");
    Fact *accelOfs = mgr->getParameter(ParameterManager::defaultComponentId, QStringLiteral("INS_ACCOFFS_X"));
    QVERIFY2(QTest::qWaitFor([&] { return !qFuzzyIsNull(accelOfs->rawValue().toFloat()); }, 10000),
             "INS_ACCOFFS_X still 0 after accel cal — param refresh did not bring back non-zero value");

    // Accel indicator green, compass still orange
    verifyAPMCalIndicators(/*compassGreen=*/false, /*accelGreen=*/true, "after accel cal");

    });
}
