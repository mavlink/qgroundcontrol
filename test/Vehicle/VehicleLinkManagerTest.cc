/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleLinkManagerTest.h"
#include "MockLink.h"
#include "LinkManager.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MultiSignalSpyV2.h"

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

void VehicleLinkManagerTest::init()
{
    UnitTest::init();

    QCOMPARE(LinkManager::instance()->links().count(), 0);
    QCOMPARE(MultiVehicleManager::instance()->vehicles()->count(), 0);
}

void VehicleLinkManagerTest::cleanup()
{
    // Disconnect all links
    if (LinkManager::instance()->links().count()) {
        QSignalSpy spyActiveVehicleChanged(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
        LinkManager::instance()->disconnectAll();
        QCOMPARE(spyActiveVehicleChanged.wait(1000),    true);
        QCOMPARE(MultiVehicleManager::instance()->vehicles()->count(), 0);
        QCOMPARE(LinkManager::instance()->links().count(),         0);
    }

    UnitTest::cleanup();
}

void VehicleLinkManagerTest::_simpleLinkTest()
{
    SharedLinkConfigurationPtr mockConfig;
    SharedLinkInterfacePtr mockLink;

    QSignalSpy spyVehicleCreate(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);

    _startMockLink(1, false /*highLatency*/, true /*incrementVehicleId*/, mockConfig, mockLink);
    QVERIFY(mockConfig);
    QVERIFY(mockLink);

    const QSignalSpy spyConfigDelete(mockConfig.get(), &QObject::destroyed);
    const QSignalSpy spyLinkDelete(mockLink.get(), &QObject::destroyed);
    QVERIFY(spyConfigDelete.isValid());
    QVERIFY(spyLinkDelete.isValid());

    QCOMPARE(spyVehicleCreate.wait(1000), true);
    QCOMPARE(MultiVehicleManager::instance()->vehicles()->count(), 1);
    Vehicle *const vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(vehicle);
    QSignalSpy spyVehicleDelete(vehicle, &QObject::destroyed);
    QSignalSpy spyVehicleInitialConnectComplete(vehicle, &Vehicle::initialConnectComplete);

    QCOMPARE(mockConfig.use_count(), 2); // Refs: This method, MockLink
    QCOMPARE(mockLink.use_count(), 3); // Refs: This method, LinkManager, Vehicle

    // We wait for the full initial connect sequence to complete to catch anby ComponentInformationManager bugs
    QCOMPARE(spyVehicleInitialConnectComplete.wait(3000), true);

    mockLink->disconnect();

    // Vehicle should go away due to disconnect
    QCOMPARE(spyVehicleDelete.wait(500), true);

    // Config/Link should still be alive due to the last refs being held by this method
    QCOMPARE(spyConfigDelete.count(), 0);
    QCOMPARE(spyLinkDelete.count(), 0);
    QCOMPARE(mockConfig.use_count(), 2); // Refs: This method, MockLink
    QCOMPARE(mockLink.use_count(), 1); // Refs: This method

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

    QSignalSpy spyVehicleCreate(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);

    _startMockLink(1, false /*highLatency*/, true /*incrementVehicleId*/, mockConfig, mockLink);
    MockLink *const pMockLink = qobject_cast<MockLink*>(mockLink.get());

    QCOMPARE(spyVehicleCreate.wait(1000), true);
    QCOMPARE(MultiVehicleManager::instance()->vehicles()->count(), 1);
    Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(vehicle);
    QSignalSpy spyVehicleInitialConnectComplete(vehicle, &Vehicle::initialConnectComplete);
    QCOMPARE(spyVehicleInitialConnectComplete.wait(3000), true);

    QSignalSpy spyCommLostChanged(vehicle->vehicleLinkManager(), &VehicleLinkManager::communicationLostChanged);
    pMockLink->setCommLost(true);
    QCOMPARE(spyCommLostChanged.wait(VehicleLinkManager::_heartbeatMaxElpasedMSecs * 2), true);
    QCOMPARE(spyCommLostChanged.count(), 1);
    QCOMPARE(spyCommLostChanged[0][0].toBool(), true);

    spyCommLostChanged.clear();
    pMockLink->setCommLost(false);
    QCOMPARE(spyCommLostChanged.wait(VehicleLinkManager::_heartbeatMaxElpasedMSecs * 2), true);
    QCOMPARE(spyCommLostChanged.count(), 1);
    QCOMPARE(spyCommLostChanged[0][0].toBool(), false);

    spyCommLostChanged.clear();
    vehicle->vehicleLinkManager()->setCommunicationLostEnabled(false);
    pMockLink->setCommLost(true);
    QCOMPARE(spyCommLostChanged.wait(VehicleLinkManager::_heartbeatMaxElpasedMSecs * 2), false);

    spyCommLostChanged.clear();
    vehicle->vehicleLinkManager()->setCommunicationLostEnabled(true);
    QCOMPARE(spyCommLostChanged.wait(VehicleLinkManager::_heartbeatMaxElpasedMSecs * 2), true);
    QCOMPARE(spyCommLostChanged.count(), 1);
}

void VehicleLinkManagerTest::_multiLinkSingleVehicleTest()
{
    SharedLinkConfigurationPtr mockConfig1;
    SharedLinkInterfacePtr mockLink1;
    SharedLinkConfigurationPtr mockConfig2;
    SharedLinkInterfacePtr mockLink2;

    QSignalSpy spyVehicleCreate(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);

    _startMockLink(1, false /*highLatency*/, false /*incrementVehicleId*/, mockConfig1, mockLink1);
    _startMockLink(2, false /*highLatency*/, false /*incrementVehicleId*/, mockConfig2, mockLink2);

    QCOMPARE(spyVehicleCreate.wait(1000), true);
    QCOMPARE(MultiVehicleManager::instance()->vehicles()->count(), 1);
    Vehicle *const vehicle = MultiVehicleManager::instance()->activeVehicle();
    VehicleLinkManager *const vehicleLinkManager = vehicle->vehicleLinkManager();
    QVERIFY(vehicle);
    QVERIFY(vehicleLinkManager);
    QSignalSpy spyVehicleInitialConnectComplete(vehicle, &Vehicle::initialConnectComplete);
    QCOMPARE(spyVehicleInitialConnectComplete.wait(3000), true);

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
    QCOMPARE(rgNames.count(), 2);
    QCOMPARE(rgNames[0], mockConfig1->name());
    QCOMPARE(rgNames[1], mockConfig2->name());
    QCOMPARE(rgStatus.count(), 2);
    QVERIFY(rgStatus[0].isEmpty());
    QVERIFY(rgStatus[1].isEmpty());

    MultiSignalSpyV2 multiSpy;
    QVERIFY(multiSpy.init(vehicleLinkManager));

    // Comm lost on 2: 1 is primary, 2 is secondary so comm loss/regain on 2 should only update status text

    pMockLink2->setCommLost(true);
    QCOMPARE(multiSpy.waitForSignal (_linkStatusesChangedSignalName, VehicleLinkManager::_heartbeatMaxElpasedMSecs * 2), true);
    QVERIFY(multiSpy.checkOnlySignalByMask(multiSpy.signalNameToMask(_linkStatusesChangedSignalName)));

    rgStatus = vehicleLinkManager->linkStatuses();
    QCOMPARE(rgStatus.count(), 2);
    QVERIFY(rgStatus[0].isEmpty());
    QVERIFY(!rgStatus[1].isEmpty());

    multiSpy.clearAllSignals();

    pMockLink2->setCommLost(false);
    QCOMPARE(multiSpy.waitForSignal (_linkStatusesChangedSignalName, VehicleLinkManager::_heartbeatMaxElpasedMSecs * 2), true);
    QVERIFY(multiSpy.checkOnlySignalByMask(multiSpy.signalNameToMask(_linkStatusesChangedSignalName)));

    rgStatus = vehicleLinkManager->linkStatuses();
    QCOMPARE(rgStatus.count(), 2);
    QVERIFY(rgStatus[0].isEmpty());
    QVERIFY(rgStatus[1].isEmpty());

    multiSpy.clearAllSignals();

    // Comm loss on 1: 1 is primary so should trigger switch of primary to 2

    pMockLink1->setCommLost(true);
    QCOMPARE(multiSpy.waitForSignal (_primaryLinkChangedSignalName, VehicleLinkManager::_heartbeatMaxElpasedMSecs * 2), true);
    const quint32 signalMask = multiSpy.signalNameToMask(_primaryLinkChangedSignalName) | multiSpy.signalNameToMask(_linkStatusesChangedSignalName);
    QVERIFY(multiSpy.checkOnlySignalByMask(signalMask));
    QCOMPARE(pMockLink2,vehicleLinkManager->primaryLink().lock().get());

    rgStatus = vehicleLinkManager->linkStatuses();
    QCOMPARE(rgStatus.count(), 2);
    QVERIFY(!rgStatus[0].isEmpty());
    QVERIFY(rgStatus[1].isEmpty());

    multiSpy.clearAllSignals();

    // Comm regained on 1 should leave 2 as primary and only update status

    pMockLink1->setCommLost(false);
    QCOMPARE(multiSpy.waitForSignal (_linkStatusesChangedSignalName, VehicleLinkManager::_heartbeatMaxElpasedMSecs * 2),    true);
    QVERIFY(multiSpy.checkOnlySignalByMask(multiSpy.signalNameToMask(_linkStatusesChangedSignalName)));
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

    QSignalSpy spyVehicleCreate(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);

    _startMockLink(1, false /*highLatency*/, true /*incrementVehicleId*/, mockConfig, mockLink);
    MockLink *const pMockLink = qobject_cast<MockLink*>(mockLink.get());

    QCOMPARE(spyVehicleCreate.wait(1000), true);
    Vehicle *const vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(vehicle);
    QSignalSpy spyVehicleInitialConnectComplete(vehicle, &Vehicle::initialConnectComplete);
    QCOMPARE(spyVehicleInitialConnectComplete.wait(3000), true);

    QSignalSpy spyCommLostChanged(vehicle->vehicleLinkManager(), &VehicleLinkManager::communicationLostChanged);

    // Connection removed should just signal communication lost

    pMockLink->simulateConnectionRemoved();
    QCOMPARE(spyCommLostChanged.wait(VehicleLinkManager::_heartbeatMaxElpasedMSecs * 2), true);
    QCOMPARE(spyCommLostChanged.count(), 1);
    QCOMPARE(spyCommLostChanged[0][0].toBool(), true);
}

void VehicleLinkManagerTest::_highLatencyLinkTest()
{
    SharedLinkConfigurationPtr mockConfig1;
    SharedLinkInterfacePtr mockLink1;
    SharedLinkConfigurationPtr mockConfig2;
    SharedLinkInterfacePtr mockLink2;

    QSignalSpy spyVehicleCreate(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);

    _startMockLink(1, true /*highLatency*/, false /*incrementVehicleId*/, mockConfig1, mockLink1);
    MockLink *const pMockLink1 = qobject_cast<MockLink*>(mockLink1.get());

    QCOMPARE(spyVehicleCreate.wait(1000), true);
    QCOMPARE(MultiVehicleManager::instance()->vehicles()->count(), 1);
    Vehicle *const vehicle = MultiVehicleManager::instance()->activeVehicle();
    VehicleLinkManager *const vehicleLinkManager = vehicle->vehicleLinkManager();
    QVERIFY(vehicle);
    QVERIFY(vehicleLinkManager);

    MultiSignalSpyV2 multiSpyVLM;
    QVERIFY(multiSpyVLM.init(vehicleLinkManager));

    // Addition of second non high latency link should:
    //  Change primary link from 1 to 2
    //  Stop high latency transmission on 1

    QSignalSpy spyTransmissionEnabledChanged(pMockLink1, &MockLink::highLatencyTransmissionEnabledChanged);
    QVERIFY(spyTransmissionEnabledChanged.isValid());
    _startMockLink(2, false /*highLatency*/, false /*incrementVehicleId*/, mockConfig2, mockLink2);
    MockLink* pMockLink2 = qobject_cast<MockLink*>(mockLink2.get());

    QCOMPARE(multiSpyVLM.waitForSignal(_primaryLinkChangedSignalName, 100), true);
    QCOMPARE(pMockLink2, vehicleLinkManager->primaryLink().lock().get());
    QCOMPARE(spyTransmissionEnabledChanged.count(), 1);
    QCOMPARE(spyTransmissionEnabledChanged.takeFirst()[0].toBool(), false);
    multiSpyVLM.clearAllSignals();
    spyTransmissionEnabledChanged.clear();

    // Comm lost on primary:2 should:
    //  Switch primary to 1
    //  Re-enable high latency transmission on 1

    pMockLink2->setCommLost(true);
    QCOMPARE(multiSpyVLM.waitForSignal(_primaryLinkChangedSignalName, VehicleLinkManager::_heartbeatMaxElpasedMSecs * 2), true);
    QCOMPARE(pMockLink1, vehicleLinkManager->primaryLink().lock().get());
    QCOMPARE(spyTransmissionEnabledChanged.count(), 1);
    QCOMPARE(spyTransmissionEnabledChanged.takeFirst()[0].toBool(), true);
    spyTransmissionEnabledChanged.clear();
}

void VehicleLinkManagerTest::_startMockLink(int mockIndex, bool highLatency, bool incrementVehicleId, SharedLinkConfigurationPtr &mockConfig, SharedLinkInterfacePtr &mockLink)
{
    MockConfiguration *const pMockConfig = new MockConfiguration(QStringLiteral("Mock %1").arg(mockIndex));

    mockConfig = SharedLinkConfigurationPtr(pMockConfig);

    pMockConfig->setDynamic(true);
    pMockConfig->setHighLatency(highLatency);
    pMockConfig->setIncrementVehicleId(incrementVehicleId);

    QVERIFY(LinkManager::instance()->createConnectedLink(mockConfig));
    QVERIFY(mockConfig->link());

    mockLink = LinkManager::instance()->sharedLinkInterfacePointerForLink(mockConfig->link());
    QVERIFY(mockLink);
}
