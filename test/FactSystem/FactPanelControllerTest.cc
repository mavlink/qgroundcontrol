#include "FactPanelControllerTest.h"

#include <QtTest/QSignalSpy>

#include "Fact.h"
#include "FactPanelController.h"
#include "ParameterManager.h"
#include "QGCApplication.h"
#include "Vehicle.h"

UT_REGISTER_TEST(FactPanelControllerTest, TestLabel::Integration, TestLabel::Vehicle)

namespace {

const QRegularExpression kRebootNotice(QStringLiteral("Vehicle reboot required for changes to take effect"));

}  // namespace

void FactPanelControllerTest::_rebootNoticeSuppressedWhileRequested_data()
{
    QTest::addColumn<bool>("suppress");

    QTest::newRow("suppressed") << true;
    QTest::newRow("notRequested") << false;
}

// Pages that mark reboot-required settings themselves suppress the notice, which returns once the page closes
void FactPanelControllerTest::_rebootNoticeSuppressedWhileRequested()
{
    QFETCH(bool, suppress);

    Fact* const fact = vehicle()->parameterManager()->getParameter(ParameterManager::defaultComponentId,
                                                                   QStringLiteral("BAT1_V_EMPTY"));
    QVERIFY(fact);
    QVERIFY(fact->vehicleRebootRequired());

    const auto setAndWaitForAck = [fact](double value) {
        QSignalSpy ackSpy(fact, &Fact::vehicleUpdated);
        fact->setRawValue(value);
        return ackSpy.wait(TestTimeout::mediumMs());
    };

    {
        FactPanelController controller;
        controller.setSuppressRebootMessage(suppress);
        QCOMPARE(controller.suppressRebootMessage(), suppress);

        qgcApp()->resetRebootMessageDebounce();
        if (!suppress) {
            expectAppMessage(kRebootNotice);
        }
        // Strict log checking fails the test if a suppressed notice is shown
        QVERIFY(setAndWaitForAck(3.5));
        if (!suppress) {
            verifyExpectedLogMessage();
        }
    }

    qgcApp()->resetRebootMessageDebounce();
    expectAppMessage(kRebootNotice);
    QVERIFY(setAndWaitForAck(3.75));
    verifyExpectedLogMessage();
}

// The suppression belongs to the vehicle's parameter manager, so it ends, and says so, when the vehicle goes away
void FactPanelControllerTest::_rebootNoticeSuppressionEndsWithVehicle()
{
    FactPanelController controller;
    controller.setSuppressRebootMessage(true);
    QVERIFY(controller.suppressRebootMessage());
    QSignalSpy changedSpy(&controller, &FactPanelController::suppressRebootMessageChanged);
    QVERIFY(changedSpy.isValid());

    _disconnectMockLink();

    QTRY_VERIFY_WITH_TIMEOUT(!controller.suppressRebootMessage(), TestTimeout::mediumMs());
    QCOMPARE(changedSpy.count(), 1);
}
