#include "PX4SensorsCalibrationUITest.h"

#include <QtCore/QElapsedTimer>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QTest>

#include "MockLink.h"
#include "ParameterManager.h"
#include "SensorsComponentController.h"
#include "Vehicle.h"

UT_REGISTER_TEST(PX4SensorsCalibrationUITest, TestLabel::Integration)

namespace {

struct PoseInfo {
    MockLinkPX4Calibration::Pose pose;
    const char *objectName;     ///< VehicleRotationCal objectName in SensorsSetup.qml
    const char* sideId;         ///< Side the stage shows when the vehicle is on this side
};

constexpr PoseInfo kPoses[MockLinkPX4Calibration::kSideCount] = {
    {MockLinkPX4Calibration::Pose::RightSideUp, "sensorsCal_downSide", "level"},
    {MockLinkPX4Calibration::Pose::UpsideDown, "sensorsCal_upsideDownSide", "flipped"},
    {MockLinkPX4Calibration::Pose::NoseDown, "sensorsCal_noseDownSide", "nose"},
    {MockLinkPX4Calibration::Pose::TailDown, "sensorsCal_tailDownSide", "tail"},
    {MockLinkPX4Calibration::Pose::Left, "sensorsCal_leftSide", "left"},
    {MockLinkPX4Calibration::Pose::Right, "sensorsCal_rightSide", "right"},
};

bool waitForCalState(QQuickItem *item, SensorsComponentController::SideCalState expected, int timeoutMs)
{
    return QTest::qWaitFor([&] { return item->property("calState").toInt() == static_cast<int>(expected); }, timeoutMs);
}

bool waitForText(QQuickItem* item, const QString& expected, int timeoutMs)
{
    return QTest::qWaitFor([&] { return item->property("text").toString() == expected; }, timeoutMs);
}

/// The SensorsComponentController behind the calibration panel, whose progress the firmware drives
QObject* panelController(QQuickItem* panel)
{
    return panel ? panel->property("sensorsController").value<QObject*>() : nullptr;
}

const char *calStateName(int state)
{
    switch (static_cast<SensorsComponentController::SideCalState>(state)) {
    case SensorsComponentController::SideCalStateIdle:       return "Idle";
    case SensorsComponentController::SideCalStateIncomplete: return "Incomplete";
    case SensorsComponentController::SideCalStateInProgress: return "InProgress";
    case SensorsComponentController::SideCalStateCompleted:  return "Completed";
    }
    return "Unknown";
}

} // namespace

void PX4SensorsCalibrationUITest::_navigateToSensorsPanel()
{
    navigateToConfigureView();
    if (QTest::currentTestFailed()) return;

    QQuickItem *sensorsBtn = clickSidebarButton(QStringLiteral("vehicleConfig_comp_Sensors"));
    if (QTest::currentTestFailed()) return;

    // The Sensors page is a single page without a section tree
    QVERIFY2(!sensorsBtn->property("expanded").toBool(), "Sensors button expanded a section tree");

    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_calibrateCompass"), 3000),
             "Calibrate Compass button not found after opening Sensors page");

    // The prerequisite-setup message panel ("%1 setup must be completed prior to %2
    // setup.") must not be shown: SYS_AUTOSTART is preserved across the param reset
    // so Airframe setup remains complete
    QVERIFY2(!findVisibleItem(_rootItem, QStringLiteral("vehicleConfig_messagePanel"), 0),
             "Prerequisite setup message panel shown instead of Sensors page");
}

void PX4SensorsCalibrationUITest::_verifyAllPosesState(int expectedState, const char *context)
{
    for (const PoseInfo &info : kPoses) {
        QQuickItem *side = findVisibleItem(_rootItem, QLatin1String(info.objectName), 5000);
        QVERIFY2(side, qPrintable(QStringLiteral("Pose indicator not visible (%1): %2")
                                      .arg(QLatin1String(context), QLatin1String(info.objectName))));
        const int state = side->property("calState").toInt();
        QVERIFY2(state == expectedState,
                 qPrintable(QStringLiteral("Pose in state %1, expected %2 (%3): %4")
                                .arg(QLatin1String(calStateName(state)), QLatin1String(calStateName(expectedState)),
                                     QLatin1String(context), QLatin1String(info.objectName))));
    }
}

void PX4SensorsCalibrationUITest::_verifySensorsSetupStates(const SensorsSetupStates &expected, const char *context)
{
    // The top-level Sensors component button shows whether setup is complete
    QQuickItem* sensorsButton = findVisibleItem(_rootItem, QStringLiteral("vehicleConfig_comp_Sensors"), 3000);
    QVERIFY2(sensorsButton, qPrintable(QStringLiteral("Sensors button not found (%1)").arg(QLatin1String(context))));
    (void) QTest::qWaitFor(
        [&] { return sensorsButton->property("setupComplete").toBool() == expected.sensorsComplete; }, 3000);
    QVERIFY2(sensorsButton->property("setupComplete").toBool() == expected.sensorsComplete,
             qPrintable(QStringLiteral("Sensors setup complete state is %1, expected %2 (%3)")
                            .arg(sensorsButton->property("setupComplete").toBool())
                            .arg(expected.sensorsComplete)
                            .arg(QLatin1String(context))));

    struct RowCheck
    {
        const char *objectName;
        bool expectedComplete;
    };

    const RowCheck checks[] = {
        {"sensors_row_compass", expected.compassComplete},
        {"sensors_row_gyro", expected.gyroscopeComplete},
        {"sensors_row_accel", expected.accelerometerComplete},
    };

    // A calibration row is "ok" once calibrated and "needs" until then
    for (const RowCheck& check : checks) {
        QQuickItem* row = findVisibleItem(_rootItem, QLatin1String(check.objectName), 3000);
        QVERIFY2(
            row,
            qPrintable(
                QStringLiteral("Row not found (%1): %2").arg(QLatin1String(context), QLatin1String(check.objectName))));
        const QString expectedState = check.expectedComplete ? QStringLiteral("ok") : QStringLiteral("needs");
        (void) QTest::qWaitFor([&] { return row->property("calibrationState").toString() == expectedState; }, 3000);
        QVERIFY2(row->property("calibrationState").toString() == expectedState,
                 qPrintable(QStringLiteral("Row state is %1, expected %2 (%3): %4")
                                .arg(row->property("calibrationState").toString(), expectedState,
                                     QLatin1String(context), QLatin1String(check.objectName))));
    }
}

void PX4SensorsCalibrationUITest::_openSensorsPage(Vehicle* vehicle)
{
    resetParamsToFirmwareDefaults(vehicle, QStringLiteral("CAL_MAG0_ID"));
    if (QTest::currentTestFailed()) return;

    _navigateToSensorsPanel();
    if (QTest::currentTestFailed()) return;

    _verifySensorsSetupStates({
        .sensorsComplete = false,
        .compassComplete = false,
        .gyroscopeComplete = false,
        .accelerometerComplete = false,
    }, "after param reset");
}

void PX4SensorsCalibrationUITest::_startCalibration(const QString& calibrateButtonObjectName)
{
    QVERIFY2(clickButtonScrolled(calibrateButtonObjectName, QStringLiteral("setupPage_flickable")),
             qPrintable(QStringLiteral("Failed to click calibrate button: %1").arg(calibrateButtonObjectName)));

    // The panel shows what to do first: Start sends MAV_CMD_PREFLIGHT_CALIBRATION
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_startCalibration"), 3000),
             "Start button not shown on the ready screen");
    QVERIFY2(!findVisibleItem(_rootItem, QStringLiteral("popupDialog_acceptButton"), 0),
             "A confirmation dialog was shown");
    QVERIFY2(clickButton(QStringLiteral("sensorsSetup_startCalibration")), "Failed to click Start");
}

void PX4SensorsCalibrationUITest::_testMagCalibration()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [&](QPointer<MockLink> mockLink, Vehicle* vehicle) {
            _openSensorsPage(vehicle);
            if (QTest::currentTestFailed())
                return;

            _startCalibration(QStringLiteral("sensorsSetup_calibrateCompass"));
            if (QTest::currentTestFailed())
                return;

            // MockLink responds with "[cal] calibration started: 2 mag" which makes all
            // six pose indicators visible and Incomplete (not yet visited)
            for (const PoseInfo& info : kPoses) {
                QQuickItem* side = findVisibleItem(_rootItem, QLatin1String(info.objectName), 5000);
                QVERIFY2(
                    side,
                    qPrintable(QStringLiteral("Pose indicator not visible: %1").arg(QLatin1String(info.objectName))));
                QVERIFY2(waitForCalState(side, SensorsComponentController::SideCalStateIncomplete, 5000),
                         qPrintable(QStringLiteral("Pose not Incomplete at start (state %1): %2")
                                        .arg(QLatin1String(calStateName(side->property("calState").toInt())),
                                             QLatin1String(info.objectName))));
            }

            QQuickItem* panel = findVisibleItem(_rootItem, QStringLiteral("sensors_calibrationPanel"), 2000);
            QObject* controller = panelController(panel);
            QVERIFY2(controller, "Calibration panel not visible during calibration");
            const auto progress = [&] { return controller->property("calibrationProgress").toDouble(); };
            QVERIFY2(qFuzzyIsNull(progress()), "Progress not at 0 at calibration start");

            QQuickItem* counter = findVisibleItem(_rootItem, QStringLiteral("sensorsCal_counter"), 2000);
            QVERIFY2(counter && waitForText(counter, QStringLiteral("0 of 6 sides done"), 3000),
                     "Counter not at 0 of 6 at calibration start");

            QQuickItem* cancelButton =
                findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_cancelCalibration"), 2000);
            QVERIFY2(cancelButton, "Cancel button not visible during calibration");

            // ---------------------------------------------------------------------
            // Place the vehicle into each pose and watch the UI track the side
            // ---------------------------------------------------------------------
            double lastProgress = 0.0;
            int sidesDone = 0;

            for (const PoseInfo& info : kPoses) {
                const QString sideName = QLatin1String(info.objectName);
                QQuickItem* side = findVisibleItem(_rootItem, sideName);
                QVERIFY2(side, qPrintable(QStringLiteral("Pose indicator disappeared: %1").arg(sideName)));

                mockLink->setCalibrationPose(info.pose);

                // Orientation detected: the side goes InProgress with the rotate animation
                QVERIFY2(waitForCalState(side, SensorsComponentController::SideCalStateInProgress, 5000),
                         qPrintable(QStringLiteral("Side never went in-progress: %1").arg(sideName)));
                QQuickItem* stage = findVisibleItem(_rootItem, QStringLiteral("sensorsCal_stage"));
                QVERIFY2(
                    stage && stage->property("side").toString() == QLatin1String(info.sideId) &&
                        stage->property("rotating").toBool() &&
                        stage->property("instruction").toString() == QLatin1String("Rotate"),
                    qPrintable(QStringLiteral("The stage does not show the vehicle rotating on %1").arg(sideName)));

                // Side completes and marks itself done
                QVERIFY2(waitForCalState(side, SensorsComponentController::SideCalStateCompleted, 5000),
                         qPrintable(QStringLiteral("Side never completed: %1").arg(sideName)));

                sidesDone++;

                // The counter follows the finished sides until the last one, which reports the result
                if (sidesDone < MockLinkPX4Calibration::kSideCount) {
                    QVERIFY2(waitForText(counter, QStringLiteral("%1 of 6 sides done").arg(sidesDone), 3000),
                             qPrintable(QStringLiteral("Counter after %1 sides is '%2'")
                                            .arg(sidesDone)
                                            .arg(counter->property("text").toString())));
                }

                // Progress must move forward with each side and stay within the
                // band expected for the number of completed sides
                const double progressNow = progress();
                QVERIFY2(progressNow > lastProgress,
                         qPrintable(QStringLiteral("Progress did not advance after %1: %2 -> %3")
                                        .arg(sideName)
                                        .arg(lastProgress)
                                        .arg(progressNow)));
                if (sidesDone < MockLinkPX4Calibration::kSideCount) {
                    constexpr double sideCount = MockLinkPX4Calibration::kSideCount;
                    QVERIFY2((progressNow >= ((sidesDone - 1) / sideCount)) &&
                                 (progressNow <= ((sidesDone / sideCount) + 0.01)),
                             qPrintable(QStringLiteral("Progress out of expected band after %1 sides: %2")
                                            .arg(sidesDone)
                                            .arg(progressNow)));
                }
                lastProgress = progressNow;

                // Previously completed sides must remain marked complete
                for (const PoseInfo& doneInfo : kPoses) {
                    if (&doneInfo == &info)
                        break;
                    QQuickItem* doneSide = findVisibleItem(_rootItem, QLatin1String(doneInfo.objectName));
                    QVERIFY2(doneSide && (doneSide->property("calState").toInt() ==
                                          SensorsComponentController::SideCalStateCompleted),
                             qPrintable(QStringLiteral("Previously completed side no longer marked complete: %1")
                                            .arg(QLatin1String(doneInfo.objectName))));
                }
            }

            // ---------------------------------------------------------------------
            // Calibration complete: progress hits 100% and the Done button shows
            // ---------------------------------------------------------------------
            QVERIFY2(QTest::qWaitFor([&] { return qFuzzyCompare(progress(), 1.0); }, 5000),
                     qPrintable(QStringLiteral("Progress never reached 1.0: %1").arg(progress())));

            QVERIFY2(waitForText(counter, QStringLiteral("Calibrated"), 5000),
                     "Counter does not say Calibrated after the calibration finished");
            QVERIFY2(vehicle->rebootRequired(), "Compass calibration did not latch rebootRequired");
            QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_dismissCalibration"), 5000),
                     "Done button not shown after compass calibration finished");
            QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_rebootVehicle"), 2000),
                     "Reboot button not shown after compass calibration finished");

            // Calibration no longer active: the cancel button hides
            QVERIFY2(QTest::qWaitFor([&] { return !cancelButton->isVisible(); }, 5000),
                     "Cancel button still visible after calibration completed");

            waitForParamRefreshQuiet(vehicle);
            if (QTest::currentTestFailed())
                return;

            // All sides must remain marked complete (green) after calibration ends and
            // the post-calibration parameter refresh settles
            _verifyAllPosesState(SensorsComponentController::SideCalStateCompleted, "after calibration finished");
            if (QTest::currentTestFailed())
                return;

            // Done dismisses the calibration panel and brings the rows back
            QVERIFY2(clickButton(QStringLiteral("sensorsSetup_dismissCalibration")), "Failed to click Done");
            QVERIFY2(QTest::qWaitFor([&] { return !panel->isVisible(); }, 5000),
                     "Calibration panel still visible after Done");

            // The completed mag calibration set CAL_MAG0_ID on the vehicle: only the
            // Compass row shows calibrated, everything else still needs calibration
            _verifySensorsSetupStates(
                {
                    .sensorsComplete = false,
                    .compassComplete = true,
                    .gyroscopeComplete = false,
                    .accelerometerComplete = false,
                },
                "after mag calibration");
        });
}

void PX4SensorsCalibrationUITest::_runCalibrationCancelTest(const QString& calibrateButtonObjectName)
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [&](QPointer<MockLink> mockLink, Vehicle* vehicle) {
            _openSensorsPage(vehicle);
            if (QTest::currentTestFailed())
                return;

            _startCalibration(calibrateButtonObjectName);
            if (QTest::currentTestFailed())
                return;

            // Start calibrating one side
            const PoseInfo& info = kPoses[0];
            QQuickItem* side = findVisibleItem(_rootItem, QLatin1String(info.objectName), 5000);
            QVERIFY2(side, "Pose indicator not visible after calibration start");

            mockLink->setCalibrationPose(info.pose);
            QVERIFY2(waitForCalState(side, SensorsComponentController::SideCalStateInProgress, 5000),
                     "Side never went in-progress");

            // Cancel mid-calibration: MockLink responds with "[cal] calibration cancelled"
            QQuickItem* cancelButton = findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_cancelCalibration"));
            QVERIFY2(cancelButton, "Cancel button not visible during calibration");
            QVERIFY2(clickButton(QStringLiteral("sensorsSetup_cancelCalibration")), "Failed to click Cancel");

            // The calibration-active UI (progress bar + cancel button row) hides once
            // the controller processes the cancel response
            QVERIFY2(QTest::qWaitFor([&] { return !cancelButton->isVisible(); }, 5000),
                     "Cancel button still visible after cancel");

            // Cancelled calibration discards results: the partially calibrated side
            // returns to the neutral Idle state, not red and not green
            QVERIFY2(waitForCalState(side, SensorsComponentController::SideCalStateIdle, 5000),
                     qPrintable(QStringLiteral("Side not Idle after cancel (state %1)")
                                    .arg(QLatin1String(calStateName(side->property("calState").toInt())))));

            // The calibrate button is clickable again
            QVERIFY2(
                findVisibleItem(_rootItem, calibrateButtonObjectName, 3000),
                qPrintable(
                    QStringLiteral("Calibrate button not available after cancel: %1").arg(calibrateButtonObjectName)));

            waitForParamRefreshQuiet(vehicle);
            if (QTest::currentTestFailed())
                return;

            // The cancelled calibration discarded its results: every sensor still
            // needs calibration
            _verifySensorsSetupStates(
                {
                    .sensorsComplete = false,
                    .compassComplete = false,
                    .gyroscopeComplete = false,
                    .accelerometerComplete = false,
                },
                "after calibration cancelled");

            // Opening the calibration again starts from level, not from the side the cancelled one was on
            QVERIFY2(clickButtonScrolled(calibrateButtonObjectName, QStringLiteral("setupPage_flickable")),
                     "Failed to click calibrate after cancel");
            QQuickItem* stage = findVisibleItem(_rootItem, QStringLiteral("sensorsCal_stage"), 3000);
            QVERIFY2(stage, "Calibration stage not visible on the ready screen after cancel");
            QCOMPARE(stage->property("side").toString(), QStringLiteral("level"));
            QVERIFY2(clickButtonScrolled(QStringLiteral("sensorsSetup_closeCalibration"),
                                         QStringLiteral("setupPage_flickable")),
                     "Failed to close the ready screen");
        });
}

void PX4SensorsCalibrationUITest::_testMagCalibrationCancel()
{
    _runCalibrationCancelTest(QStringLiteral("sensorsSetup_calibrateCompass"));
}

void PX4SensorsCalibrationUITest::_testAccelCalibration()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [&](QPointer<MockLink> mockLink, Vehicle* vehicle) {
            _openSensorsPage(vehicle);
            if (QTest::currentTestFailed())
                return;

            _startCalibration(QStringLiteral("sensorsSetup_calibrateAccel"));
            if (QTest::currentTestFailed())
                return;

            // MockLink responds with "[cal] calibration started: 2 accel" which makes all
            // six pose indicators visible and Incomplete (not yet visited)
            for (const PoseInfo& info : kPoses) {
                QQuickItem* side = findVisibleItem(_rootItem, QLatin1String(info.objectName), 5000);
                QVERIFY2(
                    side,
                    qPrintable(QStringLiteral("Pose indicator not visible: %1").arg(QLatin1String(info.objectName))));
                QVERIFY2(waitForCalState(side, SensorsComponentController::SideCalStateIncomplete, 5000),
                         qPrintable(QStringLiteral("Pose not Incomplete at start (state %1): %2")
                                        .arg(QLatin1String(calStateName(side->property("calState").toInt())),
                                             QLatin1String(info.objectName))));
            }

            QQuickItem* panel = findVisibleItem(_rootItem, QStringLiteral("sensors_calibrationPanel"), 2000);
            QObject* controller = panelController(panel);
            QVERIFY2(controller, "Calibration panel not visible during calibration");
            const auto progress = [&] { return controller->property("calibrationProgress").toDouble(); };
            QVERIFY2(qFuzzyIsNull(progress()), "Progress not at 0 at calibration start");

            QQuickItem* counter = findVisibleItem(_rootItem, QStringLiteral("sensorsCal_counter"), 2000);
            QVERIFY2(counter && waitForText(counter, QStringLiteral("0 of 6 sides done"), 3000),
                     "Counter not at 0 of 6 at calibration start");

            QQuickItem* cancelButton =
                findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_cancelCalibration"), 2000);
            QVERIFY2(cancelButton, "Cancel button not visible during calibration");

            // ---------------------------------------------------------------------
            // Place the vehicle into each pose and watch the UI track the side
            // ---------------------------------------------------------------------
            int sidesDone = 0;

            for (const PoseInfo& info : kPoses) {
                const QString sideName = QLatin1String(info.objectName);
                QQuickItem* side = findVisibleItem(_rootItem, sideName);
                QVERIFY2(side, qPrintable(QStringLiteral("Pose indicator disappeared: %1").arg(sideName)));

                mockLink->setCalibrationPose(info.pose);

                // Orientation detected: the side goes InProgress. Unlike mag cal the
                // vehicle must be held still, so the static (non-rotating) image stays up
                QVERIFY2(waitForCalState(side, SensorsComponentController::SideCalStateInProgress, 5000),
                         qPrintable(QStringLiteral("Side never went in-progress: %1").arg(sideName)));
                QQuickItem* stage = findVisibleItem(_rootItem, QStringLiteral("sensorsCal_stage"));
                QVERIFY2(
                    stage && stage->property("side").toString() == QLatin1String(info.sideId) &&
                        !stage->property("rotating").toBool() &&
                        stage->property("instruction").toString() == QLatin1String("Hold still"),
                    qPrintable(QStringLiteral("The stage does not show the vehicle held still on %1").arg(sideName)));

                // Side completes and marks itself done
                QVERIFY2(waitForCalState(side, SensorsComponentController::SideCalStateCompleted, 5000),
                         qPrintable(QStringLiteral("Side never completed: %1").arg(sideName)));

                sidesDone++;

                // The counter follows the finished sides until the last one, which reports the result
                if (sidesDone < MockLinkPX4Calibration::kSideCount) {
                    QVERIFY2(waitForText(counter, QStringLiteral("%1 of 6 sides done").arg(sidesDone), 3000),
                             qPrintable(QStringLiteral("Counter after %1 sides is '%2'")
                                            .arg(sidesDone)
                                            .arg(counter->property("text").toString())));
                }

                // The firmware reports progress in 17% jumps after each side (see PX4
                // accel_calibration_worker())
                if (sidesDone < MockLinkPX4Calibration::kSideCount) {
                    const double expected = (17.0 * sidesDone) / 100.0;
                    QVERIFY2(qAbs(progress() - expected) < 0.005,
                             qPrintable(QStringLiteral("Unexpected progress after %1 sides: %2, expected %3")
                                            .arg(sidesDone)
                                            .arg(progress())
                                            .arg(expected)));
                }

                // Previously completed sides must remain marked complete
                for (const PoseInfo& doneInfo : kPoses) {
                    if (&doneInfo == &info)
                        break;
                    QQuickItem* doneSide = findVisibleItem(_rootItem, QLatin1String(doneInfo.objectName));
                    QVERIFY2(doneSide && (doneSide->property("calState").toInt() ==
                                          SensorsComponentController::SideCalStateCompleted),
                             qPrintable(QStringLiteral("Previously completed side no longer marked complete: %1")
                                            .arg(QLatin1String(doneInfo.objectName))));
                }
            }

            // ---------------------------------------------------------------------
            // Calibration complete: progress hits 100% and the cancel button hides.
            // Unlike mag cal there is no reboot needed.
            // ---------------------------------------------------------------------
            QVERIFY2(QTest::qWaitFor([&] { return qFuzzyCompare(progress(), 1.0); }, 5000),
                     qPrintable(QStringLiteral("Progress never reached 1.0: %1").arg(progress())));

            QVERIFY2(waitForText(counter, QStringLiteral("Calibrated"), 5000),
                     "Counter does not say Calibrated after the calibration finished");
            QVERIFY2(QTest::qWaitFor([&] { return !cancelButton->isVisible(); }, 5000),
                     "Cancel button still visible after calibration completed");
            QVERIFY2(!findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_rebootVehicle"), 0),
                     "Reboot button shown after an accelerometer calibration");

            waitForParamRefreshQuiet(vehicle);
            if (QTest::currentTestFailed())
                return;

            // All sides must remain marked complete (green) after calibration ends and
            // the post-calibration parameter refresh settles
            _verifyAllPosesState(SensorsComponentController::SideCalStateCompleted, "after calibration finished");
            if (QTest::currentTestFailed())
                return;

            // Done dismisses the calibration panel and brings the rows back
            QVERIFY2(clickButton(QStringLiteral("sensorsSetup_dismissCalibration")), "Failed to click Done");
            QVERIFY2(QTest::qWaitFor([&] { return !panel->isVisible(); }, 5000),
                     "Calibration panel still visible after Done");

            // The completed accel calibration set CAL_ACC0_ID on the vehicle: only the
            // Accelerometer row shows calibrated
            _verifySensorsSetupStates(
                {
                    .sensorsComplete = false,
                    .compassComplete = false,
                    .gyroscopeComplete = false,
                    .accelerometerComplete = true,
                },
                "after accel calibration");
        });
}

void PX4SensorsCalibrationUITest::_testAccelCalibrationCancel()
{
    _runCalibrationCancelTest(QStringLiteral("sensorsSetup_calibrateAccel"));
}

void PX4SensorsCalibrationUITest::_testInlineCalibration_data()
{
    QTest::addColumn<QString>("row");
    QTest::addColumn<QString>("resultParam");  ///< Changed by the simulated firmware when the calibration succeeds
    QTest::addColumn<QString>("failureText");

    QTest::newRow("baro") << "baro" << "CAL_BARO0_ID" << "No GPS data arrived";
    QTest::newRow("level") << "level" << "SENS_BOARD_X_OFF" << "The vehicle moved";
    QTest::newRow("gyro") << "gyro" << "CAL_GYRO0_ID" << "The vehicle moved";
}

void PX4SensorsCalibrationUITest::_testInlineCalibration()
{
    QFETCH(QString, row);
    QFETCH(QString, resultParam);
    QFETCH(QString, failureText);

    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [&](QPointer<MockLink> mockLink, Vehicle* vehicle) {
            _openSensorsPage(vehicle);
            if (QTest::currentTestFailed())
                return;

            ParameterManager* params = vehicle->parameterManager();
            if (row == QStringLiteral("level")) {
                // The level horizon calibration needs the gyroscope and accelerometer calibrated first
                params->getParameter(ParameterManager::defaultComponentId, QStringLiteral("CAL_GYRO0_ID"))
                    ->setRawValue(1);
                params->getParameter(ParameterManager::defaultComponentId, QStringLiteral("CAL_ACC0_ID"))
                    ->setRawValue(1);
            }

            const QString button = QStringLiteral("sensorsSetup_calibrate") + row.at(0).toUpper() + row.mid(1);
            const auto setFails = [&](bool fails) {
                mockLink->setBaroCalibrationFails(fails);
                mockLink->setLevelCalibrationFails(fails);
                mockLink->setGyroCalibrationFails(fails);
            };
            const auto statusText = [&] {
                QQuickItem* status = findVisibleItem(_rootItem, QStringLiteral("sensors_status_") + row, 0);
                return status ? status->property("text").toString() : QString();
            };

            // The calibration runs in its row: no confirmation dialog and no calibration panel
            setFails(true);
            QVERIFY2(clickButtonScrolled(button, QStringLiteral("setupPage_flickable")),
                     "Failed to click the row's Calibrate button");
            QVERIFY2(!findVisibleItem(_rootItem, QStringLiteral("popupDialog_acceptButton"), 0),
                     "Calibration asked for confirmation");
            QVERIFY2(!findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_cancelCalibration"), 0),
                     "Calibration opened the calibration panel");

            QVERIFY2(QTest::qWaitFor([&] { return statusText() == QStringLiteral("Calibration failed"); }, 10000),
                     qPrintable(QStringLiteral("Row status is '%1'").arg(statusText())));
            QQuickItem* hint = findVisibleItem(_rootItem, QStringLiteral("sensors_hint_") + row, 2000);
            QVERIFY2(hint && hint->property("text").toString().contains(failureText),
                     "Failure reason not shown in the row");
            QQuickItem* retry = findVisibleItem(_rootItem, button, 2000);
            QVERIFY(retry);
            QCOMPARE(retry->property("text").toString(), QStringLiteral("Retry"));
            QVERIFY2(retry->property("enabled").toBool(), "Retry is not enabled");

            // Retrying succeeds once the vehicle can calibrate
            Fact* result = params->getParameter(ParameterManager::defaultComponentId, resultParam);
            QVERIFY(result);
            const QVariant before = result->rawValue();
            setFails(false);
            QVERIFY(clickButton(button));
            QVERIFY2(QTest::qWaitFor([&] { return statusText() == QStringLiteral("Calibrated"); }, 10000),
                     qPrintable(QStringLiteral("Row status after retry is '%1'").arg(statusText())));
            // The parameter refresh after the calibration finishes is asynchronous
            QVERIFY2(QTest::qWaitFor([&] { return result->rawValue() != before; }, 10000),
                     "The calibration result was not stored");
            QVERIFY2(!findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_cancelCalibration"), 0),
                     "Calibration panel shown after calibrating");
        });
}

void PX4SensorsCalibrationUITest::_testInlineCalibrationCancel()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [&](QPointer<MockLink> mockLink, Vehicle* vehicle) {
            _openSensorsPage(vehicle);
            if (QTest::currentTestFailed())
                return;

            const QString button = QStringLiteral("sensorsSetup_calibrateGyro");
            const auto buttonItem = [&] { return findVisibleItem(_rootItem, button, 0); };
            const auto statusText = [&] {
                QQuickItem* status = findVisibleItem(_rootItem, QStringLiteral("sensors_status_gyro"), 0);
                return status ? status->property("text").toString() : QString();
            };

            // A guided panel left open on its ready screen must not stay up while a row calibrates
            QVERIFY2(clickButtonScrolled(QStringLiteral("sensorsSetup_calibrateAccel"),
                                         QStringLiteral("setupPage_flickable")),
                     "Failed to open the accelerometer panel");
            QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_startCalibration"), 3000),
                     "Ready screen not shown");

            // The panel loads its picture after it opens and moves the rows below it, so wait for the row to stop
            // moving before scrolling to it
            QPointF lastPosition;
            QElapsedTimer stillFor;
            QVERIFY2(QTest::qWaitFor(
                         [&] {
                             QQuickItem* item = buttonItem();
                             if (!item) {
                                 return false;
                             }
                             const QPointF position = item->mapToScene(QPointF());
                             if (!stillFor.isValid() || position != lastPosition) {
                                 lastPosition = position;
                                 stillFor.start();
                             }
                             return stillFor.elapsed() >= 500;
                         },
                         10000),
                     "The page kept moving after the panel opened");

            // The simulated firmware keeps calibrating until it is told to stop
            mockLink->setInlineCalibrationStalls(true);
            QVERIFY2(clickButtonScrolled(button, QStringLiteral("setupPage_flickable")), "Failed to click Calibrate");
            QVERIFY2(QTest::qWaitFor(
                         [&] {
                             QQuickItem* item = buttonItem();
                             return item && item->property("text").toString() == QStringLiteral("Cancel") &&
                                    item->property("enabled").toBool();
                         },
                         5000),
                     "The row does not offer Cancel while calibrating");
            QVERIFY2(!findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_startCalibration"), 0),
                     "The ready screen stayed up while the row calibrated");
            QQuickItem* alternate =
                findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_calibrateCompassAlternate"), 0);
            QVERIFY2(alternate && !alternate->property("enabled").toBool(),
                     "Large vehicle calibration is available while a row calibrates");
            QQuickItem* orientationRow = findVisibleItem(_rootItem, QStringLiteral("sensors_row_autopilot"), 0);
            QVERIFY2(orientationRow && !orientationRow->property("enabled").toBool(),
                     "Parameters can be edited while a row calibrates");

            // Closing the panel moved the page, so scroll to the row again
            QVERIFY2(clickButtonScrolled(button, QStringLiteral("setupPage_flickable")), "Failed to click Cancel");

            // Cancelling is not a failure: the row goes back to idle and can start again
            QVERIFY2(QTest::qWaitFor(
                         [&] {
                             QQuickItem* item = buttonItem();
                             return item && item->property("text").toString() == QStringLiteral("Calibrate") &&
                                    item->property("enabled").toBool();
                         },
                         10000),
                     "The row did not return to Calibrate after Cancel");
            QVERIFY2(
                statusText() != QStringLiteral("Calibration failed") && statusText() != QStringLiteral("Calibrating…"),
                qPrintable(QStringLiteral("Row status after Cancel is '%1'").arg(statusText())));
            QVERIFY2(QTest::qWaitFor([&] { return orientationRow->property("enabled").toBool(); }, 5000),
                     "Parameters stay locked after the calibration was cancelled");
            waitForParamRefreshQuiet(vehicle);
        });
}

void PX4SensorsCalibrationUITest::_testPageTabs()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [&](QPointer<MockLink> mockLink, Vehicle* vehicle) {
            Q_UNUSED(mockLink);
            _openSensorsPage(vehicle);
            if (QTest::currentTestFailed())
                return;

            const QString calibrationRow = QStringLiteral("sensors_row_gyro");
            const QString advancedRow = QStringLiteral("sensors_row_reset");
            QVERIFY2(findVisibleItem(_rootItem, calibrationRow, 0), "Calibration row not shown on the Calibration tab");
            QVERIFY2(!findVisibleItem(_rootItem, advancedRow, 0), "Advanced row shown on the Calibration tab");

            QVERIFY2(clickButton(QStringLiteral("sensors_tab_advanced")), "Failed to click the Advanced tab");
            QVERIFY2(findVisibleItem(_rootItem, advancedRow, 3000), "Advanced row not shown on the Advanced tab");
            QVERIFY2(QTest::qWaitFor([&] { return !findVisibleItem(_rootItem, calibrationRow, 0); }, 3000),
                     "Calibration row shown on the Advanced tab");

            QVERIFY2(clickButton(QStringLiteral("sensors_tab_calibration")), "Failed to click the Calibration tab");
            QVERIFY2(findVisibleItem(_rootItem, calibrationRow, 3000),
                     "Calibration row not shown after returning to the Calibration tab");
            QVERIFY2(QTest::qWaitFor([&] { return !findVisibleItem(_rootItem, advancedRow, 0); }, 3000),
                     "Advanced row still shown on the Calibration tab");
        });
}

void PX4SensorsCalibrationUITest::_testOrientationCalibrationFailure_data()
{
    QTest::addColumn<QString>("button");

    QTest::newRow("compass") << "sensorsSetup_calibrateCompass";
    QTest::newRow("accel") << "sensorsSetup_calibrateAccel";
}

void PX4SensorsCalibrationUITest::_testOrientationCalibrationFailure()
{
    QFETCH(QString, button);

    runWithMockLink([] { return MockLink::startPX4MockLink(); },
                    [&](QPointer<MockLink> mockLink, Vehicle* vehicle) {
                        _openSensorsPage(vehicle);
                        if (QTest::currentTestFailed())
                            return;

                        _startCalibration(button);
                        if (QTest::currentTestFailed())
                            return;

                        // One side is under way when the firmware gives up
                        const PoseInfo& info = kPoses[0];
                        QQuickItem* side = findVisibleItem(_rootItem, QLatin1String(info.objectName), 5000);
                        QVERIFY2(side, "Pose indicator not visible after calibration start");
                        mockLink->setCalibrationPose(info.pose);
                        QVERIFY2(waitForCalState(side, SensorsComponentController::SideCalStateInProgress, 5000),
                                 "Side never went in-progress");

                        // The firmware reports the failure and stops, so a new request is accepted
                        QVERIFY2(mockLink->failCalibration(), "No calibration was running");

                        // The failure shows in the panel without a message box, and nothing is kept
                        QQuickItem* title = findVisibleItem(_rootItem, QStringLiteral("sensorsCal_calloutTitle"), 5000);
                        QVERIFY(title);
                        QVERIFY2(waitForText(title, QStringLiteral("Calibration failed"), 5000),
                                 qPrintable(QStringLiteral("Callout is '%1'").arg(title->property("text").toString())));
                        QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_retryCalibration"), 2000),
                                 "Retry not shown after the failure");
                        QQuickItem* counter = findVisibleItem(_rootItem, QStringLiteral("sensorsCal_counter"), 2000);
                        QVERIFY2(counter && waitForText(counter, QStringLiteral("Failed"), 3000),
                                 "Counter does not say Failed");
                        QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("sensorsCal_log"), 2000),
                                 "The log is not shown after the failure");
                        QVERIFY2(waitForCalState(side, SensorsComponentController::SideCalStateIdle, 5000),
                                 "A side kept its state after the failure");
                        QVERIFY2(!findVisibleItem(_rootItem, QStringLiteral("popupDialog_acceptButton"), 0),
                                 "The failure opened a message box");

                        // Retry starts the whole calibration again, from the first side
                        QVERIFY2(clickButton(QStringLiteral("sensorsSetup_retryCalibration")), "Failed to click Retry");
                        QVERIFY2(waitForCalState(side, SensorsComponentController::SideCalStateIncomplete, 5000),
                                 "Retry did not start the calibration again");
                        QVERIFY2(waitForText(counter, QStringLiteral("0 of 6 sides done"), 3000),
                                 "Counter not at 0 of 6 after Retry");

                        // Leave the calibration cancelled, as the other tests do
                        QQuickItem* cancelButton =
                            findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_cancelCalibration"), 3000);
                        QVERIFY2(cancelButton && clickButton(QStringLiteral("sensorsSetup_cancelCalibration")),
                                 "Failed to click Cancel");
                        QVERIFY2(QTest::qWaitFor([&] { return !cancelButton->isVisible(); }, 5000),
                                 "Cancel button still visible after cancel");

                        waitForParamRefreshQuiet(vehicle);
                    });
}

void PX4SensorsCalibrationUITest::_testLargeVehicleCalibration()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [&](QPointer<MockLink> mockLink, Vehicle* vehicle) {
            _openSensorsPage(vehicle);
            if (QTest::currentTestFailed())
                return;

            // The compass row's large vehicle option opens the ready screen with the form open
            QVERIFY2(clickButtonScrolled(QStringLiteral("sensorsSetup_calibrateCompassAlternate"),
                                         QStringLiteral("setupPage_flickable")),
                     "Failed to click Large vehicle calibration");
            QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_startCalibration"), 3000),
                     "Ready screen not shown");
            QQuickItem* calibrate =
                findVisibleItem(_rootItem, QStringLiteral("sensorsCal_largeVehicleCalibrate"), 3000);
            QVERIFY2(calibrate, "Large vehicle form not open");

            // MockLink has a 3D GPS fix, which the calibration needs
            QVERIFY2(QTest::qWaitFor([&] { return calibrate->property("enabled").toBool(); }, 5000),
                     "Calibrate is not enabled with a GPS fix");
            // The vehicle refuses the command, as it does when armed: the form says so at once and Calibrate can be
            // used again
            mockLink->setLargeVehicleCalibrationRejected(true);
            QVERIFY2(clickButtonScrolled(QStringLiteral("sensorsCal_largeVehicleCalibrate"),
                                         QStringLiteral("setupPage_flickable")),
                     "Failed to click Calibrate");
            QQuickItem* hint = findVisibleItem(_rootItem, QStringLiteral("sensorsCal_largeVehicleHint"), 3000);
            QVERIFY2(hint, "No message after the vehicle refused the calibration");
            QVERIFY2(QTest::qWaitFor(
                         [&] {
                             return hint->property("text").toString().startsWith(
                                 QStringLiteral("The vehicle did not accept"));
                         },
                         3000),
                     qPrintable(QStringLiteral("Result is '%1'").arg(hint->property("text").toString())));
            QVERIFY2(QTest::qWaitFor([&] { return calibrate->property("enabled").toBool(); }, 3000),
                     "Calibrate is not available after the refusal");
            mockLink->setLargeVehicleCalibrationRejected(false);

            // The vehicle never reports a result: the form shows it is waiting, then says so, and Calibrate can be used
            // again
            QVERIFY2(clickButtonScrolled(QStringLiteral("sensorsCal_largeVehicleCalibrate"),
                                         QStringLiteral("setupPage_flickable")),
                     "Failed to click Calibrate");
            QVERIFY2(QTest::qWaitFor(
                         [&] { return hint->property("text").toString() == QStringLiteral("Calibrating…"); }, 3000),
                     qPrintable(QStringLiteral("Result is '%1'").arg(hint->property("text").toString())));
            QVERIFY2(!calibrate->property("enabled").toBool(), "Calibrate is available while waiting for the vehicle");
            QQuickItem* orientationRow = findVisibleItem(_rootItem, QStringLiteral("sensorsCal_orientationRow"), 0);
            QVERIFY2(orientationRow && !orientationRow->property("enabled").toBool(),
                     "The orientation can be changed while waiting for the vehicle");
            QVERIFY2(
                QTest::qWaitFor(
                    [&] { return hint->property("text").toString().startsWith(QStringLiteral("No result")); }, 10000),
                qPrintable(QStringLiteral("Result is '%1'").arg(hint->property("text").toString())));
            QVERIFY2(QTest::qWaitFor([&] { return calibrate->property("enabled").toBool(); }, 5000),
                     "Calibrate is not available after the timeout");
            QVERIFY2(orientationRow->property("enabled").toBool(), "The orientation stays locked after the timeout");
            QVERIFY2(clickButtonScrolled(QStringLiteral("sensorsCal_largeVehicleCalibrate"),
                                         QStringLiteral("setupPage_flickable")),
                     "Failed to click Calibrate again");

            mockLink->sendStatusTextMessage(MAV_SEVERITY_INFO, QStringLiteral("Mag quick calibration finished"));

            QVERIFY2(
                QTest::qWaitFor(
                    [&] { return hint->property("text").toString().startsWith(QStringLiteral("Offsets set")); }, 5000),
                qPrintable(QStringLiteral("Result is '%1'").arg(hint->property("text").toString())));
            QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("sensorsCal_largeVehicleReboot"), 2000),
                     "Reboot not offered after the calibration");

            // Cancel before starting just closes the panel
            QVERIFY2(clickButtonScrolled(QStringLiteral("sensorsSetup_closeCalibration"),
                                         QStringLiteral("setupPage_flickable")),
                     "Failed to click Cancel");
            QVERIFY2(
                QTest::qWaitFor(
                    [&] { return !findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_startCalibration"), 0); },
                    5000),
                "The ready screen is still shown after Cancel");
        });
}
