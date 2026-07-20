#include "VehicleConfigUITestBase.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QPointer>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QTest>

#include "AutoPilotPlugin.h"
#include "Fact.h"
#include "ParameterManager.h"
#include "Vehicle.h"
#include "VehicleComponent.h"

void VehicleConfigUITestBase::navigateToConfigureView()
{
    QVERIFY2(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewConfigure")),
             "Failed to navigate to Configure view");
    QTest::qWait(_viewDelay);

    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("vehicleConfig_root"), 3000),
             "vehicleConfig_root not found after navigating to Configure");
}

QQuickItem *VehicleConfigUITestBase::clickSidebarButton(const QString &objectName)
{
    QQuickItem *btn = findVisibleItem(_rootItem, objectName, 3000);
    if (!btn) {
        QTest::qFail(qPrintable(QStringLiteral("Sidebar button not found: %1").arg(objectName)), __FILE__, __LINE__);
        return nullptr;
    }

    scrollIntoView(btn, QStringLiteral("vehicleConfig_sidebarFlickable"));
    const QPointF center = btn->mapToScene(QPointF(btn->width() / 2, btn->height() / 2));
    QTest::mouseClick(_window, Qt::LeftButton, Qt::NoModifier, center.toPoint());
    QTest::qWait(_pageDelay);

    return btn;
}

void VehicleConfigUITestBase::resetParamsToFirmwareDefaults(Vehicle *vehicle, const QString &sentinelParamName)
{
    ParameterManager *mgr = vehicle->parameterManager();

    // The sentinel parameter is non-default in the MockLink params file but
    // defaults to 0 in the parameter metadata, so it signals the refresh completing
    Fact *sentinelFact = mgr->getParameter(ParameterManager::defaultComponentId, sentinelParamName);
    QVERIFY2(sentinelFact, qPrintable(QStringLiteral("%1 fact not found").arg(sentinelParamName)));
    QVERIFY2(sentinelFact->rawValue().toInt() != 0,
             qPrintable(QStringLiteral("%1 already at default before reset").arg(sentinelParamName)));

    // Sends MAV_CMD_PREFLIGHT_STORAGE param1=2 which MockLink handles by resetting
    // its parameters to the metadata defaults
    mgr->resetAllParametersToDefaults();
    mgr->refreshAllParameters();

    QVERIFY2(QTest::qWaitFor([&] { return sentinelFact->rawValue().toInt() == 0; }, 30000),
             "Parameters never refreshed to firmware defaults");
}

void VehicleConfigUITestBase::resetAPMParamsToUncalibrated(Vehicle *vehicle)
{
    ParameterManager *mgr = vehicle->parameterManager();

    QVERIFY2(mgr->parameterExists(ParameterManager::defaultComponentId, QStringLiteral("COMPASS_OFS_X")),
             "COMPASS_OFS_X parameter not found");
    Fact *compassOfs = mgr->getParameter(ParameterManager::defaultComponentId, QStringLiteral("COMPASS_OFS_X"));
    QVERIFY2(compassOfs, "COMPASS_OFS_X fact not found");

    // Sends MAV_CMD_PREFLIGHT_STORAGE param1=2 which MockLink's APM branch zeroes
    // the compass and accel offset parameters.
    mgr->resetAllParametersToDefaults();
    mgr->refreshAllParameters();

    QVERIFY2(QTest::qWaitFor([&] { return qFuzzyIsNull(compassOfs->rawValue().toFloat()); }, 30000),
             "COMPASS_OFS_X never refreshed to 0 after APM param reset");
}

void VehicleConfigUITestBase::clickThroughAllComponents(Vehicle *vehicle, const QString &vehicleName)
{
    const QString prefix = vehicleName.isEmpty() ? QString() : (vehicleName + QStringLiteral(": "));

    const QVariantList components = vehicle->autopilotPlugin()->vehicleComponents();
    QVERIFY2(!components.isEmpty(),
             qPrintable(QStringLiteral("%1No vehicle components found").arg(prefix)));

    for (const QVariant &compVariant : components) {
        auto *comp = compVariant.value<VehicleComponent *>();
        if (!comp) {
            continue;
        }

        // Match the objectName set in VehicleConfigView.qml:
        // "vehicleConfig_comp_" + compName.replace(/ /g, "")
        const QString cleanName  = QString(comp->name()).remove(QLatin1Char(' '));
        const QString buttonName = QStringLiteral("vehicleConfig_comp_") + cleanName;

        QQuickItem *btn = findVisibleItem(_rootItem, buttonName, 2000);
        if (!btn) {
            // Component may be hidden (e.g. optional peripheral not present) – skip silently
            continue;
        }

        clickSidebarButton(buttonName);
        if (QTest::currentTestFailed()) return;

        QQuickItem *loader = findVisibleItem(_rootItem, QStringLiteral("vehicleConfig_panelLoader"), 2000);
        QVERIFY2(loader,
                 qPrintable(QStringLiteral("%1vehicleConfig_panelLoader not found after clicking %2")
                                .arg(prefix, comp->name())));
        QVERIFY2(loader->property("item").value<QQuickItem *>() != nullptr,
                 qPrintable(QStringLiteral("%1Panel loader has no item after clicking %2")
                                .arg(prefix, comp->name())));
    }
}

void VehicleConfigUITestBase::waitForParamRefreshQuiet(Vehicle *vehicle)
{
    ParameterManager *mgr = vehicle->parameterManager();
    QElapsedTimer sinceLastResponse;
    sinceLastResponse.start();

    QObject context;
    QObject::connect(mgr, &ParameterManager::_paramRequestReadSuccess, &context,
                     [&] { sinceLastResponse.restart(); });
    QObject::connect(mgr, &ParameterManager::_paramRequestReadFailure, &context,
                     [&] { sinceLastResponse.restart(); });

    QVERIFY2(QTest::qWaitFor([&] { return sinceLastResponse.elapsed() > 500; }, 10000),
             "waitForParamRefreshQuiet: parameter refresh traffic still active after 10s");
}

void VehicleConfigUITestBase::navigateToAPMSensorsPage()
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

void VehicleConfigUITestBase::verifyAPMCalIndicators(bool compassGreen, bool accelGreen, const char *context)
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
        QPointer<QQuickItem> btn = findVisibleItem(_rootItem, QLatin1String(c.name), 3000);
        QVERIFY2(btn, qPrintable(QStringLiteral("Button not found (%1): %2")
                                     .arg(QLatin1String(context), QLatin1String(c.name))));
        (void) QTest::qWaitFor([&] {
            return btn && (btn->property("indicatorGreen").toBool() == c.expectedGreen);
        }, 3000);
        QVERIFY2(btn, qPrintable(QStringLiteral("Button destroyed while waiting (%1): %2")
                                     .arg(QLatin1String(context), QLatin1String(c.name))));
        QVERIFY2(btn->property("indicatorGreen").toBool() == c.expectedGreen,
                 qPrintable(QStringLiteral("indicatorGreen is %1, expected %2 (%3): %4")
                                .arg(btn->property("indicatorGreen").toBool())
                                .arg(c.expectedGreen)
                                .arg(QLatin1String(context), QLatin1String(c.name))));
    }
}

void VehicleConfigUITestBase::runAPMFullAccelCal()
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
        QPointer<QQuickItem> sideItem = findVisibleItem(_rootItem, QLatin1String(pose.sideObjectName), 10000);
        QVERIFY2(sideItem, qPrintable(QStringLiteral("Side indicator not visible: %1")
                                          .arg(QLatin1String(pose.sideObjectName))));

        // calState InProgress == 2 (VehicleRotationCal.CalState enum)
        QVERIFY2(QTest::qWaitFor([&] { return sideItem && (sideItem->property("calState").toInt() == 2); }, 10000),
                 qPrintable(QStringLiteral("Side never went InProgress: %1")
                                .arg(QLatin1String(pose.sideObjectName))));

        // Click Next — sends COMMAND_ACK back to MockLink to advance to next pose.
        // The controller enables the Next button when it receives the ACCELCAL_VEHICLE_POS message,
        // which happens on the same tick as the side going InProgress, so wait briefly for it.
        QPointer<QQuickItem> nextBtn = findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_nextButton"), 3000);
        QVERIFY2(nextBtn, "Next button not found");
        QVERIFY2(QTest::qWaitFor([&] { return nextBtn && nextBtn->property("enabled").toBool(); }, 3000),
                 qPrintable(QStringLiteral("Next button not enabled for side: %1")
                                .arg(QLatin1String(pose.sideObjectName))));
        QVERIFY2(clickButton(QStringLiteral("sensorsSetup_nextButton")), "Failed to click Next button");
    }

    // After the last pose MockLink sends ACCELCAL_VEHICLE_POS_SUCCESS → _stopCalibration(Success)
    // Progress bar goes to 1.0 and the cal area closes
    QPointer<QQuickItem> progressBar = findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_progressBar"), 2000);
    QVERIFY2(progressBar, "Progress bar not found during accel cal");
    QVERIFY2(QTest::qWaitFor([&] { return progressBar && qFuzzyCompare(progressBar->property("value").toDouble(), 1.0); }, 10000),
             "Progress bar never reached 1.0 after accel cal success");

    // Accel cal completion must show the generic post-calibration dialog
    // (postCalibrationDialog), NOT the compass results dialog with fitness bars
    // (postOnboardCompassCalibrationDialog).  Dismiss it so the sensors page is
    // unblocked before callers proceed.
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("postCalibrationDialog"), 5000),
             "Post-accel-cal dialog not shown");
    QVERIFY2(!_rootItem->findChild<QQuickItem*>(QStringLiteral("postOnboardCompassCalibrationDialog")),
             "Compass results dialog incorrectly shown after accel cal");
    QVERIFY2(clickButton(QStringLiteral("popupDialog_acceptButton")),
             "Failed to dismiss post-accel-cal dialog");
}

void VehicleConfigUITestBase::runAPMCompassCal()
{
    QVERIFY2(clickButton(QStringLiteral("sensorsSetup_calibrateCompass")),
             "Failed to click Compass button");

    // Orientation dialog opens → accept to start MAV_CMD_DO_START_MAG_CAL
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("popupDialog_acceptButton"), 5000),
             "Pre-compass-cal dialog accept button not found");
    QVERIFY2(clickButton(QStringLiteral("popupDialog_acceptButton")),
             "Failed to accept pre-compass-cal dialog");

    // MockLink sends MAG_CAL_PROGRESS 0→100 at ~10Hz
    QPointer<QQuickItem> progressBar = findVisibleItem(_rootItem, QStringLiteral("sensorsSetup_progressBar"), 2000);
    QVERIFY2(progressBar, "Progress bar not found during compass cal");

    // Wait for progress > 0 (calibration is running)
    QVERIFY2(QTest::qWaitFor([&] { return progressBar && (progressBar->property("value").toDouble() > 0.0); }, 5000),
             "Compass cal progress never advanced above 0");

    // Wait for completion: MAG_CAL_REPORT → StopCalibrationSuccessShowLog → dialog opens.
    // Note: StopCalibrationSuccessShowLog resets progress to 0 (not 1.0), so wait for
    // the post-cal dialog directly rather than waiting for progress == 1.0.
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("postOnboardCompassCalibrationDialog"), 25000),
             "Post-compass-cal dialog not shown");
    QVERIFY2(clickButton(QStringLiteral("popupDialog_acceptButton")),
             "Failed to dismiss post-compass-cal dialog");
}
