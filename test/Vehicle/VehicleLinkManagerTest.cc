#include "VehicleLinkManagerTest.h"
#include "MockLink.h"
#include "LinkManager.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MultiSignalSpy.h"

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

void VehicleLinkManagerTest::init()
{
    UnitTest::init();

    QCOMPARE_EQ(LinkManager::instance()->links().count(), 0);
    QCOMPARE_EQ(MultiVehicleManager::instance()->vehicles()->count(), 0);
}

void VehicleLinkManagerTest::cleanup()
{
    // Disconnect all links
    if (LinkManager::instance()->links().count()) {
        QSignalSpy spyActiveVehicleChanged(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
        QGC_VERIFY_SPY_VALID(spyActiveVehicleChanged);
        LinkManager::instance()->disconnectAll();
        QVERIFY(spyActiveVehicleChanged.wait(TestHelpers::kShortTimeoutMs));
        QCOMPARE_EQ(MultiVehicleManager::instance()->vehicles()->count(), 0);
        QCOMPARE_EQ(LinkManager::instance()->links().count(), 0);
    }

    UnitTest::cleanup();
}

void VehicleLinkManagerTest::_simpleLinkTest()
{
    SharedLinkConfigurationPtr mockConfig;
    SharedLinkInterfacePtr mockLink;

    QSignalSpy spyVehicleCreate(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    QGC_VERIFY_SPY_VALID(spyVehicleCreate);

    _startMockLink(1, false /*highLatency*/, true /*incrementVehicleId*/, mockConfig, mockLink);
    VERIFY_NOT_NULL(mockConfig.get());
    VERIFY_NOT_NULL(mockLink.get());

    const QSignalSpy spyConfigDelete(mockConfig.get(), &QObject::destroyed);
    const QSignalSpy spyLinkDelete(mockLink.get(), &QObject::destroyed);
    QGC_VERIFY_SPY_VALID(spyConfigDelete);
    QGC_VERIFY_SPY_VALID(spyLinkDelete);

    QVERIFY(spyVehicleCreate.wait(TestHelpers::kShortTimeoutMs));
    QCOMPARE_EQ(MultiVehicleManager::instance()->vehicles()->count(), 1);
    Vehicle *const vehicle = MultiVehicleManager::instance()->activeVehicle();
    VERIFY_NOT_NULL(vehicle);
    QSignalSpy spyVehicleDelete(vehicle, &QObject::destroyed);
    QSignalSpy spyVehicleInitialConnectComplete(vehicle, &Vehicle::initialConnectComplete);
    QGC_VERIFY_SPY_VALID(spyVehicleDelete);
    QGC_VERIFY_SPY_VALID(spyVehicleInitialConnectComplete);

    QCOMPARE_EQ(mockConfig.use_count(), 2L); // Refs: This method, MockLink
    QCOMPARE_EQ(mockLink.use_count(), 3L); // Refs: This method, LinkManager, Vehicle

    // We wait for the full initial connect sequence to complete to catch any ComponentInformationManager bugs
    QVERIFY(spyVehicleInitialConnectComplete.wait(TestHelpers::kDefaultTimeoutMs));

    mockLink->disconnect();

    // Vehicle should go away due to disconnect
    QVERIFY(spyVehicleDelete.wait(TestHelpers::kShortTimeoutMs));

    // Config/Link should still be alive due to the last refs being held by this method
    QCOMPARE_EQ(spyConfigDelete.count(), 0);
    QCOMPARE_EQ(spyLinkDelete.count(), 0);
    QCOMPARE_EQ(mockConfig.use_count(), 2L); // Refs: This method, MockLink
    QCOMPARE_EQ(mockLink.use_count(), 1L); // Refs: This method

    // Let go of our refs from this method and config and link should go away
    mockConfig.reset();
    mockLink.reset();
    QCOMPARE_EQ(mockConfig.use_count(), 0L);
    QCOMPARE_EQ(mockLink.use_count(), 0L);
    QCOMPARE_EQ(spyLinkDelete.count(), 1);
    QCOMPARE_EQ(spyConfigDelete.count(), 1);
}

void VehicleLinkManagerTest::_simpleCommLossTest()
{
    SharedLinkConfigurationPtr mockConfig;
    SharedLinkInterfacePtr mockLink;

    QSignalSpy spyVehicleCreate(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    QGC_VERIFY_SPY_VALID(spyVehicleCreate);

    _startMockLink(1, false /*highLatency*/, true /*incrementVehicleId*/, mockConfig, mockLink);
    MockLink *const pMockLink = qobject_cast<MockLink*>(mockLink.get());
    VERIFY_NOT_NULL(pMockLink);

    QVERIFY(spyVehicleCreate.wait(TestHelpers::kShortTimeoutMs));
    QCOMPARE_EQ(MultiVehicleManager::instance()->vehicles()->count(), 1);
    Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();
    VERIFY_NOT_NULL(vehicle);
    QSignalSpy spyVehicleInitialConnectComplete(vehicle, &Vehicle::initialConnectComplete);
    QGC_VERIFY_SPY_VALID(spyVehicleInitialConnectComplete);
    QVERIFY(spyVehicleInitialConnectComplete.wait(TestHelpers::kDefaultTimeoutMs));

    QSignalSpy spyCommLostChanged(vehicle->vehicleLinkManager(), &VehicleLinkManager::communicationLostChanged);
    QGC_VERIFY_SPY_VALID(spyCommLostChanged);
    pMockLink->setCommLost(true);
    QVERIFY(spyCommLostChanged.wait(VehicleLinkManager::kTestCommLostDetectionTimeoutMs));
    QCOMPARE_EQ(spyCommLostChanged.count(), 1);
    QCOMPARE_EQ(spyCommLostChanged[0][0].toBool(), true);

    spyCommLostChanged.clear();
    pMockLink->setCommLost(false);
    QVERIFY(spyCommLostChanged.wait(VehicleLinkManager::kTestCommLostDetectionTimeoutMs));
    QCOMPARE_EQ(spyCommLostChanged.count(), 1);
    QCOMPARE_EQ(spyCommLostChanged[0][0].toBool(), false);

    spyCommLostChanged.clear();
    vehicle->vehicleLinkManager()->setCommunicationLostEnabled(false);
    pMockLink->setCommLost(true);
    QVERIFY(!spyCommLostChanged.wait(VehicleLinkManager::kTestCommLostDetectionTimeoutMs));

    spyCommLostChanged.clear();
    vehicle->vehicleLinkManager()->setCommunicationLostEnabled(true);
    QVERIFY(spyCommLostChanged.wait(VehicleLinkManager::kTestCommLostDetectionTimeoutMs));
    QCOMPARE_EQ(spyCommLostChanged.count(), 1);
}

void VehicleLinkManagerTest::_multiLinkSingleVehicleTest()
{
    SharedLinkConfigurationPtr mockConfig1;
    SharedLinkInterfacePtr mockLink1;
    SharedLinkConfigurationPtr mockConfig2;
    SharedLinkInterfacePtr mockLink2;

    QSignalSpy spyVehicleCreate(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    QGC_VERIFY_SPY_VALID(spyVehicleCreate);

    _startMockLink(1, false /*highLatency*/, false /*incrementVehicleId*/, mockConfig1, mockLink1);
    _startMockLink(2, false /*highLatency*/, false /*incrementVehicleId*/, mockConfig2, mockLink2);

    QVERIFY(spyVehicleCreate.wait(TestHelpers::kShortTimeoutMs));
    QCOMPARE_EQ(MultiVehicleManager::instance()->vehicles()->count(), 1);
    Vehicle *const vehicle = MultiVehicleManager::instance()->activeVehicle();
    VERIFY_NOT_NULL(vehicle);
    VehicleLinkManager *const vehicleLinkManager = vehicle->vehicleLinkManager();
    VERIFY_NOT_NULL(vehicleLinkManager);
    QSignalSpy spyVehicleInitialConnectComplete(vehicle, &Vehicle::initialConnectComplete);
    QGC_VERIFY_SPY_VALID(spyVehicleInitialConnectComplete);
    QVERIFY(spyVehicleInitialConnectComplete.wait(TestHelpers::kDefaultTimeoutMs));

    // The first link to start sending a heartbeat will be the primary link.
    // Depending on how the thread scheduling works, that could be the mockLink2.
    const SharedLinkInterfacePtr primaryLink = vehicleLinkManager->primaryLink().lock();
    QVERIFY(primaryLink == mockLink1 || primaryLink == mockLink2);
    MockLink *pMockLink1 = qobject_cast<MockLink*>(mockLink1.get());
    MockLink *pMockLink2 = qobject_cast<MockLink*>(mockLink2.get());
    if (primaryLink == mockLink2) {
        std::swap(pMockLink1, pMockLink2);
    }

    const QStringList rgNames = vehicleLinkManager->linkNames();
    QStringList rgStatus = vehicleLinkManager->linkStatuses();
    QCOMPARE_EQ(rgNames.count(), 2);
    QCOMPARE_EQ(rgNames[0], mockConfig1->name());
    QCOMPARE_EQ(rgNames[1], mockConfig2->name());
    QCOMPARE_EQ(rgStatus.count(), 2);
    QGC_VERIFY_EMPTY(rgStatus[0]);
    QGC_VERIFY_EMPTY(rgStatus[1]);

    MultiSignalSpy multiSpy;
    QVERIFY(multiSpy.init(vehicleLinkManager));

    // Comm lost on 2: 1 is primary, 2 is secondary so comm loss/regain on 2 should only update status text

    pMockLink2->setCommLost(true);
    QCOMPARE_EQ(multiSpy.waitForSignal (_linkStatusesChangedSignalName, VehicleLinkManager::kTestCommLostDetectionTimeoutMs), true);
    QVERIFY(multiSpy.checkOnlySignalByMask(multiSpy.mask(_linkStatusesChangedSignalName)));

    rgStatus = vehicleLinkManager->linkStatuses();
    QCOMPARE_EQ(rgStatus.count(), 2);
    QGC_VERIFY_EMPTY(rgStatus[0]);
    QGC_VERIFY_NOT_EMPTY(rgStatus[1]);

    multiSpy.clearAllSignals();

    pMockLink2->setCommLost(false);
    QCOMPARE_EQ(multiSpy.waitForSignal (_linkStatusesChangedSignalName, VehicleLinkManager::kTestCommLostDetectionTimeoutMs), true);
    QVERIFY(multiSpy.checkOnlySignalByMask(multiSpy.mask(_linkStatusesChangedSignalName)));

    rgStatus = vehicleLinkManager->linkStatuses();
    QCOMPARE_EQ(rgStatus.count(), 2);
    QGC_VERIFY_EMPTY(rgStatus[0]);
    QGC_VERIFY_EMPTY(rgStatus[1]);

    multiSpy.clearAllSignals();

    // Comm loss on 1: 1 is primary so should trigger switch of primary to 2

    pMockLink1->setCommLost(true);
    QCOMPARE_EQ(multiSpy.waitForSignal (_primaryLinkChangedSignalName, VehicleLinkManager::kTestCommLostDetectionTimeoutMs), true);
    const quint64 signalMask = multiSpy.mask(_primaryLinkChangedSignalName) | multiSpy.mask(_linkStatusesChangedSignalName);
    QVERIFY(multiSpy.checkOnlySignalByMask(signalMask));
    QCOMPARE_EQ(vehicleLinkManager->primaryLink().lock().get(), pMockLink2);

    rgStatus = vehicleLinkManager->linkStatuses();
    QCOMPARE_EQ(rgStatus.count(), 2);
    QGC_VERIFY_NOT_EMPTY(rgStatus[0]);
    QGC_VERIFY_EMPTY(rgStatus[1]);

    multiSpy.clearAllSignals();

    // Comm regained on 1 should leave 2 as primary and only update status

    pMockLink1->setCommLost(false);
    QCOMPARE_EQ(multiSpy.waitForSignal (_linkStatusesChangedSignalName, VehicleLinkManager::kTestCommLostDetectionTimeoutMs), true);
    QVERIFY(multiSpy.checkOnlySignalByMask(multiSpy.mask(_linkStatusesChangedSignalName)));
    QCOMPARE_EQ(vehicleLinkManager->primaryLink().lock().get(), pMockLink2);

    rgStatus = vehicleLinkManager->linkStatuses();
    QCOMPARE_EQ(rgStatus.count(), 2);
    QGC_VERIFY_EMPTY(rgStatus[0]);
    QGC_VERIFY_EMPTY(rgStatus[1]);

    multiSpy.clearAllSignals();
}

void VehicleLinkManagerTest::_connectionRemovedTest()
{
    SharedLinkConfigurationPtr mockConfig;
    SharedLinkInterfacePtr mockLink;

    QSignalSpy spyVehicleCreate(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    QGC_VERIFY_SPY_VALID(spyVehicleCreate);

    _startMockLink(1, false /*highLatency*/, true /*incrementVehicleId*/, mockConfig, mockLink);
    MockLink *const pMockLink = qobject_cast<MockLink*>(mockLink.get());
    VERIFY_NOT_NULL(pMockLink);

    QVERIFY(spyVehicleCreate.wait(TestHelpers::kShortTimeoutMs));
    Vehicle *const vehicle = MultiVehicleManager::instance()->activeVehicle();
    VERIFY_NOT_NULL(vehicle);
    QSignalSpy spyVehicleInitialConnectComplete(vehicle, &Vehicle::initialConnectComplete);
    QGC_VERIFY_SPY_VALID(spyVehicleInitialConnectComplete);
    QVERIFY(spyVehicleInitialConnectComplete.wait(TestHelpers::kDefaultTimeoutMs));

    QSignalSpy spyCommLostChanged(vehicle->vehicleLinkManager(), &VehicleLinkManager::communicationLostChanged);
    QGC_VERIFY_SPY_VALID(spyCommLostChanged);

    // Connection removed should just signal communication lost

    pMockLink->simulateConnectionRemoved();
    QVERIFY(spyCommLostChanged.wait(VehicleLinkManager::kTestCommLostDetectionTimeoutMs));
    QCOMPARE_EQ(spyCommLostChanged.count(), 1);
    QCOMPARE_EQ(spyCommLostChanged[0][0].toBool(), true);
}

void VehicleLinkManagerTest::_highLatencyLinkTest()
{
    SharedLinkConfigurationPtr mockConfig1;
    SharedLinkInterfacePtr mockLink1;
    SharedLinkConfigurationPtr mockConfig2;
    SharedLinkInterfacePtr mockLink2;

    QSignalSpy spyVehicleCreate(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    QGC_VERIFY_SPY_VALID(spyVehicleCreate);

    _startMockLink(1, true /*highLatency*/, false /*incrementVehicleId*/, mockConfig1, mockLink1);
    MockLink *const pMockLink1 = qobject_cast<MockLink*>(mockLink1.get());
    VERIFY_NOT_NULL(pMockLink1);

    QVERIFY(spyVehicleCreate.wait(TestHelpers::kShortTimeoutMs));
    QCOMPARE_EQ(MultiVehicleManager::instance()->vehicles()->count(), 1);
    Vehicle *const vehicle = MultiVehicleManager::instance()->activeVehicle();
    VERIFY_NOT_NULL(vehicle);
    VehicleLinkManager *const vehicleLinkManager = vehicle->vehicleLinkManager();
    VERIFY_NOT_NULL(vehicleLinkManager);

    // Wait for initial connect to complete before testing link switching
    QSignalSpy spyVehicleInitialConnectComplete(vehicle, &Vehicle::initialConnectComplete);
    QGC_VERIFY_SPY_VALID(spyVehicleInitialConnectComplete);
    if (!vehicle->isInitialConnectComplete()) {
        QVERIFY(spyVehicleInitialConnectComplete.wait(TestHelpers::kDefaultTimeoutMs));
    }

    MultiSignalSpy multiSpyVLM;
    QVERIFY(multiSpyVLM.init(vehicleLinkManager));

    // Addition of second non high latency link should:
    //  Change primary link from 1 to 2
    //  Stop high latency transmission on 1

    QSignalSpy spyTransmissionEnabledChanged(pMockLink1, &MockLink::highLatencyTransmissionEnabledChanged);
    QGC_VERIFY_SPY_VALID(spyTransmissionEnabledChanged);
    _startMockLink(2, false /*highLatency*/, false /*incrementVehicleId*/, mockConfig2, mockLink2);
    MockLink* pMockLink2 = qobject_cast<MockLink*>(mockLink2.get());
    VERIFY_NOT_NULL(pMockLink2);

    QVERIFY(multiSpyVLM.waitForSignal(_primaryLinkChangedSignalName, TestHelpers::kShortTimeoutMs));
    QCOMPARE_EQ(vehicleLinkManager->primaryLink().lock().get(), pMockLink2);
    // Wait for high latency transmission to be disabled - may arrive after primary link change
    if (spyTransmissionEnabledChanged.count() == 0) {
        QVERIFY(spyTransmissionEnabledChanged.wait(TestHelpers::kShortTimeoutMs));
    }
    QCOMPARE_EQ(spyTransmissionEnabledChanged.count(), 1);
    QCOMPARE_EQ(spyTransmissionEnabledChanged.takeFirst()[0].toBool(), false);
    multiSpyVLM.clearAllSignals();
    spyTransmissionEnabledChanged.clear();

    // Comm lost on primary:2 should:
    //  Switch primary to 1
    //  Re-enable high latency transmission on 1

    pMockLink2->setCommLost(true);
    QVERIFY(multiSpyVLM.waitForSignal(_primaryLinkChangedSignalName, VehicleLinkManager::kTestCommLostDetectionTimeoutMs));
    QCOMPARE_EQ(vehicleLinkManager->primaryLink().lock().get(), pMockLink1);
    // Wait for high latency transmission to be re-enabled - may arrive after primary link change
    if (spyTransmissionEnabledChanged.count() == 0) {
        QVERIFY(spyTransmissionEnabledChanged.wait(TestHelpers::kShortTimeoutMs));
    }
    QCOMPARE_EQ(spyTransmissionEnabledChanged.count(), 1);
    QCOMPARE_EQ(spyTransmissionEnabledChanged.takeFirst()[0].toBool(), true);
    spyTransmissionEnabledChanged.clear();
}

void VehicleLinkManagerTest::_startMockLink(int mockIndex, bool highLatency, bool incrementVehicleId, SharedLinkConfigurationPtr &mockConfig, SharedLinkInterfacePtr &mockLink)
{
    MockConfiguration *const pMockConfig = new MockConfiguration(QStringLiteral("Mock %1").arg(mockIndex));
    VERIFY_NOT_NULL(pMockConfig);

    mockConfig = SharedLinkConfigurationPtr(pMockConfig);

    pMockConfig->setDynamic(true);
    pMockConfig->setHighLatency(highLatency);
    pMockConfig->setIncrementVehicleId(incrementVehicleId);

    QVERIFY(LinkManager::instance()->createConnectedLink(mockConfig));
    VERIFY_NOT_NULL(mockConfig->link());

    mockLink = LinkManager::instance()->sharedLinkInterfacePointerForLink(mockConfig->link());
    VERIFY_NOT_NULL(mockLink.get());
}
