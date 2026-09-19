#include "GPSMavlinkOutputTest.h"

#include <memory>

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QtEndian>

#include "Fixtures/RAIIFixtures.h"
#include "GPSMavlinkOutput.h"
#include "LinkManager.h"
#include "LogReplayLink.h"
#include "MAVLinkLib.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "QmlObjectListModel.h"
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

void GPSMavlinkOutputTest::_replayExcludedFromLiveAdmissions()
{
    QByteArray tlog;
    for (int index = 0; index < 3; ++index) {
        const quint64 timestamp = qToBigEndian<quint64>(1700000000000000ULL + index * 1000000ULL);
        tlog.append(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
        mavlink_message_t heartbeat{};
        // Heartbeat-only replay must not start the test connect sequence.
        mavlink_msg_heartbeat_pack(1, MAV_COMP_ID_AUTOPILOT1, &heartbeat, MAV_TYPE_GENERIC, MAV_AUTOPILOT_PX4, 0, 0,
                                   MAV_STATE_ACTIVE);
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN]{};
        const int size = mavlink_msg_to_send_buffer(buffer, &heartbeat);
        tlog.append(reinterpret_cast<const char*>(buffer), size);
    }
    TestFixtures::TempFileFixture logFile(QStringLiteral("correction-replay_XXXXXX.tlog"));
    QVERIFY(logFile.isValid());
    QVERIFY(logFile.write(tlog));
    logFile.file()->close();
    const auto cleanup = qScopeGuard([this]() {
        disconnectAllLinks();
        MAVLinkProtocol::instance()->suspendLogForReplay(false);
        linkManager()->setConnectionsAllowed();
    });

    auto replayConfig = std::make_shared<LogReplayConfiguration>(QStringLiteral("Correction replay"));
    replayConfig->setDynamic(true);
    replayConfig->setLogFilename(logFile.path());
    SharedLinkConfigurationPtr sharedConfig = replayConfig;
    QVERIFY(linkManager()->createConnectedLink(sharedConfig));
    const auto replay = linkManager()->sharedLinkInterfacePointerForLink(replayConfig->link());
    QVERIFY(replay);
    auto* replayLink = qobject_cast<LogReplayLink*>(replay.get());
    QVERIFY(replayLink);
    auto* replayVehicle = waitForVehicleConnect();
    QVERIFY(replayVehicle);
    QVERIFY(replay->isConnected());
    QVERIFY(replay->isLogReplay());
    QCOMPARE(replayVehicle->vehicleLinkManager()->primaryLink().lock(), replay);
    replayLink->pause();
    QTRY_VERIFY_WITH_TIMEOUT(!replayLink->isPlaying(), TestTimeout::mediumMs());

    GPSMavlinkOutput output;
    RTCMMavlink sender;
    sender.setOutputProvider([&output]() { return output.outputs(); });
    const QByteArray payload(360, 'R');
    QVERIFY(output.outputs().isEmpty());
    QVERIFY(sender.submitToOutputs(payload).isEmpty());
    QCOMPARE(sender.totalBytesSubmitted(), 0ULL);

    expectAppMessage(QRegularExpression(QStringLiteral("Connected to Vehicle [0-9]+")));
    const auto live = createMockLink(QStringLiteral("Live corrections"));
    QVERIFY(live);
    auto* mockLink = qobject_cast<MockLink*>(live.get());
    QVERIFY(mockLink);
    auto* vehicles = MultiVehicleManager::instance();
    QTRY_VERIFY_WITH_TIMEOUT(vehicles->getVehicleById(mockLink->vehicleId()), TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    auto* liveVehicle = vehicles->getVehicleById(mockLink->vehicleId());
    QVERIFY(liveVehicle);
    QVERIFY(liveVehicle != replayVehicle);
    QTRY_VERIFY_WITH_TIMEOUT(liveVehicle->isInitialConnectComplete(), TestTimeout::mediumMs());
    QCOMPARE(vehicles->vehicles()->count(), 2);
    QVERIFY(replay->isConnected());
    QCOMPARE(liveVehicle->vehicleLinkManager()->primaryLink().lock(), live);
    const auto destinations = output.outputs();
    QCOMPARE(destinations.size(), 1);
    const auto admissions = sender.submitToOutputs(payload);
    QCOMPARE(admissions.size(), 1);
    QCOMPARE(admissions.first().id, destinations.first().id);
    QCOMPARE(admissions.first().session, destinations.first().session);
    QCOMPARE(admissions.first().queuedBytes, quint64(payload.size()));
    QVERIFY(admissions.first().complete);
    QCOMPARE(sender.totalBytesSubmitted(), quint64(payload.size()));

    replay->disconnect();
    QTRY_COMPARE_WITH_TIMEOUT(vehicles->vehicles()->count(), 1, TestTimeout::mediumMs());
    const auto liveOnly = output.outputs();
    QCOMPARE(liveOnly.size(), 1);
    QCOMPARE(liveOnly.first().id, destinations.first().id);
    QCOMPARE(liveOnly.first().session, destinations.first().session);
    const auto liveAdmissions = sender.submitToOutputs(payload);
    QCOMPARE(liveAdmissions.size(), 1);
    QVERIFY(liveAdmissions.first().complete);
    QCOMPARE(liveAdmissions.first().queuedBytes, quint64(payload.size()));
    QCOMPARE(sender.totalBytesSubmitted(), quint64(2 * payload.size()));
}

UT_REGISTER_TEST(GPSMavlinkOutputTest, TestLabel::Integration, TestLabel::Vehicle)
