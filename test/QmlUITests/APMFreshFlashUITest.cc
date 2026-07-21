#include "APMFreshFlashUITest.h"

#include <QtCore/QPointer>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtGui/QColor>
#include <QtQml/QQmlProperty>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "AutoPilotPlugin.h"
#include "Fact.h"
#include "MockLink.h"
#include "ParameterManager.h"
#include "Vehicle.h"
#include "VehicleComponent.h"

UT_REGISTER_TEST(APMFreshFlashUITest, TestLabel::Integration)

void APMFreshFlashUITest::_verifyFramePrereq(const QString &compObjectName, bool expectPrereqShown)
{
    QQuickItem *compBtn = clickSidebarButton(compObjectName);
    if (QTest::currentTestFailed()) return;
    QVERIFY2(compBtn, qPrintable(QStringLiteral("Component button not found: %1").arg(compObjectName)));

    QQuickItem *messagePanel = findVisibleItem(_rootItem, QStringLiteral("vehicleConfig_messagePanel"), expectPrereqShown ? 3000 : 0);
    if (expectPrereqShown) {
        QVERIFY2(messagePanel,
                 qPrintable(QStringLiteral("Frame prerequisite message panel not shown for: %1").arg(compObjectName)));
    } else {
        QVERIFY2(!messagePanel,
                 qPrintable(QStringLiteral("Prerequisite message panel unexpectedly shown for: %1").arg(compObjectName)));
    }
}

void APMFreshFlashUITest::_verifySetupIndicator(const QString &compObjectName, bool expectSetupComplete)
{
    QPointer<QQuickItem> compBtn = findVisibleItem(_rootItem, compObjectName, 3000);
    QVERIFY2(compBtn, qPrintable(QStringLiteral("Component button not found: %1").arg(compObjectName)));

    // The property drives the visual: wait for it to settle first
    QVERIFY2(QTest::qWaitFor([&] { return compBtn && (compBtn->property("setupComplete").toBool() == expectSetupComplete); }, 5000),
             qPrintable(QStringLiteral("%1 setupComplete property never became %2")
                            .arg(compObjectName).arg(expectSetupComplete ? "true" : "false")));

    // Verify the actual rendered indicator: ConfigButton binds icon.color to
    // red when setup is incomplete, textColor otherwise
    const QColor iconColor = QQmlProperty::read(compBtn, QStringLiteral("icon.color")).value<QColor>();
    QVERIFY2(iconColor.isValid(),
             qPrintable(QStringLiteral("%1 icon.color could not be read").arg(compObjectName)));
    if (expectSetupComplete) {
        QVERIFY2(iconColor != QColor(Qt::red),
                 qPrintable(QStringLiteral("%1 icon still red although setup is complete").arg(compObjectName)));
    } else {
        QCOMPARE(iconColor, QColor(Qt::red));
    }
}

void APMFreshFlashUITest::_testFreshFlashSetupState()
{
    // Fresh-flash vehicles must show the setup-incomplete app message on connect
    expectAppMessage(QRegularExpression(QStringLiteral("Configuration tasks remain before this vehicle is ready to fly")));
    runWithMockLink(
        [] { return MockLink::startAPMArduCopterMockLink(MockConfiguration::OptionAPMStartFreshParams); },
        [&](QPointer<MockLink> /*mockLink*/, Vehicle *vehicle) {

    // Dismiss the setup-incomplete message dialog before interacting with the UI
    QVERIFY2(acceptDialog(5000), "Setup-incomplete app message dialog never shown");
    verifyExpectedLogMessage();
    if (QTest::currentTestFailed()) return;

    // -------------------------------------------------------------------
    // Fresh-flash parameter state: no airframe, uncalibrated sensors/radio
    // -------------------------------------------------------------------
    ParameterManager *mgr = vehicle->parameterManager();
    QCOMPARE(mgr->getParameter(ParameterManager::defaultComponentId, QStringLiteral("FRAME_CLASS"))->rawValue().toInt(), 0);
    QVERIFY2(qFuzzyIsNull(mgr->getParameter(ParameterManager::defaultComponentId, QStringLiteral("COMPASS_OFS_X"))->rawValue().toFloat()),
             "COMPASS_OFS_X not 0 on fresh-flash MockLink");
    QVERIFY2(qFuzzyIsNull(mgr->getParameter(ParameterManager::defaultComponentId, QStringLiteral("INS_ACCOFFS_X"))->rawValue().toFloat()),
             "INS_ACCOFFS_X not 0 on fresh-flash MockLink");
    QCOMPARE(mgr->getParameter(ParameterManager::defaultComponentId, QStringLiteral("RC1_MIN"))->rawValue().toInt(), 1100);
    QCOMPARE(mgr->getParameter(ParameterManager::defaultComponentId, QStringLiteral("RC1_MAX"))->rawValue().toInt(), 1900);
    QCOMPARE(mgr->getParameter(ParameterManager::defaultComponentId, QStringLiteral("RC1_TRIM"))->rawValue().toInt(), 1500);

    // -------------------------------------------------------------------
    // Frame, Sensors and Radio all report setup required
    // -------------------------------------------------------------------
    VehicleComponent *frameComp = findVehicleComponent(vehicle, QStringLiteral("Frame"));
    VehicleComponent *sensorsComp = findVehicleComponent(vehicle, QStringLiteral("Sensors"));
    VehicleComponent *radioComp = findVehicleComponent(vehicle, QStringLiteral("Radio"));
    QVERIFY2(frameComp, "Frame component not found");
    QVERIFY2(sensorsComp, "Sensors component not found");
    QVERIFY2(radioComp, "Radio component not found");

    QVERIFY2(!frameComp->setupComplete(), "Frame setupComplete despite FRAME_CLASS == 0");
    QVERIFY2(!sensorsComp->setupComplete(), "Sensors setupComplete despite zero cal offsets");
    QVERIFY2(!radioComp->setupComplete(), "Radio setupComplete despite default RC min/max/trim");

    // Frame is the required prerequisite for Sensors and Radio; Frame itself has none
    AutoPilotPlugin *autopilot = vehicle->autopilotPlugin();
    QCOMPARE(autopilot->prerequisiteSetup(sensorsComp), frameComp->name());
    QCOMPARE(autopilot->prerequisiteSetup(radioComp), frameComp->name());
    QVERIFY2(autopilot->prerequisiteSetup(frameComp).isEmpty(), "Frame unexpectedly has a prerequisite");

    // -------------------------------------------------------------------
    // UI: Sensors and Radio show the prerequisite message panel, Frame opens
    // -------------------------------------------------------------------
    navigateToConfigureView();
    if (QTest::currentTestFailed()) return;

    // All three sidebar buttons must show the red setup-required icon
    _verifySetupIndicator(QStringLiteral("vehicleConfig_comp_Frame"), /*expectSetupComplete=*/false);
    if (QTest::currentTestFailed()) return;
    _verifySetupIndicator(QStringLiteral("vehicleConfig_comp_Sensors"), /*expectSetupComplete=*/false);
    if (QTest::currentTestFailed()) return;
    _verifySetupIndicator(QStringLiteral("vehicleConfig_comp_Radio"), /*expectSetupComplete=*/false);
    if (QTest::currentTestFailed()) return;

    _verifyFramePrereq(QStringLiteral("vehicleConfig_comp_Frame"), /*expectPrereqShown=*/false);
    if (QTest::currentTestFailed()) return;
    _verifyFramePrereq(QStringLiteral("vehicleConfig_comp_Sensors"), /*expectPrereqShown=*/true);
    if (QTest::currentTestFailed()) return;
    _verifyFramePrereq(QStringLiteral("vehicleConfig_comp_Radio"), /*expectPrereqShown=*/true);
    if (QTest::currentTestFailed()) return;

    // -------------------------------------------------------------------
    // Select the Quad frame class from the Frame page. Clicking the box
    // writes FRAME_TYPE (default X) then FRAME_CLASS, completing Frame setup.
    // -------------------------------------------------------------------
    clickSidebarButton(QStringLiteral("vehicleConfig_comp_Frame"));
    if (QTest::currentTestFailed()) return;

    QQuickItem *quadBox = findVisibleItem(_rootItem, QStringLiteral("apmAirframeBox_Quad"), 3000);
    QVERIFY2(quadBox, "Quad airframe box not found on Frame page");

    // FRAME_CLASS/FRAME_TYPE require a vehicle reboot, so selecting a frame
    // must pop the reboot app message
    expectAppMessage(QRegularExpression(QStringLiteral("Reboot vehicle for changes to take effect")));

    Fact *frameClassFact = mgr->getParameter(ParameterManager::defaultComponentId, QStringLiteral("FRAME_CLASS"));
    Fact *frameTypeFact = mgr->getParameter(ParameterManager::defaultComponentId, QStringLiteral("FRAME_TYPE"));
    QSignalSpy frameClassAckSpy(frameClassFact, &Fact::vehicleUpdated);
    QVERIFY2(frameClassAckSpy.isValid(), "Failed to create FRAME_CLASS vehicleUpdated spy");

    const QPointF boxCenter = quadBox->mapToScene(QPointF(quadBox->width() / 2, quadBox->height() / 2));
    QTest::mouseClick(_window, Qt::LeftButton, Qt::NoModifier, boxCenter.toPoint());
    QTest::qWait(_pageDelay);

    QVERIFY2(QTest::qWaitFor([&] { return frameClassFact->rawValue().toInt() == 1; }, 5000),
             "FRAME_CLASS never set to Quad after clicking the Quad box");
    QCOMPARE(frameTypeFact->rawValue().toInt(), 1); // X is the Quad default frame type

    // The reboot message fires when the vehicle acks the param write: dismiss
    // the resulting dialog before interacting with the UI again
    QVERIFY2(QTest::qWaitFor([&] { return frameClassAckSpy.count() >= 1; }, 5000),
             "FRAME_CLASS write never acked by the vehicle");
    QVERIFY2(acceptDialog(5000), "Reboot app message dialog never shown");
    verifyExpectedLogMessage();
    if (QTest::currentTestFailed()) return;

    // The red setup-required indicator on the Frame sidebar button must turn
    // green (setupComplete) once a frame class is selected
    QVERIFY2(QTest::qWaitFor([&] { return frameComp->setupComplete(); }, 5000),
             "Frame setupComplete still false after selecting Quad frame");
    _verifySetupIndicator(QStringLiteral("vehicleConfig_comp_Frame"), /*expectSetupComplete=*/true);
    if (QTest::currentTestFailed()) return;

    // Sensors and Radio remain uncalibrated, so their icons must stay red
    _verifySetupIndicator(QStringLiteral("vehicleConfig_comp_Sensors"), /*expectSetupComplete=*/false);
    if (QTest::currentTestFailed()) return;
    _verifySetupIndicator(QStringLiteral("vehicleConfig_comp_Radio"), /*expectSetupComplete=*/false);
    if (QTest::currentTestFailed()) return;

    // -------------------------------------------------------------------
    // Frame setup complete: Sensors and Radio prerequisites must clear
    // -------------------------------------------------------------------
    QVERIFY2(autopilot->prerequisiteSetup(sensorsComp).isEmpty(), "Sensors still has a prerequisite after frame selection");
    QVERIFY2(autopilot->prerequisiteSetup(radioComp).isEmpty(), "Radio still has a prerequisite after frame selection");

    _verifyFramePrereq(QStringLiteral("vehicleConfig_comp_Sensors"), /*expectPrereqShown=*/false);
    if (QTest::currentTestFailed()) return;
    _verifyFramePrereq(QStringLiteral("vehicleConfig_comp_Radio"), /*expectPrereqShown=*/false);
    if (QTest::currentTestFailed()) return;

    // -------------------------------------------------------------------
    // Run accel + compass calibration from the Sensors page: the Sensors
    // sidebar indicator must turn green while Radio stays red
    // -------------------------------------------------------------------
    navigateToAPMSensorsPage();
    if (QTest::currentTestFailed()) return;

    runAPMFullAccelCal();
    if (QTest::currentTestFailed()) return;
    waitForParamRefreshQuiet(vehicle);
    if (QTest::currentTestFailed()) return;

    runAPMCompassCal();
    if (QTest::currentTestFailed()) return;
    waitForParamRefreshQuiet(vehicle);
    if (QTest::currentTestFailed()) return;

    QVERIFY2(QTest::qWaitFor([&] { return sensorsComp->setupComplete(); }, 5000),
             "Sensors setupComplete still false after accel + compass calibration");
    _verifySetupIndicator(QStringLiteral("vehicleConfig_comp_Sensors"), /*expectSetupComplete=*/true);
    if (QTest::currentTestFailed()) return;

    // Radio is still uncalibrated: its indicator must remain red
    QVERIFY2(!radioComp->setupComplete(), "Radio setupComplete despite no radio calibration");
    _verifySetupIndicator(QStringLiteral("vehicleConfig_comp_Radio"), /*expectSetupComplete=*/false);

    });
}
