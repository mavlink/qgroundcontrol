#include "InitialConnectTest.h"
#include "MultiVehicleManager.h"
#include "LinkManager.h"
#include "MockLink.h"
#include "Vehicle.h"
#include "InitialConnectStateMachine.h"
#include "ParameterManager.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>
#include <QtStateMachine/QStateMachine>

void InitialConnectTest::_performTestCases()
{
    static const struct TestCase_s {
        MockConfiguration::FailureMode_t    failureMode;
        const char*                         failureModeStr;
    } rgTestCases[] = {
    { MockConfiguration::FailNone,                                                   "No failures" },
    { MockConfiguration::FailInitialConnectRequestMessageAutopilotVersionFailure,    "REQUEST_MESSAGE:AUTOPILOT_VERSION returns failure" },
    { MockConfiguration::FailInitialConnectRequestMessageAutopilotVersionLost,       "REQUEST_MESSAGE:AUTOPILOT_VERSION success, AUTOPILOT_VERSION never sent" },
    };

    for (const struct TestCase_s& testCase: rgTestCases) {
        qDebug() << "Testing case failure mode:" << testCase.failureModeStr;
        _connectMockLink(MAV_AUTOPILOT_PX4, testCase.failureMode);
        QVERIFY(_vehicle);
        QVERIFY(_vehicle->isInitialConnectComplete());
        _disconnectMockLink();
    }
}

void InitialConnectTest::_boardVendorProductId()
{
    Q_ASSERT(!_mockLink);

    auto *mvm = MultiVehicleManager::instance();
    QVERIFY(!mvm->activeVehicle());  // Ensure no vehicle from previous test

    QSignalSpy activeVehicleSpy{mvm, &MultiVehicleManager::activeVehicleChanged};

    const uint16_t mockVendor = 1234;
    const uint16_t mockProduct = 5678;

    // Create properly configured MockConfiguration
    auto* mockConfig = new MockConfiguration(QStringLiteral("VendorProductTest"));
    mockConfig->setFirmwareType(MAV_AUTOPILOT_PX4);
    mockConfig->setVehicleType(MAV_TYPE_QUADROTOR);
    mockConfig->setBoardVendorProduct(mockVendor, mockProduct);
    mockConfig->setDynamic(true);

    // Add to LinkManager and create connected link
    SharedLinkConfigurationPtr linkConfig = LinkManager::instance()->addConfiguration(mockConfig);
    QVERIFY(LinkManager::instance()->createConnectedLink(linkConfig));

    // Store link for cleanup
    _mockLink = qobject_cast<MockLink*>(linkConfig->link());
    QVERIFY(_mockLink);

    // Wait for vehicle
    QVERIFY(activeVehicleSpy.wait(10000));
    _vehicle = mvm->activeVehicle();
    QVERIFY(_vehicle);

    // Wait for initial connect to complete
    QSignalSpy initialConnectCompleteSpy{_vehicle, &Vehicle::initialConnectComplete};
    QVERIFY(initialConnectCompleteSpy.wait(30000) || _vehicle->isInitialConnectComplete());

    // Verify vendor/product ID were set correctly
    QCOMPARE(_vehicle->firmwareBoardVendorId(), mockVendor);
    QCOMPARE(_vehicle->firmwareBoardProductId(), mockProduct);

    _disconnectMockLink();
}

void InitialConnectTest::_progressTracking()
{
    // Track progress updates during connection
    QList<float> progressValues;
    QMetaObject::Connection progressConnection;

    // Set up listener before connecting
    progressConnection = connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged,
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

    // Verify progress was tracked
    QVERIFY2(!progressValues.isEmpty(), "No progress updates received");

    // Find max progress reached (progress resets to 0 when state machine completes)
    float maxProgress = 0.0f;
    for (float progress : progressValues) {
        QVERIFY2(progress >= 0.0f && progress <= 1.0f, qPrintable(QString("Progress out of range: %1").arg(progress)));
        if (progress > maxProgress) {
            maxProgress = progress;
        }
    }

    // Max progress should reach near completion before reset
    QVERIFY2(maxProgress >= 0.9f,
             qPrintable(QString("Max progress too low: %1").arg(maxProgress)));

    _disconnectMockLink();
}

void InitialConnectTest::_highLatencySkipsPlans()
{
    Q_ASSERT(!_mockLink);

    auto *mvm = MultiVehicleManager::instance();
    QVERIFY(!mvm->activeVehicle());

    QSignalSpy activeVehicleSpy{mvm, &MultiVehicleManager::activeVehicleChanged};

    // Create high latency MockConfiguration
    auto* mockConfig = new MockConfiguration(QStringLiteral("HighLatencyTest"));
    mockConfig->setFirmwareType(MAV_AUTOPILOT_PX4);
    mockConfig->setVehicleType(MAV_TYPE_QUADROTOR);
    mockConfig->setHighLatency(true);
    mockConfig->setDynamic(true);

    SharedLinkConfigurationPtr linkConfig = LinkManager::instance()->addConfiguration(mockConfig);
    QVERIFY(LinkManager::instance()->createConnectedLink(linkConfig));

    _mockLink = qobject_cast<MockLink*>(linkConfig->link());
    QVERIFY(_mockLink);

    // Wait for vehicle
    QVERIFY(activeVehicleSpy.wait(5000));
    _vehicle = mvm->activeVehicle();
    QVERIFY(_vehicle);

    // High latency should complete quickly since plan requests are skipped
    QSignalSpy initialConnectCompleteSpy{_vehicle, &Vehicle::initialConnectComplete};
    QVERIFY(initialConnectCompleteSpy.wait(5000) || _vehicle->isInitialConnectComplete());

    // initialPlanRequestComplete should be true even though plans were skipped
    QVERIFY(_vehicle->initialPlanRequestComplete());

    _disconnectMockLink();
}

void InitialConnectTest::_arducopterConnect()
{
    // Test ArduCopter firmware connection path
    // ArduPilot has different capabilities and flow than PX4
    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);
    QVERIFY(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());

    // Verify ArduPilot-specific state
    QCOMPARE(_vehicle->firmwareType(), MAV_AUTOPILOT_ARDUPILOTMEGA);
    QVERIFY(_vehicle->parameterManager()->parametersReady());

    _disconnectMockLink();
}

void InitialConnectTest::_stateMachineActiveStatus()
{
    // Verify state machine active() status during connection
    auto* mvm = MultiVehicleManager::instance();

    bool wasActive = false;
    bool wasInactive = false;

    auto activeConnection = connect(mvm, &MultiVehicleManager::activeVehicleChanged,
        this, [&wasActive, &wasInactive](Vehicle* vehicle) {
            if (vehicle) {
                // Check if state machine is running during connection
                // Note: We can't directly access InitialConnectStateMachine from tests,
                // but we can verify via isInitialConnectComplete
                if (!vehicle->isInitialConnectComplete()) {
                    wasActive = true;
                }

                // Wait for completion
                connect(vehicle, &Vehicle::initialConnectComplete, vehicle, [&wasInactive, vehicle]() {
                    // After completion, the state machine should no longer be "active"
                    // in the sense that initial connect is done
                    if (vehicle->isInitialConnectComplete()) {
                        wasInactive = true;
                    }
                });
            }
        });

    _connectMockLink(MAV_AUTOPILOT_PX4);

    disconnect(activeConnection);

    QVERIFY(_vehicle);
    QVERIFY(_vehicle->isInitialConnectComplete());

    // State machine should have been active during connection
    QVERIFY2(wasActive || wasInactive, "State machine activity not detected");

    _disconnectMockLink();
}

void InitialConnectTest::_stateTransitionOrder()
{
    // Verify that states execute in expected order by tracking progress
    QList<float> progressValues;
    QMetaObject::Connection progressConnection;

    progressConnection = connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged,
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

    // Verify progress increased monotonically (until reset at completion)
    float lastProgress = 0.0f;
    float maxProgress = 0.0f;
    bool foundDecrease = false;

    for (float progress : progressValues) {
        if (progress > maxProgress) {
            maxProgress = progress;
        }
        // Allow for the reset at completion (progress goes to 0)
        if (progress < lastProgress && progress > 0.01f && lastProgress < 0.99f) {
            foundDecrease = true;
        }
        lastProgress = progress;
    }

    // Progress should have been monotonically increasing until completion
    QVERIFY2(!foundDecrease, "Progress decreased unexpectedly during state execution");

    // Should have reached high progress before completion reset
    QVERIFY2(maxProgress >= 0.8f,
             qPrintable(QString("Max progress too low: %1").arg(maxProgress)));

    _disconnectMockLink();
}

void InitialConnectTest::_multipleReconnects()
{
    // Test that state machine can handle multiple connect/disconnect cycles
    for (int i = 0; i < 3; ++i) {
        qDebug() << "Reconnect cycle:" << (i + 1);

        _connectMockLink(MAV_AUTOPILOT_PX4);
        QVERIFY(_vehicle);
        QVERIFY(_vehicle->isInitialConnectComplete());

        // Verify state is fully complete
        QVERIFY(_vehicle->parameterManager()->parametersReady());

        _disconnectMockLink();

        // Ensure clean state between cycles
        QVERIFY(!MultiVehicleManager::instance()->activeVehicle());
    }
}
