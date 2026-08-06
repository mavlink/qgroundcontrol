#include "MAVLinkV1TrafficTest.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "LinkInterface.h"
#include "MAVLinkLib.h"
#include "MAVLinkProtocol.h"
#include "MockLink.h"

namespace {

/// Packs a MAVLink v1 framed message and returns the wire bytes.
template <typename PackFunc>
QByteArray packV1Message(int systemId, PackFunc packFunc)
{
    mavlink_status_t packStatus{};
    packStatus.flags |= MAVLINK_STATUS_FLAG_OUT_MAVLINK1;

    mavlink_message_t msg{};
    packFunc(static_cast<uint8_t>(systemId), MAV_COMP_ID_AUTOPILOT1, &packStatus, &msg);

    uint8_t buffer[MAVLINK_MAX_PACKET_LEN]{};
    const int cBuffer = mavlink_msg_to_send_buffer(buffer, &msg);
    return QByteArray(reinterpret_cast<char*>(buffer), cBuffer);
}

QByteArray v1RadioStatusBytes(int systemId)
{
    return packV1Message(systemId, [](uint8_t sysId, uint8_t compId, mavlink_status_t* status, mavlink_message_t* msg) {
        (void) mavlink_msg_radio_status_pack_status(sysId, compId, status, msg, 100, 100, 50, 10, 10, 0, 0);
    });
}

QByteArray v1VibrationBytes(int systemId)
{
    return packV1Message(systemId, [](uint8_t sysId, uint8_t compId, mavlink_status_t* status, mavlink_message_t* msg) {
        (void) mavlink_msg_vibration_pack_status(sysId, compId, status, msg, 0, 0.1f, 0.1f, 0.1f, 0, 0, 0);
    });
}

} // namespace

void MAVLinkV1TrafficTest::cleanup()
{
    LinkInterface::setMavlinkV1TrafficGraceMsecs(LinkInterface::kMavlinkV1TrafficGraceMsecsDefault);
    VehicleTestManualConnect::cleanup();
}

void MAVLinkV1TrafficTest::_testV1UpgradeToV2NoWarning()
{
    // MockLink starts out sending MAVLink v1 just like a real ArduPilot. The initial connect
    // sequence can only complete if the v1 -> v2 upgrade handshake works.
    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);

    const mavlink_status_t* const outgoingStatus = mavlink_get_channel_status(_mockLink->outgoingMavlinkChannel());
    QVERIFY2(!(outgoingStatus->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1), "MockLink should have upgraded outgoing traffic to MAVLink v2");

    // The early v1 traffic must not have triggered the v1-only warning.
    // Strict log capture additionally fails this test if the warning/dialog fired.
    QVERIFY(!_mockLink->mavlinkV1TrafficReported());
}

void MAVLinkV1TrafficTest::_testV1OnlyVehicleWarns()
{
    LinkInterface::setMavlinkV1TrafficGraceMsecs(250);

    expectLogMessage("Comms.LinkInterface", QtWarningMsg, QRegularExpression(QStringLiteral("MAVLink v1 traffic detected")));
    expectAppMessage(QRegularExpression(QStringLiteral("only supports MAVLink v2")));

    // Vehicle which never upgrades to v2: MockLink streams telemetry at 1Hz, so once the grace
    // period expires the next v1 message must trigger the warning.
    _connectMockLinkNoInitialConnectSequence(MockConfiguration::OptionStayMavlinkV1);

    QTRY_VERIFY_WITH_TIMEOUT(_mockLink->mavlinkV1TrafficReported(), TestTimeout::longMs());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
}

void MAVLinkV1TrafficTest::_testV1RadioStatusDoesNotWarn()
{
    _connectMockLinkNoInitialConnectSequence(MockConfiguration::OptionStayMavlinkV1);

    // Silence MockLink's own streams so the injected messages are the only v1 traffic.
    _mockLink->setCommLost(true);

    // Grace period must only be zeroed after the connect above: while connecting, MockLink's own
    // v1 streams are still running and must not trigger the warning (default grace covers them).
    LinkInterface::setMavlinkV1TrafficGraceMsecs(0);

    // SiK radios frame RADIO_STATUS as v1 forever — it must never trigger the warning.
    MAVLinkProtocol* const protocol = MAVLinkProtocol::instance();
    for (int i = 0; i < 2; i++) {
        protocol->receiveBytes(_mockLink, v1RadioStatusBytes(_mockLink->vehicleId()));
    }
    QVERIFY2(!_mockLink->mavlinkV1TrafficReported(), "v1 RADIO_STATUS must not trigger the v1-only warning");

    // Positive control: a non-exempt v1 message does trigger the warning once the grace period (0) expired.
    expectLogMessage("Comms.LinkInterface", QtWarningMsg, QRegularExpression(QStringLiteral("MAVLink v1 traffic detected")));
    expectAppMessage(QRegularExpression(QStringLiteral("only supports MAVLink v2")));
    for (int i = 0; i < 2; i++) {
        protocol->receiveBytes(_mockLink, v1VibrationBytes(_mockLink->vehicleId()));
    }
    QVERIFY(_mockLink->mavlinkV1TrafficReported());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
}

UT_REGISTER_TEST(MAVLinkV1TrafficTest, TestLabel::Integration, TestLabel::Comms)
