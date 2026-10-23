#include "PX4BatteryUITest.h"

#include <QtCore/QPointer>
#include <QtQuick/QQuickItem>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "Fact.h"
#include "MockLink.h"
#include "ParameterManager.h"
#include "QGCApplication.h"
#include "Vehicle.h"

UT_REGISTER_TEST(PX4BatteryUITest, TestLabel::Integration)

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
            Fact* const source1 =
                parameterManager->getParameter(ParameterManager::defaultComponentId, QStringLiteral("BAT1_SOURCE"));
            QVERIFY(source1);

            QSignalSpy ackSpy(source1, &Fact::vehicleUpdated);
            QVERIFY(ackSpy.isValid());
            qgcApp()->resetRebootMessageDebounce();
            expectAppMessage(QRegularExpression(QStringLiteral("Vehicle reboot required for changes to take effect")));
            source1->setRawValue(-1);
            QVERIFY(ackSpy.wait(5000));
            QVERIFY2(acceptDialog(5000), "Reboot-required notice never shown");
            verifyExpectedLogMessage();

            navigateToConfigureView();
            if (QTest::currentTestFailed()) {
                return;
            }
            QVERIFY(clickSidebarButton(QStringLiteral("vehicleConfig_comp_Battery")));

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
            Fact* const source1 = vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId,
                                                                            QStringLiteral("BAT1_SOURCE"));
            QVERIFY(source1);

            QSignalSpy ackSpy(source1, &Fact::vehicleUpdated);
            QVERIFY(ackSpy.isValid());
            qgcApp()->resetRebootMessageDebounce();
            expectAppMessage(QRegularExpression(QStringLiteral("Vehicle reboot required for changes to take effect")));
            source1->setRawValue(-1);
            QVERIFY(ackSpy.wait(5000));
            QVERIFY2(acceptDialog(5000), "Reboot-required notice never shown");
            verifyExpectedLogMessage();

            navigateToConfigureView();
            if (QTest::currentTestFailed()) {
                return;
            }
            QVERIFY(clickSidebarButton(QStringLiteral("vehicleConfig_comp_Battery")));

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
