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

void PX4FailsafesUITest::_setFactAndWait(Fact* fact, const QVariant& rawValue)
{
    QSignalSpy ackSpy(fact, &Fact::vehicleUpdated);
    QVERIFY(ackSpy.isValid());
    // The page marks the setting itself, so strict log checking fails the test if the reboot notice is shown
    qgcApp()->resetRebootMessageDebounce();
    fact->setRawValue(rawValue);
    QVERIFY(ackSpy.wait(5000));
}

// A reboot-required setting is a muted hint until it changes from its value when the page opened. The page shows
// this instead of the vehicle reboot notice.
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

                        _setFactAndWait(detection, initialValue.toInt() ? 0 : 1);
                        if (QTest::currentTestFailed()) {
                            return;
                        }
                        QTRY_COMPARE(chip->property("text").toString(), QStringLiteral("Reboot required"));

                        _setFactAndWait(detection, initialValue);
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

// Active hides failsafes that are turned off, and a group once all its failsafes are hidden
void PX4FailsafesUITest::_activeFilterHidesInactiveFailsafes()
{
    runWithMockLink([] { return MockLink::startPX4MockLink(); },
                    [&](QPointer<MockLink> /*mockLink*/, Vehicle* vehicle) {
                        _navigateToFailsafes();
                        if (QTest::currentTestFailed()) {
                            return;
                        }

                        QQuickItem* const filterBar =
                            findVisibleItem(_rootItem, QStringLiteral("failsafes_filter"), 3000);
                        QVERIFY(filterBar);
                        QVERIFY(findItem(_rootItem, QStringLiteral("failsafes_row_attitude")));

                        // Turn off every Vehicle Health failsafe the vehicle has on, with the page open so it marks the
                        // reboot-required ones instead of showing the reboot notice
                        const QList<QPair<QString, int>> disabled = {
                            {QStringLiteral("FD_FAIL_R"), 0},       {QStringLiteral("FD_FAIL_P"), 0},
                            {QStringLiteral("FD_ACT_EN"), 0},       {QStringLiteral("FD_ESCS_EN"), 0},
                            {QStringLiteral("FD_IMB_PROP_THR"), 0},
                        };
                        for (const auto& [name, value] : disabled) {
                            Fact* const fact =
                                vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, name);
                            QVERIFY2(fact, qPrintable(name));
                            if (fact->rawValue().toInt() != value) {
                                _setFactAndWait(fact, value);
                                if (QTest::currentTestFailed()) {
                                    return;
                                }
                            }
                        }

                        QVERIFY(QMetaObject::invokeMethod(filterBar, "filterActivated", Q_ARG(int, 1)));
                        QTRY_VERIFY(!findVisibleItem(_rootItem, QStringLiteral("failsafes_row_attitude"), 0));
                        QVERIFY(!findVisibleItem(_rootItem, QStringLiteral("failsafes_group_VehicleHealth"), 0));
                        // Rows without an active condition always show
                        QVERIFY(findVisibleItem(_rootItem, QStringLiteral("failsafes_row_battery"), 0));

                        // The summary counts the failsafes actually shown, out of every one on the page
                        const auto [visibleRows, totalRows] = countRows(QStringLiteral("failsafes_row_"));
                        QVERIFY(visibleRows < totalRows);
                        QCOMPARE(filterBar->property("summary").toString(),
                                 QStringLiteral("Showing %1 of %2 failsafes.").arg(visibleRows).arg(totalRows));

                        QVERIFY(QMetaObject::invokeMethod(filterBar, "filterActivated", Q_ARG(int, 0)));
                        QTRY_VERIFY(findVisibleItem(_rootItem, QStringLiteral("failsafes_group_VehicleHealth"), 0));
                        QCOMPARE(filterBar->property("summary").toString(),
                                 QStringLiteral("Showing all %1 failsafes.").arg(totalRows));
                    });
}
