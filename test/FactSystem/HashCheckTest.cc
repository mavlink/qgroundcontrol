#include "HashCheckTest.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "LinkManager.h"
#include "MAVLinkLib.h"
#include "MockConfiguration.h"
#include "MockLinkFTP.h"
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
// FirstConnect_NoCache:   First PX4 connection with no cache. Sends _HASH_CHECK, then FTP param.pck.
// Reconnect_CacheHit:     Reconnect after populating cache. Hash matches so no download needed.
// Reconnect_CacheMiss:    Reconnect after a parameter changed. Hash mismatch, FTP param.pck.
// HashTimeout_CacheHit:   Vehicle never responds to _HASH_CHECK. Falls back to FTP param.pck.
// HashTimeout_NoCache:    Vehicle never responds to _HASH_CHECK and no cache exists. Falls back to FTP.
// HashTimeout_CacheStale: Vehicle never responds to _HASH_CHECK and cache is stale. Falls back to FTP.
// BothTimersExhaust:      No hash, FTP file missing, PARAM_REQUEST_LIST ignored. Params never become ready.
// CacheDeleted_Between:   Cache populated then deleted before reconnect. FTP param.pck.
// ManualRefresh:          User-triggered refreshAllParameters() bypasses _HASH_CHECK and uses FTP.
// ArduPilot:              ArduPilot uses FTP for parameters, so no _HASH_CHECK or PARAM_REQUEST_LIST traffic.
// HighLatency:            High-latency links skip parameter download entirely; params marked as missing.
//
// PARAM_REQUEST_LIST (xPRL) is asserted for PX4 only when the hash miss falls back
// to the conventional stream (FTP @PARAM/param.pck unavailable). A hash miss with
// FTP enabled does not send PARAM_REQUEST_LIST.
// A cache hit (xCache) is the only path that sends PARAM_SET during connect: it answers
// the vehicle with _HASH_CHECK instead of downloading.
// missingParameters (xMiss) is only asserted when expectParametersReady (xReady) is true.

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
    QTest::addColumn<bool>("xCacheHit");
    QTest::addColumn<bool>("xPRL");
    QTest::addColumn<bool>("xReady");
    QTest::addColumn<bool>("xMissing");

    //                                          px4      hl       popC     chgP     delC     noResp   failNR   manRef    xHC      xCache   xPRL     xReady   xMiss
    QTest::newRow("FirstConnect_NoCache")    << true  << false << false << false << false << false << false << false  << true  << false << false << true  << false;
    QTest::newRow("Reconnect_CacheHit")      << true  << false << true  << false << false << false << false << false  << true  << true  << false << true  << false;
    QTest::newRow("Reconnect_CacheMiss")     << true  << false << true  << true  << false << false << false << false  << true  << false << false << true  << false;
    QTest::newRow("HashTimeout_CacheHit")    << true  << false << true  << false << false << true  << false << false  << true  << false << false << true  << false;
    QTest::newRow("HashTimeout_NoCache")     << true  << false << false << false << false << true  << false << false  << true  << false << false << true  << false;
    QTest::newRow("HashTimeout_CacheStale")  << true  << false << true  << true  << false << true  << false << false  << true  << false << false << true  << false;
    QTest::newRow("BothTimersExhaust")       << true  << false << false << false << false << true  << true  << false  << true  << false << true  << false << false;
    QTest::newRow("CacheDeleted_Between")    << true  << false << true  << false << true  << false << false << false  << true  << false << false << true  << false;
    QTest::newRow("ManualRefresh")           << true  << false << false << false << false << false << false << true   << false << false << false << true  << false;
    QTest::newRow("ArduPilot")              << false  << false << false << false << false << false << false << false  << false << false << false << true  << false;
    QTest::newRow("HighLatency")             << true  << true  << false << false << false << false << false << false  << false << false << false << true  << true;
}

void HashCheckTest::_hashCheckMatrix()
{
    // ArduPilot and high-latency variants do not have a metadata source;
    // the resulting warning is expected for those rows.
    ignoreLogMessage("ComponentInformation.RequestMetaDataTypeStateMachine", QtWarningMsg,
                     QRegularExpression("failed to load metadata"));
    // BothTimersExhaust row: parameters never become ready, producing a showAppMessage for the timeout.
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression("did not respond to request for parameters"));
    QFETCH(bool, px4);
    QFETCH(bool, highLatency);
    QFETCH(bool, populateCache);
    QFETCH(bool, changeParam);
    QFETCH(bool, deleteCache);
    QFETCH(bool, hashCheckNoResponse);
    QFETCH(bool, failNoResponse);
    QFETCH(bool, manualRefresh);
    QFETCH(bool, xHashCheck);
    QFETCH(bool, xCacheHit);
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

        // refreshAllParameters() toggles parametersReady (false -> true) as
        // the refresh state machine restarts and completes. Wait on the
        // ready-transition rather than a fixed 100 ms sleep so the test
        // advances as soon as the refresh is observable. The 100 ms ceiling
        // matches the previous qWait bound so unexpected regressions still
        // fail loudly.
        QSignalSpy readySpy(_vehicle->parameterManager(),
                            &ParameterManager::parametersReadyChanged);
        _vehicle->parameterManager()->refreshAllParameters();
        (void) readySpy.wait(100);
        QCoreApplication::processEvents();

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
        if (failNoResponse) {
            _mockLink->mockLinkFTP()->setParamPckEnabled(false);
        }
        if (changeParam) {
            _mockLink->setMockParamValue(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("BAT1_V_CHARGED"), 99.0f);
        }
        if (hashCheckNoResponse) {
            _mockLink->setHashCheckNoResponse(true);
        }
        _connectAndWaitForParams();

    } else {
        // PX4 path where params never become ready (hash, FTP, and PARAM_REQUEST_LIST all fail)
        _mockLink = _startPX4MockLinkNoIncrement(failMode);
        if (failNoResponse) {
            _mockLink->mockLinkFTP()->setParamPckEnabled(false);
        }
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
        const int maxWaitMs = ParameterManager::kTestHashCheckTimeoutMs
                            + ParameterManager::kTestMaxInitialRequestTimeMs
                            + TestTimeout::shortMs();
        QVERIFY_NO_SIGNAL_WAIT(spyParamsReady, maxWaitMs);
    }

    // Phase 4: Verify outcomes
    QCOMPARE(_mockLink->hashCheckRequestCount() > 0, xHashCheck);
    // The PARAM_SET reply is the last thing a cache hit sends, so it may still be in flight
    QCOMPARE_TRUE_WAIT(_mockLink->receivedMavlinkMessageCount(MAVLINK_MSG_ID_PARAM_SET) > 0, xCacheHit, TestTimeout::shortMs());

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
