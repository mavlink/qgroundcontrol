#include "MockLinkSigningTest.h"

#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>

#include <chrono>

#include "MAVLinkSigning.h"
#include "MAVLinkSigningKeys.h"
#include "MissionItem.h"
#include "MissionManager.h"
#include "QmlObjectListModel.h"
#include "SigningController.h"
#include "Vehicle.h"
#include "VehicleSigningController.h"

MockLinkSigningTest::MockLinkSigningTest(QObject* parent) : VehicleTest(parent) {}

void MockLinkSigningTest::initTestCase()
{
    VehicleTest::initTestCase();
    MAVLinkSigningKeys::setPbkdf2IterationsForTesting(1);
}

void MockLinkSigningTest::init()
{
    VehicleTest::init();

    MAVLinkSigningKeys::instance()->removeAllKeys();
}

void MockLinkSigningTest::cleanup()
{
    SigningController::setTimeoutForTesting(std::chrono::milliseconds(0));
    MAVLinkSigningKeys::instance()->removeAllKeys();

    VehicleTest::cleanup();
}

void MockLinkSigningTest::_testSendSetupSigning()
{
    QVERIFY(vehicle());
    QVERIFY(mockLink());

    auto* signingKeys = MAVLinkSigningKeys::instance();
    QVERIFY(signingKeys->addKey("TestKey", "TestPassphrase"));
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
    QVERIFY(signingKeys->addKey("TestKey2", "TestPassphrase2"));

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

    expectLogMessage("MAVLink.SigningController",
                     QtWarningMsg,
                     QRegularExpression("signing operation failed: Timeout"));
    expectLogMessage("Vehicle.SigningController",
                     QtWarningMsg,
                     QRegularExpression("signing failed: Timeout"));
    expectAppMessage(QRegularExpression("Signing setup not confirmed by vehicle"));

    auto* signingKeys = MAVLinkSigningKeys::instance();
    QVERIFY(signingKeys->addKey("BadKey", "BadPassphrase"));

    // Comm is lost, so only the FSM timeout resolves this — shorten it to avoid the production 5s wait.
    SigningController::setTimeoutForTesting(std::chrono::milliseconds(500));
    mockLink()->setCommLost(true);

    vehicle()->signingController()->enable(QStringLiteral("BadKey"));

    QVERIFY(!vehicle()->signingController()->signingStatus().enabled);

    QVERIFY_TRUE_WAIT(!vehicle()->signingController()->signingStatus().pending(), TestTimeout::mediumMs() + 5000);
    QVERIFY(!vehicle()->signingController()->signingStatus().enabled);
    QVERIFY(vehicle()->signingController()->signingStatus().keyName.isEmpty());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    mockLink()->setCommLost(false);
}

void MockLinkSigningTest::_testSigningKeysAddRemove()
{
    auto* signingKeys = MAVLinkSigningKeys::instance();

    QCOMPARE(signingKeys->keys()->count(), 0);

    QSignalSpy spy(signingKeys, &MAVLinkSigningKeys::keysChanged);
    QVERIFY(signingKeys->addKey("Key1", "pass1pass"));
    QCOMPARE(signingKeys->keys()->count(), 1);
    QCOMPARE(signingKeys->keyAt(0)->name(), "Key1");
    QCOMPARE(static_cast<int>(signingKeys->keyAt(0)->keyBytes().size()), 32);
    QVERIFY(spy.count() > 0);

    spy.clear();
    QVERIFY(signingKeys->addKey("Key2", "pass2pass"));
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
    QVERIFY(signingKeys->addKey("PendingKey", "PendingPass"));

    QVERIFY(!vehicle()->signingController()->signingStatus().pending());

    // Comm is lost, so only the FSM timeout resolves this — shorten it to avoid the production 5s wait.
    SigningController::setTimeoutForTesting(std::chrono::milliseconds(500));
    mockLink()->setCommLost(true);
    expectLogMessage("MAVLink.SigningController",
                     QtWarningMsg,
                     QRegularExpression("signing operation failed: Timeout"));
    expectLogMessage("Vehicle.SigningController",
                     QtWarningMsg,
                     QRegularExpression("signing failed: Timeout"));
    expectAppMessage(QRegularExpression("Signing setup not confirmed by vehicle"));

    vehicle()->signingController()->enable(QStringLiteral("PendingKey"));

    QVERIFY(vehicle()->signingController()->signingStatus().pending());
    QVERIFY(!vehicle()->signingController()->signingStatus().enabled);

    QVERIFY(vehicle()->signingController()->signingStatus().statusText.contains("Configuring"));

    QVERIFY_TRUE_WAIT(!vehicle()->signingController()->signingStatus().pending(), TestTimeout::mediumMs() + 5000);
    QVERIFY(!vehicle()->signingController()->signingStatus().enabled);
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    mockLink()->setCommLost(false);
}

void MockLinkSigningTest::_testSigningStatusChangedSignalFiresOnEnable()
{
    QVERIFY(vehicle());
    QVERIFY(mockLink());

    auto* signingKeys = MAVLinkSigningKeys::instance();
    QVERIFY(signingKeys->addKey("SignalKey", "SignalPass"));

    QSignalSpy spy(vehicle()->signingController(), &VehicleSigningController::signingStatusChanged);
    vehicle()->signingController()->enable(QStringLiteral("SignalKey"));

    QTRY_VERIFY_WITH_TIMEOUT(vehicle()->signingController()->signingStatus().enabled, TestTimeout::mediumMs());
    QVERIFY(spy.count() >= 2);
}

// Regression: confirm/fail handlers are wired once per op (Qt::SingleShotConnection), so a fresh enable after a
// completed cycle re-wires rather than relying on a consumed connection. A dropped re-wire would hang the second
// enable; a stale/double wire would surface as a spurious signingFailed. Guards the wire/transmit split in enable().
void MockLinkSigningTest::_testEnableDisableReEnableCycle()
{
    QVERIFY(vehicle());
    QVERIFY(mockLink());

    auto* signingKeys = MAVLinkSigningKeys::instance();
    QVERIFY(signingKeys->addKey("CycleA", "CyclePassphraseA"));
    QVERIFY(signingKeys->addKey("CycleB", "CyclePassphraseB"));

    auto* const sc = vehicle()->signingController();
    QSignalSpy failedSpy(sc, &VehicleSigningController::signingFailed);

    sc->enable(QStringLiteral("CycleA"));
    QVERIFY_TRUE_WAIT(sc->signingStatus().enabled, TestTimeout::mediumMs());
    QCOMPARE(sc->signingStatus().keyName, QStringLiteral("CycleA"));
    QVERIFY(mockLink()->signingEnabled());

    sc->disable();
    QVERIFY_TRUE_WAIT(!sc->signingStatus().enabled, TestTimeout::mediumMs());
    QVERIFY_TRUE_WAIT(!mockLink()->signingEnabled(), TestTimeout::mediumMs());

    sc->enable(QStringLiteral("CycleB"));
    QVERIFY_TRUE_WAIT(sc->signingStatus().enabled, TestTimeout::mediumMs());
    QCOMPARE(sc->signingStatus().keyName, QStringLiteral("CycleB"));
    QVERIFY(mockLink()->signingEnabled());

    QCOMPARE(failedSpy.count(), 0);
}

// Regression: MockLink sub-handlers (MockLinkMissionItemHandler et al) encode replies on MockLink's dedicated
// outgoing channel, whose signing state is independent of QGC's channel. If those replies fell off the signed
// stream while signing is active, QGC would silently drop them and mission transfer would time out. Verify a
// full mission write/read cycle completes with signing enabled.
void MockLinkSigningTest::_testMissionTransferWithSigningEnabled()
{
    QVERIFY(vehicle());
    QVERIFY(mockLink());

    auto* signingKeys = MAVLinkSigningKeys::instance();
    QVERIFY(signingKeys->addKey("MissionKey", "MissionPassphrase"));

    vehicle()->signingController()->enable(QStringLiteral("MissionKey"));
    QVERIFY_TRUE_WAIT(mockLink()->signingEnabled(), TestTimeout::mediumMs());
    QVERIFY_TRUE_WAIT(vehicle()->signingController()->signingStatus().enabled, TestTimeout::mediumMs());

    MissionManager* const missionManager = vehicle()->missionManager();
    QVERIFY(missionManager);

    const auto makeItem = [this](int seq, double lat, double lon, double alt) {
        MissionItem* const item = new MissionItem(this);
        item->setCommand(MAV_CMD_NAV_WAYPOINT);
        item->setFrame(MAV_FRAME_GLOBAL_RELATIVE_ALT);
        item->setParam5(lat);
        item->setParam6(lon);
        item->setParam7(alt);
        item->setSequenceNumber(seq);
        return item;
    };

    // Write: home item at seq 0 plus two waypoints (1-based sequence numbers)
    QList<MissionItem*> missionItems;
    missionItems.append(makeItem(0, 47.3769, 8.549444, 0.0));
    missionItems.append(makeItem(1, 47.3770, 8.5500, 50.0));
    missionItems.append(makeItem(2, 47.3780, 8.5510, 50.0));

    QSignalSpy sendCompleteSpy(missionManager, &MissionManager::sendComplete);
    missionManager->writeMissionItems(missionItems);
    QVERIFY(missionManager->inProgress());
    QTRY_COMPARE_WITH_TIMEOUT(sendCompleteSpy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(sendCompleteSpy.takeFirst().at(0).toBool(), false); // error == false

    // Read back: replies (MISSION_COUNT/MISSION_ITEM_INT/MISSION_ACK) are encoded on the outgoing channel
    QSignalSpy newItemsSpy(missionManager, &MissionManager::newMissionItemsAvailable);
    missionManager->loadFromVehicle();
    QTRY_COMPARE_WITH_TIMEOUT(newItemsSpy.count(), 1, TestTimeout::mediumMs());
    QVERIFY(!missionManager->inProgress());

    int expectedCount = 2; // home item is stripped on read for PX4
    if (mockLink()->getFirmwareType() == MAV_AUTOPILOT_ARDUPILOTMEGA) {
        expectedCount++; // Home position at index 0 comes from vehicle
    }
    QCOMPARE(missionManager->missionItems().count(), expectedCount);

    QVERIFY(mockLink()->signingEnabled());
}

UT_REGISTER_TEST(MockLinkSigningTest, TestLabel::Integration, TestLabel::Vehicle)
