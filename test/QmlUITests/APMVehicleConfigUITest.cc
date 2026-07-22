#include "APMVehicleConfigUITest.h"

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QTest>

#include "MockLink.h"
#include "Vehicle.h"

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
    navigateToConfigureView();
    if (QTest::currentTestFailed()) return;

    // -------------------------------------------------------------------------
    // Click Summary
    // -------------------------------------------------------------------------
    QVERIFY2(clickButton(QStringLiteral("vehicleConfig_summary")),
             qPrintable(QStringLiteral("%1: Failed to click Summary button").arg(vehicleName)));
    QTest::qWait(_viewDelay);

    // -------------------------------------------------------------------------
    // Click through each vehicle component
    // -------------------------------------------------------------------------
    clickThroughAllComponents(vehicle, vehicleName);

    });
}

// ---------------------------------------------------------------------------
// Per-vehicle-type test slots
// ---------------------------------------------------------------------------

void APMVehicleConfigUITest::_testArduCopter()
{
    ignoreAPMMockLinkWarnings();
    _runNavigateVehicleConfig(
        [] { return MockLink::startAPMArduCopterMockLink(); },
        QStringLiteral("ArduCopter"));
}

void APMVehicleConfigUITest::_testArduPlane()
{
    ignoreAPMMockLinkWarnings();
    _runNavigateVehicleConfig(
        [] { return MockLink::startAPMArduPlaneMockLink(); },
        QStringLiteral("ArduPlane"));
}

void APMVehicleConfigUITest::_testArduRover()
{
    ignoreAPMMockLinkWarnings();
    _runNavigateVehicleConfig(
        [] { return MockLink::startAPMArduRoverMockLink(); },
        QStringLiteral("ArduRover"));
}
