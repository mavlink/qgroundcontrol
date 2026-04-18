#include "QmlObjectListModel.h"
#include "MockLinkSigningTest.h"

#include "MAVLinkSigningKeys.h"
#include "MAVLinkSigning.h"
#include "Vehicle.h"

#include <QtTest/QSignalSpy>

MockLinkSigningTest::MockLinkSigningTest(QObject* parent)
    : VehicleTest(parent)
{
}

void MockLinkSigningTest::init()
{
    VehicleTest::init();

    // Ensure a clean key store before each test
    auto* signingKeys = MAVLinkSigningKeys::instance();
    while (signingKeys->keys()->count() > 0) {
        signingKeys->removeKey(0);
    }
}

void MockLinkSigningTest::cleanup()
{
    // Always clean up keys even if the test failed
    auto* signingKeys = MAVLinkSigningKeys::instance();
    while (signingKeys->keys()->count() > 0) {
        signingKeys->removeKey(0);
    }

    VehicleTest::cleanup();
}

void MockLinkSigningTest::_testSendSetupSigning()
{
    QVERIFY(vehicle());
    QVERIFY(mockLink());

    // Add a key
    auto* signingKeys = MAVLinkSigningKeys::instance();
    signingKeys->addKey("TestKey", "TestPassphrase");
    QVERIFY(!signingKeys->keyBytesAt(0).isEmpty());

    mockLink()->clearReceivedMavlinkMessageCounts();
    QVERIFY(!mockLink()->signingEnabled());

    QSignalSpy signingChangedSpy(vehicle(), &Vehicle::mavlinkSigningChanged);

    // Send signing setup to vehicle with key index 0
    vehicle()->sendSetupSigning(0);

    // Verify MockLink received the SETUP_SIGNING message (sent twice for reliability)
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavlinkMessageCount(MAVLINK_MSG_ID_SETUP_SIGNING) >= 2, TestTimeout::mediumMs());
    QVERIFY(mockLink()->signingEnabled());

    // Verify the signal was emitted and the vehicle tracks the active key name
    QVERIFY(signingChangedSpy.count() > 0);
    QCOMPARE(vehicle()->mavlinkSigningKeyName(), QStringLiteral("TestKey"));
}

void MockLinkSigningTest::_testSendDisableSigning()
{
    QVERIFY(vehicle());
    QVERIFY(mockLink());

    // Add a key and send to vehicle first
    auto* signingKeys = MAVLinkSigningKeys::instance();
    signingKeys->addKey("TestKey2", "TestPassphrase2");

    QSignalSpy signingChangedSpy(vehicle(), &Vehicle::mavlinkSigningChanged);

    vehicle()->sendSetupSigning(0);
    QVERIFY_TRUE_WAIT(mockLink()->signingEnabled(), TestTimeout::mediumMs());
    QVERIFY(signingChangedSpy.count() > 0);

    // Now disable signing
    signingChangedSpy.clear();
    mockLink()->clearReceivedMavlinkMessageCounts();
    vehicle()->sendDisableSigning();

    // Verify MockLink received the disable message and signing is now off
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavlinkMessageCount(MAVLINK_MSG_ID_SETUP_SIGNING) >= 2, TestTimeout::mediumMs());
    QVERIFY(!mockLink()->signingEnabled());

    // Verify the signal was emitted and the vehicle cleared the active key name
    QVERIFY(signingChangedSpy.count() > 0);
    QVERIFY(vehicle()->mavlinkSigningKeyName().isEmpty());
}

void MockLinkSigningTest::_testSigningKeysAddRemove()
{
    auto* signingKeys = MAVLinkSigningKeys::instance();

    // Start with no keys
    QCOMPARE(signingKeys->keys()->count(), 0);

    // Add first key
    QSignalSpy spy(signingKeys, &MAVLinkSigningKeys::keysChanged);
    signingKeys->addKey("Key1", "pass1");
    QCOMPARE(signingKeys->keys()->count(), 1);
    QCOMPARE(signingKeys->keyNameAt(0), "Key1");
    QVERIFY(!signingKeys->keyBytesAt(0).isEmpty());
    QCOMPARE(signingKeys->keyBytesAt(0).size(), 32);
    QVERIFY(spy.count() > 0);

    // Add second key
    spy.clear();
    signingKeys->addKey("Key2", "pass2");
    QCOMPARE(signingKeys->keys()->count(), 2);
    QCOMPARE(signingKeys->keyNameAt(1), "Key2");
    QVERIFY(spy.count() > 0);

    // Remove first key
    signingKeys->removeKey(0);
    QCOMPARE(signingKeys->keys()->count(), 1);
    QCOMPARE(signingKeys->keyNameAt(0), "Key2");

    // Remove remaining key
    signingKeys->removeKey(0);
    QCOMPARE(signingKeys->keys()->count(), 0);
}

UT_REGISTER_TEST(MockLinkSigningTest, TestLabel::Integration, TestLabel::Vehicle)
