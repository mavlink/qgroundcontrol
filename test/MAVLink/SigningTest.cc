#include "SigningTest.h"

#include "MAVLinkSigning.h"
#include "MAVLinkSigningKeys.h"
#include "QmlObjectListModel.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QSettings>

void SigningTest::_testInitSigning()
{
    // Create a 32-byte raw key
    QByteArray rawKey(32, '\0');
    rawKey[0] = 'A';
    rawKey[1] = 'B';

    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_0, rawKey,
                                        MAVLinkSigning::insecureConnectionAcceptUnsignedCallback));
    const mavlink_status_t* status = mavlink_get_channel_status(MAVLINK_COMM_0);
    const mavlink_signing_t* signing = status->signing;
    QVERIFY(signing);
    QVERIFY(memcmp(signing->secret_key, rawKey.constData(), sizeof(signing->secret_key)) == 0);

    // Disable signing with empty key
    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_0, QByteArrayView(),
                                        MAVLinkSigning::insecureConnectionAcceptUnsignedCallback));
    QVERIFY(!mavlink_get_channel_status(MAVLINK_COMM_0)->signing);
}

void SigningTest::_testCheckSigningLinkId()
{
    QByteArray rawKey(32, '\x01');
    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_0, rawKey,
                                        MAVLinkSigning::insecureConnectionAcceptUnsignedCallback));
    const mavlink_heartbeat_t heartbeat{};
    mavlink_message_t message;
    (void)mavlink_msg_heartbeat_encode_chan(1, MAV_COMP_ID_USER1, MAVLINK_COMM_0, &message, &heartbeat);
    QVERIFY(MAVLinkSigning::checkSigningLinkId(MAVLINK_COMM_0, message));
}

void SigningTest::_testCreateSetupSigning()
{
    QByteArray rawKey(32, '\x02');
    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_0, rawKey,
                                        MAVLinkSigning::insecureConnectionAcceptUnsignedCallback));
    mavlink_system_t target_system{};
    target_system.sysid = 1;
    target_system.compid = MAV_COMP_ID_AUTOPILOT1;
    mavlink_setup_signing_t setup_signing;
    MAVLinkSigning::createSetupSigning(MAVLINK_COMM_0, target_system, rawKey, setup_signing);
    QVERIFY(setup_signing.initial_timestamp != 0);
    QCOMPARE(setup_signing.target_system, target_system.sysid);
    QCOMPARE(setup_signing.target_component, target_system.compid);
    QVERIFY(memcmp(setup_signing.secret_key, rawKey.constData(), sizeof(setup_signing.secret_key)) == 0);
}

void SigningTest::_testVerifySignature()
{
    // Set up signing on MAVLINK_COMM_1 with a known key so the C library signs outgoing messages
    QByteArray rawKey(32, '\0');
    for (int i = 0; i < 32; ++i) {
        rawKey[i] = static_cast<char>(i);
    }

    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_1, rawKey,
                                        MAVLinkSigning::insecureConnectionAcceptUnsignedCallback));

    // Encode a heartbeat on the signing channel — the C library will sign it
    const mavlink_heartbeat_t heartbeat{};
    mavlink_message_t message;
    (void) mavlink_msg_heartbeat_encode_chan(1, MAV_COMP_ID_AUTOPILOT1, MAVLINK_COMM_1, &message, &heartbeat);

    // The message should be signed
    QVERIFY(MAVLinkSigning::isMessageSigned(message));

    // Our verifySignature should agree with the C library's signature
    QVERIFY(MAVLinkSigning::verifySignature(rawKey, message));

    // A different key should NOT verify
    QByteArray wrongKey(32, '\xFF');
    QVERIFY(!MAVLinkSigning::verifySignature(wrongKey, message));

    // Too-short key should fail gracefully
    QByteArray shortKey(16, '\x01');
    QVERIFY(!MAVLinkSigning::verifySignature(shortKey, message));

    // Disable signing on the channel to clean up
    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_1, QByteArrayView(), nullptr));
}

void SigningTest::_testTryDetectKey()
{
    auto* signingKeys = MAVLinkSigningKeys::instance();

    // Clean slate — remove any leftover keys
    while (signingKeys->keys()->count() > 0) {
        signingKeys->removeKey(0);
    }

    // Add three keys with different passphrases
    signingKeys->addKey(QStringLiteral("KeyAlpha"),   QStringLiteral("passphrase-alpha"));
    signingKeys->addKey(QStringLiteral("KeyBravo"),   QStringLiteral("passphrase-bravo"));
    signingKeys->addKey(QStringLiteral("KeyCharlie"), QStringLiteral("passphrase-charlie"));
    QCOMPARE(signingKeys->keys()->count(), 3);

    // Sign a message using the second key ("KeyBravo")
    const QByteArray bravoKey = signingKeys->keyBytesAt(1);
    QCOMPARE(bravoKey.size(), 32);

    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_2, bravoKey,
                                        MAVLinkSigning::insecureConnectionAcceptUnsignedCallback));

    const mavlink_heartbeat_t heartbeat{};
    mavlink_message_t message;
    (void) mavlink_msg_heartbeat_encode_chan(1, MAV_COMP_ID_AUTOPILOT1, MAVLINK_COMM_2, &message, &heartbeat);
    QVERIFY(MAVLinkSigning::isMessageSigned(message));

    // Disable signing on COMM_2 so it was only used to produce the signed message
    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_2, QByteArrayView(), nullptr));

    // COMM_3 has no signing configured — tryDetectKey should find "KeyBravo"
    QVERIFY(!MAVLinkSigning::isSigningEnabled(MAVLINK_COMM_3));
    const QString detected = MAVLinkSigning::tryDetectKey(MAVLINK_COMM_3, message);
    QCOMPARE(detected, QStringLiteral("KeyBravo"));

    // Signing should now be configured on COMM_3 with the bravo key
    QVERIFY(MAVLinkSigning::isSigningEnabled(MAVLINK_COMM_3));

    // Calling tryDetectKey again on an already-configured channel should return empty
    const QString again = MAVLinkSigning::tryDetectKey(MAVLINK_COMM_3, message);
    QVERIFY(again.isEmpty());

    // Clean up
    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_3, QByteArrayView(), nullptr));
    while (signingKeys->keys()->count() > 0) {
        signingKeys->removeKey(0);
    }
}

void SigningTest::_testMigrateLegacySigningKey()
{
    auto* signingKeys = MAVLinkSigningKeys::instance();

    // Clean slate
    while (signingKeys->keys()->count() > 0) {
        signingKeys->removeKey(0);
    }

    // Simulate the old Fact-based signing key at the QSettings root level
    const QString oldPassphrase = QStringLiteral("legacy-passphrase");
    {
        QSettings settings;
        settings.setValue(MAVLinkSigningKeys::kOldSigningKeySettingsKey, oldPassphrase);
        settings.sync();
    }

    // Trigger reload which should migrate the old key
    signingKeys->_load();

    // Verify migration created a key named "Migrated Key" with the correct SHA-256 hash
    QCOMPARE(signingKeys->keys()->count(), 1);
    QCOMPARE(signingKeys->keyNameAt(0), QStringLiteral("Migrated Key"));

    const QByteArray expectedHash = QCryptographicHash::hash(oldPassphrase.toUtf8(), QCryptographicHash::Sha256);
    QCOMPARE(signingKeys->keyBytesAt(0), expectedHash);

    // Verify the old setting was removed
    {
        QSettings settings;
        QVERIFY(!settings.contains(MAVLinkSigningKeys::kOldSigningKeySettingsKey));
    }

    // Verify persistence: reload from QSettings and confirm the migrated key survives the round-trip
    signingKeys->_load();
    QCOMPARE(signingKeys->keys()->count(), 1);
    QCOMPARE(signingKeys->keyNameAt(0), QStringLiteral("Migrated Key"));
    QCOMPARE(signingKeys->keyBytesAt(0), expectedHash);

    // Clean up
    while (signingKeys->keys()->count() > 0) {
        signingKeys->removeKey(0);
    }
}

UT_REGISTER_TEST(SigningTest, TestLabel::Unit)
