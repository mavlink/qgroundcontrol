#include "ToolbarIndicatorUITest.h"

#include <functional>

#include <QtCore/QFile>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QVariant>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "AppSettings.h"
#include "Fact.h"
#include "HealthAndArmingCheckReport.h"
#include "MavlinkSettings.h"
#include "MockConfiguration.h"
#include "MockLink.h"
#include "MultiVehicleManager.h"
#include "QGCPalette.h"
#include "SettingsManager.h"
#include "Vehicle.h"

UT_REGISTER_TEST(ToolbarIndicatorUITest, TestLabel::Integration)

namespace {

/// Finds the visible PreFlightCheckButton with the given name. The checklist is reparented into the dropdown, so this
/// walks visual children rather than QObject children.
QQuickItem* findChecklistButton(QQuickItem* item, const QString& name)
{
    if (!item || !item->isVisible()) {
        return nullptr;
    }
    if (item->property("manualText").isValid() && item->property("name").toString() == name) {
        return item;
    }
    for (QQuickItem* const child : item->childItems()) {
        if (QQuickItem* const found = findChecklistButton(child, name)) {
            return found;
        }
    }
    return nullptr;
}

/// Finds the first enabled PreFlightCheckButton that still needs manual confirmation, whatever the checklist contains.
QQuickItem* findPendingManualChecklistButton(QQuickItem* item)
{
    if (!item || !item->isVisible()) {
        return nullptr;
    }
    if (item->property("manualText").isValid() && !item->property("manualText").toString().isEmpty() &&
        item->isEnabled() && !item->property("passed").toBool() && !item->property("telemetryFailure").toBool()) {
        return item;
    }
    for (QQuickItem* const child : item->childItems()) {
        if (QQuickItem* const found = findPendingManualChecklistButton(child)) {
            return found;
        }
    }
    return nullptr;
}

/// Finds the visible PreFlightCheckGroup with the given name.
QQuickItem* findChecklistGroup(QQuickItem* item, const QString& name)
{
    if (!item || !item->isVisible()) {
        return nullptr;
    }
    if (item->property("failedCount").isValid() && item->property("name").toString() == name) {
        return item;
    }
    for (QQuickItem* const child : item->childItems()) {
        if (QQuickItem* const found = findChecklistGroup(child, name)) {
            return found;
        }
    }
    return nullptr;
}

/// Stands in for the PreVehicleChecks.qml a custom build provides
constexpr const char* kPreVehicleChecksFixture = R"(
import QtQuick

import QGroundControl.Controls

Item {
    property var model: listModel

    PreFlightCheckModel {
        id: listModel

        PreFlightCheckGroup {
            name: "Test pre-vehicle checks"

            PreFlightCheckButton {
                name:       "Site survey"
                manualText: "Site clear?"
            }
        }
    }
}
)";

/// Pre-vehicle checks which pass automatically, without anything to confirm
constexpr const char* kAutomaticPreVehicleChecksFixture = R"(
import QtQuick

import QGroundControl.Controls

Item {
    property var model: listModel

    PreFlightCheckModel {
        id: listModel

        PreFlightCheckGroup {
            name: "Test automatic pre-vehicle checks"

            PreFlightCheckButton {
                name: "Ground station ready"
            }
        }
    }
}
)";

/// Upper bound on pre-vehicle checks confirmed in one pass, so a check which keeps reappearing can't loop forever.
constexpr int kMaxPreVehicleChecks = 50;

/// True if the item lives in the pre-vehicle checklist, which keeps its progress when a vehicle connects.
bool isInPreVehicleChecklist(const QQuickItem* item)
{
    for (; item; item = item->parentItem()) {
        if (item->objectName() == QStringLiteral("preVehicleCheckList")) {
            return true;
        }
    }
    return false;
}

/// Finds the first visible PreFlightCheckButton outside the pre-vehicle checklist.
QQuickItem* findVehicleChecklistButton(QQuickItem* item)
{
    if (!item || !item->isVisible()) {
        return nullptr;
    }
    if (item->property("manualText").isValid() && !isInPreVehicleChecklist(item)) {
        return item;
    }
    for (QQuickItem* const child : item->childItems()) {
        if (QQuickItem* const found = findVehicleChecklistButton(child)) {
            return found;
        }
    }
    return nullptr;
}

}  // namespace

// ---------------------------------------------------------------------------
// _exerciseIndicator
// ---------------------------------------------------------------------------

bool ToolbarIndicatorUITest::_exerciseIndicator(QQuickItem *indicatorItem, const QString &indicatorName, bool expectExpand)
{
    if (!indicatorItem || !_window) {
        return false;
    }

    // 1. Click the indicator — verify the drawer opens
    if (!_clickItemAt(indicatorItem, 0.5, 0.5, indicatorName)) {
        return false;
    }

    if (!findVisibleItem(_rootItem, QStringLiteral("indicatorDrawerLoader"), 2000)) {
        qWarning() << indicatorName << ": drawer did not open after clicking indicator";
        return false;
    }

    QTest::qWait(_pageDelay);

    // 2. Expand — verify the expand button is present when expected, and that
    //    expanded content appears after clicking it
    if (expectExpand) {
        QQuickItem *expandBtn = findVisibleItem(_rootItem, QStringLiteral("indicatorDrawerExpandButton"), 500);
        if (!expandBtn) {
            qWarning() << indicatorName << ": expand button not found but was expected";
            return false;
        }
        if (!_clickItemAt(expandBtn, 0.5, 0.5, QStringLiteral("indicatorDrawerExpandButton"))) {
            return false;
        }
        QTest::qWait(_pageDelay);

        if (!findVisibleItem(_rootItem, QStringLiteral("indicatorExpandedLoader"), 2000)) {
            qWarning() << indicatorName << ": expanded content did not appear after clicking expand button";
            return false;
        }
    }

    // 3. Close the drawer with Escape — verify it closes
    QTest::keyClick(_window, Qt::Key_Escape);

    const bool drawerClosed = waitForCondition(
        [&] { return findVisibleItem(_rootItem, QStringLiteral("indicatorDrawerLoader"), 0) == nullptr; },
        2000,
        QStringLiteral("indicatorDrawerLoader hidden"));
    if (!drawerClosed) {
        qWarning() << indicatorName << ": drawer did not close after pressing Escape";
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// _runIndicatorTest
// ---------------------------------------------------------------------------

void ToolbarIndicatorUITest::_runIndicatorTest(
    const std::function<MockLink *()> &factory,
    const QString &vehicleName)
{
    runWithMockLink(factory, [&](QPointer<MockLink> /*mockLink*/, Vehicle * /*vehicle*/) {
    // -------------------------------------------------------------------------
    // Ensure we are on the Fly view (default after vehicle connects)
    // -------------------------------------------------------------------------
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("mainView_fly"), 3000),
             qPrintable(QStringLiteral("%1: Fly view not visible").arg(vehicleName)));

    // -------------------------------------------------------------------------
    // Table of indicators to exercise: { objectName, displayName, expectExpand }
    // -------------------------------------------------------------------------
    struct IndicatorSpec {
        const char *objectName;
        const char *displayName;
        bool        expectExpand;
    };
    static const IndicatorSpec kIndicators[] = {
        {"toolbar_mainStatusIndicator", "MainStatus", false},
        {"toolbar_flightModeIndicator", "FlightMode", true},
        {"toolbar_gpsIndicator", "GPS", true},
        {"toolbar_batteryIndicator", "Battery", true},
        {"toolbar_remoteIDIndicator", "RemoteID", true},
        {"toolbar_gimbalIndicator", "Gimbal", true},
        {"toolbar_escIndicator", "ESC", false},
        {"toolbar_telemetryRSSIIndicator", "TelemetryRSSI", false},
    };

    for (const IndicatorSpec &spec : kIndicators) {
        const QString objName     = QString::fromLatin1(spec.objectName);
        const QString displayName = vehicleName + QLatin1Char('/') + QLatin1String(spec.displayName);

        QQuickItem *item = findVisibleItem(_rootItem, objName, 2000);
        QVERIFY2(item,
                 qPrintable(QStringLiteral("%1: %2 not found in toolbar").arg(vehicleName, objName)));
        QVERIFY2(_exerciseIndicator(item, displayName, spec.expectExpand),
                 qPrintable(QStringLiteral("%1: exercise failed").arg(displayName)));
    }
    });
}

// ---------------------------------------------------------------------------
// Per-vehicle-type test slots
// ---------------------------------------------------------------------------

void ToolbarIndicatorUITest::_testPX4Indicators()
{
    _runIndicatorTest(
        [] { return MockLink::startPX4MockLink(MockConfiguration::OptionEnableGimbal); },
        QStringLiteral("PX4"));
}

void ToolbarIndicatorUITest::_testAPMCopterIndicators()
{
    if (!apmFirmwareSupported()) {
        QSKIP("ArduPilot support not registered in this build");
    }

    _runIndicatorTest(
        [] { return MockLink::startAPMArduCopterMockLink(MockConfiguration::OptionEnableGimbal); },
        QStringLiteral("APMCopter"));
}

void ToolbarIndicatorUITest::_testEmergencyStopReplacesDisarmInFlight_data()
{
    QTest::addColumn<int>("firmware");
    QTest::addColumn<int>("vehicleType");
    QTest::addColumn<bool>("takeoff");
    QTest::addColumn<bool>("expectEmergencyStop");

    QTest::addRow("multirotor on ground") << int(MAV_AUTOPILOT_PX4) << int(MAV_TYPE_QUADROTOR) << false << false;
    QTest::addRow("multirotor flying") << int(MAV_AUTOPILOT_PX4) << int(MAV_TYPE_QUADROTOR) << true << true;
    // Classified as a generic vehicle, not a multirotor
    QTest::addRow("dodecarotor flying") << int(MAV_AUTOPILOT_PX4) << int(MAV_TYPE_DODECAROTOR) << true << true;
    // Rover is underway while armed and moving, but accepts a normal disarm
    QTest::addRow("rover moving") << int(MAV_AUTOPILOT_ARDUPILOTMEGA) << int(MAV_TYPE_GROUND_ROVER) << true << false;
}

void ToolbarIndicatorUITest::_testEmergencyStopReplacesDisarmInFlight()
{
    QFETCH(int, firmware);
    QFETCH(int, vehicleType);
    QFETCH(bool, takeoff);
    QFETCH(bool, expectEmergencyStop);

    if ((firmware == MAV_AUTOPILOT_ARDUPILOTMEGA) && !apmFirmwareSupported()) {
        QSKIP("ArduPilot support not registered in this build");
    }

    runWithMockLink(
        [firmware, vehicleType] {
            auto* const mockConfig = new MockConfiguration(QStringLiteral("Emergency Stop MockLink"));
            mockConfig->setFirmwareType(static_cast<MAV_AUTOPILOT>(firmware));
            mockConfig->setVehicleType(static_cast<MAV_TYPE>(vehicleType));
            return MockLink::startMockLink(mockConfig);
        },
        [&](QPointer<MockLink> /*mockLink*/, Vehicle* vehicle) {
            if (takeoff) {
                // The airborne transition creates QGCPressure, which warns on hosts without a pressure backend
                ignoreLogMessage("Utilities.QGCSensors", QtWarningMsg,
                                 QRegularExpression(QStringLiteral("Failed to connect to pressure backend")));
                ignoreLogMessage("Utilities.QGCSensors", QtWarningMsg,
                                 QRegularExpression(QStringLiteral("Error Initializing Pressure Sensor")));
                // MockLink arms and climbs above home on takeoff
                vehicle->sendMavCommand(vehicle->defaultComponentId(), MAV_CMD_NAV_TAKEOFF, false /* showError */, 0.0f,
                                        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 10.0f /* altitude */);
                QVERIFY_TRUE_WAIT(vehicle->underway(), TestTimeout::longMs());
            } else {
                vehicle->setArmed(true, false /* showError */);
            }
            QVERIFY_TRUE_WAIT(vehicle->armed(), TestTimeout::mediumMs());

            QQuickItem* const indicator =
                findVisibleItem(_rootItem, QStringLiteral("toolbar_mainStatusIndicator"), TestTimeout::mediumMs());
            QVERIFY2(indicator, "Main status indicator not visible");
            QVERIFY(_clickItemAt(indicator, 0.5, 0.5, QStringLiteral("toolbar_mainStatusIndicator")));

            const QString emergencyStopName = QStringLiteral("mainStatusEmergencyStopButton");
            const QString armName = QStringLiteral("mainStatusArmButton");
            QQuickItem* const shownButton =
                findVisibleItem(_rootItem, expectEmergencyStop ? emergencyStopName : armName, TestTimeout::mediumMs());
            QVERIFY2(shownButton, "Expected arm action not shown in the main status drawer");
            QVERIFY2(!findVisibleItem(_rootItem, expectEmergencyStop ? armName : emergencyStopName, 0),
                     "Both Disarm and Emergency Stop shown");

            if (!expectEmergencyStop) {
                QCOMPARE(shownButton->property("text").toString(), QStringLiteral("Disarm"));
                return;
            }

            // Emergency stop must go through the guided action confirmation, not act directly
            QVERIFY(_clickItemAt(shownButton, 0.5, 0.5, emergencyStopName));
            QQuickItem* const confirmButton =
                findVisibleItem(_rootItem, QStringLiteral("guidedActionConfirmButton"), TestTimeout::mediumMs());
            QVERIFY2(confirmButton, "Emergency stop guided action confirmation not shown");
            QCOMPARE(confirmButton->property("text").toString(), QStringLiteral("EMERGENCY STOP"));
            QVERIFY(vehicle->armed());

            QVERIFY(QMetaObject::invokeMethod(confirmButton, "activated"));
            QVERIFY_TRUE_WAIT(!vehicle->armed(), TestTimeout::mediumMs());

            // MockLink keeps reporting in-air after the disarm, but there is nothing left to stop
            QVERIFY(vehicle->airborne());
            QVERIFY(_clickItemAt(indicator, 0.5, 0.5, QStringLiteral("toolbar_mainStatusIndicator")));
            QVERIFY2(findVisibleItem(_rootItem, armName, TestTimeout::mediumMs()), "Arm not shown after disarm");
            QVERIFY2(!findVisibleItem(_rootItem, emergencyStopName, 0), "Emergency Stop shown for a disarmed vehicle");
        });
}

void ToolbarIndicatorUITest::_testActionsDisabledWhileVehicleNotReady()
{
    if (!apmFirmwareSupported()) {
        QSKIP("ArduPilot support not registered in this build");
    }

    runWithMockLink([] { return MockLink::startAPMArduCopterMockLink(); },
                    [this](QPointer<MockLink> mockLink, Vehicle* vehicle) {
                        QQuickItem* const statusText =
                            findVisibleItem(_rootItem, QStringLiteral("mainStatusText"), TestTimeout::mediumMs());
                        QVERIFY2(statusText, "Main status text not visible");
                        QQuickItem* const takeoffButton = findVisibleItem(
                            _rootItem, QStringLiteral("flyToolStrip_takeoffButton"), TestTimeout::mediumMs());
                        QVERIFY2(takeoffButton, "Takeoff tool strip action not visible");

                        // A failing prearm check disables the actions instead of hiding them
                        mockLink->setPrearmCheck(true, false);
                        QVERIFY_TRUE_WAIT(vehicle->readyToFlyAvailable(), TestTimeout::mediumMs());
                        QTRY_COMPARE_WITH_TIMEOUT(statusText->property("text").toString(), QStringLiteral("Not Ready"),
                                                  TestTimeout::mediumMs());
                        QTRY_VERIFY_WITH_TIMEOUT(!takeoffButton->isEnabled(), TestTimeout::mediumMs());
                        QVERIFY2(takeoffButton->isVisible(), "Takeoff hidden instead of disabled");

                        QQuickItem* const indicator = findVisibleItem(
                            _rootItem, QStringLiteral("toolbar_mainStatusIndicator"), TestTimeout::mediumMs());
                        QVERIFY2(indicator, "Main status indicator not visible");
                        QVERIFY(_clickItemAt(indicator, 0.5, 0.5, QStringLiteral("toolbar_mainStatusIndicator")));
                        QQuickItem* const armButton =
                            findVisibleItem(_rootItem, QStringLiteral("mainStatusArmButton"), TestTimeout::mediumMs());
                        QVERIFY2(armButton, "Arm button not shown in the main status drawer");
                        QVERIFY2(!armButton->isEnabled(), "Arm enabled while the prearm check fails");

                        mockLink->setPrearmCheck(true, true);
                        QTRY_COMPARE_WITH_TIMEOUT(statusText->property("text").toString(), QStringLiteral("Ready"),
                                                  TestTimeout::mediumMs());
                        QTRY_VERIFY_WITH_TIMEOUT(armButton->isEnabled(), TestTimeout::mediumMs());
                        QVERIFY(takeoffButton->isEnabled());
                    });
}

void ToolbarIndicatorUITest::_testTakeoffLandShareSlot()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [this](QPointer<MockLink> /*mockLink*/, Vehicle* vehicle) {
            const QString takeoffName = QStringLiteral("flyToolStrip_takeoffButton");
            const QString landName = QStringLiteral("flyToolStrip_landButton");
            QQuickItem* const takeoffButton = findVisibleItem(_rootItem, takeoffName, TestTimeout::mediumMs());
            QVERIFY2(takeoffButton, "Takeoff not shown on the ground");
            QVERIFY(!findVisibleItem(_rootItem, landName, 0));

            // Armed but still on the ground keeps Takeoff in the slot
            vehicle->setArmed(true, false /* showError */);
            QVERIFY_TRUE_WAIT(vehicle->armed(), TestTimeout::mediumMs());
            QVERIFY2(takeoffButton->isVisible(), "Takeoff replaced while armed on the ground");
            QVERIFY(takeoffButton->isEnabled());
            QVERIFY(!findVisibleItem(_rootItem, landName, 0));

            // The airborne transition creates QGCPressure, which warns on hosts without a pressure backend
            ignoreLogMessage("Utilities.QGCSensors", QtWarningMsg,
                             QRegularExpression(QStringLiteral("Failed to connect to pressure backend")));
            ignoreLogMessage("Utilities.QGCSensors", QtWarningMsg,
                             QRegularExpression(QStringLiteral("Error Initializing Pressure Sensor")));
            vehicle->sendMavCommand(vehicle->defaultComponentId(), MAV_CMD_NAV_TAKEOFF, false /* showError */, 0.0f,
                                    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 10.0f /* altitude */);
            QVERIFY_TRUE_WAIT(vehicle->underway(), TestTimeout::longMs());

            QQuickItem* const landButton = findVisibleItem(_rootItem, landName, TestTimeout::mediumMs());
            QVERIFY2(landButton, "Land not shown in the air");
            QVERIFY(landButton->isEnabled());
            QVERIFY(!takeoffButton->isVisible());
        });
}

void ToolbarIndicatorUITest::_testArmRequiresEnforcedChecklist()
{
    AppSettings* const appSettings = SettingsManager::instance()->appSettings();
    Fact* const useChecklist = appSettings->useChecklist();
    Fact* const enforceChecklist = appSettings->enforceChecklist();
    const QVariant savedUseChecklist = useChecklist->rawValue();
    const QVariant savedEnforceChecklist = enforceChecklist->rawValue();
    const auto guard = qScopeGuard([=] {
        useChecklist->setRawValue(savedUseChecklist);
        enforceChecklist->setRawValue(savedEnforceChecklist);
    });
    useChecklist->setRawValue(true);
    enforceChecklist->setRawValue(true);

    runWithMockLink([] { return MockLink::startPX4MockLink(); },
                    [this, useChecklist](QPointer<MockLink> mockLink, Vehicle* vehicle) {
                        QQuickItem* const indicator = findVisibleItem(
                            _rootItem, QStringLiteral("toolbar_mainStatusIndicator"), TestTimeout::mediumMs());
                        QVERIFY2(indicator, "Main status indicator not visible");
                        QVERIFY(_clickItemAt(indicator, 0.5, 0.5, QStringLiteral("toolbar_mainStatusIndicator")));

                        // An incomplete checklist opens the dropdown on its checklist tab, with the commands still shown
                        QQuickItem* const checklistTab = findVisibleItem(
                            _rootItem, QStringLiteral("mainStatusChecklistTab"), TestTimeout::mediumMs());
                        QVERIFY2(checklistTab, "Checklist tab not shown in the main status drawer");
                        QVERIFY2(checklistTab->property("checked").toBool(), "Checklist tab not selected by default");

                        QQuickItem* const armButton =
                            findVisibleItem(_rootItem, QStringLiteral("mainStatusArmButton"), TestTimeout::mediumMs());
                        QVERIFY2(armButton, "Arm button not shown in the main status drawer");
                        QCOMPARE(armButton->property("text").toString(), QStringLiteral("Arm"));
                        QVERIFY2(!armButton->isEnabled(), "Arm enabled before the enforced checklist passed");

                        QQuickItem* const statusText = findVisibleItem(
                            _rootItem, QStringLiteral("mainStatusText"), TestTimeout::mediumMs());
                        QVERIFY2(statusText, "Main status text not visible");
                        QTRY_COMPARE_WITH_TIMEOUT(statusText->property("text").toString(), QStringLiteral("Not Ready"),
                                                  TestTimeout::mediumMs());

                        QQuickItem* const checklistLabel = findVisibleItem(
                            _rootItem, QStringLiteral("mainStatusChecklistTabBadge"), TestTimeout::mediumMs());
                        QVERIFY2(checklistLabel, "Checklist tab label not shown");
                        QCOMPARE(checklistLabel->property("text").toString(), QStringLiteral("In Progress"));

                        vehicle->clearMessages();
                        QQuickItem* const toolbarBadge = findVisibleItem(
                            _rootItem, QStringLiteral("mainStatusSeverityBadge"), TestTimeout::mediumMs());
                        QVERIFY2(toolbarBadge, "Toolbar severity badge not shown for an incomplete checklist");
                        QTRY_COMPARE_WITH_TIMEOUT(toolbarBadge->property("severity").toString(),
                                                  QStringLiteral("warning"), TestTimeout::mediumMs());
                        QCOMPARE(toolbarBadge->property("text").toString(), QStringLiteral("Pre-Flight"));

                        // The badge names the checklist only while it is the sole warning
                        mockLink->sendStatusTextMessage(MAV_SEVERITY_WARNING, QStringLiteral("Status text warning"));
                        QVERIFY_TRUE_WAIT(vehicle->unreadWarningMessageCount() == 1, TestTimeout::mediumMs());
                        QTRY_COMPARE_WITH_TIMEOUT(toolbarBadge->property("text").toString(), QStringLiteral("Warning"),
                                                  TestTimeout::mediumMs());
                        vehicle->clearMessages();
                        QTRY_COMPARE_WITH_TIMEOUT(toolbarBadge->property("text").toString(),
                                                  QStringLiteral("Pre-Flight"), TestTimeout::mediumMs());

                        // The checklist is pre-flight, so arming drops its warning
                        vehicle->setArmed(true, false /* showError */);
                        QVERIFY_TRUE_WAIT(vehicle->armed(), TestTimeout::mediumMs());
                        QTRY_VERIFY_WITH_TIMEOUT(!toolbarBadge->isVisible(), TestTimeout::mediumMs());
                        vehicle->setArmed(false, false /* showError */);
                        QVERIFY_TRUE_WAIT(!vehicle->armed(), TestTimeout::mediumMs());
                        QTRY_COMPARE_WITH_TIMEOUT(toolbarBadge->property("severity").toString(),
                                                  QStringLiteral("warning"), TestTimeout::mediumMs());

                        vehicle->setCheckListState(Vehicle::CheckListPassed);
                        QTRY_VERIFY_WITH_TIMEOUT(armButton->isEnabled(), TestTimeout::mediumMs());
                        QTRY_COMPARE_WITH_TIMEOUT(statusText->property("text").toString(), QStringLiteral("Ready"),
                                                  TestTimeout::mediumMs());
                        QTRY_COMPARE_WITH_TIMEOUT(checklistLabel->property("text").toString(), QStringLiteral("Passed"),
                                                  TestTimeout::mediumMs());
                        QTRY_VERIFY_WITH_TIMEOUT(!toolbarBadge->isVisible(), TestTimeout::mediumMs());

                        // Recreating the checklist must not leave the previous Passed state behind
                        useChecklist->setRawValue(false);
                        useChecklist->setRawValue(true);
                        QTRY_VERIFY_WITH_TIMEOUT(!armButton->isEnabled(), TestTimeout::mediumMs());
                        QVERIFY(vehicle->checkListState() != Vehicle::CheckListPassed);
                        QTRY_COMPARE_WITH_TIMEOUT(statusText->property("text").toString(), QStringLiteral("Not Ready"),
                                                  TestTimeout::mediumMs());
                    });
}

void ToolbarIndicatorUITest::_testDisarmReachableWithoutParameters()
{
    if (!apmFirmwareSupported()) {
        QSKIP("ArduPilot support not registered in this build");
    }

    Fact* const noInitialDownload = SettingsManager::instance()->mavlinkSettings()->noInitialDownloadWhenArmed();
    const QVariant savedNoInitialDownload = noInitialDownload->rawValue();
    const auto restoreSetting = qScopeGuard([=] { noInitialDownload->setRawValue(savedNoInitialDownload); });
    noInitialDownload->setRawValue(true);

    startUI();
    if (QTest::currentTestFailed()) {
        return;
    }

    QSignalSpy spyVehicle(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    QVERIFY(spyVehicle.isValid());

    // Already armed on connect, so the parameter download is skipped and parameters never become ready
    auto* const mockConfig = new MockConfiguration(QStringLiteral("Disarm Without Parameters MockLink"));
    mockConfig->setFirmwareType(MAV_AUTOPILOT_ARDUPILOTMEGA);
    mockConfig->setVehicleType(MAV_TYPE_GROUND_ROVER);
    mockConfig->setStartArmed(true);
    QPointer<MockLink> mockLink = MockLink::startMockLink(mockConfig);
    const auto cleanup = qScopeGuard([&] {
        disconnectMockLink(mockLink);
        closeUIWindow();
        destroyUIEngine();
    });
    QVERIFY(mockLink);

    QVERIFY(waitForSignal(spyVehicle, TestTimeout::longMs(), QStringLiteral("activeVehicleChanged")));
    Vehicle* const vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(vehicle);
    QTRY_VERIFY_WITH_TIMEOUT(vehicle->isInitialConnectComplete(), TestTimeout::longMs());
    QVERIFY(vehicle->armed());
    QVERIFY(!MultiVehicleManager::instance()->parameterReadyVehicleAvailable());

    QQuickItem* const indicator =
        findVisibleItem(_rootItem, QStringLiteral("toolbar_mainStatusIndicator"), TestTimeout::mediumMs());
    QVERIFY2(indicator, "Main status indicator not visible");
    QVERIFY(_clickItemAt(indicator, 0.5, 0.5, QStringLiteral("toolbar_mainStatusIndicator")));

    QQuickItem* const disarmButton =
        findVisibleItem(_rootItem, QStringLiteral("mainStatusArmButton"), TestTimeout::mediumMs());
    QVERIFY2(disarmButton, "Disarm not reachable without parameters");
    QCOMPARE(disarmButton->property("text").toString(), QStringLiteral("Disarm"));
}

void ToolbarIndicatorUITest::_testRebootRequiredIndicator_data()
{
    QTest::addColumn<bool>("armed");

    QTest::newRow("disarmed") << false;
    QTest::newRow("armed") << true;
}

void ToolbarIndicatorUITest::_testRebootRequiredIndicator()
{
    QFETCH(bool, armed);

    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [&](QPointer<MockLink> /*mockLink*/, Vehicle* vehicle) {
            const QString badgeName = QStringLiteral("mainStatusRebootBadge");
            QVERIFY2(!findVisibleItem(_rootItem, badgeName, 0),
                     "Reboot-required badge shown before a reboot was required");
            vehicle->setRebootRequired();
            QVERIFY2(findVisibleItem(_rootItem, badgeName, TestTimeout::mediumMs()), "Reboot-required badge not shown");

            if (armed) {
                vehicle->setArmed(true, false /* showError */);
                QVERIFY_TRUE_WAIT(vehicle->armed(), TestTimeout::mediumMs());
            }

            const QString indicatorName = QStringLiteral("toolbar_mainStatusIndicator");
            QQuickItem* const indicator = findVisibleItem(_rootItem, indicatorName, TestTimeout::mediumMs());
            QVERIFY2(indicator, "Main status indicator not visible");
            QVERIFY(_clickItemAt(indicator, 0.5, 0.5, indicatorName));

            // Reboot is only offered while disarmed
            const QString rebootName = QStringLiteral("mainStatusRebootButton");
            const QString disarmLabelName = QStringLiteral("mainStatusRebootDisarmLabel");
            QVERIFY2(findVisibleItem(_rootItem, armed ? disarmLabelName : rebootName, TestTimeout::mediumMs()),
                     "Expected reboot action not shown in the main status drawer");
            QVERIFY2(!findVisibleItem(_rootItem, armed ? rebootName : disarmLabelName, 0),
                     "Both the reboot button and the disarm hint shown");
        });
}

void ToolbarIndicatorUITest::_testIndicatorDrawerClosesOnVehicleDisconnect()
{
    runWithMockLink([] { return MockLink::startPX4MockLink(); },
                    [this](QPointer<MockLink> mockLink, Vehicle*) {
                        const QString indicatorName = QStringLiteral("toolbar_mainStatusIndicator");
                        const QString drawerName = QStringLiteral("indicatorDrawerLoader");
                        QQuickItem* const indicator =
                            findVisibleItem(_rootItem, indicatorName, TestTimeout::mediumMs());
                        QVERIFY2(indicator, "Main status indicator not visible");
                        QVERIFY(_clickItemAt(indicator, 0.5, 0.5, indicatorName));
                        QVERIFY2(findVisibleItem(_rootItem, drawerName, TestTimeout::mediumMs()),
                                 "Main status drawer did not open");

                        disconnectMockLink(mockLink);
                        QVERIFY2(!findVisibleItem(_rootItem, drawerName, 0),
                                 "Indicator drawer still open after the vehicle disconnected");
                    });
}

void ToolbarIndicatorUITest::_testVehicleMessagesMarkReadAndClear()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [this](QPointer<MockLink> mockLink, Vehicle* vehicle) {
            const QString indicatorName = QStringLiteral("toolbar_mainStatusIndicator");
            const QString drawerName = QStringLiteral("indicatorDrawerLoader");
            const QString markReadName = QStringLiteral("vehicleMessagesMarkReadButton");
            const QString clearName = QStringLiteral("vehicleMessagesClearButton");

            vehicle->clearMessages();
            QVERIFY(!vehicle->hasUnreadMessages());

            QQuickItem* const indicator = findVisibleItem(_rootItem, indicatorName, TestTimeout::mediumMs());
            QVERIFY2(indicator, "Main status indicator not visible");
            QVERIFY(_clickItemAt(indicator, 0.5, 0.5, indicatorName));
            QVERIFY2(findVisibleItem(_rootItem, drawerName, TestTimeout::mediumMs()),
                     "Main status drawer did not open");
            QQuickItem* const messagesTab =
                findVisibleItem(_rootItem, QStringLiteral("mainStatusMessagesTab"), TestTimeout::mediumMs());
            QVERIFY2(messagesTab, "Vehicle Messages tab not shown");
            QVERIFY(_clickItemAt(messagesTab, 0.5, 0.5, QStringLiteral("mainStatusMessagesTab")));

            // Critical message popups are only suppressed while the Messages tab is the one showing them
            const char* const suppressProperty = "suppressCriticalVehicleMessages";
            QTRY_VERIFY_WITH_TIMEOUT(_window->property(suppressProperty).toBool(), TestTimeout::mediumMs());
            QQuickItem* const statusTab =
                findVisibleItem(_rootItem, QStringLiteral("mainStatusOverallStatusTab"), TestTimeout::mediumMs());
            QVERIFY2(statusTab, "Overall Status tab not shown");
            QVERIFY(_clickItemAt(statusTab, 0.5, 0.5, QStringLiteral("mainStatusOverallStatusTab")));
            QTRY_VERIFY_WITH_TIMEOUT(!_window->property(suppressProperty).toBool(), TestTimeout::mediumMs());
            QVERIFY(_clickItemAt(messagesTab, 0.5, 0.5, QStringLiteral("mainStatusMessagesTab")));
            QTRY_VERIFY_WITH_TIMEOUT(_window->property(suppressProperty).toBool(), TestTimeout::mediumMs());

            QVERIFY2(!findVisibleItem(_rootItem, markReadName, 0), "Mark as Read shown with no messages");
            QVERIFY2(!findVisibleItem(_rootItem, clearName, 0), "Clear shown with no messages");

            // Messages arriving while the drawer is open are unread
            mockLink->sendStatusTextMessage(MAV_SEVERITY_INFO, QStringLiteral("Status text info"));
            QVERIFY_TRUE_WAIT(vehicle->hasUnreadMessages(), TestTimeout::mediumMs());
            QQuickItem* const messagesBadge =
                findVisibleItem(_rootItem, QStringLiteral("mainStatusMessagesTabBadge"), TestTimeout::mediumMs());
            QVERIFY2(messagesBadge, "Messages tab badge not shown with unread messages");
            QCOMPARE(messagesBadge->property("severity").toString(), QStringLiteral("info"));
            QCOMPARE(messagesBadge->property("text").toString(), QStringLiteral("Info"));
            QQuickItem* const toolbarBadge =
                findVisibleItem(_rootItem, QStringLiteral("mainStatusSeverityBadge"), TestTimeout::mediumMs());
            QVERIFY2(toolbarBadge, "Toolbar severity badge not shown with unread messages");
            QCOMPARE(toolbarBadge->property("severity").toString(), QStringLiteral("info"));

            vehicle->markMessagesRead();
            QVERIFY_TRUE_WAIT(!vehicle->hasUnreadMessages(), TestTimeout::mediumMs());
            mockLink->sendStatusTextMessage(MAV_SEVERITY_INFO, QStringLiteral("Status text info"));
            QVERIFY_TRUE_WAIT(vehicle->unreadMessageCount() == 1, TestTimeout::mediumMs());

            // Warnings outrank plain unread messages
            mockLink->sendStatusTextMessage(MAV_SEVERITY_WARNING, QStringLiteral("Status text warning"));
            QVERIFY_TRUE_WAIT(vehicle->unreadWarningMessageCount() == 1, TestTimeout::mediumMs());
            QTRY_COMPARE_WITH_TIMEOUT(messagesBadge->property("severity").toString(), QStringLiteral("warning"),
                                      TestTimeout::mediumMs());
            QCOMPARE(messagesBadge->property("text").toString(), QStringLiteral("Warning"));
            QTRY_COMPARE_WITH_TIMEOUT(toolbarBadge->property("severity").toString(), QStringLiteral("warning"),
                                      TestTimeout::mediumMs());
            QCOMPARE(toolbarBadge->property("text").toString(), QStringLiteral("Warning"));

            // Errors outrank warnings
            mockLink->sendStatusTextMessage(MAV_SEVERITY_ERROR, QStringLiteral("Status text error"));
            QVERIFY_TRUE_WAIT(vehicle->unreadCriticalMessageCount() == 1 &&
                                  vehicle->formattedMessages().contains(QStringLiteral("Status text info")),
                              TestTimeout::mediumMs());
            QCOMPARE(vehicle->formattedMessages().count(QStringLiteral("<b>")), 3);
            QTRY_COMPARE_WITH_TIMEOUT(messagesBadge->property("severity").toString(), QStringLiteral("error"),
                                      TestTimeout::mediumMs());
            QCOMPARE(messagesBadge->property("text").toString(), QStringLiteral("Error"));
            QTRY_COMPARE_WITH_TIMEOUT(toolbarBadge->property("severity").toString(), QStringLiteral("error"),
                                      TestTimeout::mediumMs());
            QCOMPARE(toolbarBadge->property("text").toString(), QStringLiteral("Error"));

            QQuickItem* const markReadButton = findVisibleItem(_rootItem, markReadName, TestTimeout::mediumMs());
            QVERIFY2(markReadButton, "Mark as Read not shown with unread messages");
            QVERIFY2(findVisibleItem(_rootItem, clearName, TestTimeout::mediumMs()), "Clear not shown with messages");

            // Mark as Read clears the badge count but keeps the messages and the drawer open
            QVERIFY(_clickItemAt(markReadButton, 0.5, 0.5, markReadName));
            QVERIFY_TRUE_WAIT(!vehicle->hasUnreadMessages(), TestTimeout::mediumMs());
            QCOMPARE(vehicle->unreadCriticalMessageCount(), 0);
            QVERIFY(vehicle->formattedMessages().contains(QStringLiteral("Status text error")));
            QVERIFY(!vehicle->formattedMessages().contains(QStringLiteral("<b>")));
            QVERIFY2(findVisibleItem(_rootItem, drawerName, 0), "Drawer closed by Mark as Read");
            QVERIFY2(waitForCondition([&] { return findVisibleItem(_rootItem, markReadName, 0) == nullptr; },
                                      TestTimeout::mediumMs(), QStringLiteral("Mark as Read hidden")),
                     "Mark as Read still shown after marking all read");
            QVERIFY2(waitForCondition(
                         [&] { return findVisibleItem(_rootItem, QStringLiteral("mainStatusMessagesTabBadge"), 0) == nullptr; },
                         TestTimeout::mediumMs(), QStringLiteral("Messages tab badge hidden")),
                     "Messages tab badge still shown after marking all read");
            const QString toolbarBadgeName = QStringLiteral("mainStatusSeverityBadge");
            QVERIFY2(waitForCondition([&] { return findVisibleItem(_rootItem, toolbarBadgeName, 0) == nullptr; },
                                      TestTimeout::mediumMs(), QStringLiteral("Toolbar severity badge hidden")),
                     "Toolbar severity badge still shown after marking all read");

            // A new message is unread again
            mockLink->sendStatusTextMessage(MAV_SEVERITY_ERROR, QStringLiteral("Status text error 2"));
            QVERIFY_TRUE_WAIT(vehicle->unreadCriticalMessageCount() == 1, TestTimeout::mediumMs());
            QCOMPARE(vehicle->formattedMessages().count(QStringLiteral("<b>")), 1);
            QVERIFY2(findVisibleItem(_rootItem, markReadName, TestTimeout::mediumMs()),
                     "Mark as Read not shown for a new message");

            // Clear deletes the messages and closes the drawer
            QQuickItem* const clearButton = findVisibleItem(_rootItem, clearName, TestTimeout::mediumMs());
            QVERIFY2(clearButton, "Clear not shown with messages");
            QVERIFY(_clickItemAt(clearButton, 0.5, 0.5, clearName));
            QVERIFY(vehicle->formattedMessages().isEmpty());
            QCOMPARE(vehicle->unreadCriticalMessageCount(), 0);
            QVERIFY2(waitForCondition([&] { return findVisibleItem(_rootItem, drawerName, 0) == nullptr; },
                                      TestTimeout::mediumMs(), QStringLiteral("drawer closed")),
                     "Drawer still open after Clear");
            QTRY_VERIFY_WITH_TIMEOUT(!_window->property(suppressProperty).toBool(), TestTimeout::mediumMs());
        });
}

void ToolbarIndicatorUITest::_testMockLinkBootStatusTextShowsNonCriticalIndicator()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [this](QPointer<MockLink> mockLink, Vehicle* vehicle) {
            // Sent directly so the result doesn't depend on the delay before MockLink's critical burst
            vehicle->clearMessages();
            mockLink->sendBootStatusTextMessages();
            QVERIFY_TRUE_WAIT(vehicle->unreadMessageCount() == 2, TestTimeout::mediumMs());
            QCOMPARE(vehicle->unreadCriticalMessageCount(), 0);

            QQuickItem* const indicator = findVisibleItem(
                _rootItem, QStringLiteral("toolbar_mainStatusIndicator"), TestTimeout::mediumMs());
            QVERIFY2(indicator, "Main status indicator not visible");
            QVERIFY(_clickItemAt(indicator, 0.5, 0.5, QStringLiteral("toolbar_mainStatusIndicator")));

            // The boot Notice outranks the Info message
            QQuickItem* const messagesBadge =
                findVisibleItem(_rootItem, QStringLiteral("mainStatusMessagesTabBadge"), TestTimeout::mediumMs());
            QVERIFY2(messagesBadge, "Messages tab badge not shown for the MockLink boot status messages");
            QCOMPARE(messagesBadge->property("severity").toString(), QStringLiteral("warning"));

            QQuickItem* const toolbarBadge =
                findVisibleItem(_rootItem, QStringLiteral("mainStatusSeverityBadge"), TestTimeout::mediumMs());
            QVERIFY2(toolbarBadge, "Toolbar severity badge not shown for the MockLink boot status messages");
            QCOMPARE(toolbarBadge->property("severity").toString(), QStringLiteral("warning"));
        });
}

void ToolbarIndicatorUITest::_testChecklistBadgeShownBesideOtherProblems()
{
    Fact* const useChecklist = SettingsManager::instance()->appSettings()->useChecklist();
    const QVariant savedUseChecklist = useChecklist->rawValue();
    const auto restoreSetting = qScopeGuard([=] { useChecklist->setRawValue(savedUseChecklist); });
    useChecklist->setRawValue(true);

    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [this](QPointer<MockLink> mockLink, Vehicle* vehicle) {
            vehicle->clearMessages();
            HealthAndArmingCheckReport* const report = vehicle->healthAndArmingCheckReport();
            QVERIFY(report);

            QQuickItem* const severityBadge =
                findVisibleItem(_rootItem, QStringLiteral("mainStatusSeverityBadge"), TestTimeout::mediumMs());
            QVERIFY2(severityBadge, "Toolbar badge not shown for an incomplete checklist");
            const QString checklistBadgeName = QStringLiteral("mainStatusChecklistBadge");
            QVERIFY2(!findVisibleItem(_rootItem, checklistBadgeName, 0),
                     "Separate checklist badge shown while the severity badge already names it");

            // Another warning turns the severity badge generic, so the incomplete checklist is shown next to it
            report->setProblemsForTest({QStringLiteral("warning")});
            QTRY_COMPARE_WITH_TIMEOUT(severityBadge->property("text").toString(), QStringLiteral("Warning"),
                                      TestTimeout::mediumMs());
            QQuickItem* const checklistBadge = findVisibleItem(_rootItem, checklistBadgeName, TestTimeout::mediumMs());
            QVERIFY2(checklistBadge, "Checklist badge not shown beside the warning badge");
            QCOMPARE(checklistBadge->property("text").toString(), QStringLiteral("Pre-Flight"));

            // An error takes over the severity badge, so the incomplete checklist stays shown next to it
            report->setProblemsForTest({QStringLiteral("error")});
            QTRY_COMPARE_WITH_TIMEOUT(severityBadge->property("severity").toString(), QStringLiteral("error"),
                                      TestTimeout::mediumMs());
            QVERIFY2(findVisibleItem(_rootItem, checklistBadgeName, TestTimeout::mediumMs()),
                     "Checklist badge not shown beside the error badge");

            report->setProblemsForTest({});
            QVERIFY2(waitForCondition([&] { return findVisibleItem(_rootItem, checklistBadgeName, 0) == nullptr; },
                                      TestTimeout::mediumMs(), QStringLiteral("Checklist badge hidden")),
                     "Checklist badge still shown once the error cleared");

            // Disconnected, the vehicle checklist no longer applies, so its badge is hidden
            report->setProblemsForTest({QStringLiteral("error")});
            QVERIFY2(findVisibleItem(_rootItem, checklistBadgeName, TestTimeout::mediumMs()),
                     "Checklist badge not shown beside the error badge");
            disconnectMockLink(mockLink);
            QVERIFY2(waitForCondition([&] { return findVisibleItem(_rootItem, checklistBadgeName, 0) == nullptr; },
                                      TestTimeout::mediumMs(), QStringLiteral("Checklist badge hidden")),
                     "Vehicle checklist badge still shown after the vehicle disconnected");
        });
}

void ToolbarIndicatorUITest::_testHealthChecksDriveToolbarSeverity()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [this](QPointer<MockLink> /*mockLink*/, Vehicle* vehicle) {
            vehicle->clearMessages();
            HealthAndArmingCheckReport* const report = vehicle->healthAndArmingCheckReport();
            QVERIFY(report);
            const QString badgeName = QStringLiteral("mainStatusSeverityBadge");

            report->setProblemsForTest({QStringLiteral("warning")});
            QQuickItem* const toolbarBadge = findVisibleItem(_rootItem, badgeName, TestTimeout::mediumMs());
            QVERIFY2(toolbarBadge, "Toolbar severity badge not shown for a health warning");
            QTRY_COMPARE_WITH_TIMEOUT(toolbarBadge->property("severity").toString(), QStringLiteral("warning"),
                                      TestTimeout::mediumMs());

            const QString indicatorName = QStringLiteral("toolbar_mainStatusIndicator");
            QQuickItem* const indicator = findVisibleItem(_rootItem, indicatorName, TestTimeout::mediumMs());
            QVERIFY2(indicator, "Main status indicator not visible");
            QVERIFY(_clickItemAt(indicator, 0.5, 0.5, indicatorName));
            const QString tabBadgeName = QStringLiteral("mainStatusOverallStatusTabBadge");
            QQuickItem* const tabBadge = findVisibleItem(_rootItem, tabBadgeName, TestTimeout::mediumMs());
            QVERIFY2(tabBadge, "Overall Status tab badge not shown for a health warning");
            QCOMPARE(tabBadge->property("severity").toString(), QStringLiteral("warning"));

            // Errors outrank warnings
            report->setProblemsForTest({QStringLiteral("warning"), QStringLiteral("error")});
            QTRY_COMPARE_WITH_TIMEOUT(toolbarBadge->property("severity").toString(), QStringLiteral("error"),
                                      TestTimeout::mediumMs());
            QTRY_COMPARE_WITH_TIMEOUT(tabBadge->property("severity").toString(), QStringLiteral("error"),
                                      TestTimeout::mediumMs());

            // Acknowledged warnings no longer count toward the warning state
            report->setProblemsForTest({QStringLiteral("warning")});
            QTRY_COMPARE_WITH_TIMEOUT(toolbarBadge->property("severity").toString(), QStringLiteral("warning"),
                                      TestTimeout::mediumMs());
            const QString ackButtonName = QStringLiteral("healthChecksAcknowledgeAllButton");
            QQuickItem* const ackButton = findVisibleItem(_rootItem, ackButtonName, TestTimeout::mediumMs());
            QVERIFY2(ackButton, "Acknowledge All button not shown for a health warning");
            QVERIFY(_clickItemAt(ackButton, 0.5, 0.5, ackButtonName));
            QTRY_COMPARE_WITH_TIMEOUT(report->warningCount(), 0, TestTimeout::mediumMs());
            QVERIFY2(waitForCondition([&] { return findVisibleItem(_rootItem, badgeName, 0) == nullptr; },
                                      TestTimeout::mediumMs(), QStringLiteral("Toolbar severity badge hidden")),
                     "Toolbar severity badge still shown after acknowledging the health warning");
            QVERIFY2(waitForCondition([&] { return findVisibleItem(_rootItem, ackButtonName, 0) == nullptr; },
                                      TestTimeout::mediumMs(), QStringLiteral("Acknowledge All button hidden")),
                     "Acknowledge All button still shown with every warning acknowledged");

            report->setProblemsForTest({});
            QVERIFY2(waitForCondition([&] { return findVisibleItem(_rootItem, badgeName, 0) == nullptr; },
                                      TestTimeout::mediumMs(), QStringLiteral("Toolbar severity badge hidden")),
                     "Toolbar severity badge still shown with no health problems");
            QVERIFY2(waitForCondition([&] { return findVisibleItem(_rootItem, tabBadgeName, 0) == nullptr; },
                                      TestTimeout::mediumMs(), QStringLiteral("Overall Status tab badge hidden")),
                     "Overall Status tab badge still shown with no health problems");
        });
}

bool ToolbarIndicatorUITest::_startChecklistUI(QPointer<MockLink>& mockLink, Vehicle*& vehicle)
{
    startUI();
    if (QTest::currentTestFailed()) {
        return false;
    }
    mockLink = connectMockLinkAndWaitReady([] { return MockLink::startPX4MockLink(); }, vehicle);
    return !mockLink.isNull();
}

void ToolbarIndicatorUITest::_stopChecklistUI(QPointer<MockLink>& mockLink)
{
    disconnectMockLink(mockLink);
    closeUIWindow();
    destroyUIEngine();
}

void ToolbarIndicatorUITest::_testChecklistProgressPersistsAcrossDropdownCloseAndReconnect()
{
    AppSettings* const appSettings = SettingsManager::instance()->appSettings();
    Fact* const useChecklist = appSettings->useChecklist();
    const QVariant savedUseChecklist = useChecklist->rawValue();
    const auto restoreSetting = qScopeGuard([=] { useChecklist->setRawValue(savedUseChecklist); });
    useChecklist->setRawValue(true);

    QPointer<MockLink> mockLink;
    Vehicle* vehicle = nullptr;
    if (!_startChecklistUI(mockLink, vehicle)) {
        return;
    }
    auto cleanup = qScopeGuard([&] { _stopChecklistUI(mockLink); });

    const QString indicatorName = QStringLiteral("toolbar_mainStatusIndicator");
    const QString drawerName = QStringLiteral("indicatorDrawerLoader");

    const auto openDropdownAndFind = [&](const std::function<QQuickItem*()>& find) -> QQuickItem* {
        QQuickItem* const indicator = findVisibleItem(_rootItem, indicatorName, TestTimeout::mediumMs());
        if (!indicator || !_clickItemAt(indicator, 0.5, 0.5, indicatorName)) {
            return nullptr;
        }
        QQuickItem* const tab =
            findVisibleItem(_rootItem, QStringLiteral("mainStatusChecklistTab"), TestTimeout::mediumMs());
        if (!tab || !_clickItemAt(tab, 0.5, 0.5, QStringLiteral("mainStatusChecklistTab"))) {
            return nullptr;
        }
        QQuickItem* button = nullptr;
        // Absence is a valid outcome for the caller, so don't log a timeout warning that strict mode would fail on
        (void) QTest::qWaitFor([&] { return (button = find()) != nullptr; }, TestTimeout::mediumMs());
        return button;
    };

    // The dropdown is opened on the checklist tab explicitly rather than relying on the default tab
    QQuickItem* button = openDropdownAndFind([&] { return findPendingManualChecklistButton(_rootItem); });

    QQuickItem* const checklistBadge =
        findVisibleItem(_rootItem, QStringLiteral("mainStatusChecklistTabBadge"), TestTimeout::mediumMs());
    QVERIFY2(checklistBadge, "Checklist tab label not shown");

    // A custom build's checklist may have no manual items, so there is nothing to confirm
    if (!button) {
        // After QSKIP, signal waits return at once and cleanup would log strict-mode timeout warnings
        cleanup.dismiss();
        _stopChecklistUI(mockLink);
        QSKIP("Checklist has no manual check item to confirm");
    }
    QCOMPARE(checklistBadge->property("text").toString(), QStringLiteral("In Progress"));
    QVERIFY(!button->property("passed").toBool());
    const QString buttonName = button->property("name").toString();
    const auto findButton = [&] { return findChecklistButton(_rootItem, buttonName); };

    QVERIFY(_clickItemAt(button, 0.5, 0.5, QStringLiteral("preFlightCheckManualItem")));
    QTRY_VERIFY_WITH_TIMEOUT(button->property("passed").toBool(), TestTimeout::mediumMs());

    // Progress survives closing and reopening the dropdown
    QTest::keyClick(_window, Qt::Key_Escape);
    QVERIFY2(waitForCondition([&] { return findVisibleItem(_rootItem, drawerName, 0) == nullptr; },
                              TestTimeout::mediumMs(), QStringLiteral("drawer closed")),
             "Drawer did not close on Escape");
    button = openDropdownAndFind(findButton);
    QVERIFY2(button, "Checklist button not shown after reopening the dropdown");
    QVERIFY2(button->property("passed").toBool(), "Checklist progress lost when the dropdown was reopened");

    // A new vehicle gets a fresh vehicle checklist, but pre-vehicle checks keep their progress
    disconnectMockLink(mockLink);
    mockLink = connectMockLinkAndWaitReady([] { return MockLink::startPX4MockLink(); }, vehicle);
    QVERIFY(mockLink);
    button = openDropdownAndFind(findButton);
    QVERIFY2(button, "Checklist button not shown after reconnecting");
    QVERIFY2(button->property("passed").toBool() == isInPreVehicleChecklist(button),
             "Checklist progress not reset for a new vehicle, or pre-vehicle progress lost");
}

void ToolbarIndicatorUITest::_testStockChecklistLocksLaterSections()
{
    AppSettings* const appSettings = SettingsManager::instance()->appSettings();
    Fact* const useChecklist = appSettings->useChecklist();
    Fact* const audioMuted = appSettings->audioMuted();
    const QVariant savedUseChecklist = useChecklist->rawValue();
    const QVariant savedAudioMuted = audioMuted->rawValue();
    const auto restoreSetting = qScopeGuard([=] {
        useChecklist->setRawValue(savedUseChecklist);
        audioMuted->setRawValue(savedAudioMuted);
    });
    useChecklist->setRawValue(true);
    audioMuted->setRawValue(false);

    QPointer<MockLink> mockLink;
    Vehicle* vehicle = nullptr;
    if (!_startChecklistUI(mockLink, vehicle)) {
        return;
    }
    auto cleanup = qScopeGuard([&] { _stopChecklistUI(mockLink); });

    const QString indicatorName = QStringLiteral("toolbar_mainStatusIndicator");
    QQuickItem* const indicator = findVisibleItem(_rootItem, indicatorName, TestTimeout::mediumMs());
    QVERIFY2(indicator, "Main status indicator not visible");
    QVERIFY(_clickItemAt(indicator, 0.5, 0.5, indicatorName));

    QQuickItem* const checklistTab =
        findVisibleItem(_rootItem, QStringLiteral("mainStatusChecklistTab"), TestTimeout::mediumMs());
    QVERIFY2(checklistTab, "Checklist tab not shown in the main status drawer");
    QVERIFY(_clickItemAt(checklistTab, 0.5, 0.5, QStringLiteral("mainStatusChecklistTab")));
    QTRY_VERIFY_WITH_TIMEOUT(checklistTab->property("checked").toBool(), TestTimeout::mediumMs());

    // These sections and items exist only in the stock checklist; a custom build replaces them. Absence skips the
    // test, so wait without the timeout warning that strict mode would fail on.
    const QString lockedGroupName = QStringLiteral("Please arm the vehicle here");
    QQuickItem* lockedButton = nullptr;
    QQuickItem* lockedGroup = nullptr;
    QQuickItem* soundButton = nullptr;
    (void) QTest::qWaitFor(
        [&] {
            lockedButton = findChecklistButton(_rootItem, QStringLiteral("Motors"));
            lockedGroup = findChecklistGroup(_rootItem, lockedGroupName);
            soundButton = findChecklistButton(_rootItem, QStringLiteral("Sound output"));
            return lockedButton && lockedGroup && soundButton;
        },
        TestTimeout::mediumMs());
    if (!lockedButton || !lockedGroup || !soundButton) {
        // After QSKIP, signal waits return at once and cleanup would log strict-mode timeout warnings
        cleanup.dismiss();
        _stopChecklistUI(mockLink);
        QSKIP("Custom pre-flight checklist: stock sections not present");
    }

    // Later sections are always shown, but locked until every section above them passes
    QVERIFY2(!lockedButton->isEnabled(), "Later checklist section not locked before the first section passed");
    QGCPalette disabledPalette;
    disabledPalette.setColorGroupEnabled(false);
    auto* const lockedLabel = lockedButton->property("contentItem").value<QQuickItem*>();
    QVERIFY(lockedLabel);
    QCOMPARE(lockedLabel->property("color").value<QColor>(), disabledPalette.textPrimary());

    // A section is failed when any of its items fails, not just its first unpassed item: Motors stays pending
    // while muting audio fails Sound output further down the same section
    QVERIFY(!lockedGroup->property("failed").toBool());
    audioMuted->setRawValue(true);
    QTRY_VERIFY_WITH_TIMEOUT(lockedGroup->property("failed").toBool(), TestTimeout::mediumMs());
    audioMuted->setRawValue(false);
    QTRY_VERIFY_WITH_TIMEOUT(!lockedGroup->property("failed").toBool(), TestTimeout::mediumMs());
}

QQuickItem* ToolbarIndicatorUITest::_openChecklistTab()
{
    const QString indicatorName = QStringLiteral("toolbar_mainStatusIndicator");
    QQuickItem* const indicator = findVisibleItem(_rootItem, indicatorName, TestTimeout::mediumMs());
    if (!indicator || !_clickItemAt(indicator, 0.5, 0.5, indicatorName)) {
        return nullptr;
    }

    const QString tabName = QStringLiteral("mainStatusChecklistTab");
    QQuickItem* tab = nullptr;
    // Absence is a valid outcome for the caller, so don't log a timeout warning that strict mode would fail on
    (void) QTest::qWaitFor([&] { return (tab = findVisibleItem(_rootItem, tabName, 0)) != nullptr; },
                           TestTimeout::mediumMs());
    if (tab && !_clickItemAt(tab, 0.5, 0.5, tabName)) {
        return nullptr;
    }
    return tab;
}

QStringList ToolbarIndicatorUITest::_passPreVehicleChecklistItems()
{
    QStringList names;
    while (true) {
        QQuickItem* const found = findPendingManualChecklistButton(_rootItem);
        if (!found || !isInPreVehicleChecklist(found)) {
            break;
        }
        // The item could be recreated while waiting, so only its name and a guarded pointer are kept
        const QString name = found->property("name").toString();
        const QPointer<QQuickItem> button(found);
        if (!_clickItemAt(found, 0.5, 0.5, QStringLiteral("preFlightCheckManualItem"))) {
            break;
        }
        if (!QTest::qWaitFor([&] { return button && button->property("passed").toBool(); }, TestTimeout::mediumMs())) {
            break;
        }
        names << name;
        if (names.size() >= kMaxPreVehicleChecks) {
            qWarning() << "Stopped confirming pre-vehicle checks after" << kMaxPreVehicleChecks
                       << "items; a check may be reappearing as pending";
            break;
        }
    }
    return names;
}

bool ToolbarIndicatorUITest::_closeIndicatorDrawer()
{
    QTest::keyClick(_window, Qt::Key_Escape);
    return waitForCondition(
        [&] { return findVisibleItem(_rootItem, QStringLiteral("indicatorDrawerLoader"), 0) == nullptr; },
        TestTimeout::mediumMs(), QStringLiteral("drawer closed"));
}

void ToolbarIndicatorUITest::_testPreVehicleChecklistSurvivesConnectAndDisconnect()
{
    AppSettings* const appSettings = SettingsManager::instance()->appSettings();
    Fact* const useChecklist = appSettings->useChecklist();
    const QVariant savedUseChecklist = useChecklist->rawValue();
    const auto restoreSetting = qScopeGuard([=] { useChecklist->setRawValue(savedUseChecklist); });
    useChecklist->setRawValue(true);
    QVERIFY(_usePreVehicleChecksFixture());

    startUI();
    if (QTest::currentTestFailed()) {
        return;
    }
    QPointer<MockLink> mockLink;
    Vehicle* vehicle = nullptr;
    auto cleanup = qScopeGuard([&] {
        if (mockLink) {
            disconnectMockLink(mockLink);
        }
        closeUIWindow();
        destroyUIEngine();
    });

    // Disconnected: only the pre-vehicle checks exist
    QVERIFY2(_openChecklistTab(), "Checklist tab not shown while disconnected");

    QQuickItem* const tabBadge =
        findVisibleItem(_rootItem, QStringLiteral("mainStatusChecklistTabBadge"), TestTimeout::mediumMs());
    QVERIFY2(tabBadge, "Checklist tab badge not shown while disconnected");
    QCOMPARE(tabBadge->property("text").toString(), QStringLiteral("In Progress"));
    QQuickItem* const toolbarBadge =
        findVisibleItem(_rootItem, QStringLiteral("mainStatusSeverityBadge"), TestTimeout::mediumMs());
    QVERIFY2(toolbarBadge, "Toolbar badge not shown for incomplete pre-vehicle checks");

    const QStringList names = _passPreVehicleChecklistItems();
    QVERIFY2(!names.isEmpty(), "No pre-vehicle check could be confirmed");
    QTRY_COMPARE_WITH_TIMEOUT(tabBadge->property("text").toString(), QStringLiteral("Passed"), TestTimeout::mediumMs());
    QTRY_VERIFY_WITH_TIMEOUT(!toolbarBadge->isVisible(), TestTimeout::mediumMs());

    const auto verifyNamesPassed = [&](const QString& when) {
        for (const QString& name : names) {
            QQuickItem* button = nullptr;
            (void) QTest::qWaitFor([&] { return (button = findChecklistButton(_rootItem, name)) != nullptr; },
                                   TestTimeout::mediumMs());
            QVERIFY2(button, qPrintable(QStringLiteral("%1 not shown %2").arg(name, when)));
            QVERIFY2(button->property("passed").toBool(),
                     qPrintable(QStringLiteral("%1 lost its progress %2").arg(name, when)));
        }
    };

    QVERIFY2(_closeIndicatorDrawer(), "Drawer did not close on Escape");
    mockLink = connectMockLinkAndWaitReady([] { return MockLink::startPX4MockLink(); }, vehicle);
    QVERIFY(mockLink);
    QVERIFY2(_openChecklistTab(), "Checklist tab not shown after connecting");
    verifyNamesPassed(QStringLiteral("when a vehicle connected"));
    if (QTest::currentTestFailed()) {
        return;
    }
    QVERIFY2(vehicle->checkListState() != Vehicle::CheckListPassed,
             "Vehicle checks are still pending, but the vehicle reports a passed checklist");

    QVERIFY2(_closeIndicatorDrawer(), "Drawer did not close on Escape");
    disconnectMockLink(mockLink);
    QVERIFY2(_openChecklistTab(), "Checklist tab not shown after disconnecting");
    verifyNamesPassed(QStringLiteral("when the vehicle disconnected"));
}

void ToolbarIndicatorUITest::_testPreVehicleChecklistLocksVehicleChecks()
{
    AppSettings* const appSettings = SettingsManager::instance()->appSettings();
    Fact* const useChecklist = appSettings->useChecklist();
    const QVariant savedUseChecklist = useChecklist->rawValue();
    const auto restoreSetting = qScopeGuard([=] { useChecklist->setRawValue(savedUseChecklist); });
    useChecklist->setRawValue(true);
    QVERIFY(_usePreVehicleChecksFixture());

    QPointer<MockLink> mockLink;
    Vehicle* vehicle = nullptr;
    if (!_startChecklistUI(mockLink, vehicle)) {
        return;
    }
    auto cleanup = qScopeGuard([&] { _stopChecklistUI(mockLink); });

    QVERIFY2(_openChecklistTab(), "Checklist tab not shown");
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("preVehicleCheckList"), TestTimeout::mediumMs()),
             "Pre-vehicle checklist not shown");

    QQuickItem* const vehicleButton = findVehicleChecklistButton(_rootItem);
    QVERIFY2(vehicleButton, "No vehicle checklist item shown");
    QVERIFY2(!vehicleButton->isEnabled(), "Vehicle checks not locked behind the incomplete pre-vehicle checks");

    QVERIFY2(!_passPreVehicleChecklistItems().isEmpty(), "No pre-vehicle check could be confirmed");
    QTRY_VERIFY_WITH_TIMEOUT(vehicleButton->isEnabled(), TestTimeout::mediumMs());
}

void ToolbarIndicatorUITest::_testPreVehicleChecklistResetsAfterFlight()
{
    AppSettings* const appSettings = SettingsManager::instance()->appSettings();
    Fact* const useChecklist = appSettings->useChecklist();
    const QVariant savedUseChecklist = useChecklist->rawValue();
    const auto restoreSetting = qScopeGuard([=] { useChecklist->setRawValue(savedUseChecklist); });
    useChecklist->setRawValue(true);
    QVERIFY(_usePreVehicleChecksFixture());

    startUI();
    if (QTest::currentTestFailed()) {
        return;
    }
    QPointer<MockLink> mockLink;
    Vehicle* vehicle = nullptr;
    auto cleanup = qScopeGuard([&] {
        if (mockLink) {
            disconnectMockLink(mockLink);
        }
        closeUIWindow();
        destroyUIEngine();
    });

    QVERIFY2(_openChecklistTab(), "Checklist tab not shown while disconnected");
    const QStringList names = _passPreVehicleChecklistItems();
    QVERIFY2(!names.isEmpty(), "No pre-vehicle check could be confirmed");
    QVERIFY2(_closeIndicatorDrawer(), "Drawer did not close on Escape");

    mockLink = connectMockLinkAndWaitReady([] { return MockLink::startPX4MockLink(); }, vehicle);
    QVERIFY(mockLink);

    // 1: passed, 0: not passed, -1: couldn't be inspected
    const auto firstCheckState = [&] {
        if (!_openChecklistTab()) {
            return -1;
        }
        QQuickItem* const button = findChecklistButton(_rootItem, names.first());
        const int state = button ? (button->property("passed").toBool() ? 1 : 0) : -1;
        return _closeIndicatorDrawer() ? state : -1;
    };

    // Arming and disarming on the ground doesn't make the checks stale
    vehicle->setArmed(true, false /* showError */);
    QVERIFY_TRUE_WAIT(vehicle->armed(), TestTimeout::mediumMs());
    vehicle->setArmed(false, false /* showError */);
    QVERIFY_TRUE_WAIT(!vehicle->armed(), TestTimeout::mediumMs());
    QCOMPARE(firstCheckState(), 1);

    // The airborne transition creates QGCPressure, which warns on hosts without a pressure backend
    ignoreLogMessage("Utilities.QGCSensors", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to connect to pressure backend")));
    ignoreLogMessage("Utilities.QGCSensors", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Error Initializing Pressure Sensor")));
    // MockLink arms and climbs above home on takeoff
    vehicle->sendMavCommand(vehicle->defaultComponentId(), MAV_CMD_NAV_TAKEOFF, false /* showError */, 0.0f, 0.0f, 0.0f,
                            0.0f, 0.0f, 0.0f, 10.0f /* altitude */);
    QVERIFY_TRUE_WAIT(vehicle->airborne(), TestTimeout::longMs());
    vehicle->setArmed(false, false /* showError */);
    QVERIFY_TRUE_WAIT(!vehicle->armed(), TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(firstCheckState(), 0, TestTimeout::mediumMs());
}

void ToolbarIndicatorUITest::_testEnforcedChecklistRequiresPreVehicleChecks()
{
    AppSettings* const appSettings = SettingsManager::instance()->appSettings();
    Fact* const useChecklist = appSettings->useChecklist();
    Fact* const enforceChecklist = appSettings->enforceChecklist();
    const QVariant savedUseChecklist = useChecklist->rawValue();
    const QVariant savedEnforceChecklist = enforceChecklist->rawValue();
    const auto restoreSettings = qScopeGuard([=] {
        useChecklist->setRawValue(savedUseChecklist);
        enforceChecklist->setRawValue(savedEnforceChecklist);
    });
    useChecklist->setRawValue(true);
    enforceChecklist->setRawValue(true);
    QVERIFY(_usePreVehicleChecksFixture());

    QPointer<MockLink> mockLink;
    Vehicle* vehicle = nullptr;
    if (!_startChecklistUI(mockLink, vehicle)) {
        return;
    }
    auto cleanup = qScopeGuard([&] { _stopChecklistUI(mockLink); });

    // MockLink can't pass every stock vehicle check, so stand in for them having passed
    QQuickItem* const vehicleChecklist = findItem(_rootItem, QStringLiteral("preFlightCheckList"));
    QVERIFY2(vehicleChecklist, "Vehicle checklist not loaded");
    QVERIFY(vehicleChecklist->setProperty("allChecksPassed", true));
    QVERIFY2(vehicle->checkListState() != Vehicle::CheckListPassed,
             "Vehicle checklist reported passed while the pre-vehicle checks are pending");

    QVERIFY2(_openChecklistTab(), "Checklist tab not shown");
    QQuickItem* const armButton =
        findVisibleItem(_rootItem, QStringLiteral("mainStatusArmButton"), TestTimeout::mediumMs());
    QVERIFY2(armButton, "Arm button not shown in the main status drawer");
    QVERIFY2(!armButton->isEnabled(), "Arm enabled while the pre-vehicle checks are pending");

    QVERIFY2(!_passPreVehicleChecklistItems().isEmpty(), "No pre-vehicle check could be confirmed");
    QTRY_VERIFY_WITH_TIMEOUT(vehicle->checkListState() == Vehicle::CheckListPassed, TestTimeout::mediumMs());
    QTRY_VERIFY_WITH_TIMEOUT(armButton->isEnabled(), TestTimeout::mediumMs());

    // Resetting the pre-vehicle checks blocks arming again, although the vehicle checks still pass
    const QString resetButtonName = QStringLiteral("preVehicleCheckListResetButton");
    QQuickItem* const resetButton = findVisibleItem(_rootItem, resetButtonName, TestTimeout::mediumMs());
    QVERIFY2(resetButton, "Pre-vehicle checklist Reset button not shown");
    QVERIFY(_clickItemAt(resetButton, 0.5, 0.5, resetButtonName));
    QTRY_VERIFY_WITH_TIMEOUT(vehicle->checkListState() != Vehicle::CheckListPassed, TestTimeout::mediumMs());
    QTRY_VERIFY_WITH_TIMEOUT(!armButton->isEnabled(), TestTimeout::mediumMs());
}

void ToolbarIndicatorUITest::_testAutomaticPreVehicleChecksPassOnLoad()
{
    AppSettings* const appSettings = SettingsManager::instance()->appSettings();
    Fact* const useChecklist = appSettings->useChecklist();
    const QVariant savedUseChecklist = useChecklist->rawValue();
    const auto restoreSetting = qScopeGuard([=] { useChecklist->setRawValue(savedUseChecklist); });
    useChecklist->setRawValue(true);
    QVERIFY(_usePreVehicleChecksFixture(kAutomaticPreVehicleChecksFixture));

    startUI();
    if (QTest::currentTestFailed()) {
        return;
    }

    QVERIFY2(_openChecklistTab(), "Checklist tab not shown while disconnected");
    QQuickItem* const tabBadge =
        findVisibleItem(_rootItem, QStringLiteral("mainStatusChecklistTabBadge"), TestTimeout::mediumMs());
    QVERIFY2(tabBadge, "Checklist tab badge not shown while disconnected");
    QTRY_COMPARE_WITH_TIMEOUT(tabBadge->property("text").toString(), QStringLiteral("Passed"), TestTimeout::mediumMs());
    QVERIFY2(!findVisibleItem(_rootItem, QStringLiteral("mainStatusSeverityBadge"), 0),
             "Toolbar badge shown for pre-vehicle checks which already passed");
}

bool ToolbarIndicatorUITest::_usePreVehicleChecksFixture(const char* fixtureQml)
{
    if (!_preVehicleChecksDir) {
        _preVehicleChecksDir = std::make_unique<QTemporaryDir>();
        if (!_preVehicleChecksDir->isValid()) {
            _preVehicleChecksDir.reset();
            return false;
        }
    }
    QFile file(_preVehicleChecksDir->filePath(QStringLiteral("PreVehicleChecks.qml")));
    if (!fixtureQml) {
        fixtureQml = kPreVehicleChecksFixture;
    }
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text) || (file.write(fixtureQml) < 0)) {
        return false;
    }
    file.close();
    overrideQmlFile(QUrl(QStringLiteral("qrc:/qml/QGroundControl/FlyView/PreVehicleChecks.qml")),
                    QUrl::fromLocalFile(_preVehicleChecksDir->filePath(QStringLiteral("PreVehicleChecks.qml"))));
    return true;
}
