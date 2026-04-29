#include "MockLinkSigningTest.h"

#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>

#include "MAVLinkSigning.h"
#include "MAVLinkSigningKeys.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"
#include "VehicleSigningController.h"

MockLinkSigningTest::MockLinkSigningTest(QObject* parent) : VehicleTest(parent) {}

void MockLinkSigningTest::init()
{
    VehicleTest::init();

    MAVLinkSigningKeys::instance()->removeAllKeys();
}

void MockLinkSigningTest::cleanup()
{
    MAVLinkSigningKeys::instance()->removeAllKeys();

    VehicleTest::cleanup();
}

void MockLinkSigningTest::_testSendSetupSigning()
{
    QVERIFY(vehicle());
    QVERIFY(mockLink());

    auto* signingKeys = MAVLinkSigningKeys::instance();
    signingKeys->addKey("TestKey", "TestPassphrase");
    QVERIFY(signingKeys->keyAt(0));
    QCOMPARE(static_cast<int>(signingKeys->keyAt(0)->keyBytes().size()), 32);

    mockLink()->clearReceivedMavlinkMessageCounts();
    QVERIFY(!mockLink()->signingEnabled());

    vehicle()->signingController()->enable(QStringLiteral("TestKey"));

    QVERIFY_TRUE_WAIT(mockLink()->receivedMavlinkMessageCount(MAVLINK_MSG_ID_SETUP_SIGNING) >= 2,
                      TestTimeout::mediumMs());
    QVERIFY(mockLink()->signingEnabled());

    QVERIFY_TRUE_WAIT(vehicle()->signingController()->signingStatus().enabled, TestTimeout::mediumMs());
    QCOMPARE(vehicle()->signingController()->signingStatus().keyName, QStringLiteral("TestKey"));
}

void MockLinkSigningTest::_testSendDisableSigning()
{
    QVERIFY(vehicle());
    QVERIFY(mockLink());

    auto* signingKeys = MAVLinkSigningKeys::instance();
    signingKeys->addKey("TestKey2", "TestPassphrase2");

    vehicle()->signingController()->enable(QStringLiteral("TestKey2"));
    QVERIFY_TRUE_WAIT(mockLink()->signingEnabled(), TestTimeout::mediumMs());
    QVERIFY_TRUE_WAIT(vehicle()->signingController()->signingStatus().enabled, TestTimeout::mediumMs());

    mockLink()->clearReceivedMavlinkMessageCounts();
    vehicle()->signingController()->disable();

    QVERIFY_TRUE_WAIT(mockLink()->receivedMavlinkMessageCount(MAVLINK_MSG_ID_SETUP_SIGNING) >= 2,
                      TestTimeout::mediumMs());
    QVERIFY(!mockLink()->signingEnabled());

    QVERIFY_TRUE_WAIT(!vehicle()->signingController()->signingStatus().enabled, TestTimeout::mediumMs());
    QVERIFY(vehicle()->signingController()->signingStatus().keyName.isEmpty());
}

void MockLinkSigningTest::_testSigningEnableTimeout()
{
    QVERIFY(vehicle());
    QVERIFY(mockLink());

    // showAppMessage uses uncategorized qDebug in test mode
    expectLogMessage(QtDebugMsg, QRegularExpression("showAppMessage.*timeout"));

    auto* signingKeys = MAVLinkSigningKeys::instance();
    signingKeys->addKey("BadKey", "BadPassphrase");

    mockLink()->setCommLost(true);

    vehicle()->signingController()->enable(QStringLiteral("BadKey"));

    QVERIFY(!vehicle()->signingController()->signingStatus().enabled);

    // Confirmation timer is 5s — wait for pending to clear with CI-safe margin
    QVERIFY_TRUE_WAIT(!vehicle()->signingController()->signingStatus().pending(), TestTimeout::mediumMs() + 5000);
    QVERIFY(!vehicle()->signingController()->signingStatus().enabled);
    QVERIFY(vehicle()->signingController()->signingStatus().keyName.isEmpty());

    mockLink()->setCommLost(false);
}

void MockLinkSigningTest::_testSigningKeysAddRemove()
{
    auto* signingKeys = MAVLinkSigningKeys::instance();

    QCOMPARE(signingKeys->keys()->count(), 0);

    QSignalSpy spy(signingKeys, &MAVLinkSigningKeys::keysChanged);
    signingKeys->addKey("Key1", "pass1");
    QCOMPARE(signingKeys->keys()->count(), 1);
    QCOMPARE(signingKeys->keyAt(0)->name(), "Key1");
    QCOMPARE(static_cast<int>(signingKeys->keyAt(0)->keyBytes().size()), 32);
    QVERIFY(spy.count() > 0);

    spy.clear();
    signingKeys->addKey("Key2", "pass2");
    QCOMPARE(signingKeys->keys()->count(), 2);
    QCOMPARE(signingKeys->keyAt(1)->name(), "Key2");
    QVERIFY(spy.count() > 0);

    signingKeys->removeKey(QStringLiteral("Key1"));
    QCOMPARE(signingKeys->keys()->count(), 1);
    QCOMPARE(signingKeys->keyAt(0)->name(), "Key2");

    signingKeys->removeKey(QStringLiteral("Key2"));
    QCOMPARE(signingKeys->keys()->count(), 0);
}

void MockLinkSigningTest::_testSigningPendingState()
{
    QVERIFY(vehicle());
    QVERIFY(mockLink());

    auto* signingKeys = MAVLinkSigningKeys::instance();
    signingKeys->addKey("PendingKey", "PendingPass");

    QVERIFY(!vehicle()->signingController()->signingStatus().pending());

    mockLink()->setCommLost(true);
    expectLogMessage(QtDebugMsg, QRegularExpression("showAppMessage.*timeout"));

    vehicle()->signingController()->enable(QStringLiteral("PendingKey"));

    QVERIFY(vehicle()->signingController()->signingStatus().pending());
    QVERIFY(!vehicle()->signingController()->signingStatus().enabled);

    QVERIFY(vehicle()->signingController()->signingStatus().statusText.contains("Configuring"));

    // Confirmation timer is 5s — wait for pending to clear with CI-safe margin
    QVERIFY_TRUE_WAIT(!vehicle()->signingController()->signingStatus().pending(), TestTimeout::mediumMs() + 5000);
    QVERIFY(!vehicle()->signingController()->signingStatus().enabled);

    mockLink()->setCommLost(false);
}

void MockLinkSigningTest::_testSigningStatusChangedSignalFiresOnEnable()
{
    QVERIFY(vehicle());
    QVERIFY(mockLink());

    auto* signingKeys = MAVLinkSigningKeys::instance();
    signingKeys->addKey("SignalKey", "SignalPass");

    QSignalSpy spy(vehicle()->signingController(), &VehicleSigningController::signingStatusChanged);
    vehicle()->signingController()->enable(QStringLiteral("SignalKey"));

    QTRY_VERIFY_WITH_TIMEOUT(vehicle()->signingController()->signingStatus().enabled, TestTimeout::mediumMs());
    // At minimum: Off→Enabling on beginEnable, Enabling→On on confirm.
    QVERIFY(spy.count() >= 2);
}

UT_REGISTER_TEST(MockLinkSigningTest, TestLabel::Integration, TestLabel::Vehicle)
