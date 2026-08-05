#include "FlyViewProximityRadarUITest.h"

#include <QtQuick/QQuickItem>
#include <QtTest/QTest>

#include "MockLink.h"

UT_REGISTER_TEST(FlyViewProximityRadarUITest, TestLabel::Integration)

void FlyViewProximityRadarUITest::_testRadarVisibleWithProximity()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(MockConfiguration::OptionEnableProximity); },
        [this](QPointer<MockLink> /*mockLink*/, Vehicle * /*vehicle*/) {
            // The radar becomes visible once DISTANCE_SENSOR telemetry arrives (1Hz task loop)
            // and the vehicle has a valid coordinate.
            QQuickItem *const radar = findVisibleItem(_rootItem, QStringLiteral("proximityRadarMapView"), 15000);
            QVERIFY2(radar, "Proximity radar map item never became visible");
        });
}

void FlyViewProximityRadarUITest::_testRadarHiddenWithoutProximity()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [this](QPointer<MockLink> /*mockLink*/, Vehicle * /*vehicle*/) {
            // Without OptionEnableProximity no distance sensor telemetry arrives, so the
            // radar item must stay invisible.
            QVERIFY2(!findVisibleItem(_rootItem, QStringLiteral("proximityRadarMapView"), 3000),
                     "Proximity radar map item visible without proximity telemetry");
        });
}
