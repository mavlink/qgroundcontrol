#include "PX4VehicleConfigUITest.h"

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "MockLink.h"
#include "MultiVehicleManager.h"

#include <QtCore/QPointer>
#include "Vehicle.h"
#include "VehicleComponent.h"

UT_REGISTER_TEST(PX4VehicleConfigUITest, TestLabel::Integration)

void PX4VehicleConfigUITest::_testNavigateVehicleConfig()
{
    startUI();
    if (QTest::currentTestFailed()) return;

    // -------------------------------------------------------------------------
    // Connect PX4 MockLink and wait for parameters to be ready
    // -------------------------------------------------------------------------
    Vehicle *vehicle = nullptr;
    QPointer<MockLink> mockLink = connectMockLinkAndWaitReady(
        [] { return MockLink::startPX4MockLink(false, false, false); }, vehicle);
    if (!mockLink) return;

    // -------------------------------------------------------------------------
    // Navigate to the Configure view
    // -------------------------------------------------------------------------
    QVERIFY2(clickButton(QStringLiteral("toolbar_qgcLogo")), "Failed to click Q logo button");
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("toolbar_viewConfigure"), 2000),
             "toolbar_viewConfigure button not found");
    QVERIFY2(clickButton(QStringLiteral("toolbar_viewConfigure")), "Failed to click Configure button");
    QTest::qWait(_viewDelay);

    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("vehicleConfig_root"), 3000),
             "vehicleConfig_root not found after navigating to Configure");

    // -------------------------------------------------------------------------
    // Click Summary
    // -------------------------------------------------------------------------
    QVERIFY2(clickButton(QStringLiteral("vehicleConfig_summary")), "Failed to click Summary button");
    QTest::qWait(_viewDelay);

    // -------------------------------------------------------------------------
    // Click through each vehicle component
    // -------------------------------------------------------------------------
    const QVariantList components = vehicle->autopilotPlugin()->vehicleComponents();
    QVERIFY2(!components.isEmpty(), "No vehicle components found for PX4 MockLink");

    for (const QVariant &compVariant : components) {
        auto *comp = compVariant.value<VehicleComponent *>();
        if (!comp) {
            continue;
        }

        // Match the objectName set in VehicleConfigView.qml:
        // "vehicleConfig_comp_" + compName.replace(/ /g, "")
        const QString cleanName  = QString(comp->name()).remove(QLatin1Char(' '));
        const QString buttonName = QStringLiteral("vehicleConfig_comp_") + cleanName;

        QQuickItem *btn = findVisibleItem(_rootItem, buttonName, 2000);
        if (!btn) {
            // Component may be hidden (e.g. ESP8266 not present) – skip silently
            continue;
        }

        scrollIntoView(btn, QStringLiteral("vehicleConfig_sidebarFlickable"));

        const QPointF center = btn->mapToScene(QPointF(btn->width() / 2, btn->height() / 2));
        QTest::mouseClick(_window, Qt::LeftButton, Qt::NoModifier, center.toPoint());
        QTest::qWait(_pageDelay);

        QQuickItem *loader = findVisibleItem(_rootItem, QStringLiteral("vehicleConfig_panelLoader"), 2000);
        QVERIFY2(loader, qPrintable(QStringLiteral("vehicleConfig_panelLoader not found after clicking %1").arg(comp->name())));
        QVERIFY2(loader->property("item").value<QQuickItem *>() != nullptr,
                 qPrintable(QStringLiteral("Panel loader has no item after clicking %1").arg(comp->name())));
    }

    // -------------------------------------------------------------------------
    // Clean up: close the window before disconnecting the MockLink so that QML
    // bindings are torn down before the vehicle object is destroyed.
    // -------------------------------------------------------------------------
    closeUIWindow();

    if (mockLink) {
        QSignalSpy spyDisconnect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
        QVERIFY2(spyDisconnect.isValid(), "Failed to create spy for disconnect");
        mockLink->disconnect();
        (void)waitForSignal(spyDisconnect, 5000, QStringLiteral("activeVehicleChanged"));
    }

    destroyUIEngine();
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
    startUI();
    if (QTest::currentTestFailed()) return;

    // -------------------------------------------------------------------------
    // Connect PX4 MockLink and wait for parameters to be ready
    // -------------------------------------------------------------------------
    Vehicle *vehicle = nullptr;
    QPointer<MockLink> mockLink = connectMockLinkAndWaitReady(
        [] { return MockLink::startPX4MockLink(false, false, false); }, vehicle);
    if (!mockLink) return;

    // -------------------------------------------------------------------------
    // Navigate to Configure view and open the PID Tuning sidebar page
    // -------------------------------------------------------------------------
    QVERIFY2(clickButton(QStringLiteral("toolbar_qgcLogo")), "Failed to click Q logo button");
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("toolbar_viewConfigure"), 2000),
             "toolbar_viewConfigure button not found");
    QVERIFY2(clickButton(QStringLiteral("toolbar_viewConfigure")), "Failed to click Configure button");
    QTest::qWait(_viewDelay);

    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("vehicleConfig_root"), 3000),
             "vehicleConfig_root not found after navigating to Configure");

    // PX4TuningComponent::name() returns "PID Tuning" -> objectName "vehicleConfig_comp_PIDTuning"
    QQuickItem *tuningBtn = findVisibleItem(_rootItem, QStringLiteral("vehicleConfig_comp_PIDTuning"), 2000);
    QVERIFY2(tuningBtn, "vehicleConfig_comp_PIDTuning button not found");

    scrollIntoView(tuningBtn, QStringLiteral("vehicleConfig_sidebarFlickable"));
    const QPointF tuningBtnCenter = tuningBtn->mapToScene(QPointF(tuningBtn->width() / 2, tuningBtn->height() / 2));
    QTest::mouseClick(_window, Qt::LeftButton, Qt::NoModifier, tuningBtnCenter.toPoint());
    QTest::qWait(_pageDelay);

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

    closeUIWindow();

    if (mockLink) {
        QSignalSpy spyDisconnect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
        QVERIFY2(spyDisconnect.isValid(), "Failed to create spy for disconnect");
        mockLink->disconnect();
        (void)waitForSignal(spyDisconnect, 5000, QStringLiteral("activeVehicleChanged"));
    }

    destroyUIEngine();
}
