#include "SigningTest.h"

#include <QtCore/QSettings>
#include <QtCore/QRegularExpression>
#include <QtTest/QTest>

#include "MAVLinkLib.h"
#include "MAVLinkSigning.h"
#include "MAVLinkSigningKeys.h"
#include "QmlObjectListModel.h"
#include "SigningChannel.h"
#include "SigningController.h"

void SigningTest::initTestCase()
{
    UnitTest::initTestCase();
    // Production PBKDF2 cost (~250ms/key) would dominate test runtime; lower for the suite.
    MAVLinkSigningKeys::setPbkdf2IterationsForTesting(1);
}

void SigningTest::_testInitSigning()
{
    SigningChannel ch;
    QByteArray rawKey(32, '\0');
    rawKey[0] = 'A';
    rawKey[1] = 'B';

    QVERIFY(ch.init(MAVLINK_COMM_0, rawKey, MAVLinkSigning::insecureConnectionAcceptUnsignedCallback));
    const mavlink_status_t* status = mavlink_get_channel_status(MAVLINK_COMM_0);
    const mavlink_signing_t* signing = status->signing;
    QVERIFY(signing);
    QVERIFY(memcmp(signing->secret_key, rawKey.constData(), sizeof(signing->secret_key)) == 0);

    QVERIFY(ch.init(MAVLINK_COMM_0, QByteArrayView(), nullptr));
    QVERIFY(!mavlink_get_channel_status(MAVLINK_COMM_0)->signing);
}

void SigningTest::_testCheckSigningLinkId()
{
    SigningChannel ch;
    QByteArray rawKey(32, '\x01');
    QVERIFY(ch.init(MAVLINK_COMM_0, rawKey, MAVLinkSigning::insecureConnectionAcceptUnsignedCallback));
    const mavlink_heartbeat_t heartbeat{};
    mavlink_message_t message;
    (void)mavlink_msg_heartbeat_encode_chan(1, MAV_COMP_ID_USER1, MAVLINK_COMM_0, &message, &heartbeat);
    QVERIFY(MAVLinkSigning::checkSigningLinkId(MAVLINK_COMM_0, message));
    QVERIFY(ch.init(MAVLINK_COMM_0, QByteArrayView(), nullptr));
}

void SigningTest::_testCreateSetupSigning()
{
    SigningChannel ch;
    QByteArray rawKey(32, '\x02');
    QVERIFY(ch.init(MAVLINK_COMM_0, rawKey, MAVLinkSigning::insecureConnectionAcceptUnsignedCallback));
    mavlink_system_t target_system{};
    target_system.sysid = 1;
    target_system.compid = MAV_COMP_ID_AUTOPILOT1;
    mavlink_setup_signing_t setup_signing;
    MAVLinkSigning::createSetupSigning(MAVLINK_COMM_0, target_system, rawKey, setup_signing);
    QVERIFY(setup_signing.initial_timestamp != 0);
    QCOMPARE(setup_signing.target_system, target_system.sysid);
    QCOMPARE(setup_signing.target_component, target_system.compid);
    QVERIFY(memcmp(setup_signing.secret_key, rawKey.constData(), sizeof(setup_signing.secret_key)) == 0);
    QVERIFY(ch.init(MAVLINK_COMM_0, QByteArrayView(), nullptr));
}

void SigningTest::_testCreateDisableSigning()
{
    mavlink_system_t target{};
    target.sysid = 42;
    target.compid = MAV_COMP_ID_AUTOPILOT1;

    mavlink_setup_signing_t setup{};
    (void)memset(&setup, 0xFF, sizeof(setup));
    MAVLinkSigning::createSetupSigning(MAVLINK_COMM_0, target, QByteArrayView{}, setup);

    QCOMPARE(setup.target_system, static_cast<uint8_t>(42));
    QCOMPARE(setup.target_component, static_cast<uint8_t>(MAV_COMP_ID_AUTOPILOT1));
    QCOMPARE(setup.initial_timestamp, static_cast<uint64_t>(0));

    const QByteArray zeroKey(sizeof(setup.secret_key), '\0');
    QVERIFY(memcmp(setup.secret_key, zeroKey.constData(), sizeof(setup.secret_key)) == 0);
}

void SigningTest::_testAcceptUnsignedCallbacks()
{
    QVERIFY(MAVLinkSigning::secureConnectionAcceptUnsignedCallback(nullptr, MAVLINK_MSG_ID_RADIO_STATUS));
    QVERIFY(!MAVLinkSigning::secureConnectionAcceptUnsignedCallback(nullptr, MAVLINK_MSG_ID_HEARTBEAT));
    QVERIFY(!MAVLinkSigning::secureConnectionAcceptUnsignedCallback(nullptr, MAVLINK_MSG_ID_STATUSTEXT));
    QVERIFY(!MAVLinkSigning::secureConnectionAcceptUnsignedCallback(nullptr, MAVLINK_MSG_ID_COMMAND_LONG));
    QVERIFY(!MAVLinkSigning::secureConnectionAcceptUnsignedCallback(nullptr, MAVLINK_MSG_ID_PARAM_VALUE));

    QVERIFY(MAVLinkSigning::insecureConnectionAcceptUnsignedCallback(nullptr, MAVLINK_MSG_ID_RADIO_STATUS));
    QVERIFY(MAVLinkSigning::insecureConnectionAcceptUnsignedCallback(nullptr, MAVLINK_MSG_ID_HEARTBEAT));
    QVERIFY(MAVLinkSigning::insecureConnectionAcceptUnsignedCallback(nullptr, MAVLINK_MSG_ID_COMMAND_LONG));
    QVERIFY(MAVLinkSigning::insecureConnectionAcceptUnsignedCallback(nullptr, 99999));

    const auto pending = MAVLinkSigning::callbackForPolicy(MAVLinkSigning::UnsignedAcceptancePolicy::Pending);
    QVERIFY(pending(nullptr, MAVLINK_MSG_ID_RADIO_STATUS));
    QVERIFY(pending(nullptr, MAVLINK_MSG_ID_HEARTBEAT));
    QVERIFY(pending(nullptr, MAVLINK_MSG_ID_STATUSTEXT));
    QVERIFY(!pending(nullptr, MAVLINK_MSG_ID_COMMAND_LONG));
    QVERIFY(!pending(nullptr, MAVLINK_MSG_ID_PARAM_VALUE));
    QVERIFY(!pending(nullptr, MAVLINK_MSG_ID_ATTITUDE));
}

void SigningTest::_testSetAcceptUnsignedCallback()
{
    SigningChannel ch;
    QVERIFY(ch.init(MAVLINK_COMM_0, QByteArrayView(), nullptr));
    QVERIFY(!ch.isEnabled());
    QVERIFY(!ch.setAcceptUnsignedCallback(MAVLinkSigning::secureConnectionAcceptUnsignedCallback));

    QByteArray rawKey(32, '\x10');
    QVERIFY(ch.init(MAVLINK_COMM_0, rawKey, MAVLinkSigning::insecureConnectionAcceptUnsignedCallback));
    QVERIFY(ch.isEnabled());

    QVERIFY(ch.setAcceptUnsignedCallback(MAVLinkSigning::secureConnectionAcceptUnsignedCallback));

    const mavlink_status_t* status = mavlink_get_channel_status(MAVLINK_COMM_0);
    QVERIFY(status && status->signing);
    QVERIFY(status->signing->accept_unsigned_callback == MAVLinkSigning::secureConnectionAcceptUnsignedCallback);

    QVERIFY(ch.init(MAVLINK_COMM_0, QByteArrayView(), nullptr));
}

void SigningTest::_testVerifySignature()
{
    QByteArray rawKey(32, '\0');
    for (int i = 0; i < 32; ++i) {
        rawKey[i] = static_cast<char>(i);
    }

    SigningChannel ch;
    QVERIFY(ch.init(MAVLINK_COMM_1, rawKey, MAVLinkSigning::insecureConnectionAcceptUnsignedCallback));

    const mavlink_heartbeat_t heartbeat{};
    mavlink_message_t message;
    (void)mavlink_msg_heartbeat_encode_chan(1, MAV_COMP_ID_AUTOPILOT1, MAVLINK_COMM_1, &message, &heartbeat);

    QVERIFY(MAVLinkSigning::isMessageSigned(message));

    QVERIFY(MAVLinkSigning::verifySignature(rawKey, message));

    QByteArray wrongKey(32, '\xFF');
    QVERIFY(!MAVLinkSigning::verifySignature(wrongKey, message));

    QByteArray shortKey(16, '\x01');
    QVERIFY(!MAVLinkSigning::verifySignature(shortKey, message));

    QVERIFY(ch.init(MAVLINK_COMM_1, QByteArrayView(), nullptr));
}

void SigningTest::_testTryDetectKey()
{
    auto* signingKeys = MAVLinkSigningKeys::instance();

    signingKeys->removeAllKeys();

    signingKeys->addKey(QStringLiteral("KeyAlpha"), QStringLiteral("passphrase-alpha"));
    signingKeys->addKey(QStringLiteral("KeyBravo"), QStringLiteral("passphrase-bravo"));
    signingKeys->addKey(QStringLiteral("KeyCharlie"), QStringLiteral("passphrase-charlie"));
    QCOMPARE(signingKeys->keys()->count(), 3);

    const auto* bravoEntry = signingKeys->keyAt(1);
    QVERIFY(bravoEntry);
    const auto& bravoKey = bravoEntry->keyBytes();

    {
        SigningChannel ch;
        QVERIFY(ch.init(MAVLINK_COMM_2, QByteArrayView(reinterpret_cast<const char*>(bravoKey.data()), bravoKey.size()),
                        MAVLinkSigning::insecureConnectionAcceptUnsignedCallback));
        const mavlink_heartbeat_t heartbeat{};
        mavlink_message_t message;
        (void)mavlink_msg_heartbeat_encode_chan(1, MAV_COMP_ID_AUTOPILOT1, MAVLINK_COMM_2, &message, &heartbeat);
        QVERIFY(MAVLinkSigning::isMessageSigned(message));
        QVERIFY(ch.init(MAVLINK_COMM_2, QByteArrayView(), nullptr));

        SigningController ctrl(MAVLINK_COMM_3);
        QVERIFY(!ctrl.isEnabled());
        const QString detected = MAVLinkSigningKeys::instance()->tryDetectKey(&ctrl, message);
        QCOMPARE(detected, QStringLiteral("KeyBravo"));
        QVERIFY(ctrl.isEnabled());

        const QString again = MAVLinkSigningKeys::instance()->tryDetectKey(&ctrl, message);
        QVERIFY(again.isEmpty());
    }

    signingKeys->removeAllKeys();
}

void SigningTest::_testTryDetectKeyUnsignedMessage()
{
    auto* signingKeys = MAVLinkSigningKeys::instance();
    signingKeys->removeAllKeys();
    signingKeys->addKey(QStringLiteral("TestKey"), QStringLiteral("passphrase"));

    SigningController ctrl(MAVLINK_COMM_0);
    const mavlink_heartbeat_t heartbeat{};
    mavlink_message_t message;
    (void)mavlink_msg_heartbeat_encode_chan(1, MAV_COMP_ID_AUTOPILOT1, MAVLINK_COMM_0, &message, &heartbeat);

    QVERIFY(!MAVLinkSigning::isMessageSigned(message));

    const QString result = MAVLinkSigningKeys::instance()->tryDetectKey(&ctrl, message);
    QVERIFY(result.isEmpty());
    QVERIFY(!ctrl.isEnabled());

    signingKeys->removeAllKeys();
}

void SigningTest::_testTryDetectKeyHintCache()
{
    auto* signingKeys = MAVLinkSigningKeys::instance();
    signingKeys->removeAllKeys();

    signingKeys->addKey(QStringLiteral("KeyAlpha"), QStringLiteral("alphapass"));
    signingKeys->addKey(QStringLiteral("KeyBravo"), QStringLiteral("bravopass"));

    const auto& bravoKey = signingKeys->keyAt(1)->keyBytes();
    const QByteArrayView bravoKV(reinterpret_cast<const char*>(bravoKey.data()), bravoKey.size());

    mavlink_message_t message;
    {
        SigningChannel ch;
        QVERIFY(ch.init(MAVLINK_COMM_2, bravoKV, MAVLinkSigning::insecureConnectionAcceptUnsignedCallback));
        const mavlink_heartbeat_t heartbeat{};
        (void)mavlink_msg_heartbeat_encode_chan(1, MAV_COMP_ID_AUTOPILOT1, MAVLINK_COMM_2, &message, &heartbeat);
        QVERIFY(MAVLinkSigning::isMessageSigned(message));
        QVERIFY(ch.init(MAVLINK_COMM_2, QByteArrayView(), nullptr));
    }

    constexpr auto kTestChannel = static_cast<mavlink_channel_t>(4);

    {
        SigningController ctrl(kTestChannel);
        QVERIFY(!ctrl.isEnabled());
        const QString detected1 = MAVLinkSigningKeys::instance()->tryDetectKey(&ctrl, message);
        QCOMPARE(detected1, QStringLiteral("KeyBravo"));
        QVERIFY(ctrl.isEnabled());

        ctrl.clearSigning();
        QVERIFY(!ctrl.isEnabled());

        const QString detected2 = MAVLinkSigningKeys::instance()->tryDetectKey(&ctrl, message);
        QCOMPARE(detected2, QStringLiteral("KeyBravo"));
        QVERIFY(ctrl.isEnabled());
    }

    signingKeys->removeAllKeys();
}

void SigningTest::_testAddRawKey()
{
    auto* signingKeys = MAVLinkSigningKeys::instance();

    signingKeys->removeAllKeys();

    const QString hexKey = QStringLiteral("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
    signingKeys->addRawKey(QStringLiteral("RawKey"), hexKey);
    QCOMPARE(signingKeys->keys()->count(), 1);
    QCOMPARE(signingKeys->keyAt(0)->name(), QStringLiteral("RawKey"));
    QCOMPARE(static_cast<int>(signingKeys->keyAt(0)->keyBytes().size()), 32);
    const auto toQBA = [](const MAVLinkSigning::SigningKey& k) {
        return QByteArray(reinterpret_cast<const char*>(k.data()), static_cast<qsizetype>(k.size()));
    };
    QCOMPARE(toQBA(signingKeys->keyAt(0)->keyBytes()), QByteArray::fromHex(hexKey.toLatin1()));

    expectLogMessage("MAVLink.SigningKeys", QtWarningMsg, QRegularExpression("Key with name already exists:"));
    signingKeys->addRawKey(QStringLiteral("RawKey"), hexKey);
    verifyExpectedLogMessage();
    QCOMPARE(signingKeys->keys()->count(), 1);

    expectLogMessage("MAVLink.SigningKeys", QtWarningMsg, QRegularExpression("Raw key must be exactly 32 bytes"));
    signingKeys->addRawKey(QStringLiteral("ShortKey"), QStringLiteral("0102030405"));
    verifyExpectedLogMessage();
    QCOMPARE(signingKeys->keys()->count(), 1);

    expectLogMessage("MAVLink.SigningKeys", QtWarningMsg, QRegularExpression("Raw key must be exactly 32 bytes"));
    signingKeys->addRawKey(QStringLiteral("LongKey"), hexKey + QStringLiteral("ff"));
    verifyExpectedLogMessage();
    QCOMPARE(signingKeys->keys()->count(), 1);

    expectLogMessage("MAVLink.SigningKeys", QtWarningMsg, QRegularExpression("Key name must not be empty"));
    signingKeys->addRawKey(QString(), hexKey);
    verifyExpectedLogMessage();
    QCOMPARE(signingKeys->keys()->count(), 1);
    expectLogMessage("MAVLink.SigningKeys", QtWarningMsg, QRegularExpression("Hex key must not be empty"));
    signingKeys->addRawKey(QStringLiteral("EmptyHex"), QString());
    verifyExpectedLogMessage();
    QCOMPARE(signingKeys->keys()->count(), 1);

    const MAVLinkSigning::SigningKey expectedBytes = signingKeys->keyAt(0)->keyBytes();
    signingKeys->_load();
    QCOMPARE(signingKeys->keys()->count(), 1);
    QCOMPARE(signingKeys->keyAt(0)->name(), QStringLiteral("RawKey"));
    QVERIFY(signingKeys->keyAt(0)->keyBytes() == expectedBytes);

    QCOMPARE(signingKeys->keyHexByName(QStringLiteral("RawKey")), hexKey);
    QVERIFY(signingKeys->keyHexByName(QStringLiteral("NonExistent")).isEmpty());

    signingKeys->removeAllKeys();
}

void SigningTest::_testMaxKeyLimit()
{
    auto* signingKeys = MAVLinkSigningKeys::instance();

    signingKeys->removeAllKeys();

    for (int i = 0; i < MAVLinkSigningKeys::kMaxKeys; ++i) {
        signingKeys->addKey(QStringLiteral("Key%1").arg(i), QStringLiteral("passphrase%1").arg(i));
    }
    QCOMPARE(signingKeys->keys()->count(), MAVLinkSigningKeys::kMaxKeys);

    expectLogMessage("MAVLink.SigningKeys", QtWarningMsg, QRegularExpression("Maximum key count reached:"));
    signingKeys->addKey(QStringLiteral("Overflow"), QStringLiteral("overflow"));
    verifyExpectedLogMessage();
    QCOMPARE(signingKeys->keys()->count(), MAVLinkSigningKeys::kMaxKeys);

    const QString hexKey = QStringLiteral("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
    expectLogMessage("MAVLink.SigningKeys", QtWarningMsg, QRegularExpression("Maximum key count reached:"));
    signingKeys->addRawKey(QStringLiteral("OverflowRaw"), hexKey);
    verifyExpectedLogMessage();
    QCOMPARE(signingKeys->keys()->count(), MAVLinkSigningKeys::kMaxKeys);

    signingKeys->removeAllKeys();
}

void SigningTest::_testGenerateRandomHexKey()
{
    const QString key1 = MAVLinkSigningKeys::generateRandomHexKey();
    const QString key2 = MAVLinkSigningKeys::generateRandomHexKey();

    QCOMPARE(key1.length(), 64);
    QCOMPARE(key2.length(), 64);

    const QByteArray bytes1 = QByteArray::fromHex(key1.toLatin1());
    const QByteArray bytes2 = QByteArray::fromHex(key2.toLatin1());
    QCOMPARE(bytes1.size(), 32);
    QCOMPARE(bytes2.size(), 32);

    QVERIFY(key1 != key2);
}

void SigningTest::_testRemoveKeyNonexistent()
{
    auto* signingKeys = MAVLinkSigningKeys::instance();
    signingKeys->removeAllKeys();

    signingKeys->addKey(QStringLiteral("Existing"), QStringLiteral("passpass"));
    QCOMPARE(signingKeys->keys()->count(), 1);

    signingKeys->removeKey(QStringLiteral("DoesNotExist"));
    QCOMPARE(signingKeys->keys()->count(), 1);
    QCOMPARE(signingKeys->keyAt(0)->name(), QStringLiteral("Existing"));

    signingKeys->removeKey(QString());
    QCOMPARE(signingKeys->keys()->count(), 1);

    signingKeys->removeAllKeys();
}

void SigningTest::_testKeyAtOutOfBounds()
{
    auto* signingKeys = MAVLinkSigningKeys::instance();
    signingKeys->removeAllKeys();

    QVERIFY(!signingKeys->keyAt(0));
    QVERIFY(!signingKeys->keyAt(-1));
    QVERIFY(!signingKeys->keyAt(100));

    signingKeys->addKey(QStringLiteral("OnlyKey"), QStringLiteral("passpass"));
    QCOMPARE(signingKeys->keys()->count(), 1);

    QVERIFY(signingKeys->keyAt(0));
    QCOMPARE(signingKeys->keyAt(0)->name(), QStringLiteral("OnlyKey"));

    QVERIFY(!signingKeys->keyAt(1));
    QVERIFY(!signingKeys->keyAt(-1));
    QVERIFY(!signingKeys->keyAt(9999));

    signingKeys->removeAllKeys();
}

void SigningTest::_testSetMessageSigned()
{
    mavlink_heartbeat_t heartbeat{};
    mavlink_message_t message;
    (void)mavlink_msg_heartbeat_encode(1, MAV_COMP_ID_AUTOPILOT1, &message, &heartbeat);

    QVERIFY(!MAVLinkSigning::isMessageSigned(message));

    MAVLinkSigning::setMessageSigned(message, true);
    QVERIFY(MAVLinkSigning::isMessageSigned(message));

    MAVLinkSigning::setMessageSigned(message, false);
    QVERIFY(!MAVLinkSigning::isMessageSigned(message));

    MAVLinkSigning::setMessageSigned(message, true);
    MAVLinkSigning::setMessageSigned(message, true);
    QVERIFY(MAVLinkSigning::isMessageSigned(message));
}

void SigningTest::_testSigningStatusString()
{
    SigningChannel ch;
    QVERIFY(ch.init(MAVLINK_COMM_0, QByteArrayView(), nullptr));
    QVERIFY(MAVLinkSigning::signingStatusString(MAVLINK_COMM_0).isEmpty());

    const QByteArray rawKey(32, '\x20');
    QVERIFY(ch.init(MAVLINK_COMM_0, rawKey, MAVLinkSigning::insecureConnectionAcceptUnsignedCallback));

    const mavlink_heartbeat_t heartbeat{};
    mavlink_message_t message;
    (void)mavlink_msg_heartbeat_encode_chan(1, MAV_COMP_ID_AUTOPILOT1, MAVLINK_COMM_0, &message, &heartbeat);

    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &message);

    mavlink_message_t parsed;
    mavlink_status_t status;
    for (uint16_t i = 0; i < len; ++i) {
        if (mavlink_parse_char(MAVLINK_COMM_0, buf[i], &parsed, &status) == MAVLINK_FRAMING_OK) {
            break;
        }
    }

    const QString statusStr = MAVLinkSigning::signingStatusString(MAVLINK_COMM_0);
    QCOMPARE(statusStr, QStringLiteral("OK"));

    QVERIFY(ch.init(MAVLINK_COMM_0, QByteArrayView(), nullptr));
}

void SigningTest::_testSigningStreamCount()
{
    SigningChannel ch;
    QVERIFY(ch.init(MAVLINK_COMM_0, QByteArrayView(), nullptr));
    QCOMPARE(MAVLinkSigning::signingStreamCount(MAVLINK_COMM_0), 0);

    const QByteArray rawKey(32, '\x30');
    QVERIFY(ch.init(MAVLINK_COMM_0, rawKey, MAVLinkSigning::insecureConnectionAcceptUnsignedCallback));

    const int countBefore = MAVLinkSigning::signingStreamCount(MAVLINK_COMM_0);

    const mavlink_heartbeat_t heartbeat{};
    mavlink_message_t message;
    (void)mavlink_msg_heartbeat_encode_chan(1, MAV_COMP_ID_AUTOPILOT1, MAVLINK_COMM_0, &message, &heartbeat);

    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &message);

    mavlink_message_t parsed;
    mavlink_status_t status;
    for (uint16_t i = 0; i < len; ++i) {
        if (mavlink_parse_char(MAVLINK_COMM_0, buf[i], &parsed, &status) == MAVLINK_FRAMING_OK) {
            break;
        }
    }

    QVERIFY(MAVLinkSigning::signingStreamCount(MAVLINK_COMM_0) >= countBefore);

    QVERIFY(ch.init(MAVLINK_COMM_0, QByteArrayView(), nullptr));
}

void SigningTest::_testTimestampPersistence()
{
    SigningChannel ch;
    const QByteArray rawKey(32, '\x40');
    QVERIFY(ch.init(MAVLINK_COMM_0, rawKey, MAVLinkSigning::insecureConnectionAcceptUnsignedCallback));

    const mavlink_signing_t* signing = mavlink_get_channel_status(MAVLINK_COMM_0)->signing;
    QVERIFY(signing);
    QVERIFY(signing->timestamp > 0);
    const uint64_t firstTs = signing->timestamp;

    QVERIFY(ch.init(MAVLINK_COMM_0, QByteArrayView(), nullptr));
    QVERIFY(ch.init(MAVLINK_COMM_0, rawKey, MAVLinkSigning::insecureConnectionAcceptUnsignedCallback));

    const mavlink_signing_t* signing2 = mavlink_get_channel_status(MAVLINK_COMM_0)->signing;
    QVERIFY(signing2);
    QVERIFY(signing2->timestamp >= firstTs);

    QVERIFY(ch.init(MAVLINK_COMM_0, QByteArrayView(), nullptr));
}

void SigningTest::_testReplayProtection()
{
    SigningChannel ch;
    const QByteArray rawKey(32, '\x55');
    QVERIFY(ch.init(MAVLINK_COMM_0, rawKey, MAVLinkSigning::secureConnectionAcceptUnsignedCallback));

    auto encodeAndCapture = [](QByteArray& out) {
        const mavlink_heartbeat_t heartbeat{};
        mavlink_message_t message;
        (void)mavlink_msg_heartbeat_encode_chan(1, MAV_COMP_ID_AUTOPILOT1, MAVLINK_COMM_0, &message, &heartbeat);
        uint8_t buf[MAVLINK_MAX_PACKET_LEN];
        const uint16_t len = mavlink_msg_to_send_buffer(buf, &message);
        out = QByteArray(reinterpret_cast<const char*>(buf), len);
    };

    QByteArray older;
    encodeAndCapture(older);
    QByteArray newer;
    encodeAndCapture(newer);
    QVERIFY(older != newer);

    auto feed = [](const QByteArray& bytes) -> mavlink_signing_status_t {
        mavlink_message_t parsed{};
        mavlink_status_t status{};
        for (qsizetype i = 0; i < bytes.size(); ++i) {
            (void)mavlink_parse_char(MAVLINK_COMM_0, static_cast<uint8_t>(bytes[i]), &parsed, &status);
        }
        const mavlink_signing_t* signing = mavlink_get_channel_status(MAVLINK_COMM_0)->signing;
        return signing ? signing->last_status : MAVLINK_SIGNING_STATUS_NONE;
    };

    QCOMPARE(static_cast<int>(feed(newer)), static_cast<int>(MAVLINK_SIGNING_STATUS_OK));
    QCOMPARE(static_cast<int>(feed(older)), static_cast<int>(MAVLINK_SIGNING_STATUS_REPLAY));

    QVERIFY(ch.init(MAVLINK_COMM_0, QByteArrayView(), nullptr));
}

void SigningTest::_testTryDetectKeySuspended()
{
    auto* signingKeys = MAVLinkSigningKeys::instance();
    signingKeys->removeAllKeys();
    QVERIFY(signingKeys->addKey(QStringLiteral("KeySuspend"), QStringLiteral("phrasephrase")));

    const auto& keyBytes = signingKeys->keyAt(0)->keyBytes();
    const QByteArrayView kv(reinterpret_cast<const char*>(keyBytes.data()), keyBytes.size());

    mavlink_message_t message;
    {
        SigningChannel ch;
        QVERIFY(ch.init(MAVLINK_COMM_2, kv, MAVLinkSigning::insecureConnectionAcceptUnsignedCallback));
        const mavlink_heartbeat_t heartbeat{};
        (void)mavlink_msg_heartbeat_encode_chan(1, MAV_COMP_ID_AUTOPILOT1, MAVLINK_COMM_2, &message, &heartbeat);
        QVERIFY(ch.init(MAVLINK_COMM_2, QByteArrayView(), nullptr));
    }

    constexpr auto kSuspendedCh = static_cast<mavlink_channel_t>(5);
    SigningController suspendedCtrl(kSuspendedCh);
    QVERIFY(!suspendedCtrl.isEnabled());
    QVERIFY(!suspendedCtrl.channel().isAutoDetectSuspended());

    QVERIFY(!suspendedCtrl.tryBeginEnable(1, QStringLiteral("KeySuspend"), keyBytes));
    QVERIFY(suspendedCtrl.channel().isAutoDetectSuspended());
    QVERIFY(MAVLinkSigningKeys::instance()->tryDetectKey(&suspendedCtrl, message).isEmpty());
    QVERIFY(!suspendedCtrl.isEnabled());
    expectLogMessage("MAVLink.SigningController", QtWarningMsg, QRegularExpression("Signing operation cancelled"));
    suspendedCtrl.cancelPending();
    verifyExpectedLogMessage();
    QVERIFY(!suspendedCtrl.channel().isAutoDetectSuspended());

    constexpr auto kActiveCh = static_cast<mavlink_channel_t>(6);
    SigningController activeCtrl(kActiveCh);
    QCOMPARE(MAVLinkSigningKeys::instance()->tryDetectKey(&activeCtrl, message), QStringLiteral("KeySuspend"));
    QVERIFY(activeCtrl.isEnabled());

    signingKeys->removeAllKeys();
}

void SigningTest::_testTryDetectKeyCooldown()
{
    auto* signingKeys = MAVLinkSigningKeys::instance();
    signingKeys->removeAllKeys();

    const QString hex = QStringLiteral("aabbccddeeff00112233445566778899aabbccddeeff00112233445566778899");
    QVERIFY(signingKeys->addRawKey(QStringLiteral("CooldownKey"), hex));
    const auto& keyBytes = signingKeys->keyAt(0)->keyBytes();
    const QByteArrayView kv(reinterpret_cast<const char*>(keyBytes.data()), keyBytes.size());

    mavlink_message_t message;
    {
        SigningChannel ch;
        QVERIFY(ch.init(MAVLINK_COMM_2, kv, MAVLinkSigning::insecureConnectionAcceptUnsignedCallback));
        const mavlink_heartbeat_t heartbeat{};
        (void)mavlink_msg_heartbeat_encode_chan(1, MAV_COMP_ID_AUTOPILOT1, MAVLINK_COMM_2, &message, &heartbeat);
        QVERIFY(ch.init(MAVLINK_COMM_2, QByteArrayView(), nullptr));
    }

    signingKeys->removeAllKeys();

    constexpr auto kCh = static_cast<mavlink_channel_t>(7);
    SigningController ctrl(kCh);
    QVERIFY(!ctrl.isEnabled());
    QVERIFY(MAVLinkSigningKeys::instance()->tryDetectKey(&ctrl, message).isEmpty());

    QVERIFY(signingKeys->addRawKey(QStringLiteral("CooldownKey"), hex));
    QVERIFY(MAVLinkSigningKeys::instance()->tryDetectKey(&ctrl, message).isEmpty());
    QVERIFY(!ctrl.isEnabled());

    QTest::qWait(SigningChannel::kDetectCooldownMs + 200);
    QCOMPARE(MAVLinkSigningKeys::instance()->tryDetectKey(&ctrl, message), QStringLiteral("CooldownKey"));
    QVERIFY(ctrl.isEnabled());

    signingKeys->removeAllKeys();
}

void SigningTest::_testChannelKeyName()
{
    auto* signingKeys = MAVLinkSigningKeys::instance();
    signingKeys->removeAllKeys();
    const QString name = QStringLiteral("Named");
    QVERIFY(signingKeys->addKey(name, QStringLiteral("phrasephrase")));

    SigningController ctrl(MAVLINK_COMM_0);
    QVERIFY(ctrl.keyName().isEmpty());

    const auto& keyBytes = signingKeys->keyAt(0)->keyBytes();
    const QByteArrayView kv(reinterpret_cast<const char*>(keyBytes.data()), keyBytes.size());
    QVERIFY(ctrl.initSigningImmediate(kv, MAVLinkSigning::UnsignedAcceptancePolicy::Strict, name));
    QCOMPARE(ctrl.keyName(), name);

    QVERIFY(ctrl.clearSigning());
    QVERIFY(ctrl.keyName().isEmpty());
    signingKeys->removeAllKeys();
}

void SigningTest::_testInitSigningWithPersistedTimestamp()
{
    auto* signingKeys = MAVLinkSigningKeys::instance();
    signingKeys->removeAllKeys();
    const QString name = QStringLiteral("PersistedKey");
    QVERIFY(signingKeys->addKey(name, QStringLiteral("phrasephrase")));

    constexpr uint64_t kFuture = uint64_t(1) << 50;
    signingKeys->recordTimestamp(name, kFuture);

    SigningController ctrl(MAVLINK_COMM_0);
    const auto& keyBytes = signingKeys->keyAt(0)->keyBytes();
    const QByteArrayView kv(reinterpret_cast<const char*>(keyBytes.data()), keyBytes.size());
    QVERIFY(ctrl.initSigningImmediate(kv, MAVLinkSigning::UnsignedAcceptancePolicy::Strict, name));

    const mavlink_signing_t* signing = mavlink_get_channel_status(MAVLINK_COMM_0)->signing;
    QVERIFY(signing);
    QVERIFY(signing->timestamp >= kFuture + SigningChannel::kPersistedTimestampSafetyBumpTicks);

    QVERIFY(ctrl.clearSigning());
    signingKeys->removeAllKeys();
}

void SigningTest::_testStripSignatureForRetransmitProducesValidCrc()
{
    auto* signingKeys = MAVLinkSigningKeys::instance();
    signingKeys->removeAllKeys();
    QVERIFY(signingKeys->addRawKey(QStringLiteral("CrcKey"),
                                   QStringLiteral("aabbccddeeff00112233445566778899aabbccddeeff00112233445566778899")));
    const auto& keyBytes = signingKeys->keyAt(0)->keyBytes();
    const QByteArrayView kv(reinterpret_cast<const char*>(keyBytes.data()), keyBytes.size());

    mavlink_message_t signed_msg;
    {
        SigningChannel ch;
        QVERIFY(ch.init(MAVLINK_COMM_0, kv, MAVLinkSigning::insecureConnectionAcceptUnsignedCallback));
        const mavlink_heartbeat_t heartbeat{};
        (void)mavlink_msg_heartbeat_encode_chan(1, MAV_COMP_ID_AUTOPILOT1, MAVLINK_COMM_0, &signed_msg, &heartbeat);
        QVERIFY(MAVLinkSigning::isMessageSigned(signed_msg));
        QVERIFY(ch.init(MAVLINK_COMM_0, QByteArrayView(), nullptr));
    }

    const QByteArray bytes = MAVLinkSigning::serializeUnsignedCopy(signed_msg);
    QVERIFY(bytes.size() > 0);

    mavlink_message_t parsed{};
    mavlink_status_t parseStatus{};
    uint8_t framing = MAVLINK_FRAMING_INCOMPLETE;
    for (qsizetype i = 0; i < bytes.size(); ++i) {
        framing = mavlink_parse_char(MAVLINK_COMM_1, static_cast<uint8_t>(bytes[i]), &parsed, &parseStatus);
    }
    QCOMPARE(static_cast<int>(framing), static_cast<int>(MAVLINK_FRAMING_OK));
    QCOMPARE(static_cast<uint32_t>(parsed.msgid), static_cast<uint32_t>(MAVLINK_MSG_ID_HEARTBEAT));
    QVERIFY(!MAVLinkSigning::isMessageSigned(parsed));

    signingKeys->removeAllKeys();
}

void SigningTest::_testTryDetectKeyInstallsSecureCallback()
{
    auto* signingKeys = MAVLinkSigningKeys::instance();
    signingKeys->removeAllKeys();
    QVERIFY(signingKeys->addRawKey(QStringLiteral("DetectSecure"),
                                   QStringLiteral("11223344556677889900aabbccddeeff11223344556677889900aabbccddeeff")));
    const auto& keyBytes = signingKeys->keyAt(0)->keyBytes();
    const QByteArrayView kv(reinterpret_cast<const char*>(keyBytes.data()), keyBytes.size());

    mavlink_message_t message;
    {
        SigningChannel ch;
        QVERIFY(ch.init(MAVLINK_COMM_2, kv, MAVLinkSigning::insecureConnectionAcceptUnsignedCallback));
        const mavlink_heartbeat_t heartbeat{};
        (void)mavlink_msg_heartbeat_encode_chan(1, MAV_COMP_ID_AUTOPILOT1, MAVLINK_COMM_2, &message, &heartbeat);
        QVERIFY(ch.init(MAVLINK_COMM_2, QByteArrayView(), nullptr));
    }

    constexpr auto kCh = static_cast<mavlink_channel_t>(8);
    SigningController ctrl(kCh);
    QCOMPARE(MAVLinkSigningKeys::instance()->tryDetectKey(&ctrl, message), QStringLiteral("DetectSecure"));

    const mavlink_status_t* const status = mavlink_get_channel_status(kCh);
    QVERIFY(status && status->signing);
    QVERIFY(status->signing->accept_unsigned_callback ==
            MAVLinkSigning::secureConnectionAcceptUnsignedCallback);
    QVERIFY(status->signing->accept_unsigned_callback !=
            MAVLinkSigning::insecureConnectionAcceptUnsignedCallback);

    signingKeys->removeAllKeys();
}

void SigningTest::_testRefreshOutgoingTimestamp()
{
    SigningChannel ch;
    const QByteArray rawKey(32, '\x77');
    QVERIFY(ch.init(MAVLINK_COMM_0, rawKey, MAVLinkSigning::insecureConnectionAcceptUnsignedCallback));

    const mavlink_signing_t* const signing = mavlink_get_channel_status(MAVLINK_COMM_0)->signing;
    QVERIFY(signing);

    // Simulate the issue #14375 stall: init was 3 minutes ago, no outbound packets sent since.
    constexpr uint64_t kThreeMinutesTicks = 3ULL * 60 * 100'000;  // 10µs ticks
    const uint64_t stale = MAVLinkSigning::currentSigningTimestampTicks() - kThreeMinutesTicks;
    const_cast<mavlink_signing_t*>(signing)->timestamp = stale;

    QVERIFY(ch.refreshOutgoingTimestamp());
    QVERIFY(signing->timestamp >= MAVLinkSigning::currentSigningTimestampTicks() - 100'000);  // within 1s of wall clock

    // Second call with no wall-clock advance should be a no-op (already at/ahead of wall clock).
    const uint64_t afterFirst = signing->timestamp;
    const_cast<mavlink_signing_t*>(signing)->timestamp = afterFirst + (10ULL * 100'000);  // 10s into the future
    QVERIFY(!ch.refreshOutgoingTimestamp());
    QCOMPARE(signing->timestamp, afterFirst + (10ULL * 100'000));

    QVERIFY(ch.init(MAVLINK_COMM_0, QByteArrayView(), nullptr));
    QVERIFY(!ch.refreshOutgoingTimestamp());  // disabled → no-op
}

// Regression: mavlink/qgroundcontrol#14430 — sendMessageMultiple caches signed bytes; without send-time re-signing
// the frozen timestamp drifts behind wall clock and the receiver rejects with OLD_TIMESTAMP.
void SigningTest::_testSignOutgoingRefreshesCachedTimestamp()
{
    SigningChannel ch;
    QByteArray rawKey(32, '\0');
    for (int i = 0; i < 32; ++i) {
        rawKey[i] = static_cast<char>(i + 1);
    }
    QVERIFY(ch.init(MAVLINK_COMM_1, rawKey, MAVLinkSigning::insecureConnectionAcceptUnsignedCallback));

    auto* const signing = mavlink_get_channel_status(MAVLINK_COMM_1)->signing;
    QVERIFY(signing);

    // Encode once while the clock is stalled 3 minutes in the past: this is the cached message whose signature
    // timestamp is frozen stale, exactly as sendMessageMultiple would hold it across retries.
    constexpr uint64_t kThreeMinutesTicks = 3ULL * 60 * 100'000;
    const uint64_t stale = MAVLinkSigning::currentSigningTimestampTicks() - kThreeMinutesTicks;
    signing->timestamp = stale;

    const mavlink_heartbeat_t heartbeat{};
    mavlink_message_t cached;
    (void)mavlink_msg_heartbeat_encode_chan(1, MAV_COMP_ID_AUTOPILOT1, MAVLINK_COMM_1, &cached, &heartbeat);
    QVERIFY(MAVLinkSigning::isMessageSigned(cached));
    QVERIFY(MAVLinkSigning::verifySignature(rawKey, cached));

    const auto sigTimestamp = [](const mavlink_message_t& m) {
        uint64_t ts = 0;
        for (int i = 0; i < 6; ++i) {
            ts |= static_cast<uint64_t>(m.signature[1 + i]) << (8 * i);
        }
        return ts;
    };
    const uint64_t cachedTs = sigTimestamp(cached);
    QVERIFY(cachedTs < MAVLinkSigning::currentSigningTimestampTicks() - (60ULL * 100'000));  // genuinely stale

    // Re-sign the cached bytes at send time.
    QVERIFY(ch.signOutgoing(cached));

    // Refreshed signature jumps forward to ~wall clock, stays monotonic, and still verifies against the key.
    const uint64_t refreshedTs = sigTimestamp(cached);
    QVERIFY(refreshedTs > cachedTs);
    QVERIFY(refreshedTs >= MAVLinkSigning::currentSigningTimestampTicks() - 100'000);
    QVERIFY(MAVLinkSigning::isMessageSigned(cached));
    QVERIFY(MAVLinkSigning::verifySignature(rawKey, cached));

    // Disabled channel → no-op.
    QVERIFY(ch.init(MAVLINK_COMM_1, QByteArrayView(), nullptr));
    QVERIFY(!ch.signOutgoing(cached));
}

// Round-trips key bytes through the QSettings store (QGCKeychain removed): _save persists, _load re-reads,
// and removal must not resurrect on reload.
void SigningTest::_testKeyStorePersistRoundTrip()
{
    auto* signingKeys = MAVLinkSigningKeys::instance();
    signingKeys->removeAllKeys();

    const QString hexA = QStringLiteral("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
    const QString hexB = QStringLiteral("202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f");
    QVERIFY(signingKeys->addRawKey(QStringLiteral("PersistA"), hexA));
    QVERIFY(signingKeys->addRawKey(QStringLiteral("PersistB"), hexB));

    signingKeys->_load();
    QCOMPARE(signingKeys->keys()->count(), 2);
    QCOMPARE(signingKeys->keyHexByName(QStringLiteral("PersistA")), hexA);
    QCOMPARE(signingKeys->keyHexByName(QStringLiteral("PersistB")), hexB);

    signingKeys->removeKey(QStringLiteral("PersistA"));
    signingKeys->_load();
    QCOMPARE(signingKeys->keys()->count(), 1);
    QVERIFY(signingKeys->keyHexByName(QStringLiteral("PersistA")).isEmpty());
    QCOMPARE(signingKeys->keyHexByName(QStringLiteral("PersistB")), hexB);

    signingKeys->removeAllKeys();
}

UT_REGISTER_TEST(SigningTest, TestLabel::Unit)
