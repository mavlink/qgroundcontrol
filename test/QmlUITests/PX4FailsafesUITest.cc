#include "PX4FailsafesUITest.h"

#include <QtCore/QPointer>
#include <QtQuick/QQuickItem>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "Fact.h"
#include "MockLink.h"
#include "ParameterManager.h"
#include "QGCApplication.h"
#include "Vehicle.h"

UT_REGISTER_TEST(PX4FailsafesUITest, TestLabel::Integration)

void PX4FailsafesUITest::_navigateToFailsafes()
{
    navigateToConfigureView();
    if (QTest::currentTestFailed()) {
        return;
    }
    (void) clickSidebarButton(QStringLiteral("vehicleConfig_comp_Failsafes"));
}

void PX4FailsafesUITest::_setRebootRequiredFact(Fact* fact, const QVariant& rawValue)
{
    QSignalSpy ackSpy(fact, &Fact::vehicleUpdated);
    QVERIFY(ackSpy.isValid());
    qgcApp()->resetRebootMessageDebounce();
    expectAppMessage(QRegularExpression(QStringLiteral("Vehicle reboot required for changes to take effect")));
    fact->setRawValue(rawValue);
    QVERIFY(ackSpy.wait(5000));
    QVERIFY2(acceptDialog(5000), "Reboot-required notice never shown");
    verifyExpectedLogMessage();
}

// A reboot-required setting is a muted hint until it changes from its value when the page opened
void PX4FailsafesUITest::_testRebootChip()
{
    runWithMockLink([] { return MockLink::startPX4MockLink(); },
                    [&](QPointer<MockLink> /*mockLink*/, Vehicle* vehicle) {
                        Fact* const detection = vehicle->parameterManager()->getParameter(
                            ParameterManager::defaultComponentId, QStringLiteral("FD_ACT_EN"));
                        QVERIFY(detection);
                        QVERIFY(detection->vehicleRebootRequired());
                        const QVariant initialValue = detection->rawValue();

                        _navigateToFailsafes();
                        if (QTest::currentTestFailed()) {
                            return;
                        }

                        QQuickItem* const chip =
                            findVisibleItem(_rootItem, QStringLiteral("failsafes_rebootChip_FD_ACT_EN"), 3000);
                        QVERIFY(chip);
                        QCOMPARE(chip->property("text").toString(), QStringLiteral("Applies after reboot"));

                        _setRebootRequiredFact(detection, initialValue.toInt() ? 0 : 1);
                        if (QTest::currentTestFailed()) {
                            return;
                        }
                        QTRY_COMPARE(chip->property("text").toString(), QStringLiteral("Reboot required"));

                        _setRebootRequiredFact(detection, initialValue);
                        if (QTest::currentTestFailed()) {
                            return;
                        }
                        QTRY_COMPARE(chip->property("text").toString(), QStringLiteral("Applies after reboot"));
                    });
}

void PX4FailsafesUITest::_testBitmaskSlider()
{
    runWithMockLink([] { return MockLink::startPX4MockLink(); },
                    [&](QPointer<MockLink> /*mockLink*/, Vehicle* vehicle) {
                        Fact* const exceptions = vehicle->parameterManager()->getParameter(
                            ParameterManager::defaultComponentId, QStringLiteral("COM_RCL_EXCEPT"));
                        QVERIFY(exceptions);
                        QVERIFY(!exceptions->bitmaskValues().isEmpty());
                        const int bit = exceptions->bitmaskValues().first().toInt();
                        exceptions->setRawValue(0);
                        QVERIFY_TRUE_WAIT(!vehicle->parameterManager()->pendingWrites(), TestTimeout::mediumMs());
                        QCOMPARE(exceptions->rawValue().toInt() & bit, 0);

                        _navigateToFailsafes();
                        if (QTest::currentTestFailed()) {
                            return;
                        }

                        const QString sliderName = QStringLiteral("failsafes_bit_COM_RCL_EXCEPT_0");
                        QQuickItem* const slider =
                            findVisibleItemScrolled(sliderName, QStringLiteral("setupPage_flickable"));
                        QVERIFY(slider);
                        QVERIFY(!slider->property("checked").toBool());

                        QVERIFY(clickButton(sliderName));
                        QTRY_COMPARE(exceptions->rawValue().toInt() & bit, bit);
                        QVERIFY(slider->property("checked").toBool());
                        // The value changes locally before the vehicle acks; finish each write before the next step
                        QVERIFY_TRUE_WAIT(!vehicle->parameterManager()->pendingWrites(), TestTimeout::mediumMs());

                        QVERIFY(clickButton(sliderName));
                        QTRY_COMPARE(exceptions->rawValue().toInt() & bit, 0);
                        QVERIFY(!slider->property("checked").toBool());
                        QVERIFY_TRUE_WAIT(!vehicle->parameterManager()->pendingWrites(), TestTimeout::mediumMs());
                    });
}
