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
