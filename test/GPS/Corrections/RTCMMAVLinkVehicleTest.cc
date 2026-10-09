#include "RTCMMAVLinkVehicleTest.h"

#include <memory>

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QtEndian>

#include "Fixtures/RAIIFixtures.h"
#include "LinkManager.h"
#include "LogReplayLink.h"
#include "MAVLinkLib.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "QmlObjectListModel.h"
#include "RTCMMAVLink.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

void RTCMMAVLinkVehicleTest::_admissionFollowsLinkLifetime()
{
    auto* vehicle = createMockLinkAndWaitForVehicle();
    QVERIFY(vehicle);
    QTRY_VERIFY_WITH_TIMEOUT(vehicle->isInitialConnectComplete(), TestTimeout::mediumMs());
    auto output = RTCMMAVLink::vehicleLinkOutputs();
    const auto destinations = output();
    QCOMPARE(destinations.size(), 1);

    const GPSRTCMPacket packet{0, QByteArray(RTCMMAVLinkPacket::kFragmentLen, 'R')};
    QVERIFY(destinations.first()(packet));
    expectLogMessage("GPS.Corrections.RTCMMavlink", QtWarningMsg,
                     QRegularExpression(QStringLiteral("RTCM packet exceeds MAVLink payload limit")));
    QVERIFY(!destinations.first()({0, packet.data + 'R'}));
    verifyExpectedLogMessage();

    disconnectAllLinks();
    QVERIFY(output().isEmpty());
    QVERIFY(!destinations.first()(packet));

    vehicle = createMockLinkAndWaitForVehicle(QStringLiteral("Replacement"));
    QVERIFY(vehicle);
    QTRY_VERIFY_WITH_TIMEOUT(vehicle->isInitialConnectComplete(), TestTimeout::mediumMs());
    const auto replacement = output();
    QCOMPARE(replacement.size(), 1);
    QVERIFY(replacement.first()(packet));
    QVERIFY(!destinations.first()(packet));
}

void RTCMMAVLinkVehicleTest::_replayExcludedFromLiveAdmissions()
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

    const auto output = RTCMMAVLink::vehicleLinkOutputs();
    RTCMMAVLink sender;
    sender.setOutputProvider(output);
    const QByteArray payload(360, 'R');
    QVERIFY(output().isEmpty());
    sender.submitToOutputs(payload);
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
    const auto destinations = output();
    QCOMPARE(destinations.size(), 1);
    sender.submitToOutputs(payload);
    QCOMPARE(sender.totalBytesSubmitted(), quint64(payload.size()));

    replay->disconnect();
    QTRY_COMPARE_WITH_TIMEOUT(vehicles->vehicles()->count(), 1, TestTimeout::mediumMs());
    QCOMPARE(output().size(), 1);
    sender.submitToOutputs(payload);
    QCOMPARE(sender.totalBytesSubmitted(), quint64(2 * payload.size()));
}

UT_REGISTER_TEST(RTCMMAVLinkVehicleTest, TestLabel::Integration, TestLabel::Vehicle)
