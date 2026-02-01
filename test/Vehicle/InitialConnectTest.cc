#include "InitialConnectTest.h"

#include <QtTest/QSignalSpy>

#include "LinkManager.h"
#include "MultiVehicleManager.h"
#include "UnitTest.h"
#include "Vehicle.h"

void InitialConnectTest::_performTestCases()
{
    static const struct TestCase_s
    {
        MockConfiguration::FailureMode_t failureMode;
        const char* failureModeStr;
    } rgTestCases[] = {
        {MockConfiguration::FailNone, "No failures"},
        {MockConfiguration::FailInitialConnectRequestMessageAutopilotVersionFailure,
         "REQUEST_MESSAGE:AUTOPILOT_VERSION returns failure"},
        {MockConfiguration::FailInitialConnectRequestMessageAutopilotVersionLost,
         "REQUEST_MESSAGE:AUTOPILOT_VERSION success, AUTOPILOT_VERSION never sent"},
    };

    for (const struct TestCase_s& testCase : rgTestCases) {
        qDebug() << "Testing case failure mode:" << testCase.failureModeStr;
        _connectMockLink(MAV_AUTOPILOT_PX4, testCase.failureMode);
        _disconnectMockLink();
    }
}

void InitialConnectTest::_boardVendorProductId()
{
    auto* mvm = MultiVehicleManager::instance();
    QSignalSpy activeVehicleSpy{mvm, &MultiVehicleManager::activeVehicleChanged};
    auto mockConfig = std::make_shared<MockConfiguration>(QString{"MockLink"});
    const uint16_t mockVendor = 1234;
    const uint16_t mockProduct = 5678;
    mockConfig->setBoardVendorProduct(mockVendor, mockProduct);
    SharedLinkConfigurationPtr linkConfig = mockConfig;
    LinkManager::instance()->createConnectedLink(linkConfig);
    QVERIFY(activeVehicleSpy.wait());
    auto* vehicle = mvm->activeVehicle();
    QSignalSpy initialConnectCompleteSpy{vehicle, &Vehicle::initialConnectComplete};
    // Both ends of mocklink (and the initial connect state machine?) operate on
    // a different thread. The initial connection may already be complete.
    QVERIFY(initialConnectCompleteSpy.wait() || vehicle->isInitialConnectComplete());
    QCOMPARE(vehicle->firmwareBoardVendorId(), mockVendor);
    QCOMPARE(vehicle->firmwareBoardProductId(), mockProduct);
    LinkManager::instance()->disconnectAll();
}

UT_REGISTER_TEST(InitialConnectTest, TestLabel::Integration, TestLabel::Vehicle)
