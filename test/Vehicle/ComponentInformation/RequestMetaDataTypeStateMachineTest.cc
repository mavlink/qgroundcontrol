#include "RequestMetaDataTypeStateMachineTest.h"
#include "RequestMetaDataTypeStateMachine.h"
#include "ComponentInformationManager.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MockLink.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void RequestMetaDataTypeStateMachineTest::_testStateCreation()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());
    _disconnectMockLink();
}

void RequestMetaDataTypeStateMachineTest::_testRequestFlow()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());
    _disconnectMockLink();
}

void RequestMetaDataTypeStateMachineTest::_testSkipDeprecatedWhenSupported()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());
    _disconnectMockLink();
}

void RequestMetaDataTypeStateMachineTest::_testArduPilotMetadata()
{
    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);
    QVERIFY(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());
    QCOMPARE(_vehicle->firmwareType(), MAV_AUTOPILOT_ARDUPILOTMEGA);
    _disconnectMockLink();
}

void RequestMetaDataTypeStateMachineTest::_testStateMachineCompletion()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());
    _disconnectMockLink();

    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);
    QVERIFY(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());
    _disconnectMockLink();
}
