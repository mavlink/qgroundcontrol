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
    const char *rotateImage;    ///< Image shown while rotating on this side
};

constexpr PoseInfo kPoses[MockLinkPX4Calibration::kSideCount] = {
    { MockLinkPX4Calibration::Pose::RightSideUp, "sensorsCal_downSide",       "VehicleDownRotate.png" },
    { MockLinkPX4Calibration::Pose::UpsideDown,  "sensorsCal_upsideDownSide", "VehicleUpsideDownRotate.png" },
    { MockLinkPX4Calibration::Pose::NoseDown,    "sensorsCal_noseDownSide",   "VehicleNoseDownRotate.png" },
    { MockLinkPX4Calibration::Pose::TailDown,    "sensorsCal_tailDownSide",   "VehicleTailDownRotate.png" },
    { MockLinkPX4Calibration::Pose::Left,        "sensorsCal_leftSide",       "VehicleLeftRotate.png" },
    { MockLinkPX4Calibration::Pose::Right,       "sensorsCal_rightSide",      "VehicleRightRotate.png" },
};

bool waitForCalState(QQuickItem *item, SensorsComponentController::SideCalState expected, int timeoutMs)
{
    return QTest::qWaitFor([&] { return item->property("calState").toInt() == static_cast<int>(expected); }, timeoutMs);
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

/// SensorsComponentController refreshes the CAL_*/SENS_* parameters whenever calibration
/// stops. Wait for that traffic to go quiet so link teardown doesn't cut off in-flight
/// PARAM_REQUEST_READs (which logs warnings that fail strict mode).
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

void PX4SensorsCalibrationUITest::_navigateToSensorsPanel()
{
    QVERIFY2(clickButton(QStringLiteral("toolbar_qgcLogo")), "Failed to click Q logo button");
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("toolbar_viewConfigure"), 2000),
             "toolbar_viewConfigure button not found");
    QVERIFY2(clickButton(QStringLiteral("toolbar_viewConfigure")), "Failed to click Configure button");
    QTest::qWait(_viewDelay);

    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("vehicleConfig_root"), 3000),
             "vehicleConfig_root not found after navigating to Configure");

    QQuickItem *sensorsBtn = findVisibleItem(_rootItem, QStringLiteral("vehicleConfig_comp_Sensors"), 2000);
    QVERIFY2(sensorsBtn, "vehicleConfig_comp_Sensors button not found");

    scrollIntoView(sensorsBtn, QStringLiteral("vehicleConfig_sidebarFlickable"));
    const QPointF center = sensorsBtn->mapToScene(QPointF(sensorsBtn->width() / 2, sensorsBtn->height() / 2));
    QTest::mouseClick(_window, Qt::LeftButton, Qt::NoModifier, center.toPoint());
    QTest::qWait(_pageDelay);

    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_calibrateCompass"), 3000),
             "Calibrate Compass button not found after opening Sensors page");
}

void PX4SensorsCalibrationUITest::_clickSidebarSection(const QString &objectName)
{
    QQuickItem *sectionBtn = findVisibleItem(_rootItem, objectName, 3000);
    QVERIFY2(sectionBtn, qPrintable(QStringLiteral("Section button not found: %1").arg(objectName)));

    scrollIntoView(sectionBtn, QStringLiteral("vehicleConfig_sidebarFlickable"));
    const QPointF center = sectionBtn->mapToScene(QPointF(sectionBtn->width() / 2, sectionBtn->height() / 2));
    QTest::mouseClick(_window, Qt::LeftButton, Qt::NoModifier, center.toPoint());
    QTest::qWait(_pageDelay);
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

void PX4SensorsCalibrationUITest::_startCompassCalibration()
{
    QVERIFY2(clickButton(QStringLiteral("sensorsSetup_calibrateCompass")), "Failed to click Calibrate Compass");

    // Accept the pre-calibration dialog which sends MAV_CMD_PREFLIGHT_CALIBRATION
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("popupDialog_acceptButton"), 3000),
             "Pre-calibration dialog Ok button not found");
    QVERIFY2(clickButton(QStringLiteral("popupDialog_acceptButton")), "Failed to accept pre-calibration dialog");
}

void PX4SensorsCalibrationUITest::_testMagCalibration()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(false, false, false); },
        [&](QPointer<MockLink> mockLink, Vehicle *vehicle) {
    _navigateToSensorsPanel();
    if (QTest::currentTestFailed()) return;

    // Select the Compass section: the idle orientation preview appears with all
    // six poses in the neutral Idle state
    _clickSidebarSection(QStringLiteral("vehicleConfig_section_Compass"));
    if (QTest::currentTestFailed()) return;

    _verifyAllPosesState(SensorsComponentController::SideCalStateIdle, "idle preview");
    if (QTest::currentTestFailed()) return;

    _startCompassCalibration();
    if (QTest::currentTestFailed()) return;

    // MockLink responds with "[cal] calibration started: 2 mag" which makes all
    // six pose indicators visible and Incomplete (red, not yet visited)
    for (const PoseInfo &info : kPoses) {
        QQuickItem *side = findVisibleItem(_rootItem, QLatin1String(info.objectName), 5000);
        QVERIFY2(side, qPrintable(QStringLiteral("Pose indicator not visible: %1").arg(QLatin1String(info.objectName))));
        QVERIFY2(waitForCalState(side, SensorsComponentController::SideCalStateIncomplete, 5000),
                 qPrintable(QStringLiteral("Pose not Incomplete at start (state %1): %2")
                                .arg(QLatin1String(calStateName(side->property("calState").toInt())),
                                     QLatin1String(info.objectName))));
    }

    QQuickItem *progressBar = findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_progressBar"), 2000);
    QVERIFY2(progressBar, "Progress bar not visible during calibration");
    QVERIFY2(qFuzzyIsNull(progressBar->property("value").toDouble()), "Progress bar not at 0 at calibration start");

    QQuickItem *cancelButton = findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_cancelCalibration"), 2000);
    QVERIFY2(cancelButton, "Cancel button not visible during calibration");

    // ---------------------------------------------------------------------
    // Place the vehicle into each pose and watch the UI track the side
    // ---------------------------------------------------------------------
    double lastProgress = 0.0;
    int sidesDone = 0;

    for (const PoseInfo &info : kPoses) {
        const QString sideName = QLatin1String(info.objectName);
        QQuickItem *side = findVisibleItem(_rootItem, sideName);
        QVERIFY2(side, qPrintable(QStringLiteral("Pose indicator disappeared: %1").arg(sideName)));

        mockLink->setCalibrationPose(info.pose);

        // Orientation detected: the side goes InProgress with the rotate animation
        QVERIFY2(waitForCalState(side, SensorsComponentController::SideCalStateInProgress, 5000),
                 qPrintable(QStringLiteral("Side never went in-progress: %1").arg(sideName)));
        QCOMPARE(side->property("calInProgressText").toString(), QStringLiteral("Rotate"));
        QVERIFY2(side->property("imageSource").toString().endsWith(QLatin1String(info.rotateImage)),
                 qPrintable(QStringLiteral("Wrong rotate image for %1: %2")
                                .arg(sideName, side->property("imageSource").toString())));

        // Side completes and marks itself done (green/Completed)
        QVERIFY2(waitForCalState(side, SensorsComponentController::SideCalStateCompleted, 5000),
                 qPrintable(QStringLiteral("Side never completed: %1").arg(sideName)));

        sidesDone++;

        // Progress bar must move forward with each side and stay within the
        // band expected for the number of completed sides
        const double progress = progressBar->property("value").toDouble();
        QVERIFY2(progress > lastProgress,
                 qPrintable(QStringLiteral("Progress did not advance after %1: %2 -> %3")
                                .arg(sideName).arg(lastProgress).arg(progress)));
        if (sidesDone < MockLinkPX4Calibration::kSideCount) {
            constexpr double sideCount = MockLinkPX4Calibration::kSideCount;
            QVERIFY2((progress >= ((sidesDone - 1) / sideCount)) && (progress <= ((sidesDone / sideCount) + 0.01)),
                     qPrintable(QStringLiteral("Progress out of expected band after %1 sides: %2")
                                    .arg(sidesDone).arg(progress)));
        }
        lastProgress = progress;

        // Previously completed sides must remain marked complete
        for (const PoseInfo &doneInfo : kPoses) {
            if (&doneInfo == &info) break;
            QQuickItem *doneSide = findVisibleItem(_rootItem, QLatin1String(doneInfo.objectName));
            QVERIFY2(doneSide && (doneSide->property("calState").toInt() == SensorsComponentController::SideCalStateCompleted),
                     qPrintable(QStringLiteral("Previously completed side no longer marked complete: %1")
                                    .arg(QLatin1String(doneInfo.objectName))));
        }
    }

    // ---------------------------------------------------------------------
    // Calibration complete: progress hits 100% and the completion dialog opens
    // ---------------------------------------------------------------------
    QVERIFY2(QTest::qWaitFor([&] { return qFuzzyCompare(progressBar->property("value").toDouble(), 1.0); }, 5000),
             qPrintable(QStringLiteral("Progress bar never reached 1.0: %1")
                            .arg(progressBar->property("value").toDouble())));

    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("popupDialog_acceptButton"), 5000),
             "Compass Calibration Complete dialog not shown");
    QVERIFY2(clickButton(QStringLiteral("popupDialog_acceptButton")), "Failed to dismiss completion dialog");

    // Calibration no longer active: the cancel button row hides
    QVERIFY2(QTest::qWaitFor([&] { return !cancelButton->isVisible(); }, 5000),
             "Cancel button still visible after calibration completed");

    waitForParamRefreshQuiet(vehicle);

    // All sides must remain marked complete (green) after calibration ends and
    // the post-calibration parameter refresh settles
    _verifyAllPosesState(SensorsComponentController::SideCalStateCompleted, "after calibration finished");
    if (QTest::currentTestFailed()) return;

    // Switching the preview to a different sensor resets the completed sides:
    // the Accelerometer preview must show all poses in the neutral Idle state
    _clickSidebarSection(QStringLiteral("vehicleConfig_section_Accelerometer"));
    if (QTest::currentTestFailed()) return;

    _verifyAllPosesState(SensorsComponentController::SideCalStateIdle, "after section switch");

    });
}

void PX4SensorsCalibrationUITest::_testMagCalibrationCancel()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(false, false, false); },
        [&](QPointer<MockLink> mockLink, Vehicle *vehicle) {
    _navigateToSensorsPanel();
    if (QTest::currentTestFailed()) return;

    _startCompassCalibration();
    if (QTest::currentTestFailed()) return;

    // Start calibrating one side
    const PoseInfo &info = kPoses[0];
    QQuickItem *side = findVisibleItem(_rootItem, QLatin1String(info.objectName), 5000);
    QVERIFY2(side, "Pose indicator not visible after calibration start");

    mockLink->setCalibrationPose(info.pose);
    QVERIFY2(waitForCalState(side, SensorsComponentController::SideCalStateInProgress, 5000),
             "Side never went in-progress");

    // Cancel mid-calibration: MockLink responds with "[cal] calibration cancelled"
    QQuickItem *cancelButton = findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_cancelCalibration"));
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

    // Calibrate Compass button is clickable again
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_calibrateCompass"), 3000),
             "Calibrate Compass button not available after cancel");

    waitForParamRefreshQuiet(vehicle);

    });
}
