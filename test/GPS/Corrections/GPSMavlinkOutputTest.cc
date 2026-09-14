#include "GPSMavlinkOutputTest.h"

#include <memory>

#include <QtCore/QRegularExpression>

#include "GPSMavlinkOutput.h"
#include "LinkManager.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

void GPSMavlinkOutputTest::_admissionFollowsLinkLifetime()
{
    auto* vehicle = createMockLinkAndWaitForVehicle();
    QVERIFY(vehicle);
    QTRY_VERIFY_WITH_TIMEOUT(vehicle->isInitialConnectComplete(), TestTimeout::mediumMs());
    GPSMavlinkOutput output;
    const auto destinations = output.outputs();
    QCOMPARE(destinations.size(), 1);
    QVERIFY(!destinations.first().id.isEmpty());
    QVERIFY(destinations.first().session > 0);
    QCOMPARE(output.outputs().first().session, destinations.first().session);

    const GpsRtcmPacket packet{0, QByteArray(RTCMMavlinkPacket::kFragmentLen, 'R')};
    QVERIFY(destinations.first().submit(packet));
    expectLogMessage("GPS.Corrections.GPSMavlinkOutput", QtWarningMsg,
                     QRegularExpression(QStringLiteral("RTCM packet exceeds MAVLink payload limit")));
    QVERIFY(!destinations.first().submit({0, packet.data + 'R'}));
    verifyExpectedLogMessage();

    disconnectAllLinks();
    QVERIFY(output.outputs().isEmpty());
    QVERIFY(!destinations.first().submit(packet));

    vehicle = createMockLinkAndWaitForVehicle(QStringLiteral("Replacement"));
    QVERIFY(vehicle);
    QTRY_VERIFY_WITH_TIMEOUT(vehicle->isInitialConnectComplete(), TestTimeout::mediumMs());
    const auto replacement = output.outputs();
    QCOMPARE(replacement.size(), 1);
    QVERIFY(replacement.first().session != destinations.first().session);
    QVERIFY(replacement.first().id != destinations.first().id);
    QVERIFY(!destinations.first().submit(packet));
}

void GPSMavlinkOutputTest::_primarySwitchPreservesIdentity()
{
    const auto createLink = [this](const QString& name) -> SharedLinkInterfacePtr {
        auto config = std::make_shared<MockConfiguration>(name);
        config->setDynamic(true);
        config->setFirmwareType(MAV_AUTOPILOT_PX4);
        config->setIncrementVehicleId(false);
        SharedLinkConfigurationPtr sharedConfig = config;
        if (!linkManager()->createConnectedLink(sharedConfig)) {
            return {};
        }
        return linkManager()->sharedLinkInterfacePointerForLink(config->link());
    };
    const auto first = createLink(QStringLiteral("Correction A"));
    QVERIFY(first);
    const auto second = createLink(QStringLiteral("Correction B"));
    QVERIFY(second);
    auto* vehicle = waitForVehicleConnect();
    QVERIFY(vehicle);
    auto* links = vehicle->vehicleLinkManager();
    QTRY_VERIFY_WITH_TIMEOUT(links->containsLink(first.get()) && links->containsLink(second.get()),
                             TestTimeout::mediumMs());
    QTRY_VERIFY_WITH_TIMEOUT(vehicle->isInitialConnectComplete(), TestTimeout::mediumMs());
    GPSMavlinkOutput output;
    links->setPrimaryLinkByName(first->linkConfiguration()->name());
    const auto original = output.outputs();
    QCOMPARE(original.size(), 1);
    links->setPrimaryLinkByName(second->linkConfiguration()->name());
    const auto alternate = output.outputs();
    QCOMPARE(alternate.size(), 1);
    QVERIFY(alternate.first().id != original.first().id);
    QVERIFY(first->isConnected());
    links->setPrimaryLinkByName(first->linkConfiguration()->name());
    const auto restored = output.outputs();
    QCOMPARE(restored.size(), 1);
    QCOMPARE(restored.first().id, original.first().id);
    QCOMPARE(restored.first().session, original.first().session);
}

UT_REGISTER_TEST(GPSMavlinkOutputTest, TestLabel::Integration, TestLabel::Vehicle)
