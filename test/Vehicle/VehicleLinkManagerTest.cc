#include "VehicleLinkManagerTest.h"

#include <QtTest/QSignalSpy>

#include "LinkManager.h"
#include "MultiSignalSpy.h"
#include "MultiVehicleManager.h"
#include "UnitTest.h"
#include "Vehicle.h"

void VehicleLinkManagerTest::_simpleLinkTest()
{
    SharedLinkConfigurationPtr mockConfig;
    SharedLinkInterfacePtr mockLink;
    _startMockLink(1, false /*highLatency*/, true /*incrementVehicleId*/, mockConfig, mockLink);
    QVERIFY(mockConfig);
    QVERIFY(mockLink);
    const QSignalSpy spyConfigDelete(mockConfig.get(), &QObject::destroyed);
    const QSignalSpy spyLinkDelete(mockLink.get(), &QObject::destroyed);
    QVERIFY(spyConfigDelete.isValid());
    QVERIFY(spyLinkDelete.isValid());
    Vehicle* const vehicle = waitForVehicleConnect(TestTimeout::shortMs());
    QVERIFY(vehicle);
    QVERIFY_TRUE_WAIT(MultiVehicleManager::instance()->vehicles()->count() == 1, TestTimeout::shortMs());
    QSignalSpy spyVehicleDelete(vehicle, &QObject::destroyed);
    QSignalSpy spyVehicleInitialConnectComplete(vehicle, &Vehicle::initialConnectComplete);
    QCOMPARE(mockConfig.use_count(), 2);  // Refs: This method, MockLink
    QCOMPARE(mockLink.use_count(), 3);    // Refs: This method, LinkManager, Vehicle
    // We wait for the full initial connect sequence to complete to catch anby ComponentInformationManager bugs
    QVERIFY_TRUE_WAIT(spyVehicleInitialConnectComplete.count() > 0 || vehicle->isInitialConnectComplete(),
                      TestTimeout::mediumMs());
    // Drain queued command traffic before disconnect to avoid racing pending writes with link teardown.
    UnitTest::settleEventLoopForCleanup(2, 10);
    mockLink->disconnect();
    // Vehicle should go away due to disconnect
    QVERIFY_SIGNAL_WAIT(spyVehicleDelete, TestTimeout::shortMs());
    // Config/Link should still be alive due to the last refs being held by this method
    QCOMPARE(spyConfigDelete.count(), 0);
    QCOMPARE(spyLinkDelete.count(), 0);
    QCOMPARE(mockConfig.use_count(), 2);  // Refs: This method, MockLink
    QCOMPARE(mockLink.use_count(), 1);    // Refs: This method
    // Let go of our refs from this method and config and link should go away
    mockConfig.reset();
    mockLink.reset();
    QCOMPARE(mockConfig.use_count(), 0);
    QCOMPARE(mockLink.use_count(), 0);
    QCOMPARE(spyLinkDelete.count(), 1);
    QCOMPARE(spyConfigDelete.count(), 1);
}

void VehicleLinkManagerTest::_simpleCommLossTest()
{
    SharedLinkConfigurationPtr mockConfig;
    SharedLinkInterfacePtr mockLink;
    _startMockLink(1, false /*highLatency*/, true /*incrementVehicleId*/, mockConfig, mockLink);
    MockLink* const pMockLink = qobject_cast<MockLink*>(mockLink.get());
    Vehicle* const vehicle = waitForVehicleConnect(TestTimeout::shortMs());
    QVERIFY(vehicle);
    QVERIFY_TRUE_WAIT(MultiVehicleManager::instance()->vehicles()->count() == 1, TestTimeout::mediumMs());
    QSignalSpy spyVehicleInitialConnectComplete(vehicle, &Vehicle::initialConnectComplete);
    QVERIFY_TRUE_WAIT(spyVehicleInitialConnectComplete.count() > 0 || vehicle->isInitialConnectComplete(),
                      TestTimeout::mediumMs());
    QSignalSpy spyCommLostChanged(vehicle->vehicleLinkManager(), &VehicleLinkManager::communicationLostChanged);
    pMockLink->setCommLost(true);
    QVERIFY_SIGNAL_WAIT(spyCommLostChanged, VehicleLinkManager::kTestCommLostDetectionTimeoutMs);
    QCOMPARE(spyCommLostChanged.count(), 1);
    QCOMPARE(spyCommLostChanged[0][0].toBool(), true);
    spyCommLostChanged.clear();
    pMockLink->setCommLost(false);
    QVERIFY_SIGNAL_WAIT(spyCommLostChanged, VehicleLinkManager::kTestCommLostDetectionTimeoutMs);
    QCOMPARE(spyCommLostChanged.count(), 1);
    QCOMPARE(spyCommLostChanged[0][0].toBool(), false);
    spyCommLostChanged.clear();
    vehicle->vehicleLinkManager()->setCommunicationLostEnabled(false);
    pMockLink->setCommLost(true);
    QVERIFY_NO_SIGNAL_WAIT(spyCommLostChanged, VehicleLinkManager::kTestCommLostDetectionTimeoutMs);
    spyCommLostChanged.clear();
    vehicle->vehicleLinkManager()->setCommunicationLostEnabled(true);
    QVERIFY_SIGNAL_WAIT(spyCommLostChanged, VehicleLinkManager::kTestCommLostDetectionTimeoutMs);
    QCOMPARE(spyCommLostChanged.count(), 1);
}

void VehicleLinkManagerTest::_multiLinkSingleVehicleTest()
{
    SharedLinkConfigurationPtr mockConfig1;
    SharedLinkInterfacePtr mockLink1;
    SharedLinkConfigurationPtr mockConfig2;
    SharedLinkInterfacePtr mockLink2;
    _startMockLink(1, false /*highLatency*/, false /*incrementVehicleId*/, mockConfig1, mockLink1);
    _startMockLink(2, false /*highLatency*/, false /*incrementVehicleId*/, mockConfig2, mockLink2);
    Vehicle* const vehicle = waitForVehicleConnect(TestTimeout::shortMs());
    QVERIFY(vehicle);
    QVERIFY_TRUE_WAIT(MultiVehicleManager::instance()->vehicles()->count() == 1, TestTimeout::mediumMs());
    VehicleLinkManager* const vehicleLinkManager = vehicle->vehicleLinkManager();
    QVERIFY(vehicleLinkManager);
    QSignalSpy spyVehicleInitialConnectComplete(vehicle, &Vehicle::initialConnectComplete);
    QVERIFY_TRUE_WAIT(spyVehicleInitialConnectComplete.count() > 0 || vehicle->isInitialConnectComplete(),
                      TestTimeout::mediumMs());
    // The first link to start sending a heartbeat will be the primary link.
    // Depending on how the thread scheduling works, that could be the mockLink2.
    const SharedLinkInterfacePtr primaryLink = vehicleLinkManager->primaryLink().lock();
    QVERIFY(primaryLink == mockLink1 || primaryLink == mockLink2);
    MockLink* pMockLink1 = qobject_cast<MockLink*>(mockLink1.get());
    MockLink* pMockLink2 = qobject_cast<MockLink*>(mockLink2.get());
    if (primaryLink == mockLink2) {
        std::swap(pMockLink1, pMockLink2);
    }
    const QStringList rgNames = vehicleLinkManager->linkNames();
    QStringList rgStatus = vehicleLinkManager->linkStatuses();
    QCOMPARE(rgNames.count(), 2);
    QCOMPARE(rgNames[0], mockConfig1->name());
    QCOMPARE(rgNames[1], mockConfig2->name());
    QCOMPARE(rgStatus.count(), 2);
    QVERIFY(rgStatus[0].isEmpty());
    QVERIFY(rgStatus[1].isEmpty());
    MultiSignalSpy multiSpy;
    QVERIFY(multiSpy.init(vehicleLinkManager));
    // Comm lost on 2: 1 is primary, 2 is secondary so comm loss/regain on 2 should only update status text
    pMockLink2->setCommLost(true);
    QCOMPARE(multiSpy.waitForSignal(_linkStatusesChangedSignalName, VehicleLinkManager::kTestCommLostDetectionTimeoutMs),
             true);
    QVERIFY(multiSpy.onlyEmittedOnce(_linkStatusesChangedSignalName));
    rgStatus = vehicleLinkManager->linkStatuses();
    QCOMPARE(rgStatus.count(), 2);
    QVERIFY(rgStatus[0].isEmpty());
    QVERIFY(!rgStatus[1].isEmpty());
    multiSpy.clearAllSignals();
    pMockLink2->setCommLost(false);
    QCOMPARE(multiSpy.waitForSignal(_linkStatusesChangedSignalName, VehicleLinkManager::kTestCommLostDetectionTimeoutMs),
             true);
    QVERIFY(multiSpy.onlyEmittedOnce(_linkStatusesChangedSignalName));
    rgStatus = vehicleLinkManager->linkStatuses();
    QCOMPARE(rgStatus.count(), 2);
    QVERIFY(rgStatus[0].isEmpty());
    QVERIFY(rgStatus[1].isEmpty());
    multiSpy.clearAllSignals();
    // Comm loss on 1: 1 is primary so should trigger switch of primary to 2
    pMockLink1->setCommLost(true);
    QCOMPARE(multiSpy.waitForSignal(_primaryLinkChangedSignalName, VehicleLinkManager::kTestCommLostDetectionTimeoutMs),
             true);
    QVERIFY(
        multiSpy.onlyEmittedOnceByMask(multiSpy.mask(_primaryLinkChangedSignalName, _linkStatusesChangedSignalName)));
    QCOMPARE(pMockLink2, vehicleLinkManager->primaryLink().lock().get());
    rgStatus = vehicleLinkManager->linkStatuses();
    QCOMPARE(rgStatus.count(), 2);
    QVERIFY(!rgStatus[0].isEmpty());
    QVERIFY(rgStatus[1].isEmpty());
    multiSpy.clearAllSignals();
    // Comm regained on 1 should leave 2 as primary and only update status
    pMockLink1->setCommLost(false);
    QCOMPARE(multiSpy.waitForSignal(_linkStatusesChangedSignalName, VehicleLinkManager::kTestCommLostDetectionTimeoutMs),
             true);
    QVERIFY(multiSpy.onlyEmittedOnce(_linkStatusesChangedSignalName));
    QCOMPARE(pMockLink2, vehicleLinkManager->primaryLink().lock().get());
    rgStatus = vehicleLinkManager->linkStatuses();
    QCOMPARE(rgStatus.count(), 2);
    QVERIFY(rgStatus[0].isEmpty());
    QVERIFY(rgStatus[1].isEmpty());
    multiSpy.clearAllSignals();
}

void VehicleLinkManagerTest::_connectionRemovedTest()
{
    SharedLinkConfigurationPtr mockConfig;
    SharedLinkInterfacePtr mockLink;
    _startMockLink(1, false /*highLatency*/, true /*incrementVehicleId*/, mockConfig, mockLink);
    MockLink* const pMockLink = qobject_cast<MockLink*>(mockLink.get());
    Vehicle* const vehicle = waitForVehicleConnect(TestTimeout::mediumMs());
    QVERIFY(vehicle);
    QVERIFY_TRUE_WAIT(MultiVehicleManager::instance()->vehicles()->count() == 1, TestTimeout::mediumMs());
    QSignalSpy spyVehicleInitialConnectComplete(vehicle, &Vehicle::initialConnectComplete);
    QVERIFY_TRUE_WAIT(spyVehicleInitialConnectComplete.count() > 0 || vehicle->isInitialConnectComplete(),
                      TestTimeout::mediumMs());
    QSignalSpy spyCommLostChanged(vehicle->vehicleLinkManager(), &VehicleLinkManager::communicationLostChanged);
    // Connection removed should just signal communication lost
    pMockLink->simulateConnectionRemoved();
    QVERIFY_SIGNAL_WAIT(spyCommLostChanged, VehicleLinkManager::kTestCommLostDetectionTimeoutMs);
    QCOMPARE(spyCommLostChanged.count(), 1);
    QCOMPARE(spyCommLostChanged[0][0].toBool(), true);
}

void VehicleLinkManagerTest::_highLatencyLinkTest()
{
    SharedLinkConfigurationPtr mockConfig1;
    SharedLinkInterfacePtr mockLink1;
    SharedLinkConfigurationPtr mockConfig2;
    SharedLinkInterfacePtr mockLink2;
    _startMockLink(1, true /*highLatency*/, false /*incrementVehicleId*/, mockConfig1, mockLink1);
    MockLink* const pMockLink1 = qobject_cast<MockLink*>(mockLink1.get());
    Vehicle* const vehicle = waitForVehicleConnect(TestTimeout::mediumMs());
    QVERIFY(vehicle);
    QVERIFY_TRUE_WAIT(MultiVehicleManager::instance()->vehicles()->count() == 1, TestTimeout::mediumMs());
    VehicleLinkManager* const vehicleLinkManager = vehicle->vehicleLinkManager();
    QVERIFY(vehicleLinkManager);
    MultiSignalSpy multiSpyVLM;
    QVERIFY(multiSpyVLM.init(vehicleLinkManager));
    // Addition of second non high latency link should:
    //  Change primary link from 1 to 2
    //  Stop high latency transmission on 1
    QSignalSpy spyTransmissionEnabledChanged(pMockLink1, &MockLink::highLatencyTransmissionEnabledChanged);
    QVERIFY(spyTransmissionEnabledChanged.isValid());
    _startMockLink(2, false /*highLatency*/, false /*incrementVehicleId*/, mockConfig2, mockLink2);
    MockLink* pMockLink2 = qobject_cast<MockLink*>(mockLink2.get());
    QCOMPARE(multiSpyVLM.waitForSignal(_primaryLinkChangedSignalName, TestTimeout::shortMs()), true);
    QCOMPARE(pMockLink2, vehicleLinkManager->primaryLink().lock().get());
    // Wait for the MAV_CMD_CONTROL_HIGH_LATENCY command to be processed
    if (spyTransmissionEnabledChanged.count() == 0) {
        QVERIFY_SIGNAL_WAIT(spyTransmissionEnabledChanged, TestTimeout::shortMs());
    }
    QCOMPARE(spyTransmissionEnabledChanged.count(), 1);
    QCOMPARE(spyTransmissionEnabledChanged.takeFirst()[0].toBool(), false);
    multiSpyVLM.clearAllSignals();
    spyTransmissionEnabledChanged.clear();
    // Comm lost on primary:2 should:
    //  Switch primary to 1
    //  Re-enable high latency transmission on 1
    pMockLink2->setCommLost(true);
    QCOMPARE(
        multiSpyVLM.waitForSignal(_primaryLinkChangedSignalName, VehicleLinkManager::kTestCommLostDetectionTimeoutMs),
        true);
    QCOMPARE(pMockLink1, vehicleLinkManager->primaryLink().lock().get());
    // Wait for the MAV_CMD_CONTROL_HIGH_LATENCY command to be processed
    if (spyTransmissionEnabledChanged.count() == 0) {
        QVERIFY_SIGNAL_WAIT(spyTransmissionEnabledChanged, TestTimeout::shortMs());
    }
    QCOMPARE(spyTransmissionEnabledChanged.count(), 1);
    QCOMPARE(spyTransmissionEnabledChanged.takeFirst()[0].toBool(), true);
    spyTransmissionEnabledChanged.clear();
}

void VehicleLinkManagerTest::_startMockLink(int mockIndex, bool highLatency, bool incrementVehicleId,
                                            SharedLinkConfigurationPtr& mockConfig, SharedLinkInterfacePtr& mockLink)
{
    MockConfiguration* const pMockConfig = new MockConfiguration(QStringLiteral("Mock %1").arg(mockIndex));
    mockConfig = SharedLinkConfigurationPtr(pMockConfig);
    pMockConfig->setDynamic(true);
    pMockConfig->setHighLatency(highLatency);
    pMockConfig->setIncrementVehicleId(incrementVehicleId);
    QVERIFY(linkManager()->createConnectedLink(mockConfig));
    QVERIFY(mockConfig->link());
    mockLink = linkManager()->sharedLinkInterfacePointerForLink(mockConfig->link());
    QVERIFY(mockLink);
}

UT_REGISTER_TEST(VehicleLinkManagerTest, TestLabel::Integration, TestLabel::Vehicle)
