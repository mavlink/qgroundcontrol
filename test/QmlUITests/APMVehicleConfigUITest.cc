#include "APMVehicleConfigUITest.h"

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QTest>

#include "MockLink.h"
#include "Vehicle.h"

#include <QtCore/QPointer>

UT_REGISTER_TEST(APMVehicleConfigUITest, TestLabel::Integration)

void APMVehicleConfigUITest::init()
{
    if (!apmFirmwareSupported()) {
        QSKIP("ArduPilot support not registered in this build");
    }
    VehicleConfigUITestBase::init();
}

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
    // Click through each vehicle component, in English and Chinese
    // -------------------------------------------------------------------------
    clickThroughAllComponentsAllLocales(vehicle, vehicleName);

    });
}

// ---------------------------------------------------------------------------
// Per-vehicle-type test slots
// ---------------------------------------------------------------------------

void APMVehicleConfigUITest::_testArduCopter()
{
    _runNavigateVehicleConfig(
        [] { return MockLink::startAPMArduCopterMockLink(); },
        QStringLiteral("ArduCopter"));
}

void APMVehicleConfigUITest::_testArduPlane()
{
    _runNavigateVehicleConfig(
        [] { return MockLink::startAPMArduPlaneMockLink(); },
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
    _runNavigateVehicleConfig(
        [] { return MockLink::startAPMArduRoverMockLink(); },
        QStringLiteral("ArduRover"));
}

void APMVehicleConfigUITest::_testArduSubMotorSlidersEnabledWhenArmed()
{
    runWithMockLink([] { return MockLink::startAPMArduSubMockLink(); },
                    [&](QPointer<MockLink> /*mockLink*/, Vehicle* vehicle) {
                        navigateToConfigureView();
                        if (QTest::currentTestFailed())
                            return;
                        clickSidebarButton(QStringLiteral("vehicleConfig_comp_Motors"));
                        if (QTest::currentTestFailed())
                            return;

                        QQuickItem* sliders = findVisibleItem(_rootItem, QStringLiteral("apmSubMotor_sliders"), 3000);
                        QVERIFY2(sliders, "ArduSub motor sliders not found");
                        QVERIFY(!sliders->isEnabled());

                        vehicle->setArmed(true, false /* showError */);
                        QTRY_VERIFY_WITH_TIMEOUT(vehicle->armed(), 10000);
                        QTRY_VERIFY_WITH_TIMEOUT(sliders->isEnabled(), 5000);
                    });
}
