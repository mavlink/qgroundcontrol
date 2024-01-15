/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "InitialConnectTest.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "LinkManager.h"
#include "MockLink.h"

void InitialConnectTest::_performTestCases(void)
{
    static const struct TestCase_s {
        MockConfiguration::FailureMode_t    failureMode;
        const char*                         failureModeStr;
    } rgTestCases[] = {
    { MockConfiguration::FailNone,                                                   "No failures" },
    { MockConfiguration::FailInitialConnectRequestMessageAutopilotVersionFailure,    "REQUEST_MESSAGE:AUTOPILOT_VERSION returns failure" },
    { MockConfiguration::FailInitialConnectRequestMessageAutopilotVersionLost,       "REQUEST_MESSAGE:AUTOPILOT_VERSION success, AUTOPILOT_VERSION never sent" },
    { MockConfiguration::FailInitialConnectRequestMessageProtocolVersionFailure,     "REQUEST_MESSAGE:PROTOCOL_VERSION returns failure" },
    { MockConfiguration::FailInitialConnectRequestMessageProtocolVersionLost,        "REQUEST_MESSAGE:PROTOCOL_VERSION success, PROTOCOL_VERSION never sent" },
    };

    for (const struct TestCase_s& testCase: rgTestCases) {
        qDebug() << "Testing case failure mode:" << testCase.failureModeStr;
        _connectMockLink(MAV_AUTOPILOT_PX4, testCase.failureMode);
        _disconnectMockLink();
    }
}

void InitialConnectTest::_boardVendorProductId(void)
{
    auto *mvm = qgcApp()->toolbox()->multiVehicleManager();
    QSignalSpy activeVehicleSpy{mvm, &MultiVehicleManager::activeVehicleChanged};

    auto mockConfig = std::make_shared<MockConfiguration>(QString{"MockLink"});
    const uint16_t mockVendor = 1234;
    const uint16_t mockProduct = 5678;
    mockConfig->setBoardVendorProduct(mockVendor, mockProduct);

    SharedLinkConfigurationPtr linkConfig = mockConfig;
    _linkManager->createConnectedLink(linkConfig);

    QVERIFY(activeVehicleSpy.wait());
    auto *vehicle = mvm->activeVehicle();
    QSignalSpy initialConnectCompleteSpy{vehicle, &Vehicle::initialConnectComplete};

    // Both ends of mocklink (and the initial connect state machine?) operate on
    // a different thread. The initial connection may already be complete.
    QVERIFY(initialConnectCompleteSpy.wait() || vehicle->isInitialConnectComplete());

    QCOMPARE(vehicle->firmwareBoardVendorId(), mockVendor);
    QCOMPARE(vehicle->firmwareBoardProductId(), mockProduct);

    _linkManager->disconnectAll();
}
