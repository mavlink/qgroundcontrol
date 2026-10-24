#include "PX4BatteryUITest.h"

#include <QtCore/QPointer>
#include <QtQuick/QQuickItem>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "AutoPilotPlugin.h"
#include "Fact.h"
#include "MockLink.h"
#include "ParameterManager.h"
#include "QGCApplication.h"
#include "Vehicle.h"
#include "VehicleComponent.h"

UT_REGISTER_TEST(PX4BatteryUITest, TestLabel::Integration)

void PX4BatteryUITest::_setBattery1Source(Vehicle* vehicle, int source)
{
    Fact* const source1 =
        vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, QStringLiteral("BAT1_SOURCE"));
    QVERIFY(source1);

    QSignalSpy ackSpy(source1, &Fact::vehicleUpdated);
    QVERIFY(ackSpy.isValid());
    qgcApp()->resetRebootMessageDebounce();
    expectAppMessage(QRegularExpression(QStringLiteral("Vehicle reboot required for changes to take effect")));
    source1->setRawValue(source);
    QVERIFY(ackSpy.wait(5000));
    QVERIFY2(acceptDialog(5000), "Reboot-required notice never shown");
    verifyExpectedLogMessage();
}

void PX4BatteryUITest::_openBatteryPage()
{
    navigateToConfigureView();
    if (QTest::currentTestFailed()) {
        return;
    }
    QVERIFY(clickSidebarButton(QStringLiteral("vehicleConfig_comp_Battery")));
}

// Battery 2 is not in PX4's initial download. The page requests it, shows it once it arrives, and only picks the
// In Use filter then, so a vehicle whose only battery arrives late still opens on what is in use.
void PX4BatteryUITest::_lateParamShownAndFilterChosenOnceLoaded()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [&](QPointer<MockLink> /*mockLink*/, Vehicle* vehicle) {
            ParameterManager* const parameterManager = vehicle->parameterManager();
            QVERIFY(!parameterManager->parameterExists(ParameterManager::defaultComponentId,
                                                       QStringLiteral("BAT2_SOURCE")));
            _setBattery1Source(vehicle, -1);
            if (QTest::currentTestFailed()) {
                return;
            }
            _openBatteryPage();
            if (QTest::currentTestFailed()) {
                return;
            }

            QVERIFY(findVisibleItem(_rootItem, QStringLiteral("battery_row_battery2"), 5000));
            QTRY_VERIFY_WITH_TIMEOUT(!findVisibleItem(_rootItem, QStringLiteral("battery_loading"), 0), 5000);
            // In Use hides the disabled battery 1
            QVERIFY(!findVisibleItem(_rootItem, QStringLiteral("battery_row_battery1"), 0));
            QVERIFY(findItem(_rootItem, QStringLiteral("battery_row_battery1")));
        });
}

// All shows a battery that is not in use, and In Use hides it again
void PX4BatteryUITest::_filterTogglesUnusedBattery()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [&](QPointer<MockLink> /*mockLink*/, Vehicle* vehicle) {
            _setBattery1Source(vehicle, -1);
            if (QTest::currentTestFailed()) {
                return;
            }
            _openBatteryPage();
            if (QTest::currentTestFailed()) {
                return;
            }

            QQuickItem* const filterBar = findVisibleItem(_rootItem, QStringLiteral("battery_filter"), 3000);
            QVERIFY(filterBar);
            QTRY_VERIFY_WITH_TIMEOUT(!findVisibleItem(_rootItem, QStringLiteral("battery_loading"), 0), 5000);

            // The summary counts batteries and power monitors, not the settings every battery shares
            const QString rowPrefix = QStringLiteral("battery_row_");
            const QStringList sharedRows = {QStringLiteral("battery_row_levels"),
                                            QStringLiteral("battery_row_flightTime"),
                                            QStringLiteral("battery_row_analogOffset")};

            QVERIFY(QMetaObject::invokeMethod(filterBar, "filterActivated", Q_ARG(int, 0)));
            QTRY_VERIFY(findVisibleItem(_rootItem, QStringLiteral("battery_row_battery1"), 0));
            QCOMPARE(filterBar->property("summary").toString(),
                     QStringLiteral("Showing all %1 batteries and power monitors.")
                         .arg(countRows(rowPrefix, sharedRows).second));

            QVERIFY(QMetaObject::invokeMethod(filterBar, "filterActivated", Q_ARG(int, 1)));
            QTRY_VERIFY(!findVisibleItem(_rootItem, QStringLiteral("battery_row_battery1"), 0));
            // Settings shared by every battery stay shown
            QVERIFY(findVisibleItem(_rootItem, QStringLiteral("battery_row_levels"), 0));
            const auto [visibleRows, totalRows] = countRows(rowPrefix, sharedRows);
            QVERIFY(visibleRows < totalRows);
            QCOMPARE(filterBar->property("summary").toString(),
                     QStringLiteral("Showing %1 of %2 batteries and power monitors.").arg(visibleRows).arg(totalRows));
        });
}

// A wrapped label reports its single-line width as its implicit width, so a long note directly under the panel makes
// the page wider than the window and scrolls it sideways instead of letting the cells wrap.
void PX4BatteryUITest::_unclaimedBatteryNoteDoesNotWidenPage()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [&](QPointer<MockLink> /*mockLink*/, Vehicle* vehicle) {
            // An External battery cannot be matched to the telemetry it reports, so that telemetry is listed under
            // Other Reported Batteries
            _setBattery1Source(vehicle, 1);
            if (QTest::currentTestFailed()) {
                return;
            }
            _openBatteryPage();
            if (QTest::currentTestFailed()) {
                return;
            }
            QTRY_VERIFY_WITH_TIMEOUT(!findVisibleItem(_rootItem, QStringLiteral("battery_loading"), 0), 5000);
            QTRY_VERIFY_WITH_TIMEOUT(countRows(QStringLiteral("battery_other_")).second > 0, 5000);

            QQuickItem* const flickable = findItem(_rootItem, QStringLiteral("setupPage_flickable"));
            QVERIFY(flickable);
            const qreal contentWidth = flickable->property("contentWidth").toReal();
            QVERIFY2(
                contentWidth <= flickable->width() + 1,
                qPrintable(
                    QStringLiteral("Page is %1 wide in a %2 wide window").arg(contentWidth).arg(flickable->width())));
        });
}

// A row has a rule above it only when a row above it in its group is shown, so the first shown row does not double the
// group heading's rule.
void PX4BatteryUITest::_rowSeparatorsFollowTheShownRows()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [&](QPointer<MockLink> /*mockLink*/, Vehicle* vehicle) {
            _setBattery1Source(vehicle, -1);
            if (QTest::currentTestFailed()) {
                return;
            }
            _openBatteryPage();
            if (QTest::currentTestFailed()) {
                return;
            }

            // In Use hides the disabled battery 1, so battery 2 is the first battery row shown
            const QString separatorName = QStringLiteral("configRowSeparator");
            QQuickItem* const battery2 = findVisibleItem(_rootItem, QStringLiteral("battery_row_battery2"), 5000);
            QVERIFY(battery2);
            QTRY_VERIFY_WITH_TIMEOUT(!findVisibleItem(_rootItem, QStringLiteral("battery_loading"), 0), 5000);
            QVERIFY(!findVisibleItem(_rootItem, QStringLiteral("battery_row_battery1"), 0));
            QVERIFY2(!findVisibleItem(battery2, separatorName, 0), "First shown battery row has a rule above it");

            // All shows battery 1 first, which puts battery 2 under another row
            QQuickItem* const filterBar = findVisibleItem(_rootItem, QStringLiteral("battery_filter"), 3000);
            QVERIFY(filterBar);
            QVERIFY(QMetaObject::invokeMethod(filterBar, "filterActivated", Q_ARG(int, 0)));
            QQuickItem* battery1 = nullptr;
            QTRY_VERIFY_WITH_TIMEOUT(
                (battery1 = findVisibleItem(_rootItem, QStringLiteral("battery_row_battery1"), 0)) != nullptr, 5000);
            QVERIFY2(!findVisibleItem(battery1, separatorName, 0), "First battery row has a rule above it");
            QQuickItem* const shownBattery2 = findVisibleItem(_rootItem, QStringLiteral("battery_row_battery2"), 0);
            QVERIFY(shownBattery2);
            QTRY_VERIFY2_WITH_TIMEOUT(findVisibleItem(shownBattery2, separatorName, 0) != nullptr,
                                      "Second battery row has no rule above it", 5000);
        });
}

// A reported battery is marked active once it has reported a voltage. MockLink reports battery 1 with values and
// battery 2 without any.
void PX4BatteryUITest::_reportedBatteryDotFollowsTheReportedVoltage()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [&](QPointer<MockLink> /*mockLink*/, Vehicle* vehicle) {
            _setBattery1Source(vehicle, 1);
            if (QTest::currentTestFailed()) {
                return;
            }
            _openBatteryPage();
            if (QTest::currentTestFailed()) {
                return;
            }
            QTRY_VERIFY_WITH_TIMEOUT(!findVisibleItem(_rootItem, QStringLiteral("battery_loading"), 0), 5000);

            QQuickItem* const withValues = findVisibleItem(_rootItem, QStringLiteral("battery_other_1"), 5000);
            QQuickItem* const withoutValues = findVisibleItem(_rootItem, QStringLiteral("battery_other_2"), 5000);
            QVERIFY(withValues);
            QVERIFY(withoutValues);
            QTRY_VERIFY_WITH_TIMEOUT(withValues->property("active").toBool(), 5000);
            QVERIFY(!withoutValues->property("active").toBool());
        });
}

// Cells and voltages only count toward setup when PX4 estimates the battery, since only then does the page show them
void PX4BatteryUITest::_setupCompleteIgnoresCellsForExternalBattery()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [&](QPointer<MockLink> /*mockLink*/, Vehicle* vehicle) {
            VehicleComponent* batteryComponent = nullptr;
            for (const QVariant& component : vehicle->autopilotPlugin()->vehicleComponents()) {
                VehicleComponent* const candidate = component.value<VehicleComponent*>();
                if (candidate && candidate->KnownVehicleComponent() == AutoPilotPlugin::KnownPowerVehicleComponent) {
                    batteryComponent = candidate;
                }
            }
            QVERIFY(batteryComponent);
            QVERIFY(batteryComponent->setupComplete());

            Fact* const cells = vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId,
                                                                          QStringLiteral("BAT1_N_CELLS"));
            QVERIFY(cells);
            QSignalSpy cellsAckSpy(cells, &Fact::vehicleUpdated);
            QVERIFY(cellsAckSpy.isValid());
            cells->setRawValue(0);
            QVERIFY(cellsAckSpy.wait(5000));
            QVERIFY(!batteryComponent->setupComplete());

            _setBattery1Source(vehicle, 1);
            if (QTest::currentTestFailed()) {
                return;
            }
            QVERIFY(batteryComponent->setupComplete());
        });
}
