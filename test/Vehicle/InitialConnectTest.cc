#include "InitialConnectTest.h"

#include <memory>

#include <QtTest/QSignalSpy>

#include "GeoFenceManager.h"
#include "InitialConnectStateMachine.h"
#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "MockConfiguration.h"
#include "MockLink.h"
#include "MockLinkMissionItemHandler.h"
#include "MissionManager.h"
#include "ParameterManager.h"
#include "RallyPointManager.h"
#include "StandardModes.h"
#include "UnitTest.h"
#include "Vehicle.h"
#include "ComponentInformationManager.h"
#include "MavlinkSettings.h"
#include "SettingsManager.h"

#include <QtCore/qscopeguard.h>
#include <QtTest/QTest>

void InitialConnectTest::_performTestCases_data()
{
    QTest::addColumn<int>("failureMode");
    QTest::addColumn<QString>("failureModeStr");

    static const struct TestCase_s {
        MockConfiguration::FailureMode_t failureMode;
        const char* failureModeStr;
    } rgTestCases[] = {
        {MockConfiguration::FailNone, "No failures"},
        {MockConfiguration::FailInitialConnectRequestMessageAutopilotVersionFailure,
         "REQUEST_MESSAGE:AUTOPILOT_VERSION returns failure"},
        {MockConfiguration::FailInitialConnectRequestMessageAutopilotVersionLost,
         "REQUEST_MESSAGE:AUTOPILOT_VERSION success, AUTOPILOT_VERSION never sent"},
    };

    int i = 0;
    for (const auto& testCase : rgTestCases) {
        QTest::addRow("case_%d", i)
            << static_cast<int>(testCase.failureMode)
            << QString::fromLatin1(testCase.failureModeStr);
        ++i;
    }
}

void InitialConnectTest::_performTestCases()
{
    QFETCH(int, failureMode);
    QFETCH(QString, failureModeStr);
    TEST_DEBUG(QStringLiteral("Testing case failure mode: %1").arg(failureModeStr));
    _connectMockLink(MAV_AUTOPILOT_PX4, static_cast<MockConfiguration::FailureMode_t>(failureMode));
    _disconnectMockLink();
}

void InitialConnectTest::_boardVendorProductId()
{
    LinkManager::instance()->setConnectionsAllowed();

    auto* mvm = MultiVehicleManager::instance();
    QSignalSpy activeVehicleSpy{mvm, &MultiVehicleManager::activeVehicleChanged};
    auto mockConfig = std::make_shared<MockConfiguration>(QString{"MockLink"});
    const uint16_t mockVendor = 1234;
    const uint16_t mockProduct = 5678;
    mockConfig->setBoardVendorProduct(mockVendor, mockProduct);
    SharedLinkConfigurationPtr linkConfig = mockConfig;
    LinkManager::instance()->createConnectedLink(linkConfig);
    QVERIFY_SIGNAL_WAIT(activeVehicleSpy, TestTimeout::mediumMs());
    auto* vehicle = mvm->activeVehicle();
    QVERIFY(vehicle);
    QSignalSpy initialConnectCompleteSpy{vehicle, &Vehicle::initialConnectComplete};
    // Both ends of mocklink (and the initial connect state machine?) operate on
    // a different thread. The initial connection may already be complete.
    QVERIFY_TRUE_WAIT(initialConnectCompleteSpy.count() > 0 || vehicle->isInitialConnectComplete(),
                      TestTimeout::mediumMs());
    QCOMPARE(vehicle->firmwareBoardVendorId(), mockVendor);
    QCOMPARE(vehicle->firmwareBoardProductId(), mockProduct);
    LinkManager::instance()->disconnectAll();
    QSignalSpy vehicleRemovedSpy{mvm, &MultiVehicleManager::activeVehicleChanged};
    QVERIFY_SIGNAL_WAIT(vehicleRemovedSpy, TestTimeout::mediumMs());
}

void InitialConnectTest::_progressTracking()
{
    QList<float> progressValues;

    QMetaObject::Connection progressConnection =
        connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this,
                [&progressValues](Vehicle* vehicle) {
                    if (!vehicle) {
                        return;
                    }
                    connect(vehicle, &Vehicle::loadProgressChanged, vehicle, [&progressValues](float progress) {
                        progressValues.append(progress);
                    });
                });

    _connectMockLink(MAV_AUTOPILOT_PX4);

    disconnect(progressConnection);

    QVERIFY(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());
    QVERIFY2(!progressValues.isEmpty(), "No progress updates were emitted");

    bool foundIntermediate = false;
    float maxProgress = 0.0f;

    for (const float progress : progressValues) {
        QVERIFY2(progress >= 0.0f && progress <= 1.0f,
                 qPrintable(QStringLiteral("Progress out of bounds: %1").arg(progress)));
        if (progress > 0.0f && progress < 1.0f) {
            foundIntermediate = true;
        }
        maxProgress = qMax(maxProgress, progress);
    }

    QVERIFY2(foundIntermediate, "No intermediate progress values were seen");
    QVERIFY2(maxProgress >= 0.8f, qPrintable(QStringLiteral("Peak progress too low: %1").arg(maxProgress)));

    _disconnectMockLink();
}

void InitialConnectTest::_highLatencySkipsPlanRequests()
{
    LinkManager::instance()->setConnectionsAllowed();

    auto* mvm = MultiVehicleManager::instance();
    QVERIFY(!mvm->activeVehicle());

    QSignalSpy activeVehicleSpy{mvm, &MultiVehicleManager::activeVehicleChanged};

    auto* mockConfig = new MockConfiguration(QStringLiteral("HighLatencyMock"));
    mockConfig->setFirmwareType(MAV_AUTOPILOT_PX4);
    mockConfig->setVehicleType(MAV_TYPE_QUADROTOR);
    mockConfig->setHighLatency(true);
    mockConfig->setDynamic(true);

    SharedLinkConfigurationPtr linkConfig = LinkManager::instance()->addConfiguration(mockConfig);
    QVERIFY(LinkManager::instance()->createConnectedLink(linkConfig));

    _mockLink = qobject_cast<MockLink*>(linkConfig->link());
    QVERIFY(_mockLink);
    _mockLink->clearReceivedMavlinkMessageCounts();

    QVERIFY(activeVehicleSpy.wait(TestTimeout::longMs()));
    _vehicle = mvm->activeVehicle();
    QVERIFY(_vehicle);

    QSignalSpy initialConnectCompleteSpy{_vehicle, &Vehicle::initialConnectComplete};
    QVERIFY(initialConnectCompleteSpy.wait(TestTimeout::longMs()) || _vehicle->isInitialConnectComplete());
    QVERIFY(_vehicle->initialPlanRequestComplete());
    QCOMPARE(_mockLink->receivedMavlinkMessageCount(MAVLINK_MSG_ID_MISSION_REQUEST_LIST), 0);

    _disconnectMockLink();
}

void InitialConnectTest::_genericAutopilotVersionFailureSkipsUnsupportedPlanTypes()
{
    _connectMockLink(MAV_AUTOPILOT_GENERIC, MockConfiguration::FailInitialConnectRequestMessageAutopilotVersionFailure);

    QVERIFY(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());
    QVERIFY(_vehicle->capabilitiesKnown());

    const uint64_t capabilityBits = _vehicle->capabilityBits();
    QVERIFY((capabilityBits & MAV_PROTOCOL_CAPABILITY_MISSION_FENCE) == 0);
    QVERIFY((capabilityBits & MAV_PROTOCOL_CAPABILITY_MISSION_RALLY) == 0);

    auto* geoFenceManager = _vehicle->findChild<GeoFenceManager*>();
    auto* rallyPointManager = _vehicle->findChild<RallyPointManager*>();
    QVERIFY(geoFenceManager);
    QVERIFY(rallyPointManager);
    QVERIFY(!geoFenceManager->supported());
    QVERIFY(!rallyPointManager->supported());
    QVERIFY(_vehicle->initialPlanRequestComplete());

    _disconnectMockLink();
}

void InitialConnectTest::_multipleReconnects()
{
    for (int i = 0; i < 3; ++i) {
        const MAV_AUTOPILOT autopilot = (i % 2 == 0) ? MAV_AUTOPILOT_PX4 : MAV_AUTOPILOT_ARDUPILOTMEGA;
        _connectMockLink(autopilot);
        QVERIFY(_vehicle);
        QVERIFY(_vehicle->isInitialConnectComplete());
        _disconnectMockLink();
        QVERIFY(!MultiVehicleManager::instance()->activeVehicle());
    }
}

void InitialConnectTest::_rallyTimeoutPathDoesNotLeakCompletionHandler()
{
    LinkManager::instance()->setConnectionsAllowed();

    auto* mvm = MultiVehicleManager::instance();
    QSignalSpy activeVehicleSpy{mvm, &MultiVehicleManager::activeVehicleChanged};
    auto* mockConfig = new MockConfiguration(QStringLiteral("RallyTimeoutMock"));
    mockConfig->setFirmwareType(MAV_AUTOPILOT_PX4);
    mockConfig->setVehicleType(MAV_TYPE_QUADROTOR);
    mockConfig->setDynamic(true);

    SharedLinkConfigurationPtr linkConfig = LinkManager::instance()->addConfiguration(mockConfig);
    QVERIFY(LinkManager::instance()->createConnectedLink(linkConfig));

    _mockLink = qobject_cast<MockLink*>(linkConfig->link());
    QVERIFY(_mockLink);

    QVERIFY(activeVehicleSpy.wait(TestTimeout::longMs()));
    _vehicle = mvm->activeVehicle();
    QVERIFY(_vehicle);

    auto* geoFenceManager = _vehicle->findChild<GeoFenceManager*>();
    auto* rallyPointManager = _vehicle->findChild<RallyPointManager*>();
    auto* initialConnectStateMachine = _vehicle->findChild<InitialConnectStateMachine*>();
    QVERIFY(geoFenceManager);
    QVERIFY(rallyPointManager);
    QVERIFY(initialConnectStateMachine);

    connect(geoFenceManager, &GeoFenceManager::loadComplete, this, [this, initialConnectStateMachine]() {
        _mockLink->setMissionItemFailureMode(
            MockLinkMissionItemHandler::FailReadRequestListNoResponse, MAV_MISSION_ACCEPTED);
        initialConnectStateMachine->setTimeoutOverride(QStringLiteral("RequestRallyPoints"), 100);
    });

    QSignalSpy initialConnectCompleteSpy{_vehicle, &Vehicle::initialConnectComplete};
    QVERIFY(initialConnectCompleteSpy.wait(TestTimeout::longMs()) || _vehicle->isInitialConnectComplete());
    QVERIFY(!_vehicle->initialPlanRequestComplete());

    _mockLink->setMissionItemFailureMode(MockLinkMissionItemHandler::FailNone, MAV_MISSION_ACCEPTED);

    QSignalSpy planCompleteSpy{_vehicle, &Vehicle::initialPlanRequestCompleteChanged};
    QSignalSpy rallyLoadCompleteSpy{rallyPointManager, &RallyPointManager::loadComplete};

    rallyPointManager->loadFromVehicle();
    QVERIFY(rallyLoadCompleteSpy.wait(TestTimeout::longMs()));
    QCOMPARE(planCompleteSpy.count(), 1);

    _disconnectMockLink();
}

void InitialConnectTest::_stateTimeoutFallsThrough_data()
{
    QTest::addColumn<QList<uint32_t>>("blockedMessageIds");
    QTest::addColumn<int>("configFailureMode");
    QTest::addColumn<bool>("blockMissionProtocolImmediately");
    QTest::addColumn<bool>("blockMissionProtocolAfterMissionLoad");
    QTest::addColumn<QStringList>("timeoutOverrideStates");
    QTest::addColumn<bool>("expectParametersReady");
    QTest::addColumn<bool>("expectPlanRequestComplete");

    // Timeout matrix:
    // +----------------+-------------------+------------------+---------+--------+
    // | State          | Failure Injection | Timeout States   | Params? | Plans? |
    // +----------------+-------------------+------------------+---------+--------+
    // | StandardModes  | Block AVAIL_MODES | StdModes         | Yes     | Yes    |
    // | CompInfo       | Block COMP_META   | CompInfo         | Yes     | Yes    |
    // | Parameters     | No param response | Parameters       | No      | Yes    |
    // | Mission        | Block mission req | Msn+Fence+Rally  | Yes     | No     |
    // | GeoFence       | Block after msn   | Fence+Rally      | Yes     | No     |
    // +----------------+-------------------+------------------+---------+--------+

    QTest::addRow("StandardModes")
        << QList<uint32_t>{MAVLINK_MSG_ID_AVAILABLE_MODES}
        << static_cast<int>(MockConfiguration::FailNone)
        << false << false
        << QStringList{QStringLiteral("RequestStandardModes")}
        << true << true;

    QTest::addRow("CompInfo")
        << QList<uint32_t>{MAVLINK_MSG_ID_COMPONENT_METADATA}
        << static_cast<int>(MockConfiguration::FailNone)
        << false << false
        << QStringList{QStringLiteral("RequestCompInfo")}
        << true << true;

    QTest::addRow("Parameters")
        << QList<uint32_t>{}
        << static_cast<int>(MockConfiguration::FailParamNoResponseToRequestList)
        << false << false
        << QStringList{QStringLiteral("RequestParameters")}
        << false << true;

    QTest::addRow("Mission")
        << QList<uint32_t>{}
        << static_cast<int>(MockConfiguration::FailNone)
        << true << false
        << QStringList{QStringLiteral("RequestMission"),
                       QStringLiteral("RequestGeoFence"),
                       QStringLiteral("RequestRallyPoints")}
        << true << false;

    QTest::addRow("GeoFence")
        << QList<uint32_t>{}
        << static_cast<int>(MockConfiguration::FailNone)
        << false << true
        << QStringList{QStringLiteral("RequestGeoFence"),
                       QStringLiteral("RequestRallyPoints")}
        << true << false;
}

void InitialConnectTest::_stateTimeoutFallsThrough()
{
    QFETCH(QList<uint32_t>, blockedMessageIds);
    QFETCH(int, configFailureMode);
    QFETCH(bool, blockMissionProtocolImmediately);
    QFETCH(bool, blockMissionProtocolAfterMissionLoad);
    QFETCH(QStringList, timeoutOverrideStates);
    QFETCH(bool, expectParametersReady);
    QFETCH(bool, expectPlanRequestComplete);

    LinkManager::instance()->setConnectionsAllowed();

    auto* mvm = MultiVehicleManager::instance();
    QSignalSpy activeVehicleSpy{mvm, &MultiVehicleManager::activeVehicleChanged};
    auto* mockConfig = new MockConfiguration(QStringLiteral("TimeoutFallthroughMock"));
    mockConfig->setFirmwareType(MAV_AUTOPILOT_PX4);
    mockConfig->setVehicleType(MAV_TYPE_QUADROTOR);
    if (configFailureMode != static_cast<int>(MockConfiguration::FailNone)) {
        mockConfig->setFailureMode(static_cast<MockConfiguration::FailureMode_t>(configFailureMode));
    }
    mockConfig->setDynamic(true);

    SharedLinkConfigurationPtr linkConfig = LinkManager::instance()->addConfiguration(mockConfig);
    QVERIFY(LinkManager::instance()->createConnectedLink(linkConfig));

    _mockLink = qobject_cast<MockLink*>(linkConfig->link());
    QVERIFY(_mockLink);

    for (const uint32_t messageId : blockedMessageIds) {
        _mockLink->setRequestMessageNoResponse(messageId);
    }

    if (blockMissionProtocolImmediately) {
        _mockLink->setMissionItemFailureMode(
            MockLinkMissionItemHandler::FailReadRequestListNoResponse, MAV_MISSION_ACCEPTED);
    }

    QVERIFY(activeVehicleSpy.wait(TestTimeout::longMs()));
    _vehicle = mvm->activeVehicle();
    QVERIFY(_vehicle);

    auto* initialConnectStateMachine = _vehicle->findChild<InitialConnectStateMachine*>();
    QVERIFY(initialConnectStateMachine);

    for (const QString& stateName : timeoutOverrideStates) {
        initialConnectStateMachine->setTimeoutOverride(stateName, 100);
    }

    if (blockMissionProtocolAfterMissionLoad) {
        auto* missionManager = _vehicle->findChild<MissionManager*>();
        QVERIFY(missionManager);
        connect(missionManager, &MissionManager::newMissionItemsAvailable, this, [this]() {
            _mockLink->setMissionItemFailureMode(
                MockLinkMissionItemHandler::FailReadRequestListNoResponse, MAV_MISSION_ACCEPTED);
        });
    }

    QSignalSpy initialConnectCompleteSpy{_vehicle, &Vehicle::initialConnectComplete};
    if (!_vehicle->isInitialConnectComplete()) {
        QVERIFY(initialConnectCompleteSpy.wait(TestTimeout::longMs()));
    }
    QCOMPARE(_vehicle->parameterManager()->parametersReady(), expectParametersReady);
    QCOMPARE(_vehicle->initialPlanRequestComplete(), expectPlanRequestComplete);

    _disconnectMockLink();
}

void InitialConnectTest::_stateRunMatrix_data()
{
    QTest::addColumn<bool>("highLatency");
    QTest::addColumn<bool>("logReplay");
    QTest::addColumn<bool>("flying");
    QTest::addColumn<bool>("expectAutopilotVersionRequest");
    QTest::addColumn<bool>("expectAvailableModesRequest");
    QTest::addColumn<bool>("expectParamRequest");
    QTest::addColumn<bool>("expectHashCheckOnly");
    QTest::addColumn<bool>("expectPlanRequestListTraffic");
    QTest::addColumn<bool>("expectParameterDownloadSkipped");

    // Matrix reference for generated rows and expected request behavior.
    //
    // +----+----+-----+------------+------------+--------+------------+----------------+----------+
    // | HL | LR | Fly | AP_VERSION | AVAIL_MODS | PARAMS | HASH_CHECK | PLAN_REQ_LISTS | DL_SKIP  |
    // +----+----+-----+------------+------------+--------+------------+----------------+----------+
    // | 0  | 0  | 0   | Run        | Run        | Run    | -          | Run            | false    |
    // | 0  | 0  | 1   | Run        | Run        | Skip   | Yes        | Skip           | true     |
    // | 1  | 0  | 0   | Skip       | Run        | Skip*  | -          | Skip           | false    |
    // | 1  | 0  | 1   | Skip       | Run        | Skip*  | -          | Skip           | true     |
    // | 0  | 1  | 0   | Skip       | Run        | Skip*  | -          | Skip           | false    |
    // | 0  | 1  | 1   | Skip       | Run        | Skip*  | -          | Skip           | true     |
    // | 1  | 1  | 0   | Skip       | Run        | Skip*  | -          | Skip           | false    |
    // | 1  | 1  | 1   | Skip       | Run        | Skip*  | -          | Skip           | true     |
    // +----+----+-----+------------+------------+--------+------------+----------------+----------+
    // * HL/LR params are handled internally (no PARAM_REQUEST_LIST).
    // Flying (PX4): tries cache-only hash check; cache miss advances without params.
    // Flying rows enable noInitialDownloadWhenFlying + startArmed.

    for (int bits = 0; bits < 8; ++bits) {
        const bool highLatency = bits & 0x1;
        const bool logReplay = bits & 0x2;
        const bool flying = bits & 0x4;
        const bool skipForLinkType = highLatency || logReplay;

        const bool expectAutopilotVersionRequest = !skipForLinkType;
        const bool expectAvailableModesRequest = true;
        const bool expectParamRequest = !skipForLinkType && !flying;
        const bool expectHashCheckOnly = !skipForLinkType && flying;
        const bool expectPlanRequestListTraffic = !skipForLinkType && !flying;
        const bool expectParameterDownloadSkipped = flying;

        QTest::addRow("HL_%d_LR_%d_Fly_%d", highLatency ? 1 : 0, logReplay ? 1 : 0, flying ? 1 : 0)
            << highLatency << logReplay << flying
            << expectAutopilotVersionRequest
            << expectAvailableModesRequest
            << expectParamRequest
            << expectHashCheckOnly
            << expectPlanRequestListTraffic
            << expectParameterDownloadSkipped;
    }
}

void InitialConnectTest::_stateRunMatrix()
{
    QFETCH(bool, highLatency);
    QFETCH(bool, logReplay);
    QFETCH(bool, flying);
    QFETCH(bool, expectAutopilotVersionRequest);
    QFETCH(bool, expectAvailableModesRequest);
    QFETCH(bool, expectParamRequest);
    QFETCH(bool, expectHashCheckOnly);
    QFETCH(bool, expectPlanRequestListTraffic);
    QFETCH(bool, expectParameterDownloadSkipped);

    // Effective skip path in InitialConnectStateMachine is (isHighLatency || isLogReplay)
    const bool skipForLinkType = highLatency || logReplay;

    // Enable noInitialDownloadWhenFlying setting for flying rows
    auto* noInitialDownloadWhenFlying = SettingsManager::instance()->mavlinkSettings()->noInitialDownloadWhenFlying();
    const QVariant previousNoInitialDownloadWhenFlying = noInitialDownloadWhenFlying->rawValue();
    const auto restoreNoInitialDownloadWhenFlying = qScopeGuard([noInitialDownloadWhenFlying, previousNoInitialDownloadWhenFlying]() {
        noInitialDownloadWhenFlying->setRawValue(previousNoInitialDownloadWhenFlying);
    });
    noInitialDownloadWhenFlying->setRawValue(flying);

    LinkManager::instance()->setConnectionsAllowed();

    auto* mvm = MultiVehicleManager::instance();
    QVERIFY(!mvm->activeVehicle());

    QSignalSpy activeVehicleSpy{mvm, &MultiVehicleManager::activeVehicleChanged};

    auto* mockConfig = new MockConfiguration(QStringLiteral("StateRunMatrixMock"));
    mockConfig->setFirmwareType(MAV_AUTOPILOT_PX4);
    mockConfig->setVehicleType(MAV_TYPE_QUADROTOR);
    mockConfig->setHighLatency(skipForLinkType);
    mockConfig->setStartArmed(flying);
    mockConfig->setDynamic(true);

    SharedLinkConfigurationPtr linkConfig = LinkManager::instance()->addConfiguration(mockConfig);
    QVERIFY(LinkManager::instance()->createConnectedLink(linkConfig));

    _mockLink = qobject_cast<MockLink*>(linkConfig->link());
    QVERIFY(_mockLink);

    QVERIFY(activeVehicleSpy.wait(TestTimeout::longMs()));
    _vehicle = mvm->activeVehicle();
    QVERIFY(_vehicle);

    // Initial connection likely completed already.
    QSignalSpy initialConnectCompleteSpy{_vehicle, &Vehicle::initialConnectComplete};
    QVERIFY(initialConnectCompleteSpy.wait(TestTimeout::longMs()) || _vehicle->isInitialConnectComplete());

    const int autopilotVersionReqCount =
        _mockLink->receivedRequestMessageCount(MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_AUTOPILOT_VERSION);
    const int availableModesReqCount =
        _mockLink->receivedRequestMessageCount(MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_AVAILABLE_MODES);
    const int paramRequestListCount = _mockLink->receivedMavlinkMessageCount(MAVLINK_MSG_ID_PARAM_REQUEST_LIST);

    // AutopilotVersion expectation is matrix-driven.
    QCOMPARE(autopilotVersionReqCount > 0, expectAutopilotVersionRequest);

    // StandardModes expectation is matrix-driven.
    QCOMPARE(availableModesReqCount > 0, expectAvailableModesRequest);

    // parameterDownloadSkipped flag: true when params were intentionally not downloaded
    QCOMPARE(_vehicle->parameterManager()->parameterDownloadSkipped(), expectParameterDownloadSkipped);

    // Parameters: skipped when flying (with setting enabled) or on HL/LR links.
    // PX4 flying: cache-only hash check attempted, no full download.
    if (expectParamRequest) {
        QVERIFY2(paramRequestListCount > 0, "Expected PARAM_REQUEST_LIST");
    } else if (expectHashCheckOnly) {
        // PX4 flying: hash check was attempted but no full download
        QCOMPARE(paramRequestListCount, 0);
        QVERIFY2(_mockLink->hashCheckRequestCount() > 0, "Expected _HASH_CHECK request in cache-only mode");
        // No cache file in test env → cache miss → params not ready
        QVERIFY(!_vehicle->parameterManager()->parametersReady());
    }
    if (!flying) {
        // When not flying, params are either loaded normally or via HL/LR internal path
        QVERIFY(_vehicle->parameterManager()->parametersReady());
    }

    // Mission/GeoFence/Rally are skipped for high-latency/log-replay or when flying.
    // Check each plan type individually via per-mission-type request list counts.
    const int missionReqCount = _mockLink->receivedMissionRequestListCount(MAV_MISSION_TYPE_MISSION);
    const int fenceReqCount   = _mockLink->receivedMissionRequestListCount(MAV_MISSION_TYPE_FENCE);
    const int rallyReqCount   = _mockLink->receivedMissionRequestListCount(MAV_MISSION_TYPE_RALLY);
    if (!expectPlanRequestListTraffic) {
        QCOMPARE(missionReqCount, 0);
        QCOMPARE(fenceReqCount, 0);
        QCOMPARE(rallyReqCount, 0);
    } else {
        QVERIFY2(missionReqCount > 0, "Expected MISSION_REQUEST_LIST for missions");
        QVERIFY2(fenceReqCount > 0, "Expected MISSION_REQUEST_LIST for geofence");
        QVERIFY2(rallyReqCount > 0, "Expected MISSION_REQUEST_LIST for rally points");
    }

    _disconnectMockLink();
}

UT_REGISTER_TEST(InitialConnectTest, TestLabel::Integration, TestLabel::Vehicle)
