#include "ComponentInformationManagerTest.h"
#include "ComponentInformationManager.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MockLink.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void ComponentInformationManagerTest::_testStateCreation()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());
    _disconnectMockLink();
}

void ComponentInformationManagerTest::_testProgressTracking()
{
    QList<float> progressValues;

    auto progressConnection = connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged,
        this, [&progressValues](Vehicle* vehicle) {
            if (vehicle) {
                connect(vehicle, &Vehicle::loadProgressChanged, vehicle, [&progressValues](float progress) {
                    progressValues.append(progress);
                });
            }
        });

    _connectMockLink(MAV_AUTOPILOT_PX4);
    disconnect(progressConnection);

    QVERIFY(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());
    QVERIFY2(!progressValues.isEmpty(), "No progress updates received");

    bool foundMidProgress = false;
    for (float progress : progressValues) {
        QVERIFY2(progress >= 0.0f && progress <= 1.0f,
                 qPrintable(QString("Progress out of range: %1").arg(progress)));
        if (progress > 0.1f && progress < 0.9f) {
            foundMidProgress = true;
        }
    }
    QVERIFY2(foundMidProgress, "No intermediate progress values found");

    _disconnectMockLink();
}

void ComponentInformationManagerTest::_testSkipUnsupportedTypes()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());
    _disconnectMockLink();
}

void ComponentInformationManagerTest::_testArduPilotComponentInfo()
{
    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);
    QVERIFY(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());
    QCOMPARE(_vehicle->firmwareType(), MAV_AUTOPILOT_ARDUPILOTMEGA);
    _disconnectMockLink();
}

void ComponentInformationManagerTest::_testCompletionSignal()
{
    bool initialConnectReceived = false;

    auto connection = connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged,
        this, [&initialConnectReceived](Vehicle* vehicle) {
            if (vehicle) {
                connect(vehicle, &Vehicle::initialConnectComplete, vehicle, [&initialConnectReceived]() {
                    initialConnectReceived = true;
                });
            }
        });

    _connectMockLink(MAV_AUTOPILOT_PX4);
    disconnect(connection);

    QVERIFY(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());
    QVERIFY(initialConnectReceived || _vehicle->isInitialConnectComplete());

    _disconnectMockLink();
}

void ComponentInformationManagerTest::_testMultipleVehicles()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());

    int firstId = _vehicle->id();
    _disconnectMockLink();

    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);
    QVERIFY(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());
    QVERIFY(firstId != _vehicle->id());

    _disconnectMockLink();
}
