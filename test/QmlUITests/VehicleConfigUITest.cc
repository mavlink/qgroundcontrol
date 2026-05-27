#include "VehicleConfigUITest.h"

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "MockConfiguration.h"
#include "MockLink.h"
#include "MultiVehicleManager.h"

#include <QtCore/QPointer>
#include "Vehicle.h"
#include "VehicleComponent.h"

UT_REGISTER_TEST(VehicleConfigUITest, TestLabel::Integration)

void VehicleConfigUITest::_testNavigateVehicleConfig()
{
    startUI();
    if (QTest::currentTestFailed()) return;

    // -------------------------------------------------------------------------
    // Connect PX4 MockLink and wait for parameters to be ready
    // -------------------------------------------------------------------------
    QSignalSpy spyVehicle(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    QVERIFY2(spyVehicle.isValid(), "Failed to create spy for activeVehicleChanged");

    QPointer<MockLink> mockLink = MockLink::startPX4MockLink(false /* sendStatusText */, false /* enableCamera */, false /* enableGimbal */);
    QVERIFY2(mockLink, "Failed to start PX4 MockLink");

    QVERIFY2(waitForSignal(spyVehicle, 10000, QStringLiteral("activeVehicleChanged")),
             "Timeout waiting for vehicle connection");

    Vehicle *vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY2(vehicle, "No active vehicle after MockLink connection");

    QSignalSpy spyConnect(vehicle, &Vehicle::initialConnectComplete);
    QVERIFY2(spyConnect.isValid(), "Failed to create spy for initialConnectComplete");
    if (!vehicle->isInitialConnectComplete()) {
        QVERIFY2(waitForSignal(spyConnect, 10000, QStringLiteral("initialConnectComplete")),
                 "Timeout waiting for initial connect");
    }
    QVERIFY2(vehicle->isInitialConnectComplete(), "Initial connect should be complete");

    // Wait for parameters to be fully ready so the Config page sidebar populates
    QSignalSpy spyParamsReady(MultiVehicleManager::instance(), &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    QVERIFY2(spyParamsReady.isValid(), "Failed to create spy for parameterReadyVehicleAvailableChanged");
    if (!MultiVehicleManager::instance()->property("parameterReadyVehicleAvailable").toBool()) {
        QVERIFY2(waitForSignal(spyParamsReady, 15000, QStringLiteral("parameterReadyVehicleAvailableChanged")),
                 "Timeout waiting for parameters to be ready");
    }
    QVERIFY2(MultiVehicleManager::instance()->property("parameterReadyVehicleAvailable").toBool(),
             "Parameters should be ready after signal");

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
