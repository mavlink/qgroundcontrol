#include "SigningTest.h"
#include "MAVLinkSigning.h"

#include <QtCore/QCryptographicHash>
#include <QtTest/QTest>

void SigningTest::cleanup()
{
    // Clear signing on all channels to avoid test interference
    for (int i = 0; i < MAVLINK_COMM_NUM_BUFFERS; ++i) {
        MAVLinkSigning::initSigning(static_cast<mavlink_channel_t>(i), QByteArrayView(), nullptr);
    }
    OfflineTest::cleanup();
}

// ============================================================================
// initSigning Tests
// ============================================================================

void SigningTest::_testInitSigningWithKey()
{
    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_0, "test_key",
                                        MAVLinkSigning::insecureConnectionAccceptUnsignedCallback));

    const mavlink_status_t* status = mavlink_get_channel_status(MAVLINK_COMM_0);
    VERIFY_NOT_NULL(status);
    VERIFY_NOT_NULL(status->signing);
}

void SigningTest::_testInitSigningWithEmptyKey()
{
    // First initialize with a key
    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_0, "test_key",
                                        MAVLinkSigning::insecureConnectionAccceptUnsignedCallback));

    // Then disable signing with empty key
    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_0, QByteArrayView(), nullptr));

    const mavlink_status_t* status = mavlink_get_channel_status(MAVLINK_COMM_0);
    VERIFY_NOT_NULL(status);
    QVERIFY(status->signing == nullptr);
}

void SigningTest::_testInitSigningKeyHashing()
{
    const QString key = "secret_key";
    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_0, key.toUtf8(),
                                        MAVLinkSigning::insecureConnectionAccceptUnsignedCallback));

    const mavlink_status_t* status = mavlink_get_channel_status(MAVLINK_COMM_0);
    VERIFY_NOT_NULL(status);
    VERIFY_NOT_NULL(status->signing);

    const QByteArray expectedHash = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha256);
    QCOMPARE_EQ(memcmp(status->signing->secret_key, expectedHash.constData(),
                       sizeof(status->signing->secret_key)), 0);
}

void SigningTest::_testInitSigningWithByteArray()
{
    const QByteArray key(32, 'A');
    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_0, key,
                                        MAVLinkSigning::insecureConnectionAccceptUnsignedCallback));

    const mavlink_status_t* status = mavlink_get_channel_status(MAVLINK_COMM_0);
    VERIFY_NOT_NULL(status);
    VERIFY_NOT_NULL(status->signing);

    const QByteArray expectedHash = QCryptographicHash::hash(key, QCryptographicHash::Sha256);
    QCOMPARE_EQ(memcmp(status->signing->secret_key, expectedHash.constData(),
                       sizeof(status->signing->secret_key)), 0);
}

void SigningTest::_testInitSigningNoCallbackFails()
{
    // Key with no callback should fail
    QVERIFY(!MAVLinkSigning::initSigning(MAVLINK_COMM_0, "test_key", nullptr));
}

void SigningTest::_testInitSigningDifferentChannels()
{
    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_0, "key0",
                                        MAVLinkSigning::insecureConnectionAccceptUnsignedCallback));
    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_1, "key1",
                                        MAVLinkSigning::secureConnectionAccceptUnsignedCallback));

    const mavlink_status_t* status0 = mavlink_get_channel_status(MAVLINK_COMM_0);
    const mavlink_status_t* status1 = mavlink_get_channel_status(MAVLINK_COMM_1);

    VERIFY_NOT_NULL(status0->signing);
    VERIFY_NOT_NULL(status1->signing);

    // Different keys should produce different hashes
    QVERIFY(memcmp(status0->signing->secret_key, status1->signing->secret_key,
                   sizeof(status0->signing->secret_key)) != 0);
}

void SigningTest::_testInitSigningReinitialize()
{
    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_0, "first_key",
                                        MAVLinkSigning::insecureConnectionAccceptUnsignedCallback));

    const mavlink_status_t* status = mavlink_get_channel_status(MAVLINK_COMM_0);
    const QByteArray firstHash = QCryptographicHash::hash("first_key", QCryptographicHash::Sha256);
    QCOMPARE_EQ(memcmp(status->signing->secret_key, firstHash.constData(),
                       sizeof(status->signing->secret_key)), 0);

    // Reinitialize with different key
    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_0, "second_key",
                                        MAVLinkSigning::insecureConnectionAccceptUnsignedCallback));

    const QByteArray secondHash = QCryptographicHash::hash("second_key", QCryptographicHash::Sha256);
    QCOMPARE_EQ(memcmp(status->signing->secret_key, secondHash.constData(),
                       sizeof(status->signing->secret_key)), 0);
}

// ============================================================================
// Callback Tests
// ============================================================================

void SigningTest::_testSecureCallbackAlwaysTrue()
{
    // Secure callback accepts all unsigned messages
    QVERIFY(MAVLinkSigning::secureConnectionAccceptUnsignedCallback(nullptr, MAVLINK_MSG_ID_HEARTBEAT));
    QVERIFY(MAVLinkSigning::secureConnectionAccceptUnsignedCallback(nullptr, MAVLINK_MSG_ID_RADIO_STATUS));
    QVERIFY(MAVLinkSigning::secureConnectionAccceptUnsignedCallback(nullptr, MAVLINK_MSG_ID_COMMAND_LONG));
    QVERIFY(MAVLinkSigning::secureConnectionAccceptUnsignedCallback(nullptr, 0));
    QVERIFY(MAVLinkSigning::secureConnectionAccceptUnsignedCallback(nullptr, 65535));
}

void SigningTest::_testInsecureCallbackRadioStatus()
{
    // Insecure callback accepts RADIO_STATUS unsigned
    QVERIFY(MAVLinkSigning::insecureConnectionAccceptUnsignedCallback(nullptr, MAVLINK_MSG_ID_RADIO_STATUS));
}

void SigningTest::_testInsecureCallbackOtherMessages()
{
    // Insecure callback rejects other messages unsigned
    QVERIFY(!MAVLinkSigning::insecureConnectionAccceptUnsignedCallback(nullptr, MAVLINK_MSG_ID_HEARTBEAT));
    QVERIFY(!MAVLinkSigning::insecureConnectionAccceptUnsignedCallback(nullptr, MAVLINK_MSG_ID_COMMAND_LONG));
    QVERIFY(!MAVLinkSigning::insecureConnectionAccceptUnsignedCallback(nullptr, MAVLINK_MSG_ID_PARAM_VALUE));
    QVERIFY(!MAVLinkSigning::insecureConnectionAccceptUnsignedCallback(nullptr, 0));
}

// ============================================================================
// checkSigningLinkId Tests
// ============================================================================

void SigningTest::_testCheckSigningLinkIdValid()
{
    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_0, "test_key",
                                        MAVLinkSigning::insecureConnectionAccceptUnsignedCallback));

    mavlink_heartbeat_t heartbeat = {0};
    mavlink_message_t message;
    (void) mavlink_msg_heartbeat_encode_chan(1, MAV_COMP_ID_USER1, MAVLINK_COMM_0, &message, &heartbeat);

    QVERIFY(MAVLinkSigning::checkSigningLinkId(MAVLINK_COMM_0, message));
}

void SigningTest::_testCheckSigningLinkIdNoSigning()
{
    // Ensure signing is disabled
    MAVLinkSigning::initSigning(MAVLINK_COMM_0, QByteArrayView(), nullptr);

    mavlink_heartbeat_t heartbeat = {0};
    mavlink_message_t message;
    (void) mavlink_msg_heartbeat_encode_chan(1, MAV_COMP_ID_USER1, MAVLINK_COMM_0, &message, &heartbeat);

    // Should fail when signing not initialized
    QVERIFY(!MAVLinkSigning::checkSigningLinkId(MAVLINK_COMM_0, message));
}

// ============================================================================
// createSetupSigning Tests
// ============================================================================

void SigningTest::_testCreateSetupSigningTimestamp()
{
    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_0, "test_key",
                                        MAVLinkSigning::insecureConnectionAccceptUnsignedCallback));

    const mavlink_system_t target = {1, MAV_COMP_ID_AUTOPILOT1};
    mavlink_setup_signing_t setup;
    MAVLinkSigning::createSetupSigning(MAVLINK_COMM_0, target, setup);

    QCOMPARE_NE(setup.initial_timestamp, static_cast<uint64_t>(0));
}

void SigningTest::_testCreateSetupSigningTarget()
{
    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_0, "test_key",
                                        MAVLinkSigning::insecureConnectionAccceptUnsignedCallback));

    const mavlink_system_t target = {42, MAV_COMP_ID_CAMERA};
    mavlink_setup_signing_t setup;
    MAVLinkSigning::createSetupSigning(MAVLINK_COMM_0, target, setup);

    QCOMPARE_EQ(setup.target_system, static_cast<uint8_t>(42));
    QCOMPARE_EQ(setup.target_component, static_cast<uint8_t>(MAV_COMP_ID_CAMERA));
}

void SigningTest::_testCreateSetupSigningSecretKey()
{
    const QString key = "my_secret_key";
    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_0, key.toUtf8(),
                                        MAVLinkSigning::insecureConnectionAccceptUnsignedCallback));

    const mavlink_system_t target = {1, MAV_COMP_ID_AUTOPILOT1};
    mavlink_setup_signing_t setup;
    MAVLinkSigning::createSetupSigning(MAVLINK_COMM_0, target, setup);

    const QByteArray expectedHash = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha256);
    QCOMPARE_EQ(memcmp(setup.secret_key, expectedHash.constData(), sizeof(setup.secret_key)), 0);
}

void SigningTest::_testCreateSetupSigningNoSigning()
{
    // Disable signing
    MAVLinkSigning::initSigning(MAVLINK_COMM_0, QByteArrayView(), nullptr);

    const mavlink_system_t target = {1, MAV_COMP_ID_AUTOPILOT1};
    mavlink_setup_signing_t setup;
    MAVLinkSigning::createSetupSigning(MAVLINK_COMM_0, target, setup);

    // Target should still be set
    QCOMPARE_EQ(setup.target_system, static_cast<uint8_t>(1));
    QCOMPARE_EQ(setup.target_component, static_cast<uint8_t>(MAV_COMP_ID_AUTOPILOT1));
    // But timestamp should be 0 when no signing
    QCOMPARE_EQ(setup.initial_timestamp, static_cast<uint64_t>(0));
}

// ============================================================================
// Signing State Tests
// ============================================================================

void SigningTest::_testSigningFlags()
{
    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_0, "test_key",
                                        MAVLinkSigning::insecureConnectionAccceptUnsignedCallback));

    const mavlink_status_t* status = mavlink_get_channel_status(MAVLINK_COMM_0);
    VERIFY_NOT_NULL(status->signing);

    // Should have SIGN_OUTGOING flag set
    QVERIFY(status->signing->flags & MAVLINK_SIGNING_FLAG_SIGN_OUTGOING);
}

void SigningTest::_testSigningLinkId()
{
    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_0, "test_key",
                                        MAVLinkSigning::insecureConnectionAccceptUnsignedCallback));

    const mavlink_status_t* status = mavlink_get_channel_status(MAVLINK_COMM_0);
    VERIFY_NOT_NULL(status->signing);

    QCOMPARE_EQ(status->signing->link_id, static_cast<uint8_t>(MAVLINK_COMM_0));

    // Test another channel
    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_1, "test_key",
                                        MAVLinkSigning::insecureConnectionAccceptUnsignedCallback));

    const mavlink_status_t* status1 = mavlink_get_channel_status(MAVLINK_COMM_1);
    VERIFY_NOT_NULL(status1->signing);

    QCOMPARE_EQ(status1->signing->link_id, static_cast<uint8_t>(MAVLINK_COMM_1));
}
