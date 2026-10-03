#include "ToolbarIndicatorUITest.h"

#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QTest>

#include "MockConfiguration.h"
#include "MockLink.h"
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
    // Rover reports flying while armed and moving, but accepts a normal disarm
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
                // The flying transition creates QGCPressure, which warns on hosts without a pressure backend
                ignoreLogMessage("Utilities.QGCSensors", QtWarningMsg,
                                 QRegularExpression(QStringLiteral("Failed to connect to pressure backend")));
                ignoreLogMessage("Utilities.QGCSensors", QtWarningMsg,
                                 QRegularExpression(QStringLiteral("Error Initializing Pressure Sensor")));
                // MockLink arms and climbs above home on takeoff
                vehicle->sendMavCommand(vehicle->defaultComponentId(), MAV_CMD_NAV_TAKEOFF, false /* showError */, 0.0f,
                                        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 10.0f /* altitude */);
                QVERIFY_TRUE_WAIT(vehicle->flying(), TestTimeout::longMs());
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

            // Held until activated: emergency stop disarms the vehicle in the air
            const QPoint center =
                shownButton->mapToScene(QPointF(shownButton->width() / 2, shownButton->height() / 2)).toPoint();
            QTest::mousePress(_window, Qt::LeftButton, Qt::NoModifier, center);
            QVERIFY_TRUE_WAIT(!vehicle->armed(), TestTimeout::mediumMs());
            QTest::mouseRelease(_window, Qt::LeftButton, Qt::NoModifier, center);
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
