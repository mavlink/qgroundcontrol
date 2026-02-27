#include "HashCheckTest.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "LinkManager.h"
#include "MAVLinkLib.h"
#include "MockConfiguration.h"
#include "MultiVehicleManager.h"
#include "ParameterManager.h"
#include "Vehicle.h"

void HashCheckTest::cleanup()
{
    if (_mockLink && MultiVehicleManager::instance()->activeVehicle()) {
        QSignalSpy spy(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
        _mockLink->disconnect();
        _mockLink = nullptr;
        (void) UnitTest::waitForSignal(spy, TestTimeout::mediumMs(), QStringLiteral("activeVehicleChanged"));
    }
    VehicleTestManualConnect::cleanup();
}

void HashCheckTest::_deleteCacheFiles()
{
    const QDir cacheDir = ParameterManager::parameterCacheDir();
    if (cacheDir.exists()) {
        const QStringList cacheFiles = cacheDir.entryList(QStringList() << QStringLiteral("*.v2"), QDir::Files);
        for (const QString &file : cacheFiles) {
            QFile::remove(cacheDir.filePath(file));
        }
    }
}

void HashCheckTest::_connectAndWaitForParams()
{
    MultiVehicleManager *const vehicleMgr = MultiVehicleManager::instance();
    QVERIFY(vehicleMgr);

    QSignalSpy spyVehicle(vehicleMgr, &MultiVehicleManager::activeVehicleAvailableChanged);
    QVERIFY_SIGNAL_WAIT(spyVehicle, TestTimeout::mediumMs());

    Vehicle *const vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);

    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    QVERIFY_SIGNAL_WAIT(spyParamsReady, TestTimeout::longMs());

    const QList<QVariant> arguments = spyParamsReady.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toBool(), true);
}

void HashCheckTest::_disconnectAndSettle()
{
    _mockLink->disconnect();
    _mockLink = nullptr;
    QSignalSpy spyDisconnect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    QVERIFY(UnitTest::waitForSignal(spyDisconnect, TestTimeout::longMs(), QStringLiteral("activeVehicleChanged")));
    UnitTest::settleEventLoopForCleanup();
}

MockLink *HashCheckTest::_startPX4MockLinkNoIncrement(MockConfiguration::FailureMode_t failureMode)
{
    auto *const mockConfig = new MockConfiguration(QStringLiteral("PX4 MockLink"));
    mockConfig->setFirmwareType(MAV_AUTOPILOT_PX4);
    mockConfig->setVehicleType(MAV_TYPE_QUADROTOR);
    mockConfig->setIncrementVehicleId(false);
    mockConfig->setFailureMode(failureMode);
    mockConfig->setDynamic(true);

    SharedLinkConfigurationPtr config = LinkManager::instance()->addConfiguration(mockConfig);
    if (LinkManager::instance()->createConnectedLink(config)) {
        return qobject_cast<MockLink *>(config->link());
    }
    return nullptr;
}

MockLink *HashCheckTest::_startPX4MockLinkHighLatency()
{
    auto *const mockConfig = new MockConfiguration(QStringLiteral("PX4 HighLatency MockLink"));
    mockConfig->setFirmwareType(MAV_AUTOPILOT_PX4);
    mockConfig->setVehicleType(MAV_TYPE_QUADROTOR);
    mockConfig->setHighLatency(true);
    mockConfig->setDynamic(true);

    SharedLinkConfigurationPtr config = LinkManager::instance()->addConfiguration(mockConfig);
    if (LinkManager::instance()->createConnectedLink(config)) {
        return qobject_cast<MockLink *>(config->link());
    }
    return nullptr;
}

// Data-driven test matrix for all _HASH_CHECK parameter cache scenarios.
//
// FirstConnect_NoCache:   First PX4 connection with no cache. Sends _HASH_CHECK, gets full param list.
// Reconnect_CacheHit:     Reconnect after populating cache. Hash matches so no PARAM_REQUEST_LIST needed.
// Reconnect_CacheMiss:    Reconnect after a parameter changed. Hash mismatch triggers full param reload.
// HashTimeout_CacheHit:   Vehicle never responds to _HASH_CHECK. Falls back to PARAM_REQUEST_LIST, cache still valid.
// HashTimeout_NoCache:    Vehicle never responds to _HASH_CHECK and no cache exists. Falls back to full param list.
// HashTimeout_CacheStale: Vehicle never responds to _HASH_CHECK and cache is stale. Falls back to full param list.
// BothTimersExhaust:      Vehicle responds to neither _HASH_CHECK nor PARAM_REQUEST_LIST. Params never become ready.
// CacheDeleted_Between:   Cache populated then deleted before reconnect. Forces full param reload despite same vehicle.
// ManualRefresh:          User-triggered refreshAllParameters() bypasses _HASH_CHECK and requests full param list.
// ArduPilot:              ArduPilot uses FTP for parameters, so no _HASH_CHECK or PARAM_REQUEST_LIST traffic.
// HighLatency:            High-latency links skip parameter download entirely; params marked as missing.
// LogReplay:              Log replay shares the high-latency code path; params marked as missing.
//
// PARAM_REQUEST_LIST (xPRL) is only asserted for PX4 vehicles; ArduPilot uses FTP.
// missingParameters (xMiss) is only asserted when expectParametersReady (xReady) is true.
// Scenarios 11 and 12 exercise the same code path via setHighLatency(true).
// MockLink doesn't support isLogReplay(), so high latency serves as proxy for the
// shared guard: if (isHighLatency || _logReplay) { signal ready immediately }.

void HashCheckTest::_hashCheckMatrix_data()
{
    // MockLink configuration flags
    QTest::addColumn<bool>("px4");
    QTest::addColumn<bool>("highLatency");
    QTest::addColumn<bool>("populateCache");
    QTest::addColumn<bool>("changeParam");
    QTest::addColumn<bool>("deleteCache");
    QTest::addColumn<bool>("hashCheckNoResponse");
    QTest::addColumn<bool>("failNoResponse");
    QTest::addColumn<bool>("manualRefresh");

    // Expected outcomes
    QTest::addColumn<bool>("xHashCheck");
    QTest::addColumn<bool>("xPRL");
    QTest::addColumn<bool>("xReady");
    QTest::addColumn<bool>("xMissing");

    //                                          px4      hl       popC     chgP     delC     noResp   failNR   manRef    xHC      xPRL     xReady   xMiss
    QTest::newRow("FirstConnect_NoCache")    << true  << false << false << false << false << false << false << false  << true  << true  << true  << false;
    QTest::newRow("Reconnect_CacheHit")      << true  << false << true  << false << false << false << false << false  << true  << false << true  << false;
    QTest::newRow("Reconnect_CacheMiss")     << true  << false << true  << true  << false << false << false << false  << true  << true  << true  << false;
    QTest::newRow("HashTimeout_CacheHit")    << true  << false << true  << false << false << true  << false << false  << true  << true  << true  << false;
    QTest::newRow("HashTimeout_NoCache")     << true  << false << false << false << false << true  << false << false  << true  << true  << true  << false;
    QTest::newRow("HashTimeout_CacheStale")  << true  << false << true  << true  << false << true  << false << false  << true  << true  << true  << false;
    QTest::newRow("BothTimersExhaust")       << true  << false << false << false << false << true  << true  << false  << true  << true  << false << false;
    QTest::newRow("CacheDeleted_Between")    << true  << false << true  << false << true  << false << false << false  << true  << true  << true  << false;
    QTest::newRow("ManualRefresh")           << true  << false << false << false << false << false << false << true   << false << true  << true  << false;
    QTest::newRow("ArduPilot")              << false  << false << false << false << false << false << false << false  << false << false << true  << false;
    QTest::newRow("HighLatency")             << true  << true  << false << false << false << false << false << false  << false << false << true  << true;
    QTest::newRow("LogReplay")               << true  << true  << false << false << false << false << false << false  << false << false << true  << true;
}

void HashCheckTest::_hashCheckMatrix()
{
    QFETCH(bool, px4);
    QFETCH(bool, highLatency);
    QFETCH(bool, populateCache);
    QFETCH(bool, changeParam);
    QFETCH(bool, deleteCache);
    QFETCH(bool, hashCheckNoResponse);
    QFETCH(bool, failNoResponse);
    QFETCH(bool, manualRefresh);
    QFETCH(bool, xHashCheck);
    QFETCH(bool, xPRL);
    QFETCH(bool, xReady);
    QFETCH(bool, xMissing);

    const MAV_AUTOPILOT firmwareType = px4 ? MAV_AUTOPILOT_PX4 : MAV_AUTOPILOT_ARDUPILOTMEGA;
    const auto failMode = failNoResponse ? MockConfiguration::FailParamNoResponseToRequestList
                                         : MockConfiguration::FailNone;

    // Phase 1: Clean slate
    _deleteCacheFiles();

    // Phase 2: Optional preconnect to populate the parameter cache
    if (populateCache) {
        _mockLink = _startPX4MockLinkNoIncrement();
        _connectAndWaitForParams();
        _disconnectAndSettle();

        if (deleteCache) {
            _deleteCacheFiles();
        }
    }

    // Phase 3: Create the test link and connect
    if (manualRefresh) {
        _connectMockLink(firmwareType);
        QVERIFY(_vehicle);
        QVERIFY(_vehicle->parameterManager()->parametersReady());

        _mockLink->clearReceivedMavlinkMessageCounts();
        _vehicle->parameterManager()->refreshAllParameters();
        QTest::qWait(100);

    } else if (highLatency) {
        _mockLink = _startPX4MockLinkHighLatency();

        MultiVehicleManager *const vehicleMgr = MultiVehicleManager::instance();
        QVERIFY(vehicleMgr);

        QSignalSpy spyVehicle(vehicleMgr, &MultiVehicleManager::activeVehicleAvailableChanged);
        QVERIFY_SIGNAL_WAIT(spyVehicle, TestTimeout::mediumMs());

        Vehicle *const vehicle = vehicleMgr->activeVehicle();
        QVERIFY(vehicle);

        QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
        QVERIFY_SIGNAL_WAIT(spyParamsReady, TestTimeout::longMs());

    } else if (!px4) {
        _connectMockLink(firmwareType);
        QVERIFY(_vehicle);

    } else if (xReady) {
        // PX4 normal path — params will become ready
        _mockLink = _startPX4MockLinkNoIncrement(failMode);
        if (changeParam) {
            _mockLink->setMockParamValue(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("BAT1_V_CHARGED"), 99.0f);
        }
        if (hashCheckNoResponse) {
            _mockLink->setHashCheckNoResponse(true);
        }
        _connectAndWaitForParams();

    } else {
        // PX4 path where params never become ready (both timers exhaust)
        _mockLink = _startPX4MockLinkNoIncrement(failMode);
        if (hashCheckNoResponse) {
            _mockLink->setHashCheckNoResponse(true);
        }

        MultiVehicleManager *const vehicleMgr = MultiVehicleManager::instance();
        QVERIFY(vehicleMgr);

        QSignalSpy spyVehicle(vehicleMgr, &MultiVehicleManager::activeVehicleAvailableChanged);
        QVERIFY_SIGNAL_WAIT(spyVehicle, TestTimeout::mediumMs());

        Vehicle *const vehicle = vehicleMgr->activeVehicle();
        QVERIFY(vehicle);

        QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
        const int maxWaitMs = ParameterManager::kHashCheckTimeoutMs
                            + ParameterManager::kTestMaxInitialRequestTimeMs
                            + TestTimeout::shortMs();
        QVERIFY_NO_SIGNAL_WAIT(spyParamsReady, maxWaitMs);
    }

    // Phase 4: Verify outcomes
    QCOMPARE(_mockLink->hashCheckRequestCount() > 0, xHashCheck);

    if (xReady) {
        Vehicle *const vehicle = MultiVehicleManager::instance()->activeVehicle();
        QVERIFY(vehicle);
        QVERIFY(vehicle->parameterManager()->parametersReady());
        QCOMPARE(vehicle->parameterManager()->missingParameters(), xMissing);
    }

    // PARAM_REQUEST_LIST is only checked for PX4 (ArduPilot uses FTP)
    if (px4) {
        QCOMPARE(_mockLink->receivedMavlinkMessageCount(MAVLINK_MSG_ID_PARAM_REQUEST_LIST) > 0, xPRL);
    }

    // Phase 5: Cleanup for tests that used _connectMockLink
    if (manualRefresh || !px4) {
        _disconnectMockLink();
    }
}

UT_REGISTER_TEST(HashCheckTest, TestLabel::Integration, TestLabel::Vehicle, TestLabel::Serial)
