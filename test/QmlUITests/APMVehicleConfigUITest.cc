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
