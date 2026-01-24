#include "ParameterManagerTest.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "ParameterManager.h"
#include "MockLinkFTP.h"
#include "QGC.h"

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

#include <cmath>
#include <limits>

namespace {
    // MockLink sends params at 500Hz, ~1000 params take ~2s. Allow generous margin.
    constexpr int kParamLoadTimeoutMs = 8000;
    // Short wait when verifying signals are NOT emitted
    constexpr int kNoSignalWaitMs = 500;
}

// ============================================================================
// Helper Methods
// ============================================================================

int ParameterManagerTest::_paramRetryTimeout(int retryCount, int baseTimeoutMs)
{
    // Add margin for processing time
    return baseTimeoutMs * (retryCount + 1) + 500;
}

Vehicle* ParameterManagerTest::_startMockLinkAndWaitForVehicle(MockConfiguration::FailureMode_t failureMode)
{
    Q_ASSERT(!_mockLink);
    _mockLink = MockLink::startPX4MockLink(false, failureMode);

    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    if (!vehicleMgr) {
        return nullptr;
    }

    QSignalSpy spyVehicle(vehicleMgr, &MultiVehicleManager::activeVehicleAvailableChanged);
    if (!spyVehicle.isValid()) {
        return nullptr;
    }

    if (!spyVehicle.wait(TestHelpers::kDefaultTimeoutMs)) {
        return nullptr;
    }

    if (spyVehicle.count() != 1 || !spyVehicle.first().first().toBool()) {
        return nullptr;
    }

    return vehicleMgr->activeVehicle();
}

Vehicle* ParameterManagerTest::_startFTPMockLinkAndWaitForParams()
{
    Q_ASSERT(!_mockLink);

    // Use ArduPlane mock with FTP param file enabled
    // FailParamNoResponseToRequestList forces the FTP path for parameter loading
    _mockLink = MockLink::startAPMArduPlaneMockLink(false, MockConfiguration::FailParamNoResponseToRequestList);
    if (!_mockLink) {
        return nullptr;
    }
    _mockLink->mockLinkFTP()->enableBinParamFile(true);

    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    if (!vehicleMgr) {
        return nullptr;
    }

    // Wait for vehicle creation
    QSignalSpy spyVehicle(vehicleMgr, &MultiVehicleManager::activeVehicleAvailableChanged);
    if (!spyVehicle.isValid() || !spyVehicle.wait(TestHelpers::kDefaultTimeoutMs)) {
        return nullptr;
    }

    Vehicle* vehicle = vehicleMgr->activeVehicle();
    if (!vehicle) {
        return nullptr;
    }

    // Wait for FTP parameter load to complete
    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    if (!spyParamsReady.isValid()) {
        return nullptr;
    }

    // FTP is fast in unit tests, but allow reasonable timeout
    constexpr int kFtpTimeout = 10000;
    if (spyParamsReady.count() == 0 && !spyParamsReady.wait(kFtpTimeout)) {
        return nullptr;
    }

    if (!vehicle->parameterManager()->parametersReady()) {
        return nullptr;
    }

    return vehicle;
}

// ============================================================================
// Parameter List Loading Tests
// ============================================================================

void ParameterManagerTest::_noFailureWorker(MockConfiguration::FailureMode_t failureMode)
{
    Vehicle* vehicle = _startMockLinkAndWaitForVehicle(failureMode);
    VERIFY_NOT_NULL(vehicle);

    ParameterManager* paramMgr = vehicle->parameterManager();
    VERIFY_NOT_NULL(paramMgr);

    // Monitor progress updates during load
    QSignalSpy spyProgress(paramMgr, &ParameterManager::loadProgressChanged);
    QGC_VERIFY_SPY_VALID(spyProgress);
    QVERIFY(spyProgress.wait(TestHelpers::kDefaultTimeoutMs));
    QCOMPARE_GT(spyProgress.first().first().toFloat(), 0.0f);

    // Wait for parameters to be ready
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    QGC_VERIFY_SPY_VALID(spyParamsReady);

    QVERIFY(spyParamsReady.wait(kParamLoadTimeoutMs));
    QCOMPARE_EQ(spyParamsReady.first().first().toBool(), true);

    // Progress should reset to 0 when complete
    QCOMPARE_EQ(spyProgress.last().first().toFloat(), 0.0f);
}

void ParameterManagerTest::_noFailure()
{
    _noFailureWorker(MockConfiguration::FailNone);
}

void ParameterManagerTest::_requestListMissingParamSuccess()
{
    _noFailureWorker(MockConfiguration::FailMissingParamOnInitialRequest);
}

void ParameterManagerTest::_requestListNoResponse()
{
    Vehicle* vehicle = _startMockLinkAndWaitForVehicle(MockConfiguration::FailParamNoResponseToRequestList);
    VERIFY_NOT_NULL(vehicle);

    ParameterManager* paramMgr = vehicle->parameterManager();
    VERIFY_NOT_NULL(paramMgr);

    QSignalSpy spyProgress(paramMgr, &ParameterManager::loadProgressChanged);
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    QGC_VERIFY_SPY_VALID(spyProgress);
    QGC_VERIFY_SPY_VALID(spyParamsReady);

    // Should NOT get progress or params ready signals
    // Unit test mode: 500ms timeout × 5 retries = 2.5s
    QVERIFY(!spyProgress.wait(kNoSignalWaitMs));
    QVERIFY(!spyParamsReady.wait(TestHelpers::kDefaultTimeoutMs));
}

void ParameterManagerTest::_requestListMissingParamFail()
{
    Vehicle* vehicle = _startMockLinkAndWaitForVehicle(MockConfiguration::FailMissingParamOnAllRequests);
    VERIFY_NOT_NULL(vehicle);

    ParameterManager* paramMgr = vehicle->parameterManager();
    VERIFY_NOT_NULL(paramMgr);

    QSignalSpy spyProgress(paramMgr, &ParameterManager::loadProgressChanged);
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QSignalSpy spyParamsReady(vehicleMgr, &MultiVehicleManager::parameterReadyVehicleAvailableChanged);
    QGC_VERIFY_SPY_VALID(spyProgress);
    QGC_VERIFY_SPY_VALID(spyParamsReady);

    // Should get progress updates (partial load succeeds)
    QVERIFY(spyProgress.wait(TestHelpers::kDefaultTimeoutMs));
    QCOMPARE_GT(spyProgress.first().first().toFloat(), 0.0f);

    // Should eventually signal params ready, but with missing params
    QVERIFY(spyParamsReady.wait(TestHelpers::kDefaultTimeoutMs));
    QCOMPARE_EQ(paramMgr->missingParameters(), true);
}

// ============================================================================
// Parameter Write Tests
// ============================================================================

void ParameterManagerTest::_paramWriteNoAckRetry()
{
    _setParamWithFailureMode(MockLink::FailParamSetFirstAttemptNoAck, true /* expectSuccess */);
}

void ParameterManagerTest::_paramWriteNoAckPermanent()
{
    _setParamWithFailureMode(MockLink::FailParamSetNoAck, false /* expectSuccess */);
}

void ParameterManagerTest::_setParamWithFailureMode(MockLink::ParamSetFailureMode_t failureMode, bool expectSuccess)
{
    Q_ASSERT(!_mockLink);

    _connectMockLink();
    QVERIFY(_mockLink);
    QVERIFY(_vehicle);

    _mockLink->setParamSetFailureMode(failureMode);

    ParameterManager* paramMgr = _vehicle->parameterManager();
    VERIFY_NOT_NULL(paramMgr);
    QCOMPARE_EQ(paramMgr->pendingWrites(), false);

    // Use a parameter that exists in mock PX4 with floating point range
    Fact* fact = paramMgr->getParameter(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("BAT1_V_CHARGED"));
    VERIFY_NOT_NULL(fact);

    const double originalValue = fact->rawValue().toDouble();
    const FactMetaData* metaData = fact->metaData();

    // Calculate a new value within valid range
    const double minVal = metaData && metaData->rawMin().isValid() ? metaData->rawMin().toDouble() : -1e9;
    const double maxVal = metaData && metaData->rawMax().isValid() ? metaData->rawMax().toDouble() : 1e9;
    double newValue = originalValue + 0.1;
    if (newValue > maxVal) {
        newValue = originalValue - 0.1;
    }
    if (newValue < minVal || qFuzzyCompare(newValue, originalValue)) {
        newValue = (minVal + maxVal) / 2.0;
    }
    QVERIFY(!qFuzzyCompare(newValue, originalValue));

    // Set up spies for outcome signals
    QSignalSpy successSpy(paramMgr, &ParameterManager::_paramSetSuccess);
    QSignalSpy failureSpy(paramMgr, &ParameterManager::_paramSetFailure);
    QSignalSpy valueChangedSpy(fact, &Fact::rawValueChanged);
    QGC_VERIFY_SPY_VALID(successSpy);
    QGC_VERIFY_SPY_VALID(failureSpy);
    QGC_VERIFY_SPY_VALID(valueChangedSpy);

    // Trigger the write
    fact->setRawValue(newValue);

    // Wait for the outcome signal directly - this ensures the state machine has fully completed
    const int timeout = _paramRetryTimeout(ParameterManager::kParamSetRetryCount, ParameterManager::kWaitForParamValueAckMs);
    if (expectSuccess) {
        QVERIFY2(successSpy.wait(timeout), "Timeout waiting for param set success signal");
        QCOMPARE_EQ(successSpy.count(), 1);
        QCOMPARE_EQ(failureSpy.count(), 0);
        QCOMPARE_EQ(valueChangedSpy.count(), 1);
    } else {
        QVERIFY2(failureSpy.wait(timeout), "Timeout waiting for param set failure signal");
        QCOMPARE_EQ(successSpy.count(), 0);
        QCOMPARE_EQ(failureSpy.count(), 1);
        // On failure: value changed to new, then reverted to original
        // Allow some time for the revert to happen
        if (valueChangedSpy.count() < 2) {
            QVERIFY(TestHelpers::waitFor([&]() { return valueChangedSpy.count() >= 2; }, 2000));
        }
        QCOMPARE_EQ(valueChangedSpy.count(), 2);
        QVERIFY(QGC::fuzzyCompare(valueChangedSpy.last().first().toDouble(), originalValue, 0.00001));
    }

    // Verify pending writes flag is cleared
    QCOMPARE_EQ(paramMgr->pendingWrites(), false);

    _mockLink->setParamSetFailureMode(MockLink::FailParamSetNone);
}

// ============================================================================
// Parameter Read Tests
// ============================================================================

void ParameterManagerTest::_paramReadFirstAttemptNoResponseRetry()
{
    Q_ASSERT(!_mockLink);

    _connectMockLink();
    QVERIFY(_mockLink);
    QVERIFY(_vehicle);

    ParameterManager* paramMgr = _vehicle->parameterManager();
    VERIFY_NOT_NULL(paramMgr);

    _mockLink->setParamRequestReadFailureMode(MockLink::FailParamRequestReadFirstAttemptNoResponse);

    Fact* fact = paramMgr->getParameter(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("BAT1_V_CHARGED"));
    VERIFY_NOT_NULL(fact);

    QSignalSpy vehicleUpdatedSpy(fact, &Fact::vehicleUpdated);
    QSignalSpy successSpy(paramMgr, &ParameterManager::_paramRequestReadSuccess);
    QGC_VERIFY_SPY_VALID(vehicleUpdatedSpy);
    QGC_VERIFY_SPY_VALID(successSpy);

    paramMgr->refreshParameter(MAV_COMP_ID_AUTOPILOT1, fact->name());

    const int timeout = _paramRetryTimeout(ParameterManager::kParamRequestReadRetryCount, ParameterManager::kWaitForParamValueAckMs);
    QVERIFY(successSpy.wait(timeout));
    QCOMPARE_EQ(successSpy.count(), 1);
    QCOMPARE_EQ(vehicleUpdatedSpy.count(), 1);
}

void ParameterManagerTest::_paramReadNoResponse()
{
    Q_ASSERT(!_mockLink);

    _connectMockLink();
    QVERIFY(_mockLink);
    QVERIFY(_vehicle);

    ParameterManager* paramMgr = _vehicle->parameterManager();
    VERIFY_NOT_NULL(paramMgr);

    Fact* fact = paramMgr->getParameter(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("BAT1_V_CHARGED"));
    VERIFY_NOT_NULL(fact);

    QSignalSpy vehicleUpdatedSpy(fact, &Fact::vehicleUpdated);
    QSignalSpy failureSpy(paramMgr, &ParameterManager::_paramRequestReadFailure);
    QGC_VERIFY_SPY_VALID(vehicleUpdatedSpy);
    QGC_VERIFY_SPY_VALID(failureSpy);

    _mockLink->setParamRequestReadFailureMode(MockLink::FailParamRequestReadNoResponse);
    paramMgr->refreshParameter(MAV_COMP_ID_AUTOPILOT1, fact->name());

    const int timeout = _paramRetryTimeout(ParameterManager::kParamRequestReadRetryCount, ParameterManager::kWaitForParamValueAckMs);
    QVERIFY(failureSpy.wait(timeout));
    QCOMPARE_EQ(failureSpy.count(), 1);
    QCOMPARE_EQ(vehicleUpdatedSpy.count(), 0);
}

// ============================================================================
// FTP Parameter Loading Tests
// ============================================================================

void ParameterManagerTest::_FTPnoFailure()
{
    Vehicle* vehicle = _startFTPMockLinkAndWaitForParams();
    QVERIFY2(vehicle, "Failed to start FTP MockLink and load parameters");
    QCOMPARE_EQ(vehicle->parameterManager()->parametersReady(), true);
}

void ParameterManagerTest::_FTPChangeParam()
{
    Vehicle* vehicle = _startFTPMockLinkAndWaitForParams();
    QVERIFY2(vehicle, "Failed to start FTP MockLink and load parameters");

    ParameterManager* paramMgr = vehicle->parameterManager();
    VERIFY_NOT_NULL(paramMgr);
    QCOMPARE_EQ(paramMgr->parametersReady(), true);

    // Verify parameters were loaded
    QStringList paramNames = paramMgr->parameterNames(MAV_COMP_ID_AUTOPILOT1);
    QCOMPARE_GT(paramNames.count(), 10);

    // Verify we can read parameter values
    bool foundReadableParam = false;
    for (const QString& name : paramNames) {
        Fact* fact = paramMgr->getParameter(MAV_COMP_ID_AUTOPILOT1, name);
        if (fact && fact->metaData()) {
            QVERIFY(fact->rawValue().isValid());
            foundReadableParam = true;
            break;
        }
    }
    QVERIFY2(foundReadableParam, "Could not read any parameter values after FTP load");
}
