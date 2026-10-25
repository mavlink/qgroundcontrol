#include "PX4AirframeSetupUITest.h"

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "MockLink.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"

UT_REGISTER_TEST(PX4AirframeSetupUITest, TestLabel::Integration)

void PX4AirframeSetupUITest::_navigateToAirframePanel()
{
    navigateToConfigureView();
    if (QTest::currentTestFailed()) return;

    QQuickItem *airframeBtn = findVisibleItem(_rootItem, QStringLiteral("vehicleConfig_comp_Airframe"), 2000);
    QVERIFY2(airframeBtn, "vehicleConfig_comp_Airframe button not found");

    // SYS_AUTOSTART was reset to 0 so AirframeComponent::setupComplete is false:
    // the button must show the config-required indicator
    (void) QTest::qWaitFor([&] { return !airframeBtn->property("setupComplete").toBool(); }, 3000);
    QVERIFY2(!airframeBtn->property("setupComplete").toBool(),
             "Airframe button does not show config required after SYS_AUTOSTART reset");

    clickSidebarButton(QStringLiteral("vehicleConfig_comp_Airframe"));
    if (QTest::currentTestFailed()) return;

    QVERIFY2(QTest::qWaitFor([&] { return airframeBtn->property("checked").toBool(); }, 3000),
             "Airframe button not selected after clicking it");

    // SYS_AUTOSTART is 0 (no airframe set) which is a standard metadata value, so
    // the normal airframe-selection page shows rather than the custom config panel
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("airframe_filter"), 3000),
             "Airframe selection page not found after opening Airframe page");

    // No airframe has been selected yet so Apply and Restart must not show
    QVERIFY2(!findVisibleItem(_rootItem, QStringLiteral("airframeSetup_applyButton"), 0),
             "Apply and Restart button shown before an airframe is selected");

    // The standard airframes show first so users don't have to scroll past the
    // exotic frames to find a regular quad
    QQuickItem *firstTypeBox = findVisibleItem(_rootItem, QStringLiteral("airframeTypeBox_0"), 3000);
    QVERIFY2(firstTypeBox, "First airframe type box not found");
    QCOMPARE(firstTypeBox->property("airframeTypeName").toString(), QStringLiteral("Quadrotor x"));

    // No airframe is configured (SYS_AUTOSTART is 0) so the category filter
    // starts on All
    QQuickItem* filterBar = findVisibleItem(_rootItem, QStringLiteral("airframe_filter"), 3000);
    QVERIFY2(filterBar, "Airframe category filter not found");
    QCOMPARE(filterBar->property("filterIndex").toInt(), 0);

    // ...and no airframe type box may show as selected
    for (int i = 0; ; i++) {
        QQuickItem *typeBox = findVisibleItem(_rootItem, QStringLiteral("airframeTypeBox_%1").arg(i), 0);
        if (!typeBox) {
            QVERIFY2(i > 0, "No airframe type boxes found");
            break;
        }
        QVERIFY2(!typeBox->property("airframeTypeSelected").toBool(),
                 qPrintable(QStringLiteral("Airframe type box selected with no airframe configured: %1")
                                .arg(typeBox->property("airframeTypeName").toString())));
    }

    // Airframe is the first setup step and has no prerequisite, so the
    // prerequisite-setup message panel must not be shown
    QVERIFY2(!findVisibleItem(_rootItem, QStringLiteral("vehicleConfig_messagePanel"), 0),
             "Prerequisite setup message panel shown instead of Airframe page");
}

void PX4AirframeSetupUITest::_verifyAirframePrereq(const QString &compObjectName, bool expectPrereqShown, const QString &checkedSectionObjectName)
{
    QQuickItem *compBtn = clickSidebarButton(compObjectName);
    if (QTest::currentTestFailed()) return;

    QQuickItem *messagePanel = findVisibleItem(_rootItem, QStringLiteral("vehicleConfig_messagePanel"), expectPrereqShown ? 3000 : 0);
    if (expectPrereqShown) {
        QVERIFY2(messagePanel,
                 qPrintable(QStringLiteral("Airframe prerequisite message panel not shown for: %1").arg(compObjectName)));
    } else {
        QVERIFY2(!messagePanel,
                 qPrintable(QStringLiteral("Prerequisite message panel unexpectedly shown for: %1").arg(compObjectName)));
    }

    // Components with children must expand on click even when the prerequisite
    // is not met
    if (compBtn->property("expandable").toBool()) {
        QVERIFY2(QTest::qWaitFor([&] { return compBtn->property("expanded").toBool(); }, 3000),
                 qPrintable(QStringLiteral("Component tree did not expand after click: %1").arg(compObjectName)));
    }

    // Components which auto-select their first section on root click must do so
    // even when the prerequisite is not met
    if (!checkedSectionObjectName.isEmpty()) {
        QQuickItem *sectionBtn = findVisibleItem(_rootItem, checkedSectionObjectName, 3000);
        QVERIFY2(sectionBtn,
                 qPrintable(QStringLiteral("Section button not found: %1").arg(checkedSectionObjectName)));
        QVERIFY2(QTest::qWaitFor([&] { return sectionBtn->property("sectionChecked").toBool(); }, 3000),
                 qPrintable(QStringLiteral("First child section not selected after clicking %1: %2").arg(compObjectName, checkedSectionObjectName)));
    }
}

void PX4AirframeSetupUITest::_testNavigateToAirframe()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [&](QPointer<MockLink> mockLink, Vehicle *vehicle) {
    // Reset all params including SYS_AUTOSTART so the test starts with no
    // airframe configured
    mockLink->setResetSysAutostartOnParamReset(true);
    resetParamsToFirmwareDefaults(vehicle, QStringLiteral("SYS_AUTOSTART"));
    if (QTest::currentTestFailed()) return;

    _navigateToAirframePanel();
    });
}

void PX4AirframeSetupUITest::_testCurrentAirframeCategorySelected()
{
    runWithMockLink([] { return MockLink::startPX4MockLink(); },
                    [&](QPointer<MockLink>, Vehicle*) {
                        navigateToConfigureView();
                        if (QTest::currentTestFailed())
                            return;

                        clickSidebarButton(QStringLiteral("vehicleConfig_comp_Airframe"));
                        if (QTest::currentTestFailed())
                            return;

                        // The MockLink keeps its default SYS_AUTOSTART, a Quadrotor x, so the
                        // page opens on the Copter tab with that frame type selected
                        QQuickItem* filterBar = findVisibleItem(_rootItem, QStringLiteral("airframe_filter"), 3000);
                        QVERIFY2(filterBar, "Airframe category filter not found");
                        QCOMPARE(filterBar->property("filterIndex").toInt(), 1);

                        QQuickItem* typeBox = findVisibleItem(_rootItem, QStringLiteral("airframeTypeBox_0"), 3000);
                        QVERIFY2(typeBox, "airframeTypeBox_0 not found");
                        QCOMPARE(typeBox->property("airframeTypeName").toString(), QStringLiteral("Quadrotor x"));
                        QVERIFY2(typeBox->property("airframeTypeSelected").toBool(),
                                 "Current airframe type not selected");
                    });
}

void PX4AirframeSetupUITest::_testPendingAirframeDiscarded()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [&](QPointer<MockLink> mockLink, Vehicle* vehicle) {
            mockLink->setResetSysAutostartOnParamReset(true);
            resetParamsToFirmwareDefaults(vehicle, QStringLiteral("SYS_AUTOSTART"));
            if (QTest::currentTestFailed())
                return;

            _navigateToAirframePanel();
            if (QTest::currentTestFailed())
                return;

            const QString flickable = QStringLiteral("setupPage_flickable");
            const auto pendingButtonsShown = [&](bool shown) {
                return QTest::qWaitFor(
                    [&] {
                        return !!findVisibleItem(_rootItem, QStringLiteral("airframeSetup_applyButton"), 0) == shown &&
                               !!findVisibleItem(_rootItem, QStringLiteral("airframeSetup_discardButton"), 0) == shown;
                    },
                    3000);
            };

            QVERIFY2(clickButtonScrolled(QStringLiteral("airframeTypeBox_0"), flickable),
                     "Failed to click first type box");
            QVERIFY2(clickButtonScrolled(QStringLiteral("airframeOption_0"), flickable),
                     "Failed to click first airframe option");
            QVERIFY2(pendingButtonsShown(true), "Apply/Discard not shown after selecting an airframe");

            // Discard forgets the pending airframe
            QVERIFY2(clickButtonScrolled(QStringLiteral("airframeSetup_discardButton"), flickable),
                     "Failed to click Discard");
            QVERIFY2(pendingButtonsShown(false), "Apply/Discard still shown after Discard");

            // Selecting a different type forgets it too
            QVERIFY2(clickButtonScrolled(QStringLiteral("airframeOption_0"), flickable),
                     "Failed to click first airframe option again");
            QVERIFY2(pendingButtonsShown(true), "Apply/Discard not shown after selecting an airframe again");
            QVERIFY2(clickButtonScrolled(QStringLiteral("airframeTypeBox_1"), flickable),
                     "Failed to click second type box");
            QVERIFY2(pendingButtonsShown(false), "Apply/Discard still shown after selecting a different type");
        });
}

void PX4AirframeSetupUITest::_testAirframePrereqPages()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [&](QPointer<MockLink> mockLink, Vehicle *vehicle) {
    // Reset all params including SYS_AUTOSTART so Airframe setup is incomplete
    mockLink->setResetSysAutostartOnParamReset(true);
    resetParamsToFirmwareDefaults(vehicle, QStringLiteral("SYS_AUTOSTART"));
    if (QTest::currentTestFailed()) return;

    navigateToConfigureView();
    if (QTest::currentTestFailed()) return;

    // With Airframe setup incomplete, PX4AutoPilotPlugin::prerequisiteSetup()
    // requires Airframe first for Sensors, ESC, Battery, Return & Land, Collision, PID Tuning and
    // Flight Modes (COM_RC_IN_MODE defaults to 0), so clicking those pages
    // shows the prerequisite message panel instead of the page. Expandable
    // components must still expand.
    struct PrereqCheck {
        const char *objectName;
        bool expectPrereqShown;
        const char *checkedSectionObjectName;
    };
    // clang-format off
    const PrereqCheck checks[] = {
        { .objectName = "vehicleConfig_comp_Sensors",     .expectPrereqShown = true,  .checkedSectionObjectName = "" },
        { .objectName = "vehicleConfig_comp_ESC",         .expectPrereqShown = true,  .checkedSectionObjectName = "" },
        { .objectName = "vehicleConfig_comp_Battery",     .expectPrereqShown = true,  .checkedSectionObjectName = "" },
        { .objectName = "vehicleConfig_comp_Return&Land", .expectPrereqShown = true,  .checkedSectionObjectName = "" },
        { .objectName = "vehicleConfig_comp_Collision",   .expectPrereqShown = true,  .checkedSectionObjectName = "" },
        { .objectName = "vehicleConfig_comp_PIDTuning",   .expectPrereqShown = true,  .checkedSectionObjectName = "" },
        { .objectName = "vehicleConfig_comp_FlightModes", .expectPrereqShown = true,  .checkedSectionObjectName = "" },
        { .objectName = "vehicleConfig_comp_Radio",       .expectPrereqShown = false, .checkedSectionObjectName = "" },
        { .objectName = "vehicleConfig_comp_Joystick",    .expectPrereqShown = false, .checkedSectionObjectName = "" },
        { .objectName = "vehicleConfig_comp_Airframe",    .expectPrereqShown = false, .checkedSectionObjectName = "" },
    };
    // clang-format on
    for (const PrereqCheck &check : checks) {
        _verifyAirframePrereq(QLatin1String(check.objectName), check.expectPrereqShown, QLatin1String(check.checkedSectionObjectName));
        if (QTest::currentTestFailed()) return;
    }
    });
}

void PX4AirframeSetupUITest::_testApplyAirframe()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [&](QPointer<MockLink> mockLink, Vehicle *vehicle) {
    // Reset all params including SYS_AUTOSTART so the test starts with no
    // airframe configured
    mockLink->setResetSysAutostartOnParamReset(true);
    resetParamsToFirmwareDefaults(vehicle, QStringLiteral("SYS_AUTOSTART"));
    if (QTest::currentTestFailed()) return;

    _navigateToAirframePanel();
    if (QTest::currentTestFailed()) return;

    // Select the Quadrotor x airframe type box
    QQuickItem *typeBox = findVisibleItem(_rootItem, QStringLiteral("airframeTypeBox_0"), 3000);
    QVERIFY2(typeBox, "airframeTypeBox_0 not found");
    const QPointF boxCenter = typeBox->mapToScene(QPointF(typeBox->width() / 2, typeBox->height() / 2));
    QTest::mouseClick(_window, Qt::LeftButton, Qt::NoModifier, boxCenter.toPoint());
    QTest::qWait(_pageDelay);

    // The clicked airframe type box must now show as selected
    QVERIFY2(QTest::qWaitFor([&] { return typeBox->property("airframeTypeSelected").toBool(); }, 3000),
             "Quadrotor x type box not selected after clicking it");

    // Choosing an airframe of the selected type shows Apply and Restart. The
    // expected autostart id comes from the option itself so the test doesn't
    // hardcode airframe metadata values.
    QQuickItem* firstOption = findVisibleItem(_rootItem, QStringLiteral("airframeOption_0"), 3000);
    QVERIFY2(firstOption, "First airframe option not found for the selected type");
    const int expectedAutostartId = firstOption->property("autostartId").toInt();
    QVERIFY2(expectedAutostartId != 0, "First Quadrotor x airframe has no autostart id");
    QVERIFY2(clickButtonScrolled(QStringLiteral("airframeOption_0"), QStringLiteral("setupPage_flickable")),
             "Failed to click first airframe option");

    QQuickItem *applyButton = findVisibleItem(_rootItem, QStringLiteral("airframeSetup_applyButton"), 3000);
    QVERIFY2(applyButton, "Apply and Restart button not shown after selecting an airframe");

    // Apply and confirm the restart dialog. The reboot command, its ack and the
    // link disconnect all happen within a single timer callback, so a signal spy
    // is required to observe the command result: polling MockLink would only see
    // the state after the link is already gone.
    QSignalSpy spyCmdResult(vehicle, &Vehicle::mavCommandResult);
    QVERIFY2(spyCmdResult.isValid(), "Failed to create mavCommandResult spy");

    // SYS_AUTOSTART requires a vehicle reboot, so applying the airframe must
    // pop the reboot-required notice
    expectAppMessage(QRegularExpression(QStringLiteral("Vehicle reboot required for changes to take effect")));

    QVERIFY2(clickButtonScrolled(QStringLiteral("airframeSetup_applyButton"), QStringLiteral("setupPage_flickable")),
             "Failed to click Apply and Restart");
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("popupDialog_acceptButton"), 3000),
             "Apply confirmation dialog not shown");
    QVERIFY2(clickButton(QStringLiteral("popupDialog_acceptButton")), "Failed to confirm Apply dialog");

    // Both params must arrive at the simulated firmware via PARAM_SET
    QVERIFY2(QTest::qWaitFor([&] {
                 return mockLink && mockLink->paramValue(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("SYS_AUTOSTART")).toInt() == expectedAutostartId;
             }, 10000),
             "SYS_AUTOSTART never reached the expected autostart id on MockLink");
    QVERIFY2(QTest::qWaitFor([&] {
                 return mockLink && mockLink->paramValue(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("SYS_AUTOCONFIG")).toInt() == 1;
             }, 10000),
             "SYS_AUTOCONFIG never set to 1 on MockLink");

    // The controller then reboots the vehicle: MockLink must ack
    // MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN with MAV_RESULT_ACCEPTED
    const auto rebootAccepted = [&] {
        for (const QList<QVariant> &args : spyCmdResult) {
            if ((args.at(2).toInt() == MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN) && (args.at(3).toInt() == MAV_RESULT_ACCEPTED)) {
                return true;
            }
        }
        return false;
    };
    QVERIFY2(QTest::qWaitFor(rebootAccepted, 10000),
             "MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN never accepted by MockLink");

    // The restart flow disconnects the link, dropping the active vehicle
    QVERIFY2(QTest::qWaitFor([&] { return MultiVehicleManager::instance()->activeVehicle() == nullptr; }, 10000),
             "Vehicle never disconnected after Apply and Restart");

    // By now the SYS_AUTOSTART write has happened, so the reboot-required
    // notice must have been shown — dismiss and verify it.
    QVERIFY2(acceptDialog(5000), "Vehicle reboot-required notice never shown");
    verifyExpectedLogMessage();
    });
}
