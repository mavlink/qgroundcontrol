#include "ParameterManagerTest.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "ParameterManager.h"
#include "MockLinkFTP.h"
#include "QGC.h"

#include <QtCore/QElapsedTimer>
#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

#include <cmath>
#include <limits>

// Timing constants for unit tests - use shorter timeouts than production
namespace {
    // Shorter timeouts for faster test execution
    constexpr int kTestInitialRequestTimeoutMs = 500;   ///< 500ms instead of 5000ms
    constexpr int kTestWaitingParamTimeoutMs = 300;     ///< 300ms instead of 3000ms

    // Calculated wait times based on test timeouts
    // Initial param load: timeout × 4 retries + buffer
    constexpr int kInitialLoadTimeoutMs = kTestInitialRequestTimeoutMs * 5 + 1000;
    // For tests expecting no response, wait for retries to exhaust
    constexpr int kNoResponseTestMs = kTestInitialRequestTimeoutMs * 5 + 1000;
    // Standard buffer for signal processing
    constexpr int kSignalBufferMs = 500;
}

/// Helper to configure test timeouts on ParameterManager
static void setTestTimeouts(Vehicle* vehicle)
{
    if (vehicle && vehicle->parameterManager()) {
        vehicle->parameterManager()->setTestTimeouts(kTestInitialRequestTimeoutMs, kTestWaitingParamTimeoutMs);
    }
}

/// Test failure modes which should still lead to param load success
void ParameterManagerTest::_noFailureWorker(MockConfiguration::FailureMode_t failureMode)
{
    Q_ASSERT(!_mockLink);
    _mockLink = MockLink::startPX4MockLink(false, failureMode);

    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QVERIFY(vehicleMgr);

    // Wait for the Vehicle to get created
    QSignalSpy spyVehicle(vehicleMgr, &MultiVehicleManager::activeVehicleAvailableChanged);
    QVERIFY(spyVehicle.wait(5000));
    QCOMPARE(spyVehicle.count(), 1);
    QCOMPARE(spyVehicle.takeFirst().at(0).toBool(), true);

    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);
    setTestTimeouts(vehicle);

    // We should get progress bar updates during load
    QSignalSpy spyProgress(vehicle->parameterManager(), &ParameterManager::loadProgressChanged);
    QVERIFY(spyProgress.wait(2000));
    QVERIFY(spyProgress.takeFirst().at(0).toFloat() > 0.0f);

    // When param load is complete we get the param ready signal
    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    QVERIFY(spyParamsReady.wait(kInitialLoadTimeoutMs));
    QCOMPARE(spyParamsReady.takeFirst().at(0).toBool(), true);

    // Progress should have been set back to 0
    QCOMPARE(spyProgress.takeLast().at(0).toFloat(), 0.0f);
}

void ParameterManagerTest::_noFailure()
{
    _noFailureWorker(MockConfiguration::FailNone);
}

void ParameterManagerTest::_requestListMissingParamSuccess()
{
    _noFailureWorker(MockConfiguration::FailMissingParamOnInitialReqest);
}

// Test no response to param_request_list
void ParameterManagerTest::_requestListNoResponse()
{
    Q_ASSERT(!_mockLink);
    _mockLink = MockLink::startPX4MockLink(false, MockConfiguration::FailParamNoReponseToRequestList);

    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QVERIFY(vehicleMgr);

    // Wait for the Vehicle to get created
    QSignalSpy spyVehicle(vehicleMgr, &MultiVehicleManager::activeVehicleAvailableChanged);
    QVERIFY(spyVehicle.wait(5000));
    QCOMPARE(spyVehicle.count(), 1);
    QCOMPARE(spyVehicle.takeFirst().at(0).toBool(), true);

    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);
    setTestTimeouts(vehicle);

    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    QSignalSpy spyProgress(vehicle->parameterManager(), &ParameterManager::loadProgressChanged);

    // We should not get any progress bar updates, nor a parameter ready signal
    // Wait for slightly longer than the initial request timeout to confirm no response
    QCOMPARE(spyProgress.wait(kTestInitialRequestTimeoutMs + 200), false);

    // Wait for max retries to exhaust
    QCOMPARE(spyParamsReady.wait(kNoResponseTestMs), false);
}

// MockLink will fail to send a param on initial request, it will also fail to send it on subsequent
// param_read requests.
void ParameterManagerTest::_requestListMissingParamFail()
{
    Q_ASSERT(!_mockLink);
    _mockLink = MockLink::startPX4MockLink(false, MockConfiguration::FailMissingParamOnAllRequests);

    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QVERIFY(vehicleMgr);

    // Wait for the Vehicle to get created
    QSignalSpy spyVehicle(vehicleMgr, &MultiVehicleManager::activeVehicleAvailableChanged);
    QVERIFY(spyVehicle.wait(5000));
    QCOMPARE(spyVehicle.count(), 1);
    QCOMPARE(spyVehicle.takeFirst().at(0).toBool(), true);

    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);
    setTestTimeouts(vehicle);

    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    QSignalSpy spyProgress(vehicle->parameterManager(), &ParameterManager::loadProgressChanged);

    // We will get progress bar updates, since it will fail after getting partially through the request
    QVERIFY(spyProgress.wait(2000));
    QVERIFY(spyProgress.takeFirst().at(0).toFloat() > 0.0f);

    // We should get a parameters ready signal, but Vehicle should indicate missing params
    QVERIFY(spyParamsReady.wait(kInitialLoadTimeoutMs));
    QCOMPARE(vehicle->parameterManager()->missingParameters(), true);
}

void ParameterManagerTest::_paramWriteNoAckRetry()
{
    _setParamWithFailureMode(MockLink::FailParamSetFirstAttemptNoAck, true /* expectSuccess */);
}

void ParameterManagerTest::_paramWriteNoAckPermanent()
{
    _setParamWithFailureMode(MockLink::FailParamSetNoAck, false /* expectSuccess */);
}

void ParameterManagerTest::_paramReadFirstAttemptNoResponseRetry()
{
    Q_ASSERT(!_mockLink);

    _connectMockLink();
    QVERIFY(_mockLink);
    QVERIFY(_vehicle);
    setTestTimeouts(_vehicle);

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

    // Wait for: (retries + 1) × ack timeout + buffer
    const int maxWaitTimeMs = ParameterManager::kWaitForParamValueAckMs * (ParameterManager::kParamRequestReadRetryCount + 1) + kSignalBufferMs;

    QVERIFY(paramReadSuccessSpy.wait(maxWaitTimeMs));
    QCOMPARE(paramReadSuccessSpy.count(), 1);
    QCOMPARE(vehicleUpdatedSpy.count(), 1);

    _disconnectMockLink();
}

void ParameterManagerTest::_paramReadNoResponse()
{
    Q_ASSERT(!_mockLink);

    _connectMockLink();
    QVERIFY(_mockLink);
    QVERIFY(_vehicle);
    setTestTimeouts(_vehicle);

    ParameterManager* const paramManager = _vehicle->parameterManager();
    QVERIFY(paramManager);

    Fact* const fact = paramManager->getParameter(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("BAT1_V_CHARGED"));
    QVERIFY(fact);

    QSignalSpy vehicleUpdatedSpy(fact, &Fact::vehicleUpdated);
    QSignalSpy paramReadFailureSpy(paramManager, &ParameterManager::_paramRequestReadFailure);

    QVERIFY(vehicleUpdatedSpy.isValid());
    QVERIFY(paramReadFailureSpy.isValid());

    _mockLink->setParamRequestReadFailureMode(MockLink::FailParamRequestReadNoResponse);

    paramManager->refreshParameter(MAV_COMP_ID_AUTOPILOT1, fact->name());

    // Wait for: (retries + 1) × ack timeout + buffer
    const int maxWaitTimeMs = ParameterManager::kWaitForParamValueAckMs * (ParameterManager::kParamRequestReadRetryCount + 1) + kSignalBufferMs;

    QVERIFY(paramReadFailureSpy.wait(maxWaitTimeMs));
    QCOMPARE(paramReadFailureSpy.count(), 1);
    QCOMPARE(vehicleUpdatedSpy.count(), 0);

    _disconnectMockLink();
}

void ParameterManagerTest::_setParamWithFailureMode(MockLink::ParamSetFailureMode_t failureMode, bool expectSuccess)
{
    Q_ASSERT(!_mockLink);

    // Bring up a clean mock vehicle for each run
    _connectMockLink();
    QVERIFY(_mockLink);
    QVERIFY(_vehicle);
    setTestTimeouts(_vehicle);

    _mockLink->setParamSetFailureMode(failureMode);

    ParameterManager* const paramManager = _vehicle->parameterManager();
    QVERIFY(paramManager);
    QVERIFY(!paramManager->pendingWrites());

    // Use a parameter that exists in the mock PX4 set and has floating point range
    Fact* const fact = paramManager->getParameter(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("BAT1_V_CHARGED"));
    QVERIFY(fact);

    const QVariant originalValue = fact->rawValue();
    const double originalDouble = originalValue.toDouble();

    const FactMetaData* const metaData = fact->metaData();
    const double minValue = (metaData && metaData->rawMin().isValid()) ? metaData->rawMin().toDouble() : -std::numeric_limits<double>::infinity();
    const double maxValue = (metaData && metaData->rawMax().isValid()) ? metaData->rawMax().toDouble() : std::numeric_limits<double>::infinity();
    constexpr double step = 0.1;

    // Calculate a new value within bounds that's different from the original
    auto adjustedValue = [&](double candidate) -> double {
        if (candidate > maxValue) candidate = originalDouble - step;
        if (candidate < minValue) candidate = originalDouble + step;
        if (qFuzzyCompare(candidate + 1.0, originalDouble + 1.0)) candidate = originalDouble + (step * 2.0);
        if (candidate > maxValue) candidate = originalDouble - (step * 2.0);
        if (candidate < minValue) candidate = originalDouble;
        return candidate;
    };

    const double newValueDouble = adjustedValue(originalDouble + step);
    QVERIFY(!qFuzzyCompare(newValueDouble + 1.0, originalDouble + 1.0));
    const QVariant newValue(newValueDouble);

    QSignalSpy rawValueChangedSpy(fact, &Fact::rawValueChanged);
    QSignalSpy pendingSpy(paramManager, &ParameterManager::pendingWritesChanged);
    QSignalSpy paramSetSuccessSpy(paramManager, &ParameterManager::_paramSetSuccess);
    QSignalSpy paramSetFailureSpy(paramManager, &ParameterManager::_paramSetFailure);

    QVERIFY(rawValueChangedSpy.isValid());
    QVERIFY(pendingSpy.isValid());
    QVERIFY(paramSetSuccessSpy.isValid());
    QVERIFY(paramSetFailureSpy.isValid());

    // Trigger the parameter write
    fact->setRawValue(newValue);

    // Calculate max wait time: initial + (retries × ack timeout) + buffer for state machine transitions
    // For permanent failure: 3 sends × 1s timeout each = 3s, plus state machine overhead
    const int maxWaitTimeMs = ParameterManager::kWaitForParamValueAckMs * (ParameterManager::kParamSetRetryCount + 1) + 3000;

    // Wait for the result signal (success or failure)
    if (expectSuccess) {
        QVERIFY2(paramSetSuccessSpy.wait(maxWaitTimeMs), "Timed out waiting for paramSetSuccess signal");
        QCOMPARE(paramSetSuccessSpy.count(), 1);
        QCOMPARE(paramSetFailureSpy.count(), 0);
    } else {
        QVERIFY2(paramSetFailureSpy.wait(maxWaitTimeMs), "Timed out waiting for paramSetFailure signal");
        QCOMPARE(paramSetSuccessSpy.count(), 0);
        QCOMPARE(paramSetFailureSpy.count(), 1);
    }

    // Verify pendingWrites went true then false
    bool sawPendingTrue = false;
    bool sawPendingFalse = false;
    for (int i = 0; i < pendingSpy.count(); i++) {
        const bool isPending = pendingSpy.at(i).at(0).toBool();
        if (isPending) sawPendingTrue = true;
        else sawPendingFalse = true;
    }
    QVERIFY2(sawPendingTrue, "Expected pendingWritesChanged(true) signal");
    QVERIFY2(sawPendingFalse, "Expected pendingWritesChanged(false) signal");

    // Verify rawValue signals
    if (expectSuccess) {
        QCOMPARE(rawValueChangedSpy.count(), 1);
    } else {
        // On failure we get: 1) our change, 2) refresh restores original
        if (rawValueChangedSpy.count() == 1) {
            rawValueChangedSpy.wait(2000);
        }
        QCOMPARE(rawValueChangedSpy.count(), 2);
    }

    // Verify the values in the signals
    QVERIFY(QGC::fuzzyCompare(rawValueChangedSpy[0][0].toDouble(), newValueDouble, 0.00001));
    if (!expectSuccess) {
        QVERIFY(QGC::fuzzyCompare(rawValueChangedSpy[1][0].toDouble(), originalDouble, 0.00001));
    }

    _mockLink->setParamSetFailureMode(MockLink::FailParamSetNone);
    _disconnectMockLink();
}

// FTP-based parameter tests (currently disabled - need MockLinkFTP work)
#if 0
void ParameterManagerTest::_FTPnoFailure()
{
    Q_ASSERT(!_mockLink);
    _mockLink = MockLink::startAPMArduPlaneMockLink(false, MockConfiguration::FailParamNoReponseToRequestList);
    _mockLink->mockLinkFTP()->enableBinParamFile(true);

    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QVERIFY(vehicleMgr);

    // Wait for the Vehicle to get created
    QSignalSpy spyVehicle(vehicleMgr, &MultiVehicleManager::activeVehicleAvailableChanged);
    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);

    QVERIFY(spyVehicle.wait(5000));
    QCOMPARE(spyVehicle.count(), 1);
    QCOMPARE(spyVehicle.takeFirst().at(0).toBool(), true);

    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);

    QVERIFY(spyParamsReady.wait(5000));
    QCOMPARE(spyParamsReady.count(), 1);
    QCOMPARE(spyParamsReady.takeFirst().at(0).toBool(), true);

    // Request all parameters again and check the progress bar
    QSignalSpy spyProgress(vehicle->parameterManager(), &ParameterManager::loadProgressChanged);
    vehicle->parameterManager()->refreshAllParameters();

    QVERIFY(spyParamsReady.wait(5000));
    QVERIFY(spyProgress.count() > 1);
    QVERIFY(spyProgress.takeFirst().at(0).toFloat() > 0.0f);

    // Progress should have been set back to 0
    QCOMPARE(spyProgress.takeLast().at(0).toFloat(), 0.0f);
}

void ParameterManagerTest::_FTPChangeParam()
{
    Q_ASSERT(!_mockLink);
    _mockLink = MockLink::startAPMArduPlaneMockLink(false, MockConfiguration::FailParamNoReponseToRequestList);
    _mockLink->mockLinkFTP()->enableBinParamFile(true);

    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QVERIFY(vehicleMgr);

    // Wait for the Vehicle to get created
    QSignalSpy spyVehicle(vehicleMgr, &MultiVehicleManager::activeVehicleAvailableChanged);
    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);

    QVERIFY(spyVehicle.wait(5000));
    QCOMPARE(spyVehicle.count(), 1);
    QCOMPARE(spyVehicle.takeFirst().at(0).toBool(), true);

    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);

    if (spyParamsReady.count() == 0) {
        QVERIFY(spyParamsReady.wait(5000));
    }
    QCOMPARE(spyParamsReady.count(), 1);
    QCOMPARE(spyParamsReady.takeFirst().at(0).toBool(), true);

    // Now try to change a parameter and check the progress
    QSignalSpy spyProgress(vehicle->parameterManager(), &ParameterManager::loadProgressChanged);
    Fact* fact = vehicle->parameterManager()->getParameter(MAV_COMP_ID_AUTOPILOT1, "THR_MIN");
    QVERIFY(fact);

    const float value = fact->rawValue().toFloat();
    QCOMPARE(value, 0.0f);

    constexpr float testValue = 0.87f;
    fact->setRawValue(QVariant(testValue));

    // Should set the progress to 0.5 and then back to 0
    QVERIFY(spyProgress.wait(1000));
    if (spyProgress.count() < 2) {
        QVERIFY(spyProgress.wait(1000));
    }
    QCOMPARE(spyProgress.count(), 2);
    QVERIFY(spyProgress.takeFirst().at(0).toFloat() > 0.4f);

    // Progress should have been set back to 0
    QCOMPARE(spyProgress.takeLast().at(0).toFloat(), 0.0f);
}
#endif
