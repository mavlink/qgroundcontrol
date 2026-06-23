#include "PX4VehicleConfigUITest.h"

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QTest>

#include "MockLink.h"

#include <QtCore/QPointer>
#include "Vehicle.h"

UT_REGISTER_TEST(PX4VehicleConfigUITest, TestLabel::Integration)

void PX4VehicleConfigUITest::_testNavigateVehicleConfig()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(false, false, false); },
        [&](QPointer<MockLink> /*mockLink*/, Vehicle *vehicle) {
    navigateToConfigureView();
    if (QTest::currentTestFailed()) return;

    // -------------------------------------------------------------------------
    // Click Summary
    // -------------------------------------------------------------------------
    QVERIFY2(clickButton(QStringLiteral("vehicleConfig_summary")), "Failed to click Summary button");
    QTest::qWait(_viewDelay);

    // -------------------------------------------------------------------------
    // Click through each vehicle component
    // -------------------------------------------------------------------------
    clickThroughAllComponents(vehicle);

    });
}

// Cycle through the axis buttons on a PID tuning tab. Each click exercises
// the LineSeries remove/re-add path on GraphsView; qWait lets the polish pass run.
void PX4VehicleConfigUITest::_cycleAxisButtons(const QStringList &axisNames)
{
    for (const QString &name : axisNames) {
        const QString objectName = QStringLiteral("pidTuning_axisButton_") + name;
        QVERIFY2(findVisibleItem(_rootItem, objectName, 2000),
                 qPrintable(QStringLiteral("Axis button not found: %1").arg(objectName)));
        QVERIFY2(clickButton(objectName),
                 qPrintable(QStringLiteral("Failed to click axis button: %1").arg(objectName)));
        QTest::qWait(_pageDelay > 0 ? _pageDelay : 200); // let the polish pass run
    }
}

void PX4VehicleConfigUITest::_testDisconnectWithPIDTuningOpen()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(false, false, false); },
        [&](QPointer<MockLink> /*mockLink*/, Vehicle * /*vehicle*/) {
    // -------------------------------------------------------------------------
    // Navigate to Configure view and open the PID Tuning sidebar page
    // -------------------------------------------------------------------------
    navigateToConfigureView();
    if (QTest::currentTestFailed()) return;

    // PX4TuningComponent::name() returns "PID Tuning" -> objectName "vehicleConfig_comp_PIDTuning"
    clickSidebarButton(QStringLiteral("vehicleConfig_comp_PIDTuning"));
    if (QTest::currentTestFailed()) return;

    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("vehicleConfig_panelLoader"), 2000),
             "vehicleConfig_panelLoader not found after opening PID Tuning");

    // -------------------------------------------------------------------------
    // Rate Controller: Roll -> Pitch -> Yaw -> Roll
    // -------------------------------------------------------------------------
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("pidTuning_tab_RateController"), 2000),
             "Rate Controller tab not found");
    QVERIFY2(clickButton(QStringLiteral("pidTuning_tab_RateController")), "Failed to click Rate Controller tab");
    QTest::qWait(_pageDelay);
    _cycleAxisButtons({QStringLiteral("Roll"), QStringLiteral("Pitch"), QStringLiteral("Yaw"), QStringLiteral("Roll")});
    if (QTest::currentTestFailed()) return;

    // -------------------------------------------------------------------------
    // Attitude Controller: Roll -> Pitch -> Yaw -> Roll
    // -------------------------------------------------------------------------
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("pidTuning_tab_AttitudeController"), 2000),
             "Attitude Controller tab not found");
    QVERIFY2(clickButton(QStringLiteral("pidTuning_tab_AttitudeController")), "Failed to click Attitude Controller tab");
    QTest::qWait(_pageDelay);
    _cycleAxisButtons({QStringLiteral("Roll"), QStringLiteral("Pitch"), QStringLiteral("Yaw"), QStringLiteral("Roll")});
    if (QTest::currentTestFailed()) return;

    // -------------------------------------------------------------------------
    // Velocity Controller: Horizontal -> Vertical -> Horizontal
    // -------------------------------------------------------------------------
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("pidTuning_tab_VelocityController"), 2000),
             "Velocity Controller tab not found");
    QVERIFY2(clickButton(QStringLiteral("pidTuning_tab_VelocityController")), "Failed to click Velocity Controller tab");
    QTest::qWait(_pageDelay);
    _cycleAxisButtons({QStringLiteral("Horizontal"), QStringLiteral("Vertical"), QStringLiteral("Horizontal")});
    if (QTest::currentTestFailed()) return;

    // -------------------------------------------------------------------------
    // Position Controller: Horizontal -> Vertical -> Horizontal
    // -------------------------------------------------------------------------
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("pidTuning_tab_PositionController"), 2000),
             "Position Controller tab not found");
    QVERIFY2(clickButton(QStringLiteral("pidTuning_tab_PositionController")), "Failed to click Position Controller tab");
    QTest::qWait(_pageDelay);
    _cycleAxisButtons({QStringLiteral("Horizontal"), QStringLiteral("Vertical"), QStringLiteral("Horizontal")});
    if (QTest::currentTestFailed()) return;

    });
}
