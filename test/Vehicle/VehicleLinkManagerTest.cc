#include "QmlObjectListModel.h"
#include "VehicleLinkManagerTest.h"

#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "LinkManager.h"
#include "MultiSignalSpy.h"
#include "MultiVehicleManager.h"
#include "UnitTest.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

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

    // Flush currently-queued command traffic before disconnect; the disconnect->destroyed
    // wait below absorbs any remaining async, so a single event-loop drain suffices here.
    QTest::qWait(0);
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
    // Comm loss with pending vehicle commands causes MavCommandQueue to give up.
    ignoreLogMessage("Vehicle.MavCommandQueue", QtWarningMsg,
                     QRegularExpression("Giving up sending command after max retries:"));
    // Losing comms while the AVAILABLE_MODES enumeration is still in flight fails the request.
    ignoreLogMessage("Vehicle.StandardModes", QtWarningMsg,
                     QRegularExpression("Failed to retrieve available modes"));
    SharedLinkConfigurationPtr mockConfig;
    SharedLinkInterfacePtr mockLink;
    _startMockLink(1, false /*highLatency*/, true /*incrementVehicleId*/, mockConfig, mockLink);
    MockLink* const pMockLink = qobject_cast<MockLink*>(mockLink.get());
    QVERIFY(pMockLink);

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
    // Losing comms while the AVAILABLE_MODES enumeration is still in flight fails the request.
    ignoreLogMessage("Vehicle.StandardModes", QtWarningMsg,
                     QRegularExpression("Failed to retrieve available modes"));
    struct MockLinkInfo {
        SharedLinkConfigurationPtr config;
        SharedLinkInterfacePtr link;
        MockLink* mock = nullptr;
    };
    MockLinkInfo primary;
    MockLinkInfo secondary;
    _startMockLink(1, false /*highLatency*/, false /*incrementVehicleId*/, primary.config, primary.link);
    _startMockLink(2, false /*highLatency*/, false /*incrementVehicleId*/, secondary.config, secondary.link);

    Vehicle* const vehicle = waitForVehicleConnect(TestTimeout::shortMs());
    QVERIFY(vehicle);
    QVERIFY_TRUE_WAIT(MultiVehicleManager::instance()->vehicles()->count() == 1, TestTimeout::mediumMs());

    VehicleLinkManager* const vehicleLinkManager = vehicle->vehicleLinkManager();
    QVERIFY(vehicleLinkManager);

    QSignalSpy spyVehicleInitialConnectComplete(vehicle, &Vehicle::initialConnectComplete);
    QVERIFY_TRUE_WAIT(spyVehicleInitialConnectComplete.count() > 0 || vehicle->isInitialConnectComplete(),
                      TestTimeout::mediumMs());

    // The first link to start sending a heartbeat will be the primary link.
    // Depending on how the thread scheduling works, that could be the second link started.
    const SharedLinkInterfacePtr primaryLink = vehicleLinkManager->primaryLink().lock();
    QVERIFY(primaryLink == primary.link || primaryLink == secondary.link);
    if (primaryLink == secondary.link) {
        std::swap(primary, secondary);
    }

    primary.mock = qobject_cast<MockLink*>(primary.link.get());
    secondary.mock = qobject_cast<MockLink*>(secondary.link.get());
    QVERIFY(primary.mock);
    QVERIFY(secondary.mock);

    const QStringList rgNames = vehicleLinkManager->linkNames();
    QStringList rgStatus = vehicleLinkManager->linkStatuses();
    QCOMPARE(rgNames.count(), 2);
    // linkNames()/linkStatuses() are ordered by link add order, not primary-first,
    // so look up indices by name
    const qsizetype primaryIdx = rgNames.indexOf(primary.config->name());
    const qsizetype secondaryIdx = rgNames.indexOf(secondary.config->name());
    QVERIFY(primaryIdx != -1);
    QVERIFY(secondaryIdx != -1);
    QVERIFY(primaryIdx != secondaryIdx);
    QCOMPARE(rgStatus.count(), 2);
    QVERIFY(rgStatus[primaryIdx].isEmpty());
    QVERIFY(rgStatus[secondaryIdx].isEmpty());

    MultiSignalSpy multiSpy;
    QVERIFY(multiSpy.init(vehicleLinkManager));

    // Comm loss/regain on the secondary link should only update status text
    secondary.mock->setCommLost(true);
    QCOMPARE(multiSpy.waitForSignal(_linkStatusesChangedSignalName, VehicleLinkManager::kTestCommLostDetectionTimeoutMs),
             true);
    QVERIFY(multiSpy.onlyEmitted(_linkStatusesChangedSignalName));

    rgStatus = vehicleLinkManager->linkStatuses();
    QCOMPARE(rgStatus.count(), 2);
    QVERIFY(rgStatus[primaryIdx].isEmpty());
    QVERIFY(!rgStatus[secondaryIdx].isEmpty());

    multiSpy.clearAllSignals();

    secondary.mock->setCommLost(false);
    QCOMPARE(multiSpy.waitForSignal(_linkStatusesChangedSignalName, VehicleLinkManager::kTestCommLostDetectionTimeoutMs),
             true);
    QVERIFY(multiSpy.onlyEmitted(_linkStatusesChangedSignalName));

    rgStatus = vehicleLinkManager->linkStatuses();
    QCOMPARE(rgStatus.count(), 2);
    QVERIFY(rgStatus[primaryIdx].isEmpty());
    QVERIFY(rgStatus[secondaryIdx].isEmpty());

    multiSpy.clearAllSignals();

    // Comm loss on the primary link should switch primary to the secondary
    // Switching primary produces a showAppMessage debug log.
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression("Switching communication to secondary link"));
    primary.mock->setCommLost(true);
    QCOMPARE(multiSpy.waitForSignal(_primaryLinkChangedSignalName, VehicleLinkManager::kTestCommLostDetectionTimeoutMs),
             true);
    QVERIFY(
        multiSpy.onlyEmittedOnce(_primaryLinkChangedSignalName, _linkStatusesChangedSignalName));

    QCOMPARE(secondary.mock, vehicleLinkManager->primaryLink().lock().get());
    // Primary switch must not reorder the link list, otherwise the cached indices are invalid
    QCOMPARE(vehicleLinkManager->linkNames(), rgNames);
    rgStatus = vehicleLinkManager->linkStatuses();
    QCOMPARE(rgStatus.count(), 2);
    QVERIFY(!rgStatus[primaryIdx].isEmpty());
    QVERIFY(rgStatus[secondaryIdx].isEmpty());

    multiSpy.clearAllSignals();

    // Comm regained on the original primary should leave the secondary as primary and only update status
    primary.mock->setCommLost(false);
    QCOMPARE(multiSpy.waitForSignal(_linkStatusesChangedSignalName, VehicleLinkManager::kTestCommLostDetectionTimeoutMs),
             true);
    QVERIFY(multiSpy.onlyEmitted(_linkStatusesChangedSignalName));

    QCOMPARE(secondary.mock, vehicleLinkManager->primaryLink().lock().get());
    rgStatus = vehicleLinkManager->linkStatuses();
    QCOMPARE(rgStatus.count(), 2);
    QVERIFY(rgStatus[primaryIdx].isEmpty());
    QVERIFY(rgStatus[secondaryIdx].isEmpty());

    multiSpy.clearAllSignals();
}

void VehicleLinkManagerTest::_multiLinkTotalCommLossRecoveryTest()
{
    // Comm loss with pending vehicle commands causes MavCommandQueue to give up.
    ignoreLogMessage("Vehicle.MavCommandQueue", QtWarningMsg,
                     QRegularExpression("Giving up sending command after max retries:"));
    // Primary link switchover produces a showAppMessage debug log.
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression("Switching communication to"));
    // Losing comms while the AVAILABLE_MODES enumeration is still in flight fails the request.
    ignoreLogMessage("Vehicle.StandardModes", QtWarningMsg,
                     QRegularExpression("Failed to retrieve available modes"));

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

    MockLink* const pMockLink1 = qobject_cast<MockLink*>(mockLink1.get());
    MockLink* const pMockLink2 = qobject_cast<MockLink*>(mockLink2.get());
    QVERIFY(pMockLink1);
    QVERIFY(pMockLink2);

    QSignalSpy spyCommLostChanged(vehicleLinkManager, &VehicleLinkManager::communicationLostChanged);

    // Lose both links to reach total communication loss
    pMockLink1->setCommLost(true);
    pMockLink2->setCommLost(true);
    QVERIFY_SIGNAL_WAIT(spyCommLostChanged, VehicleLinkManager::kTestCommLostDetectionTimeoutMs);
    QCOMPARE(spyCommLostChanged.count(), 1);
    QCOMPARE(spyCommLostChanged[0][0].toBool(), true);
    spyCommLostChanged.clear();

    // Regaining a single link should end total communication loss
    pMockLink1->setCommLost(false);
    QVERIFY_SIGNAL_WAIT(spyCommLostChanged, VehicleLinkManager::kTestCommLostDetectionTimeoutMs);
    QCOMPARE(spyCommLostChanged.count(), 1);
    QCOMPARE(spyCommLostChanged[0][0].toBool(), false);

    // Exactly one link should still be comm lost (link ordering is scheduling dependent)
    const QStringList rgStatus = vehicleLinkManager->linkStatuses();
    QCOMPARE(rgStatus.count(), 2);
    QCOMPARE(rgStatus.count(QString()), 1);
}

void VehicleLinkManagerTest::_connectionRemovedTest()
{
    // Connection removal makes MavCommandQueue give up pending commands, same as the comm-loss tests.
    ignoreLogMessage("Vehicle.MavCommandQueue", QtWarningMsg,
                     QRegularExpression("Giving up sending command after max retries:"));
    // AVAILABLE_MODES may still be pending when the connection is removed.
    ignoreLogMessage("Vehicle.StandardModes", QtWarningMsg, QRegularExpression("Failed to retrieve available modes"));

    SharedLinkConfigurationPtr mockConfig;
    SharedLinkInterfacePtr mockLink;
    _startMockLink(1, false /*highLatency*/, true /*incrementVehicleId*/, mockConfig, mockLink);
    MockLink* const pMockLink = qobject_cast<MockLink*>(mockLink.get());
    QVERIFY(pMockLink);

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
    // Comm loss causes MavCommandQueue to give up pending commands; link switch triggers showAppMessage.
    ignoreLogMessage("Vehicle.MavCommandQueue", QtWarningMsg,
                     QRegularExpression("Giving up sending command after max retries:"));
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression("Switching communication to secondary link"));
    // Giving up on the COMPONENT_METADATA request leaves the metadata load unable to complete.
    ignoreLogMessage("ComponentInformation.RequestMetaDataTypeStateMachine", QtWarningMsg,
                     QRegularExpression("failed to load metadata \\(primary and fallback\\)"));
    // The slow high-latency link can still have the AVAILABLE_MODES request in flight when the
    // test forces comm loss, which fails the request.
    ignoreLogMessage("Vehicle.StandardModes", QtWarningMsg,
                     QRegularExpression("Failed to retrieve available modes"));
    SharedLinkConfigurationPtr mockConfig1;
    SharedLinkInterfacePtr mockLink1;
    SharedLinkConfigurationPtr mockConfig2;
    SharedLinkInterfacePtr mockLink2;
    _startMockLink(1, true /*highLatency*/, false /*incrementVehicleId*/, mockConfig1, mockLink1);
    MockLink* const pMockLink1 = qobject_cast<MockLink*>(mockLink1.get());
    QVERIFY(pMockLink1);

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
    QVERIFY(pMockLink2);
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
