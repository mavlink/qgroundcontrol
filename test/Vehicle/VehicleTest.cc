#include "VehicleTest.h"
#include "MultiVehicleManager.h"
#include "LinkManager.h"
#include "MockLink.h"
#include "Vehicle.h"
#include "InitialConnectStateMachine.h"
#include "ParameterManager.h"
#include "MissionManager.h"
#include "GeoFenceManager.h"
#include "RallyPointManager.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void VehicleTest::_testVehicleCreation()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);

    QVERIFY(_vehicle);
    QVERIFY(_vehicle->id() > 0);
    QCOMPARE(_vehicle->firmwareType(), MAV_AUTOPILOT_PX4);

    _disconnectMockLink();
}

void VehicleTest::_testVehicleInitialState()
{
    auto* mvm = MultiVehicleManager::instance();
    bool checkedInitialState = false;

    auto connection = connect(mvm, &MultiVehicleManager::activeVehicleChanged,
        this, [&checkedInitialState](Vehicle* vehicle) {
            if (vehicle && !vehicle->isInitialConnectComplete()) {
                checkedInitialState = true;
                Q_UNUSED(vehicle->parameterManager()->parametersReady());
            }
        });

    _connectMockLink(MAV_AUTOPILOT_PX4);

    disconnect(connection);

    QVERIFY(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());
    QVERIFY(_vehicle->parameterManager());
    QVERIFY(_vehicle->missionManager());
    QVERIFY(_vehicle->geoFenceManager());
    QVERIFY(_vehicle->rallyPointManager());

    _disconnectMockLink();
}

void VehicleTest::_testInitialConnectStateMachineIntegration()
{
    QList<float> progressValues;

    auto connection = connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged,
        this, [&progressValues](Vehicle* vehicle) {
            if (vehicle) {
                connect(vehicle, &Vehicle::loadProgressChanged, vehicle, [&progressValues](float progress) {
                    progressValues.append(progress);
                });
            }
        });

    _connectMockLink(MAV_AUTOPILOT_PX4);

    disconnect(connection);

    QVERIFY(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());
    QVERIFY2(!progressValues.isEmpty(), "No progress updates received");

    for (float progress : progressValues) {
        QVERIFY2(progress >= 0.0f && progress <= 1.0f,
                 qPrintable(QString("Invalid progress: %1").arg(progress)));
    }

    _disconnectMockLink();
}

void VehicleTest::_testStateMachineCompletion()
{
    bool completionReceived = false;
    QSignalSpy* completionSpy = nullptr;

    auto connection = connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged,
        this, [&completionReceived, &completionSpy](Vehicle* vehicle) {
            if (vehicle) {
                completionSpy = new QSignalSpy(vehicle, &Vehicle::initialConnectComplete);
                connect(vehicle, &Vehicle::initialConnectComplete, vehicle, [&completionReceived]() {
                    completionReceived = true;
                });
            }
        });

    _connectMockLink(MAV_AUTOPILOT_PX4);
    disconnect(connection);

    QVERIFY(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());
    QVERIFY(completionReceived || _vehicle->isInitialConnectComplete());

    delete completionSpy;
    _disconnectMockLink();
}

void VehicleTest::_testStateMachineProgressSignals()
{
    QList<float> progressValues;
    float maxProgress = 0.0f;

    auto connection = connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged,
        this, [&progressValues, &maxProgress](Vehicle* vehicle) {
            if (vehicle) {
                connect(vehicle, &Vehicle::loadProgressChanged, vehicle, [&progressValues, &maxProgress](float progress) {
                    progressValues.append(progress);
                    if (progress > maxProgress) {
                        maxProgress = progress;
                    }
                });
            }
        });

    _connectMockLink(MAV_AUTOPILOT_PX4);
    disconnect(connection);

    QVERIFY(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());
    QVERIFY2(progressValues.size() >= 3, "Expected at least 3 progress updates");
    QVERIFY2(maxProgress >= 0.8f, qPrintable(QString("Max progress too low: %1").arg(maxProgress)));

    _disconnectMockLink();
}

void VehicleTest::_testPX4Vehicle()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);

    QVERIFY(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());
    QCOMPARE(_vehicle->firmwareType(), MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle->parameterManager()->parametersReady());
    QVERIFY(_vehicle->missionManager());

    _disconnectMockLink();
}

void VehicleTest::_testArduPilotVehicle()
{
    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);

    QVERIFY(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());
    QCOMPARE(_vehicle->firmwareType(), MAV_AUTOPILOT_ARDUPILOTMEGA);
    QVERIFY(_vehicle->parameterManager()->parametersReady());
    QVERIFY(_vehicle->missionManager());

    _disconnectMockLink();
}

void VehicleTest::_testConnectionTimeout()
{
    static const MockConfiguration::FailureMode_t failureModes[] = {
        MockConfiguration::FailInitialConnectRequestMessageAutopilotVersionFailure,
        MockConfiguration::FailInitialConnectRequestMessageAutopilotVersionLost,
    };

    for (auto failureMode : failureModes) {
        qDebug() << "Testing failure mode:" << static_cast<int>(failureMode);
        _connectMockLink(MAV_AUTOPILOT_PX4, failureMode);
        QVERIFY(_vehicle);
        QVERIFY(_vehicle->isInitialConnectComplete());
        _disconnectMockLink();
    }
}

void VehicleTest::_testReconnectAfterDisconnect()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());

    int firstVehicleId = _vehicle->id();
    _disconnectMockLink();

    QVERIFY(!MultiVehicleManager::instance()->activeVehicle());

    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());
    QVERIFY(_vehicle->id() != firstVehicleId);
    QVERIFY(_vehicle->parameterManager()->parametersReady());

    _disconnectMockLink();
}

void VehicleTest::_testMultipleVehicleStateMachines()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());

    int id1 = _vehicle->id();
    QCOMPARE(_vehicle->firmwareType(), MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle->parameterManager()->parametersReady());

    _disconnectMockLink();

    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);
    QVERIFY(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());

    int id2 = _vehicle->id();

    // Verify this is a different vehicle
    QVERIFY(id1 != id2);
    QCOMPARE(_vehicle->firmwareType(), MAV_AUTOPILOT_ARDUPILOTMEGA);
    QVERIFY(_vehicle->parameterManager()->parametersReady());

    _disconnectMockLink();

    // Each vehicle state machine worked independently
}
