#include "SigningTest.h"

#include "MAVLinkSigning.h"

void SigningTest::_testInitSigning()
{
    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_0, "secret_key",
                                        MAVLinkSigning::insecureConnectionAccceptUnsignedCallback));
    const mavlink_status_t* status = mavlink_get_channel_status(MAVLINK_COMM_0);
    const mavlink_signing_t* signing = status->signing;
    QVERIFY(memcmp(signing->secret_key, QCryptographicHash::hash("secret_key", QCryptographicHash::Sha256).constData(),
                   sizeof(signing->secret_key)) == 0);
    QByteArray keyData(32, '\0');
    keyData[0] = '1';
    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_0, keyData,
                                        MAVLinkSigning::insecureConnectionAccceptUnsignedCallback));
    QVERIFY(memcmp(signing->secret_key,
                   QCryptographicHash::hash(keyData, QCryptographicHash::Sha256).constData(),
                   sizeof(signing->secret_key)) == 0);
    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_0, QByteArrayView(),
                                        MAVLinkSigning::insecureConnectionAccceptUnsignedCallback));
}

void SigningTest::_testCheckSigningLinkId()
{
    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_0, "secret_key",
                                        MAVLinkSigning::insecureConnectionAccceptUnsignedCallback));
    const mavlink_heartbeat_t heartbeat{};
    mavlink_message_t message;
    (void)mavlink_msg_heartbeat_encode_chan(1, MAV_COMP_ID_USER1, MAVLINK_COMM_0, &message, &heartbeat);
    QVERIFY(MAVLinkSigning::checkSigningLinkId(MAVLINK_COMM_0, message));
}

void SigningTest::_testCreateSetupSigning()
{
    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_0, "secret_key",
                                        MAVLinkSigning::insecureConnectionAccceptUnsignedCallback));
    mavlink_system_t target_system{};
    target_system.sysid = 1;
    target_system.compid = MAV_COMP_ID_AUTOPILOT1;
    mavlink_setup_signing_t setup_signing;
    MAVLinkSigning::createSetupSigning(MAVLINK_COMM_0, target_system, setup_signing);
    QVERIFY(setup_signing.initial_timestamp != 0);
    QCOMPARE(setup_signing.target_system, target_system.sysid);
    QCOMPARE(setup_signing.target_component, target_system.compid);
}

UT_REGISTER_TEST(SigningTest, TestLabel::Unit)
