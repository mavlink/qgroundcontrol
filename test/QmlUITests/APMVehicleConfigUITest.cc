#include "APMVehicleConfigUITest.h"

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QTest>

#include "MockLink.h"
#include "Vehicle.h"
#include "VehicleComponent.h"

#include <QtCore/QPointer>

UT_REGISTER_TEST(APMVehicleConfigUITest, TestLabel::Integration)

// ---------------------------------------------------------------------------
// Shared implementation
// ---------------------------------------------------------------------------

void APMVehicleConfigUITest::_runNavigateVehicleConfig(
    const std::function<MockLink *()> &factory, const QString &vehicleName)
{
    runWithMockLink(factory, [&](QPointer<MockLink> /*mockLink*/, Vehicle *vehicle) {
    // -------------------------------------------------------------------------
    // Navigate to the Configure view
    // -------------------------------------------------------------------------
    QVERIFY2(clickButton(QStringLiteral("toolbar_qgcLogo")),
             qPrintable(QStringLiteral("%1: Failed to click Q logo button").arg(vehicleName)));
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("toolbar_viewConfigure"), 2000),
             qPrintable(QStringLiteral("%1: toolbar_viewConfigure button not found").arg(vehicleName)));
    QVERIFY2(clickButton(QStringLiteral("toolbar_viewConfigure")),
             qPrintable(QStringLiteral("%1: Failed to click Configure button").arg(vehicleName)));
    QTest::qWait(_viewDelay);

    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("vehicleConfig_root"), 3000),
             qPrintable(QStringLiteral("%1: vehicleConfig_root not found after navigating to Configure").arg(vehicleName)));

    // -------------------------------------------------------------------------
    // Click Summary
    // -------------------------------------------------------------------------
    QVERIFY2(clickButton(QStringLiteral("vehicleConfig_summary")),
             qPrintable(QStringLiteral("%1: Failed to click Summary button").arg(vehicleName)));
    QTest::qWait(_viewDelay);

    // -------------------------------------------------------------------------
    // Click through each vehicle component
    // -------------------------------------------------------------------------
    const QVariantList components = vehicle->autopilotPlugin()->vehicleComponents();
    QVERIFY2(!components.isEmpty(),
             qPrintable(QStringLiteral("%1: No vehicle components found").arg(vehicleName)));

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
            // Component may be hidden (e.g. optional peripheral not present) – skip silently
            continue;
        }

        scrollIntoView(btn, QStringLiteral("vehicleConfig_sidebarFlickable"));

        const QPointF center = btn->mapToScene(QPointF(btn->width() / 2, btn->height() / 2));
        QTest::mouseClick(_window, Qt::LeftButton, Qt::NoModifier, center.toPoint());
        QTest::qWait(_pageDelay);

        QQuickItem *loader = findVisibleItem(_rootItem, QStringLiteral("vehicleConfig_panelLoader"), 2000);
        QVERIFY2(loader,
                 qPrintable(QStringLiteral("%1: vehicleConfig_panelLoader not found after clicking %2")
                            .arg(vehicleName, comp->name())));
        QVERIFY2(loader->property("item").value<QQuickItem *>() != nullptr,
                 qPrintable(QStringLiteral("%1: Panel loader has no item after clicking %2")
                            .arg(vehicleName, comp->name())));
    }

    });
}

// ---------------------------------------------------------------------------
// Per-vehicle-type test slots
// ---------------------------------------------------------------------------

void APMVehicleConfigUITest::_testArduCopter()
{
    ignoreAPMMockLinkWarnings();
    _runNavigateVehicleConfig(
        [] { return MockLink::startAPMArduCopterMockLink(false, false, false); },
        QStringLiteral("ArduCopter"));
}

void APMVehicleConfigUITest::_testArduPlane()
{
    ignoreAPMMockLinkWarnings();
    _runNavigateVehicleConfig(
        [] { return MockLink::startAPMArduPlaneMockLink(false, false, false); },
        QStringLiteral("ArduPlane"));
}

void APMVehicleConfigUITest::_testArduSub()
{
    // TODO: APMTuningComponentSub.qml references parameters that don't exist in
    // the Sub MockLink, causing null-fact TypeErrors. Needs investigation.
    QSKIP("ArduSub Tuning page has parameter mismatches with MockLink – skipping pending fix");
}

void APMVehicleConfigUITest::_testArduRover()
{
    ignoreAPMMockLinkWarnings();
    _runNavigateVehicleConfig(
        [] { return MockLink::startAPMArduRoverMockLink(false, false, false); },
        QStringLiteral("ArduRover"));
}
