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
#include "QGCApplication.h"
#include "ParameterManager.h"

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

/// Test failure modes which should still lead to param load success
void ParameterManagerTest::_noFailureWorker(MockConfiguration::FailureMode_t failureMode)
{
    MultiVehicleManager* vehicleMgr = qgcApp()->toolbox()->multiVehicleManager();
    QVERIFY(vehicleMgr);

    QSignalSpy spyVehicle(vehicleMgr, SIGNAL(activeVehicleAvailableChanged(bool)));
    QSignalSpy spyParamsReady(vehicleMgr, SIGNAL(parameterReadyVehicleAvailableChanged(bool)));

    Q_ASSERT(!_mockLink);
    _mockLink = MockLink::startMockLink(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR, false /* sendStatusText */, false /* isSecureConnection */, QString() /* signingKey */, failureMode);
    QVERIFY(_mockLink);

    // Wait for the Vehicle to get created
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
    MultiVehicleManager* vehicleMgr = qgcApp()->toolbox()->multiVehicleManager();
    QVERIFY(vehicleMgr);

    QSignalSpy spyVehicle(vehicleMgr, SIGNAL(activeVehicleAvailableChanged(bool)));
    QSignalSpy spyParamsReady(vehicleMgr, SIGNAL(parameterReadyVehicleAvailableChanged(bool)));

    // Will pop error about request failure
    setExpectedMessageBox(QMessageBox::Ok);

    Q_ASSERT(!_mockLink);
    _mockLink = MockLink::startMockLink(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR, false /* sendStatusText */, false /* isSecureConnection */, QString() /* signingKey */, MockConfiguration::FailParamNoReponseToRequestList);
    QVERIFY(_mockLink);

    // Wait for the Vehicle to get created
    QCOMPARE(spyVehicle.wait(5000), true);
    QCOMPARE(spyVehicle.count(), 1);
    QList<QVariant> arguments = spyVehicle.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toBool(), true);

    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);

    QSignalSpy spyProgress(vehicle->parameterManager(), SIGNAL(loadProgressChanged(float)));

    // We should still get parameterReadyVehicleAvailableChanged but it should be false to signal not loaded
    QCOMPARE(spyParamsReady.wait(40000), true);
    QCOMPARE(spyParamsReady.count(), 1);
    QCOMPARE(spyProgress.count(), 0);
    arguments = spyParamsReady.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toBool(), false);

    // User should have been notified
    checkExpectedMessageBox();
}

// MockLink will fail to send a param on initial request, it will also fail to send it on subsequent
// param_read requests.
void ParameterManagerTest::_requestListMissingParamFail(void)
{
    // Will pop error about missing params
    setExpectedMessageBox(QMessageBox::Ok);

    MultiVehicleManager* vehicleMgr = qgcApp()->toolbox()->multiVehicleManager();
    QVERIFY(vehicleMgr);

    QSignalSpy spyVehicle(vehicleMgr, SIGNAL(activeVehicleAvailableChanged(bool)));
    QSignalSpy spyParamsReady(vehicleMgr, SIGNAL(parameterReadyVehicleAvailableChanged(bool)));

    Q_ASSERT(!_mockLink);
    _mockLink = MockLink::startMockLink(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR, false /* sendStatusText */, false /* isSecureConnection */, QString() /* signingKey */, MockConfiguration::FailMissingParamOnAllRequests);
    QVERIFY(_mockLink);

    // Wait for the Vehicle to get created
    QCOMPARE(spyVehicle.wait(5000), true);
    QCOMPARE(spyVehicle.count(), 1);
    QList<QVariant> arguments = spyVehicle.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toBool(), true);

    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);

    QSignalSpy spyProgress(vehicle->parameterManager(), SIGNAL(loadProgressChanged(float)));

    // We will get progress bar updates, since it will fail after getting partially through the request
    QCOMPARE(spyProgress.wait(2000), true);
    arguments = spyProgress.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QVERIFY(arguments.at(0).toFloat() > 0.0f);

    // We should get a parameters ready signal, but Vehicle should indicate missing params
    if (!spyParamsReady.count()) {
        QCOMPARE(spyParamsReady.wait(40000), true);
    }
    arguments = spyParamsReady.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toBool(), false);
    QCOMPARE(vehicle->parameterManager()->missingParameters(), true);

    // User should have been notified
    checkExpectedMessageBox();
}

void ParameterManagerTest::_FTPnoFailure()
{
    MultiVehicleManager* vehicleMgr = qgcApp()->toolbox()->multiVehicleManager();
    QVERIFY(vehicleMgr);

    QSignalSpy spyParamsReady(vehicleMgr, SIGNAL(parameterReadyVehicleAvailableChanged(bool)));

    Q_ASSERT(!_mockLink);
    _mockLink = MockLink::startMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA, MAV_TYPE_FIXED_WING, false /* sendStatusText */, false /* isSecureConnection */, QString() /* signingKey */, MockConfiguration::FailParamNoReponseToRequestList);
    QVERIFY(_mockLink);
    _mockLink->mockLinkFTP()->enableBinParamFile(true);

    // When param load is complete we get the param ready signal
    QCOMPARE(spyParamsReady.wait(5000), true);
    QCOMPARE(spyParamsReady.count(), 1);
    auto arguments = spyParamsReady.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toBool(), false);

    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);

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
    MultiVehicleManager* vehicleMgr = qgcApp()->toolbox()->multiVehicleManager();
    QVERIFY(vehicleMgr);

    QSignalSpy spyVehicle(vehicleMgr, SIGNAL(activeVehicleAvailableChanged(bool)));
    QSignalSpy spyParamsReady(vehicleMgr, SIGNAL(parameterReadyVehicleAvailableChanged(bool)));

    Q_ASSERT(!_mockLink);
    _mockLink = MockLink::startMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA, MAV_TYPE_FIXED_WING, false /* sendStatusText */, false /* isSecureConnection */, QString() /* signingKey */, MockConfiguration::FailParamNoReponseToRequestList);
    QVERIFY(_mockLink);
    _mockLink->mockLinkFTP()->enableBinParamFile(true);

    // When param load is complete we get the param ready signal
    QCOMPARE(spyParamsReady.wait(5000), true);
    QCOMPARE(spyParamsReady.count(), 1);
    auto arguments = spyParamsReady.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toBool(), true);

    // Now try to change a parameter and check the progress
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    QVERIFY(vehicle);
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
