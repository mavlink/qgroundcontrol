/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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

/// Test failure modes which should still lead to param load success
void ParameterManagerTest::_noFailureWorker(MockConfiguration::FailureMode_t failureMode)
{
    Q_ASSERT(!_mockLink);
    _mockLink = MockLink::startPX4MockLink(false, failureMode);

    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QVERIFY(vehicleMgr);

    // Wait for the Vehicle to get created
    QSignalSpy spyVehicle(vehicleMgr, SIGNAL(activeVehicleAvailableChanged(bool)));
    QCOMPARE(spyVehicle.wait(5000), true);
    QCOMPARE(spyVehicle.count(), 1);
    QList<QVariant> arguments = spyVehicle.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toBool(), true);

    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);

    // We should get progress bar updates during load
    QSignalSpy spyProgress(vehicle->parameterManager(), SIGNAL(loadProgressChanged(float)));
    QCOMPARE(spyProgress.wait(2000), true);
    arguments = spyProgress.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QVERIFY(arguments.at(0).toFloat() > 0.0f);

    // When param load is complete we get the param ready signal
    QSignalSpy spyParamsReady(vehicleMgr, SIGNAL(parameterReadyVehicleAvailableChanged(bool)));
    QCOMPARE(spyParamsReady.wait(60000), true);
    arguments = spyParamsReady.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toBool(), true);

    // Progress should have been set back to 0
    arguments = spyProgress.takeLast();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toFloat(), 0.0f);
}


void ParameterManagerTest::_noFailure(void)
{
    _noFailureWorker(MockConfiguration::FailNone);
}

void ParameterManagerTest::_requestListMissingParamSuccess(void)
{
    _noFailureWorker(MockConfiguration::FailMissingParamOnInitialReqest);
}

// Test no response to param_request_list
void ParameterManagerTest::_requestListNoResponse(void)
{
    Q_ASSERT(!_mockLink);
    _mockLink = MockLink::startPX4MockLink(false, MockConfiguration::FailParamNoReponseToRequestList);

    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QVERIFY(vehicleMgr);

    // Wait for the Vehicle to get created
    QSignalSpy spyVehicle(vehicleMgr, SIGNAL(activeVehicleAvailableChanged(bool)));
    QCOMPARE(spyVehicle.wait(5000), true);
    QCOMPARE(spyVehicle.count(), 1);
    QList<QVariant> arguments = spyVehicle.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toBool(), true);

    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);

    QSignalSpy spyParamsReady(vehicleMgr, SIGNAL(parameterReadyVehicleAvailableChanged(bool)));
    QSignalSpy spyProgress(vehicle->parameterManager(), SIGNAL(loadProgressChanged(float)));

    // We should not get any progress bar updates, nor a parameter ready signal
    QCOMPARE(spyProgress.wait(500), false);
    QCOMPARE(spyParamsReady.wait(40000), false);
}

// MockLink will fail to send a param on initial request, it will also fail to send it on subsequent
// param_read requests.
void ParameterManagerTest::_requestListMissingParamFail(void)
{
    Q_ASSERT(!_mockLink);
    _mockLink = MockLink::startPX4MockLink(false, MockConfiguration::FailMissingParamOnAllRequests);

    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QVERIFY(vehicleMgr);

    // Wait for the Vehicle to get created
    QSignalSpy spyVehicle(vehicleMgr, SIGNAL(activeVehicleAvailableChanged(bool)));
    QCOMPARE(spyVehicle.wait(5000), true);
    QCOMPARE(spyVehicle.count(), 1);
    QList<QVariant> arguments = spyVehicle.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toBool(), true);

    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);

    QSignalSpy spyParamsReady(vehicleMgr, SIGNAL(parameterReadyVehicleAvailableChanged(bool)));
    QSignalSpy spyProgress(vehicle->parameterManager(), SIGNAL(loadProgressChanged(float)));

    // We will get progress bar updates, since it will fail after getting partially through the request
    QCOMPARE(spyProgress.wait(2000), true);
    arguments = spyProgress.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QVERIFY(arguments.at(0).toFloat() > 0.0f);

    // We should get a parameters ready signal, but Vehicle should indicate missing params
    QCOMPARE(spyParamsReady.wait(40000), true);
    QCOMPARE(vehicle->parameterManager()->missingParameters(), true);
}

void ParameterManagerTest::_paramWriteNoAckRetry(void)
{
    _setParamWithFailureMode(MockLink::FailParamSetFirstAttemptNoAck, true /* expectSuccess */);
}

void ParameterManagerTest::_paramWriteNoAckPermanent(void)
{
    _setParamWithFailureMode(MockLink::FailParamSetNoAck, false /* expectSuccess */);
}

void ParameterManagerTest::_paramReadFirstAttemptNoResponseRetry(void)
{
    Q_ASSERT(!_mockLink);

    _connectMockLink();
    QVERIFY(_mockLink);
    QVERIFY(_vehicle);

    ParameterManager *const paramManager = _vehicle->parameterManager();
    QVERIFY(paramManager);

    _mockLink->setParamRequestReadFailureMode(MockLink::FailParamRequestReadFirstAttemptNoResponse);

    Fact *const fact = paramManager->getParameter(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("BAT1_V_CHARGED"));
    QVERIFY(fact);

    QSignalSpy vehicleUpdatedSpy(fact, &Fact::vehicleUpdated);
    QSignalSpy paramReadSuccessSpy(paramManager, &ParameterManager::_paramRequestReadSuccess);

    QVERIFY(vehicleUpdatedSpy.isValid());
    QVERIFY(paramReadSuccessSpy.isValid());

    paramManager->refreshParameter(MAV_COMP_ID_AUTOPILOT1, fact->name());

    const int maxWaitTimeMs = ParameterManager::kWaitForParamValueAckMs * (ParameterManager::kParamRequestReadRetryCount + 1) + 500;

    QVERIFY(paramReadSuccessSpy.wait(maxWaitTimeMs));
    QCOMPARE(paramReadSuccessSpy.count(), 1);

    QCOMPARE(vehicleUpdatedSpy.count(), 1);

    _disconnectMockLink();
}

void ParameterManagerTest::_paramReadNoResponse(void)
{
    Q_ASSERT(!_mockLink);

    _connectMockLink();
    QVERIFY(_mockLink);
    QVERIFY(_vehicle);

    ParameterManager *const paramManager = _vehicle->parameterManager();
    QVERIFY(paramManager);

    Fact *const fact = paramManager->getParameter(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("BAT1_V_CHARGED"));
    QVERIFY(fact);

    QSignalSpy vehicleUpdatedSpy(fact, &Fact::vehicleUpdated);
    QSignalSpy paramReadFailureSpy(paramManager, &ParameterManager::_paramRequestReadFailure);

    QVERIFY(vehicleUpdatedSpy.isValid());
    QVERIFY(paramReadFailureSpy.isValid());

    _mockLink->setParamRequestReadFailureMode(MockLink::FailParamRequestReadNoResponse);

    paramManager->refreshParameter(MAV_COMP_ID_AUTOPILOT1, fact->name());

    const int maxWaitTimeMs = ParameterManager::kWaitForParamValueAckMs * (ParameterManager::kParamRequestReadRetryCount + 1) + 500;

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

    _mockLink->setParamSetFailureMode(failureMode);

    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QVERIFY(vehicleMgr);
    ParameterManager *const paramManager = _vehicle->parameterManager();
    QVERIFY(paramManager);
    QVERIFY(!_vehicle->parameterManager()->pendingWrites());

    // Use a parameter that exists in the mock PX4 set and has floating point range
    Fact *const fact = paramManager->getParameter(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("BAT1_V_CHARGED"));
    QVERIFY(fact);
    QSignalSpy rawValueChangedSpy(fact, &Fact::rawValueChanged);

    const QVariant originalValue = fact->rawValue();
    const double originalDouble = originalValue.toDouble();

    const FactMetaData *const metaData = fact->metaData();
    const double minValue = (metaData && metaData->rawMin().isValid()) ? metaData->rawMin().toDouble() : -std::numeric_limits<double>::infinity();
    const double maxValue = (metaData && metaData->rawMax().isValid()) ? metaData->rawMax().toDouble() : std::numeric_limits<double>::infinity();
    const double step = 0.1;

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
    QElapsedTimer waitTimer;
    waitTimer.start();

    while ((!sawPendingTrue || !sawPendingFalse) && waitTimer.elapsed() < 20000) {
        (void) pendingSpy.wait(500);
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
    }

    QVERIFY2(sawPendingTrue, "Expected pendingWritesChanged(true) signal");
    QVERIFY2(sawPendingFalse, "Expected pendingWritesChanged(false) signal");

    // We should get two rawValueChanged signals if unsuccessful (one for the set, one for the refresh)
    // We should get one rawValueChanged signal if successful (just the set)

    bool rawValueChangedCountMatches = false;
    rawValueChangedSpy.wait(ParameterManager::kWaitForParamValueAckMs * ParameterManager::kParamSetRetryCount + 500);
    if (expectSuccess) {
        QCOMPARE(rawValueChangedSpy.count(), 1);
        QCOMPARE(paramSetSuccessSpy.count(), 1);
        QCOMPARE(paramSetFailureSpy.count(), 0);
    } else {
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
            rawValueChangedSpy.wait(1000);
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

#if 0
void ParameterManagerTest::_FTPnoFailure()
{
    Q_ASSERT(!_mockLink);
    _mockLink = MockLink::startAPMArduPlaneMockLink(false, MockConfiguration::FailParamNoReponseToRequestList);
    _mockLink->mockLinkFTP()->enableBinParamFile(true);
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QVERIFY(vehicleMgr);

    // Wait for the Vehicle to get created
    QSignalSpy spyVehicle(vehicleMgr, SIGNAL(activeVehicleAvailableChanged(bool)));
    // When param load is complete we get the param ready signal
    QSignalSpy spyParamsReady(vehicleMgr, SIGNAL(parameterReadyVehicleAvailableChanged(bool)));
    QCOMPARE(spyVehicle.wait(5000), true);
    QCOMPARE(spyVehicle.count(), 1);
    QList<QVariant> arguments = spyVehicle.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toBool(), true);
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);

    spyParamsReady.wait(5000);
    QCOMPARE(spyParamsReady.count(), 1);
    arguments = spyParamsReady.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toBool(), true);

    // Request all parameters again and check the progress bar. The initial parameterdownload
    // is so fast that I cannot connect to the loadprogress early enough.
    QSignalSpy spyProgress(vehicle->parameterManager(), SIGNAL(loadProgressChanged(float)));
    vehicle->parameterManager()->refreshAllParameters();
    spyParamsReady.wait(5000);
    QVERIFY(spyProgress.count() > 1);
    arguments = spyProgress.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QVERIFY(arguments.at(0).toFloat() > 0.0f);
    // Progress should have been set back to 0
    arguments = spyProgress.takeLast();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toFloat(), 0.0f);
}

void ParameterManagerTest::_FTPChangeParam()
{
    Q_ASSERT(!_mockLink);
    _mockLink = MockLink::startAPMArduPlaneMockLink(false, MockConfiguration::FailParamNoReponseToRequestList);
    _mockLink->mockLinkFTP()->enableBinParamFile(true);
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    QVERIFY(vehicleMgr);

    // Wait for the Vehicle to get created
    QSignalSpy spyVehicle(vehicleMgr, SIGNAL(activeVehicleAvailableChanged(bool)));
    // When param load is complete we get the param ready signal
    QSignalSpy spyParamsReady(vehicleMgr, SIGNAL(parameterReadyVehicleAvailableChanged(bool)));
    QCOMPARE(spyVehicle.wait(5000), true);
    QCOMPARE(spyVehicle.count(), 1);
    QList<QVariant> arguments = spyVehicle.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toBool(), true);
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);

    if (spyParamsReady.count() == 0)
        spyParamsReady.wait(5000);
    QCOMPARE(spyParamsReady.count(), 1);
    arguments = spyParamsReady.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toBool(), true);

    // Now try to change a parameter and check the progress
    QSignalSpy spyProgress(vehicle->parameterManager(), SIGNAL(loadProgressChanged(float)));
    Fact* fact = vehicle->parameterManager()->getParameter(MAV_COMP_ID_AUTOPILOT1, "THR_MIN");
    QVERIFY(fact);
    float value = fact->rawValue().toFloat();
    QCOMPARE(value, 0.0);
    float testvalue = 0.87f;
    QVariant sendv = testvalue;
    fact->setRawValue(sendv); // This should trigger a parameter upload to the vehicle
    /* That should set the progress to 0.5 and then back to 0 */
    spyProgress.wait(1000);
    if (spyProgress.count() < 2)
        spyProgress.wait(1000);
    QCOMPARE(spyProgress.count(), 2);
    arguments = spyProgress.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QVERIFY(arguments.at(0).toFloat() > 0.4f);

    // Progress should have been set back to 0
    Q_ASSERT(!spyProgress.empty());
    arguments = spyProgress.takeLast();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toFloat(), 0.0f);
}
#endif
