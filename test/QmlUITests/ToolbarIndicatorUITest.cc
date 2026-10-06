#include "ToolbarIndicatorUITest.h"

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
#include "MavlinkSettings.h"
#include "MockConfiguration.h"
#include "MockLink.h"
#include "MultiVehicleManager.h"
#include "SettingsManager.h"
#include "Vehicle.h"

UT_REGISTER_TEST(ToolbarIndicatorUITest, TestLabel::Integration)

// ---------------------------------------------------------------------------
// _exerciseIndicator
// ---------------------------------------------------------------------------

bool ToolbarIndicatorUITest::_exerciseIndicator(QQuickItem *indicatorItem, const QString &indicatorName, bool expectExpand)
{
    if (!indicatorItem || !_window) {
        return false;
    }

    // 1. Click the indicator — verify the drawer opens
    const QPointF indicatorCenter = indicatorItem->mapToScene(
        QPointF(indicatorItem->width() / 2.0, indicatorItem->height() / 2.0));
    QTest::mouseClick(_window, Qt::LeftButton, Qt::NoModifier, indicatorCenter.toPoint());

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
        const QPointF expandCenter = expandBtn->mapToScene(
            QPointF(expandBtn->width() / 2.0, expandBtn->height() / 2.0));
        QTest::mouseClick(_window, Qt::LeftButton, Qt::NoModifier, expandCenter.toPoint());
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
                    [this](QPointer<MockLink> /*mockLink*/, Vehicle* vehicle) {
                        QQuickItem* const indicator = findVisibleItem(
                            _rootItem, QStringLiteral("toolbar_mainStatusIndicator"), TestTimeout::mediumMs());
                        QVERIFY2(indicator, "Main status indicator not visible");
                        QVERIFY(_clickItemAt(indicator, 0.5, 0.5, QStringLiteral("toolbar_mainStatusIndicator")));

                        QQuickItem* const armButton =
                            findVisibleItem(_rootItem, QStringLiteral("mainStatusArmButton"), TestTimeout::mediumMs());
                        QVERIFY2(armButton, "Arm button not shown in the main status drawer");
                        QCOMPARE(armButton->property("text").toString(), QStringLiteral("Arm"));
                        QVERIFY2(!armButton->isEnabled(), "Arm enabled before the enforced checklist passed");

                        vehicle->setCheckListState(Vehicle::CheckListPassed);
                        QTRY_VERIFY_WITH_TIMEOUT(armButton->isEnabled(), TestTimeout::mediumMs());
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
            const QString iconName = QStringLiteral("mainStatusRebootRequiredIcon");
            QVERIFY2(!findVisibleItem(_rootItem, iconName, 0),
                     "Reboot-required icon shown before a reboot was required");
            vehicle->setRebootRequired();
            QVERIFY2(findVisibleItem(_rootItem, iconName, TestTimeout::mediumMs()), "Reboot-required icon not shown");

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
