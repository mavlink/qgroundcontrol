#include "FailureInjectionUITest.h"

#include <QtQuick/QQuickItem>
#include <QtTest/QTest>

#include "MockLink.h"

UT_REGISTER_TEST(FailureInjectionUITest, TestLabel::Integration)

void FailureInjectionUITest::_testInjectAndReset()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(false, false, false); },
        [&](QPointer<MockLink> /*mockLink*/, Vehicle * /*vehicle*/) {

    navigateToConfigureView();
    if (QTest::currentTestFailed()) return;

    clickSidebarButton(QStringLiteral("vehicleConfig_comp_FailureInjection"));
    if (QTest::currentTestFailed()) return;

    // PX4MockLink.params ships SYS_FAILURE_EN=1, so the page starts already armed.
    QVERIFY2(verifyChecked(QStringLiteral("failureInjection_enableCheckbox"), true, "on page open"),
             "SYS_FAILURE_EN checkbox not checked on page open");
    QQuickItem *armedLabel = findVisibleItem(_rootItem, QStringLiteral("failureInjection_armedLabel"), 2000);
    QVERIFY2(armedLabel, "Armed label not visible on page open");

    // Default selection (unit index 4 = GPS, type index 1 = OFF, instance 1) is injected as-is.
    QVERIFY2(clickButton(QStringLiteral("failureInjection_injectButton")), "Failed to click Inject failure");

    QQuickItem *activityList = findVisibleItem(_rootItem, QStringLiteral("failureInjection_activityList"), 2000);
    QVERIFY2(activityList, "Activity list not visible after injecting");
    QVERIFY2(QTest::qWaitFor([&] { return activityList->property("count").toInt() == 1; }, 3000),
             "Activity row not added after injecting");

    QVERIFY2(verifyText(QStringLiteral("failureInjection_unitName_0"), QStringLiteral("GPS"), "after inject"),
             "Injected row does not show unit GPS");
    QVERIFY2(verifyText(QStringLiteral("failureInjection_typeName_0"), QStringLiteral("OFF"), "after inject"),
             "Injected row does not show type OFF");

    // MockLink doesn't handle MAV_CMD_INJECT_FAILURE, so it acks MAV_RESULT_UNSUPPORTED.
    QVERIFY2(verifyText(QStringLiteral("failureInjection_result_0"), QStringLiteral("× Unsupported"), "after inject ack"),
             "Injected row result never resolved to Unsupported");

    // Reset all reverts the unit tracked by the injection above: a second row appears (GPS / OK).
    QVERIFY2(clickButton(QStringLiteral("failureInjection_resetAllButton")), "Failed to click Reset all");

    QVERIFY2(QTest::qWaitFor([&] { return activityList->property("count").toInt() == 2; }, 3000),
             "Activity row not added after reset");
    QVERIFY2(verifyText(QStringLiteral("failureInjection_unitName_0"), QStringLiteral("GPS"), "after reset"),
             "Reset row does not show unit GPS");
    QVERIFY2(verifyText(QStringLiteral("failureInjection_typeName_0"), QStringLiteral("OK"), "after reset"),
             "Reset row does not show type OK");
    QVERIFY2(verifyText(QStringLiteral("failureInjection_result_0"), QStringLiteral("× Unsupported"), "after reset ack"),
             "Reset row result never resolved to Unsupported");

    });
}
