#include "ParameterManagerTest.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>

#include <cmath>
#include <limits>

#include "BulkRefreshJob.h"
#include "MockLinkCamera.h"
#include "MockLinkFTP.h"
#include "MultiVehicleManager.h"
#include "ParameterManager.h"
#include "QGCMath.h"
#include "Vehicle.h"

namespace {
    /// Int typed ext param served by MockLinkCamera on MAV_COMP_ID_CAMERA
    const QString kExtIntParam = QStringLiteral("CAM_EXPMODE");
}

// Call from tests that deliberately let PARAM_SET / PARAM_REQUEST_READ waits time out.
void ParameterManagerTest::_ignoreParamResponseTimeouts()
{
    ignoreLogMessage("Utilities.QGCStateMachine", QtWarningMsg,
                     QRegularExpression("Timeout \".*WaitForParamResponseState\""));
}

// Call from tests that deliberately let PARAM_EXT_ACK waits time out.
void ParameterManagerTest::_ignoreExtParamAckTimeouts()
{
    ignoreLogMessage("Utilities.QGCStateMachine", QtWarningMsg,
                     QRegularExpression("Timeout \".*WaitForMavlinkMessageState.*\""));
}

void ParameterManagerTest::cleanup()
{
    // Some tests create MockLink directly (not via _connectMockLink), so we need special handling.
    // For those cases, disconnect directly and wait for cleanup before calling base class.
    if (_mockLink && MultiVehicleManager::instance()->activeVehicle()) {
        _mockLink->disconnect();
        _mockLink = nullptr;
        QSignalSpy spy(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
        if (!UnitTest::waitForSignal(spy, TestTimeout::mediumMs(), QStringLiteral("activeVehicleChanged"))) {
            qWarning() << "ParameterManagerTest::cleanup: timeout waiting for vehicle disconnect";
        }
    }
    VehicleTestManualConnect::cleanup();
}

/// Test failure modes which should still lead to param load success
void ParameterManagerTest::_noFailureWorker(MockConfiguration::FailureMode_t failureMode)
{
    QVERIFY2(!_mockLink, "MockLink already connected");
    _mockLink = MockLink::startPX4MockLink(MockConfiguration::OptionNone, failureMode);
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QVERIFY(vehicleMgr);
    // Wait for the Vehicle to get created
    QSignalSpy spyVehicle(vehicleMgr, &MultiVehicleManager::activeVehicleAvailableChanged);
    QVERIFY_SIGNAL_WAIT(spyVehicle, TestTimeout::mediumMs());
    QCOMPARE(spyVehicle.count(), 1);
    QList<QVariant> arguments = spyVehicle.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toBool(), true);
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);
    // We should get progress bar updates during load
    QSignalSpy spyProgress(vehicle->parameterManager(), &ParameterManager::loadProgressChanged);
    QVERIFY_SIGNAL_WAIT(spyProgress, TestTimeout::mediumMs());
    arguments = spyProgress.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QVERIFY(arguments.at(0).toFloat() > 0.0f);
    // When param load is complete we get the param ready signal
    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    QVERIFY_SIGNAL_WAIT(spyParamsReady, TestTimeout::longMs());
    arguments = spyParamsReady.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toBool(), true);
    // Progress should have been set back to 0
    arguments = spyProgress.takeLast();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toFloat(), 0.0f);
}

void ParameterManagerTest::_noFailure()
{
    _noFailureWorker(MockConfiguration::FailNone);
}

void ParameterManagerTest::_requestListMissingParamSuccess()
{
    _noFailureWorker(MockConfiguration::FailMissingParamOnInitialRequest);
}

// Test no response to param_request_list
void ParameterManagerTest::_requestListNoResponse()
{
    QVERIFY2(!_mockLink, "MockLink already connected");
    _mockLink = MockLink::startPX4MockLink(MockConfiguration::OptionNone, MockConfiguration::FailParamNoResponseToRequestList);
    _mockLink->mockLinkFTP()->setParamPckEnabled(false);
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QVERIFY(vehicleMgr);
    // Wait for the Vehicle to get created
    QSignalSpy spyVehicle(vehicleMgr, &MultiVehicleManager::activeVehicleAvailableChanged);
    QVERIFY_SIGNAL_WAIT(spyVehicle, TestTimeout::mediumMs());
    QCOMPARE(spyVehicle.count(), 1);
    QList<QVariant> arguments = spyVehicle.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toBool(), true);
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);
    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    QSignalSpy spyProgress(vehicle->parameterManager(), &ParameterManager::loadProgressChanged);
    expectAppMessage(QRegularExpression("did not respond to request for parameters"));
    // We should not get any progress bar updates, nor a parameter ready signal.
    // ParameterManager exhausts initial request retries in bounded test intervals.
    QVERIFY_NO_SIGNAL_WAIT(spyProgress, TestTimeout::shortMs());
    QVERIFY_NO_SIGNAL_WAIT(spyParamsReady, ParameterManager::kTestMaxInitialRequestTimeMs);
    verifyExpectedLogMessage();
}

// MockLink will fail to send a param on initial request, it will also fail to send it on subsequent
// param_read requests. The packed file is disabled so the stream path is exercised.
void ParameterManagerTest::_requestListMissingParamFail()
{
    _ignoreParamResponseTimeouts();
    QVERIFY2(!_mockLink, "MockLink already connected");
    _mockLink = MockLink::startPX4MockLink(MockConfiguration::OptionNone, MockConfiguration::FailMissingParamOnAllRequests);
    _mockLink->mockLinkFTP()->setParamPckEnabled(false);
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QVERIFY(vehicleMgr);
    // Wait for the Vehicle to get created
    QSignalSpy spyVehicle(vehicleMgr, &MultiVehicleManager::activeVehicleAvailableChanged);
    QVERIFY_SIGNAL_WAIT(spyVehicle, TestTimeout::mediumMs());
    QCOMPARE(spyVehicle.count(), 1);
    QList<QVariant> arguments = spyVehicle.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toBool(), true);
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);
    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    QSignalSpy spyProgress(vehicle->parameterManager(), &ParameterManager::loadProgressChanged);
    // We will get progress bar updates, since it will fail after getting partially through the request
    QVERIFY_SIGNAL_WAIT(spyProgress, TestTimeout::mediumMs());
    arguments = spyProgress.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QVERIFY(arguments.at(0).toFloat() > 0.0f);
    expectAppMessage(QRegularExpression("was unable to retrieve the full set of parameters"));
    // We should get a parameters ready signal, but Vehicle should indicate missing params
    QVERIFY_SIGNAL_WAIT(spyParamsReady, TestTimeout::longMs());
    QCOMPARE(vehicle->parameterManager()->missingParameters(), true);
    verifyExpectedLogMessage();
}

void ParameterManagerTest::_paramWriteNoAckRetry()
{
    _ignoreParamResponseTimeouts();
    // BAT1_V_CHARGED requires a vehicle reboot, so writing it pops the reboot
    // app message (debounce is reset per-test by the framework)
    expectAppMessage(QRegularExpression("Reboot vehicle for changes to take effect"));
    _setParamWithFailureMode(MockLink::FailParamSetFirstAttemptNoAck, true /* expectSuccess */,
                             QStringLiteral("BAT1_V_CHARGED"), MAV_AUTOPILOT_PX4);
    verifyExpectedLogMessage();
}

void ParameterManagerTest::_paramWriteNoAckPermanent()
{
    _ignoreParamResponseTimeouts();
    // Expectations verify in FIFO order: reboot message first (fires at local
    // setRawValue), then the write-failed message (fires after retries exhaust)
    expectAppMessage(QRegularExpression("Reboot vehicle for changes to take effect"));
    expectAppMessage(QRegularExpression("Parameter write failed"));
    _setParamWithFailureMode(MockLink::FailParamSetNoAck, false /* expectSuccess */,
                             QStringLiteral("BAT1_V_CHARGED"), MAV_AUTOPILOT_PX4);
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
}

void ParameterManagerTest::_paramWriteUInt8()
{
    _setParamWithFailureMode(MockLink::FailParamSetNone, true /* expectSuccess */,
                             QStringLiteral("TEST_UINT8"), MAV_AUTOPILOT_GENERIC);
}

void ParameterManagerTest::_paramWriteUInt16()
{
    _setParamWithFailureMode(MockLink::FailParamSetNone, true /* expectSuccess */,
                             QStringLiteral("TEST_UINT16"), MAV_AUTOPILOT_GENERIC);
}

void ParameterManagerTest::_paramReadFirstAttemptNoResponseRetry()
{
    _ignoreParamResponseTimeouts();
    QVERIFY2(!_mockLink, "MockLink already connected");
    _connectMockLink();
    QVERIFY(_mockLink);
    QVERIFY(_vehicle);
    ParameterManager* const paramManager = _vehicle->parameterManager();
    QVERIFY(paramManager);
    _mockLink->setParamRequestReadFailureMode(MockLink::FailParamRequestReadFirstAttemptNoResponse);
    Fact* const fact = paramManager->getParameter(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("BAT1_V_CHARGED"));
    QVERIFY(fact);
    QSignalSpy vehicleUpdatedSpy(fact, &Fact::vehicleUpdated);
    QSignalSpy paramReadSuccessSpy(paramManager, &ParameterManager::_paramRequestReadSuccess);
    QVERIFY(vehicleUpdatedSpy.isValid());
    QVERIFY(paramReadSuccessSpy.isValid());
    paramManager->refreshParameter(MAV_COMP_ID_AUTOPILOT1, fact->name());
    const int maxWaitTimeMs = ParameterManager::kWaitForParamValueAckMs
                              * (ParameterManager::kParamRequestReadRetryCount + 1) + TestTimeout::shortMs();
    QVERIFY_SIGNAL_WAIT(paramReadSuccessSpy, maxWaitTimeMs);
    QCOMPARE(paramReadSuccessSpy.count(), 1);
    QCOMPARE(vehicleUpdatedSpy.count(), 1);
    _disconnectMockLink();
}

void ParameterManagerTest::_paramReadNoResponse()
{
    _ignoreParamResponseTimeouts();
    QVERIFY2(!_mockLink, "MockLink already connected");
    _connectMockLink();
    QVERIFY(_mockLink);
    QVERIFY(_vehicle);
    ParameterManager* const paramManager = _vehicle->parameterManager();
    QVERIFY(paramManager);
    Fact* const fact = paramManager->getParameter(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("BAT1_V_CHARGED"));
    QVERIFY(fact);
    QSignalSpy vehicleUpdatedSpy(fact, &Fact::vehicleUpdated);
    QSignalSpy paramReadFailureSpy(paramManager, &ParameterManager::_paramRequestReadFailure);
    QVERIFY(vehicleUpdatedSpy.isValid());
    QVERIFY(paramReadFailureSpy.isValid());
    _mockLink->setParamRequestReadFailureMode(MockLink::FailParamRequestReadNoResponse);
    expectAppMessage(QRegularExpression("Parameter read failed"));
    paramManager->refreshParameter(MAV_COMP_ID_AUTOPILOT1, fact->name());
    const int maxWaitTimeMs = ParameterManager::kWaitForParamValueAckMs
                              * (ParameterManager::kParamRequestReadRetryCount + 1) + TestTimeout::shortMs();
    QVERIFY_SIGNAL_WAIT(paramReadFailureSpy, maxWaitTimeMs);
    QCOMPARE(paramReadFailureSpy.count(), 1);
    QCOMPARE(vehicleUpdatedSpy.count(), 0);
    verifyExpectedLogMessage();
    _disconnectMockLink();
}

void ParameterManagerTest::_paramWriteParamError()
{
    // Expectations verify in FIFO order: reboot message first (fires at local
    // setRawValue), then the write-failed message (fires on the PARAM_ERROR ack)
    expectAppMessage(QRegularExpression("Reboot vehicle for changes to take effect"));
    expectAppMessage(QRegularExpression("Parameter write failed"));
    _setParamWithFailureMode(MockLink::FailParamSetParamError, false /* expectSuccess */,
                             QStringLiteral("BAT1_V_CHARGED"), MAV_AUTOPILOT_PX4);
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
}

void ParameterManagerTest::_paramReadParamError()
{
    QVERIFY2(!_mockLink, "MockLink already connected");
    _connectMockLink();
    QVERIFY(_mockLink);
    QVERIFY(_vehicle);
    ParameterManager* const paramManager = _vehicle->parameterManager();
    QVERIFY(paramManager);
    _mockLink->setParamRequestReadFailureMode(MockLink::FailParamRequestReadParamError);
    Fact* const fact = paramManager->getParameter(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("BAT1_V_CHARGED"));
    QVERIFY(fact);
    QSignalSpy vehicleUpdatedSpy(fact, &Fact::vehicleUpdated);
    QSignalSpy paramReadFailureSpy(paramManager, &ParameterManager::_paramRequestReadFailure);
    QVERIFY(vehicleUpdatedSpy.isValid());
    QVERIFY(paramReadFailureSpy.isValid());
    expectAppMessage(QRegularExpression("Parameter read failed"));
    paramManager->refreshParameter(MAV_COMP_ID_AUTOPILOT1, fact->name());
    // PARAM_ERROR should cause immediate failure - no retries needed, so wait just one ack interval plus buffer
    const int maxWaitTimeMs = ParameterManager::kWaitForParamValueAckMs + TestTimeout::shortMs();
    QVERIFY_SIGNAL_WAIT(paramReadFailureSpy, maxWaitTimeMs);
    QCOMPARE(paramReadFailureSpy.count(), 1);
    QCOMPARE(vehicleUpdatedSpy.count(), 0);
    verifyExpectedLogMessage();
    _disconnectMockLink();
}

void ParameterManagerTest::_setParamWithFailureMode(MockLink::ParamSetFailureMode_t failureMode, bool expectSuccess,
                                                     const QString &paramName, MAV_AUTOPILOT autopilot)
{
    QVERIFY2(!_mockLink, "MockLink already connected");
    if (autopilot == MAV_AUTOPILOT_GENERIC) {
        // Generic mock link has no metadata source; this warning is expected for generic autopilot
        ignoreLogMessage("ComponentInformation.RequestMetaDataTypeStateMachine", QtWarningMsg,
                         QRegularExpression("failed to load metadata"));
    }
    // Bring up a clean mock vehicle for each run
    _connectMockLink(autopilot);
    QVERIFY(_mockLink);
    QVERIFY(_vehicle);
    _mockLink->setParamSetFailureMode(failureMode);
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QVERIFY(vehicleMgr);
    ParameterManager* const paramManager = _vehicle->parameterManager();
    QVERIFY(paramManager);
    QVERIFY(!_vehicle->parameterManager()->pendingWrites());
    Fact* const fact = paramManager->getParameter(MAV_COMP_ID_AUTOPILOT1, paramName);
    QVERIFY(fact);
    QSignalSpy rawValueChangedSpy(fact, &Fact::rawValueChanged);
    const QVariant originalValue = fact->rawValue();
    const double originalDouble = originalValue.toDouble();
    const FactMetaData* const metaData = fact->metaData();
    const double minValue = (metaData && metaData->rawMin().isValid()) ? metaData->rawMin().toDouble()
                                                                       : -std::numeric_limits<double>::infinity();
    const double maxValue = (metaData && metaData->rawMax().isValid()) ? metaData->rawMax().toDouble()
                                                                       : std::numeric_limits<double>::infinity();
    const double step = fact->type() == FactMetaData::valueTypeFloat ? 0.1 : 1.0;
    auto adjustedValue = [&](double candidate) -> double {
        if (candidate > maxValue) {
            candidate = originalDouble - step;
        }
        if (candidate < minValue) {
            candidate = originalDouble + step;
        }
        if (qFuzzyCompare(candidate + 1.0, originalDouble + 1.0)) {
            candidate = originalDouble + (step * 2.0);
        }
        if (candidate > maxValue) {
            candidate = originalDouble - (step * 2.0);
        }
        if (candidate < minValue) {
            candidate = originalDouble;
        }
        return candidate;
    };
    const double newValueDouble = adjustedValue(originalDouble + step);
    QVERIFY(!qFuzzyCompare(newValueDouble + 1.0, originalDouble + 1.0));
    const QVariant newValue(newValueDouble);
    QSignalSpy pendingSpy(paramManager, &ParameterManager::pendingWritesChanged);
    QVERIFY(pendingSpy.isValid());
    QSignalSpy paramSetSuccessSpy(paramManager, &ParameterManager::_paramSetSuccess);
    QVERIFY(paramSetSuccessSpy.isValid());
    QSignalSpy paramSetFailureSpy(paramManager, &ParameterManager::_paramSetFailure);
    QVERIFY(paramSetFailureSpy.isValid());
    fact->setRawValue(newValue);
    // We should see pendingWrites go to true and then back to false
    bool sawPendingTrue = false;
    bool sawPendingFalse = false;
    int processedCount = 0;
    const auto processPendingSignals = [&]() {
        while (processedCount < pendingSpy.count()) {
            const QList<QVariant> arguments = pendingSpy.at(processedCount++);
            QCOMPARE(arguments.count(), 1);
            const bool isPending = arguments.at(0).toBool();
            if (isPending) {
                sawPendingTrue = true;
            } else {
                sawPendingFalse = true;
            }
        }
    };

    processPendingSignals();
    QElapsedTimer waitTimer;
    waitTimer.start();
    while ((!sawPendingTrue || !sawPendingFalse) && waitTimer.elapsed() < TestTimeout::longMs()) {
        const int remainingMs = TestTimeout::longMs() - static_cast<int>(waitTimer.elapsed());
        if (remainingMs <= 0) {
            break;
        }

        const int nextExpectedCount = processedCount + 1;
        if (!UnitTest::waitForSignalCount(pendingSpy, nextExpectedCount, remainingMs,
                                          QStringLiteral("pendingWritesChanged"))) {
            break;
        }

        processPendingSignals();
    }
    QVERIFY2(sawPendingTrue, "Expected pendingWritesChanged(true) signal");
    QVERIFY2(sawPendingFalse, "Expected pendingWritesChanged(false) signal");
    // We should get two rawValueChanged signals if unsuccessful (one for the set, one for the refresh)
    // We should get one rawValueChanged signal if successful (just the set)
    QVERIFY_SIGNAL_COUNT_WAIT(
        rawValueChangedSpy, 1,
        ParameterManager::kWaitForParamValueAckMs * ParameterManager::kParamSetRetryCount + TestTimeout::shortMs());
    const int maxSetResultWaitMs = (ParameterManager::kWaitForParamValueAckMs * ParameterManager::kParamSetRetryCount)
                                   + TestTimeout::mediumMs();
    if (expectSuccess) {
        QVERIFY_SIGNAL_COUNT_WAIT(paramSetSuccessSpy, 1, maxSetResultWaitMs);
        QCOMPARE(rawValueChangedSpy.count(), 1);
        QCOMPARE(paramSetSuccessSpy.count(), 1);
        QCOMPARE(paramSetFailureSpy.count(), 0);
    } else {
        QVERIFY_SIGNAL_COUNT_WAIT(paramSetFailureSpy, 1, maxSetResultWaitMs);
        QVERIFY(rawValueChangedSpy.count() == 1 || rawValueChangedSpy.count() == 2);
        QCOMPARE(paramSetSuccessSpy.count(), 0);
        QCOMPARE(paramSetFailureSpy.count(), 1);
    }
    // The first signal is the change we made, so we start checking from there
    QVERIFY(QGC::fuzzyCompare(rawValueChangedSpy[0][0].toDouble(), newValueDouble, 0.00001));
    if (!expectSuccess) {
        // If the write failed the second signal should be the value reverting to original
        if (rawValueChangedSpy.count() == 1) {
            // Wait a bit longer for the refresh to come in
            QVERIFY_SIGNAL_COUNT_WAIT(rawValueChangedSpy, 2, TestTimeout::shortMs());
        }
        QCOMPARE(rawValueChangedSpy.count(), 2);
        QVERIFY(QGC::fuzzyCompare(rawValueChangedSpy[1][0].toDouble(), originalDouble, 0.00001));
        // We should have also alerted the user of the failure
        // Note that we can't easily check for the ShowAppMessageState usage here due to the
        // complexity of the state machine and timing of the signals.
    }
    _mockLink->setParamSetFailureMode(MockLink::FailParamSetNone);
    _disconnectMockLink();
}

void ParameterManagerTest::_FTPnoFailure()
{
    // Test APM FTP-based parameter download (param.pck).
    // FailParamNoResponseToRequestList forces the FTP path by blocking PARAM_REQUEST_LIST.
    QVERIFY2(!_mockLink, "MockLink already connected");
    _mockLink = MockLink::startAPMArduPlaneMockLink(MockConfiguration::OptionNone, MockConfiguration::FailParamNoResponseToRequestList);

    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QVERIFY(vehicleMgr);

    QSignalSpy spyVehicle(vehicleMgr, &MultiVehicleManager::activeVehicleAvailableChanged);
    QVERIFY_SIGNAL_WAIT(spyVehicle, TestTimeout::mediumMs());
    QCOMPARE(spyVehicle.count(), 1);
    QCOMPARE(spyVehicle.first().at(0).toBool(), true);

    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);

    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    QVERIFY_SIGNAL_WAIT(spyParamsReady, TestTimeout::longMs());
    QCOMPARE(spyParamsReady.takeFirst().at(0).toBool(), true);

    // Verify FTP was used and PARAM_REQUEST_LIST was not
    QVERIFY2(_mockLink->receivedMavlinkMessageCount(MAVLINK_MSG_ID_FILE_TRANSFER_PROTOCOL) > 0, "FTP messages should have been sent");
    QCOMPARE(_mockLink->receivedMavlinkMessageCount(MAVLINK_MSG_ID_PARAM_REQUEST_LIST), 0);

    // Verify parameters were loaded with correct values from param.pck
    ParameterManager* paramManager = vehicle->parameterManager();
    QVERIFY(paramManager);
    QVERIFY(paramManager->parametersReady());
    QVERIFY(paramManager->parameterExists(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("BATT_LOW_VOLT")));
    QVERIFY(paramManager->parameterExists(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("RC1_MIN")));
    QVERIFY(paramManager->parameterExists(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("BATT2_MONITOR")));

    Fact* battLowVoltFact = paramManager->getParameter(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("BATT_LOW_VOLT"));
    Fact* rc1MinFact      = paramManager->getParameter(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("RC1_MIN"));
    Fact* batt2MonFact    = paramManager->getParameter(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("BATT2_MONITOR"));

    QCOMPARE(battLowVoltFact->rawValue().toFloat(), 0.0f);
    QCOMPARE(rc1MinFact->rawValue().toInt(), 1000);
    QCOMPARE(batt2MonFact->rawValue().toInt(), 4);
}

void ParameterManagerTest::_FTPChangeParam()
{
    // Test that parameter set works after APM FTP param download
    QVERIFY2(!_mockLink, "MockLink already connected");
    _mockLink = MockLink::startAPMArduPlaneMockLink(MockConfiguration::OptionNone, MockConfiguration::FailParamNoResponseToRequestList);

    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QVERIFY(vehicleMgr);

    QSignalSpy spyVehicle(vehicleMgr, &MultiVehicleManager::activeVehicleAvailableChanged);
    QVERIFY_SIGNAL_WAIT(spyVehicle, TestTimeout::mediumMs());
    QCOMPARE(spyVehicle.takeFirst().at(0).toBool(), true);

    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);

    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    QVERIFY_SIGNAL_WAIT(spyParamsReady, TestTimeout::longMs());
    QCOMPARE(spyParamsReady.takeFirst().at(0).toBool(), true);

    ParameterManager* paramManager = vehicle->parameterManager();
    QVERIFY(paramManager);

    // Change a float parameter and verify it round-trips
    Fact* fact = paramManager->getParameter(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("BATT_LOW_VOLT"));
    QVERIFY(fact);
    const QVariant originalValue = fact->rawValue();
    const float testValue = originalValue.toFloat() + 1.5f;

    QSignalSpy spyValueChanged(fact, &Fact::vehicleUpdated);
    fact->setRawValue(QVariant(testValue));
    QVERIFY_SIGNAL_WAIT(spyValueChanged, TestTimeout::mediumMs());
    QCOMPARE(fact->rawValue().toFloat(), testValue);
}

UT_REGISTER_TEST(ParameterManagerTest, TestLabel::Integration, TestLabel::Vehicle, TestLabel::Serial)

// ---------------------------------------------------------------------------
// bulkRefresh tests
// ---------------------------------------------------------------------------

// Two exact param names — both should resolve and succeed on round 0.
void ParameterManagerTest::_bulkRefreshExactNamesAllSucceed()
{
    _connectMockLink();
    QVERIFY(_vehicle);
    ParameterManager* const paramManager = _vehicle->parameterManager();
    QVERIFY(paramManager);

    QSignalSpy successSpy(paramManager, &ParameterManager::_paramRequestReadSuccess);
    QSignalSpy failureSpy(paramManager, &ParameterManager::_paramRequestReadFailure);
    QVERIFY(successSpy.isValid());
    QVERIFY(failureSpy.isValid());

    paramManager->bulkRefresh(MAV_COMP_ID_AUTOPILOT1,
                               {QStringLiteral("BAT1_V_CHARGED"), QStringLiteral("BAT1_N_CELLS")});

    const int maxWaitMs = ParameterManager::kWaitForParamValueAckMs
                          * (ParameterManager::kParamRequestReadRetryCount + 1)
                          + TestTimeout::shortMs();
    QVERIFY_SIGNAL_COUNT_WAIT(successSpy, 2, maxWaitMs);
    QCOMPARE(failureSpy.count(), 0);

    QStringList succeededNames;
    for (int i = 0; i < successSpy.count(); ++i) {
        succeededNames << successSpy.at(i).at(1).toString();
    }
    QVERIFY(succeededNames.contains(QStringLiteral("BAT1_V_CHARGED")));
    QVERIFY(succeededNames.contains(QStringLiteral("BAT1_N_CELLS")));

    _disconnectMockLink();
}

// A wildcard prefix "BAT1_*" should expand to all BAT1_ parameters and all should succeed.
void ParameterManagerTest::_bulkRefreshPrefixExpansion()
{
    _connectMockLink();
    QVERIFY(_vehicle);
    ParameterManager* const paramManager = _vehicle->parameterManager();
    QVERIFY(paramManager);

    // Count BAT1_ params actually present on the mock vehicle.
    int bat1Count = 0;
    for (const QString &name : paramManager->parameterNames(MAV_COMP_ID_AUTOPILOT1)) {
        if (name.startsWith(QStringLiteral("BAT1_"))) {
            ++bat1Count;
        }
    }
    QVERIFY2(bat1Count > 0, "Mock link must have at least one BAT1_ parameter");

    QSignalSpy successSpy(paramManager, &ParameterManager::_paramRequestReadSuccess);
    QSignalSpy failureSpy(paramManager, &ParameterManager::_paramRequestReadFailure);
    QVERIFY(successSpy.isValid());
    QVERIFY(failureSpy.isValid());

    paramManager->bulkRefresh(MAV_COMP_ID_AUTOPILOT1, {QStringLiteral("BAT1_*")});

    const int maxWaitMs = ParameterManager::kWaitForParamValueAckMs
                          * (ParameterManager::kParamRequestReadRetryCount + 1)
                          + TestTimeout::shortMs();
    QVERIFY_SIGNAL_COUNT_WAIT(successSpy, bat1Count, maxWaitMs);
    QCOMPARE(failureSpy.count(), 0);
    QCOMPARE(successSpy.count(), bat1Count);

    _disconnectMockLink();
}

// Passing only non-existent names should resolve to an empty set — no requests are sent.
void ParameterManagerTest::_bulkRefreshUnknownNameSkipped()
{
    _connectMockLink();
    QVERIFY(_vehicle);
    ParameterManager* const paramManager = _vehicle->parameterManager();
    QVERIFY(paramManager);

    QSignalSpy successSpy(paramManager, &ParameterManager::_paramRequestReadSuccess);
    QSignalSpy failureSpy(paramManager, &ParameterManager::_paramRequestReadFailure);
    QVERIFY(successSpy.isValid());
    QVERIFY(failureSpy.isValid());

    expectLogMessage("FactSystem.ParameterManager", QtWarningMsg, QRegularExpression("bulkRefresh: unknown param name \\(skipped\\):.*ZZZZ_DOES_NOT_EXIST"));
    paramManager->bulkRefresh(MAV_COMP_ID_AUTOPILOT1, {QStringLiteral("ZZZZ_DOES_NOT_EXIST")});
    verifyExpectedLogMessage();

    QVERIFY_NO_SIGNAL_WAIT(successSpy, TestTimeout::shortMs());
    QCOMPARE(failureSpy.count(), 0);

    _disconnectMockLink();
}

// Round 0 fails (no response from MockLink), round 1 succeeds after failure mode is cleared.
void ParameterManagerTest::_bulkRefreshRetrySucceeds()
{
    _ignoreParamResponseTimeouts();
    _connectMockLink();
    QVERIFY(_mockLink);
    QVERIFY(_vehicle);
    ParameterManager* const paramManager = _vehicle->parameterManager();
    QVERIFY(paramManager);

    QSignalSpy successSpy(paramManager, &ParameterManager::_paramRequestReadSuccess);
    QSignalSpy failureSpy(paramManager, &ParameterManager::_paramRequestReadFailure);
    QVERIFY(successSpy.isValid());
    QVERIFY(failureSpy.isValid());

    // Round 0: MockLink drops all responses — per-param SM exhausts retries → _paramRequestReadFailure
    _mockLink->setParamRequestReadFailureMode(MockLink::FailParamRequestReadNoResponse);
    paramManager->bulkRefresh(MAV_COMP_ID_AUTOPILOT1, {QStringLiteral("BAT1_V_CHARGED")});

    const int roundTimeMs = ParameterManager::kWaitForParamValueAckMs
                            * (ParameterManager::kParamRequestReadRetryCount + 1)
                            + TestTimeout::shortMs();
    QVERIFY_SIGNAL_WAIT(failureSpy, roundTimeMs);
    QCOMPARE(failureSpy.count(), 1);

    // Allow BulkRefreshJob's retry round to succeed
    _mockLink->setParamRequestReadFailureMode(MockLink::FailParamRequestReadNone);
    QVERIFY_SIGNAL_WAIT(successSpy, roundTimeMs);
    QCOMPARE(successSpy.count(), 1);
    QCOMPARE(successSpy.at(0).at(1).toString(), QStringLiteral("BAT1_V_CHARGED"));

    _disconnectMockLink();
}

// All kMaxRetryRounds+1 rounds fail — BulkRefreshJob gives up without a success signal.
void ParameterManagerTest::_bulkRefreshAllRetriesExhausted()
{
    _ignoreParamResponseTimeouts();
    _connectMockLink();
    QVERIFY(_mockLink);
    QVERIFY(_vehicle);
    ParameterManager* const paramManager = _vehicle->parameterManager();
    QVERIFY(paramManager);

    QSignalSpy successSpy(paramManager, &ParameterManager::_paramRequestReadSuccess);
    QSignalSpy failureSpy(paramManager, &ParameterManager::_paramRequestReadFailure);
    QVERIFY(successSpy.isValid());
    QVERIFY(failureSpy.isValid());

    _mockLink->setParamRequestReadFailureMode(MockLink::FailParamRequestReadNoResponse);
    paramManager->bulkRefresh(MAV_COMP_ID_AUTOPILOT1, {QStringLiteral("BAT1_V_CHARGED")});

    // Each round exhausts the per-param SM retries. Upper bound uses production constants;
    // actual duration is ~1s in test mode (kWaitForParamValueAckMs and kRetryBaseDelayMs are
    // reduced to 50ms when QGC::runningUnitTests() is true).
    const int roundTimeMs = ParameterManager::kWaitForParamValueAckMs
                            * (ParameterManager::kParamRequestReadRetryCount + 1);
    const int maxWaitMs = roundTimeMs * (BulkRefreshJob::kMaxRetryRounds + 1) + TestTimeout::mediumMs();
    expectAppMessage(QRegularExpression("Parameter refresh failed"));
    QVERIFY_SIGNAL_COUNT_WAIT(failureSpy, BulkRefreshJob::kMaxRetryRounds + 1, maxWaitMs);
    QCOMPARE(successSpy.count(), 0);
    verifyExpectedLogMessage();

    _disconnectMockLink();
}

// MockLinkCamera serves ext params on camera 1 only, so this also covers that components which
// don't answer PARAM_EXT_REQUEST_LIST are simply left out of the parameter view.
void ParameterManagerTest::_extParamsDownloaded()
{
    _connectMockLink(MAV_AUTOPILOT_PX4, MockConfiguration::FailNone, MockConfiguration::OptionEnableCamera);
    QVERIFY(_vehicle);
    ParameterManager* const paramManager = _vehicle->parameterManager();
    QVERIFY(paramManager);

    QVERIFY_TRUE_WAIT(paramManager->componentIds().contains(MAV_COMP_ID_CAMERA), TestTimeout::longMs());
    QVERIFY(!paramManager->componentIds().contains(MAV_COMP_ID_CAMERA2));

    const QVector<MockLinkCamera::ExtParam> expectedParams = MockLinkCamera::defaultExtParams();
    for (const MockLinkCamera::ExtParam& expectedParam: expectedParams) {
        QVERIFY_TRUE_WAIT(paramManager->extParameterExists(MAV_COMP_ID_CAMERA, expectedParam.name), TestTimeout::mediumMs());
        Fact* const fact = paramManager->getParameter(MAV_COMP_ID_CAMERA, expectedParam.name);
        QVERIFY(fact);
        QCOMPARE(fact->rawValue().toString(), expectedParam.value.toString());
    }

    // Ext params must not shadow or disturb the autopilot parameters
    QVERIFY(paramManager->parametersReady());
    QVERIFY(!paramManager->extParameterExists(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("CAM_EV")));

    _disconnectMockLink();
}

Fact* ParameterManagerTest::_connectAndWaitForExtParams()
{
    _connectMockLink(MAV_AUTOPILOT_PX4, MockConfiguration::FailNone, MockConfiguration::OptionEnableCamera);
    if (!_vehicle || !_vehicle->parameterManager()) {
        return nullptr;
    }

    ParameterManager* const paramManager = _vehicle->parameterManager();
    if (!UnitTest::waitForCondition([paramManager]() { return paramManager->extParameterExists(MAV_COMP_ID_CAMERA, kExtIntParam); },
                                    TestTimeout::longMs(),
                                    QStringLiteral("ext params downloaded"))) {
        return nullptr;
    }

    return paramManager->getParameter(MAV_COMP_ID_CAMERA, kExtIntParam);
}

void ParameterManagerTest::_extParamWrite()
{
    Fact* const fact = _connectAndWaitForExtParams();
    QVERIFY(fact);
    ParameterManager* const paramManager = _vehicle->parameterManager();

    QSignalSpy successSpy(paramManager, &ParameterManager::_paramSetSuccess);
    QVERIFY(successSpy.isValid());

    fact->setRawValue(3);
    QVERIFY_SIGNAL_WAIT(successSpy, TestTimeout::mediumMs());
    QCOMPARE(_mockLink->mockLinkCamera()->extParamValue(kExtIntParam).toInt(), 3);

    // A re-read must go out over the ext protocol and come back with the value the camera stored
    fact->containerSetRawValue(0);
    paramManager->refreshParameter(MAV_COMP_ID_CAMERA, kExtIntParam);
    QCOMPARE_TRUE_WAIT(fact->rawValue().toInt(), 3, TestTimeout::mediumMs());

    _disconnectMockLink();
}

// A camera which refuses the value must leave the vehicle value in place and tell the user,
// rather than leaving the editor showing a value the camera never took.
void ParameterManagerTest::_extParamWriteRejected()
{
    Fact* const fact = _connectAndWaitForExtParams();
    QVERIFY(fact);
    ParameterManager* const paramManager = _vehicle->parameterManager();

    const int originalValue = fact->rawValue().toInt();
    _mockLink->mockLinkCamera()->setExtParamSetFailureMode(MockLinkCamera::FailExtParamSetRejected);

    QSignalSpy failureSpy(paramManager, &ParameterManager::_paramSetFailure);
    QVERIFY(failureSpy.isValid());

    expectAppMessage(QRegularExpression("Parameter write failed"));
    fact->setRawValue(originalValue + 1);
    QVERIFY_SIGNAL_WAIT(failureSpy, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    QCOMPARE(_mockLink->mockLinkCamera()->extParamValue(kExtIntParam).toInt(), originalValue);
    QCOMPARE_TRUE_WAIT(fact->rawValue().toInt(), originalValue, TestTimeout::mediumMs());

    _disconnectMockLink();
}

// Retries must not leave the pending write count stranded, which would keep warning the user
// about unsaved parameters on app close.
void ParameterManagerTest::_extParamWriteNoAck()
{
    _ignoreExtParamAckTimeouts();

    Fact* const fact = _connectAndWaitForExtParams();
    QVERIFY(fact);
    ParameterManager* const paramManager = _vehicle->parameterManager();

    const int originalValue = fact->rawValue().toInt();
    _mockLink->mockLinkCamera()->setExtParamSetFailureMode(MockLinkCamera::FailExtParamSetNoAck);

    QSignalSpy failureSpy(paramManager, &ParameterManager::_paramSetFailure);
    QVERIFY(failureSpy.isValid());

    expectAppMessage(QRegularExpression("Parameter write failed"));
    fact->setRawValue(originalValue + 1);

    const int maxWaitMs = ParameterManager::kWaitForParamValueAckMs * (ParameterManager::kParamSetRetryCount + 1)
                          + TestTimeout::mediumMs();
    QVERIFY_SIGNAL_WAIT(failureSpy, maxWaitMs);
    verifyExpectedLogMessage();

    QVERIFY_TRUE_WAIT(!paramManager->pendingWrites(), TestTimeout::mediumMs());

    _disconnectMockLink();
}

// PARAM_ACK_IN_PROGRESS must not wedge or abandon the write. The delayed second ack a camera
// would send cannot be simulated here - the test ack timeout is shorter than the mock's task
// tick - so this covers the retry that follows instead.
void ParameterManagerTest::_extParamWriteInProgress()
{
    _ignoreExtParamAckTimeouts();

    Fact* const fact = _connectAndWaitForExtParams();
    QVERIFY(fact);
    ParameterManager* const paramManager = _vehicle->parameterManager();

    const int newValue = fact->rawValue().toInt() + 1;
    _mockLink->mockLinkCamera()->setExtParamSetFailureMode(MockLinkCamera::FailExtParamSetInProgress);

    QSignalSpy successSpy(paramManager, &ParameterManager::_paramSetSuccess);
    QSignalSpy failureSpy(paramManager, &ParameterManager::_paramSetFailure);
    QVERIFY(successSpy.isValid());
    QVERIFY(failureSpy.isValid());

    fact->setRawValue(newValue);

    const int maxWaitMs = ParameterManager::kWaitForParamValueAckMs * (ParameterManager::kParamSetRetryCount + 1)
                          + TestTimeout::mediumMs();
    QVERIFY_SIGNAL_WAIT(successSpy, maxWaitMs);
    QCOMPARE(failureSpy.count(), 0);
    QCOMPARE(_mockLink->mockLinkCamera()->extParamValue(kExtIntParam).toInt(), newValue);

    _disconnectMockLink();
}

// A value lost from the PARAM_EXT_REQUEST_LIST stream must be picked up by the indexed re-request,
// otherwise a dropped packet silently hides a parameter from the editor.
void ParameterManagerTest::_extParamMissingIndexRetry()
{
    _connectMockLink(MAV_AUTOPILOT_PX4, MockConfiguration::FailNone, MockConfiguration::OptionEnableCamera);
    QVERIFY(_vehicle);
    ParameterManager* const paramManager = _vehicle->parameterManager();
    QVERIFY(paramManager);
    QVERIFY(_mockLink->mockLinkCamera());

    // Index 0 is dropped from the stream, so it can only arrive via the indexed re-request
    _mockLink->mockLinkCamera()->setExtParamListDropIndex(0);
    QCOMPARE(MockLinkCamera::defaultExtParams().at(0).name, kExtIntParam);

    QVERIFY_TRUE_WAIT(paramManager->extParameterExists(MAV_COMP_ID_CAMERA, kExtIntParam), TestTimeout::longMs());

    _disconnectMockLink();
}
