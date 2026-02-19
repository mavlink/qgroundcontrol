#include "InitialConnectTest.h"

#include <QtTest/QSignalSpy>

#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "UnitTest.h"
#include "Vehicle.h"

#include <QtTest/QTest>

void InitialConnectTest::init()
{
    VehicleTestManualConnect::init();
    LinkManager::instance()->setConnectionsAllowed();
    MAVLinkProtocol::deleteTempLogFiles();
}

void InitialConnectTest::_performTestCases_data()
{
    QTest::addColumn<int>("failureMode");
    QTest::addColumn<QString>("failureModeStr");

    static const struct TestCase_s {
        MockConfiguration::FailureMode_t failureMode;
        const char* failureModeStr;
    } rgTestCases[] = {
        {MockConfiguration::FailNone, "No failures"},
        {MockConfiguration::FailInitialConnectRequestMessageAutopilotVersionFailure,
         "REQUEST_MESSAGE:AUTOPILOT_VERSION returns failure"},
        {MockConfiguration::FailInitialConnectRequestMessageAutopilotVersionLost,
         "REQUEST_MESSAGE:AUTOPILOT_VERSION success, AUTOPILOT_VERSION never sent"},
    };

    int i = 0;
    for (const auto& testCase : rgTestCases) {
        QTest::addRow("case_%d", i)
            << static_cast<int>(testCase.failureMode)
            << QString::fromLatin1(testCase.failureModeStr);
        ++i;
    }
}

void InitialConnectTest::_performTestCases()
{
    QFETCH(int, failureMode);
    QFETCH(QString, failureModeStr);
    TEST_DEBUG(QStringLiteral("Testing case failure mode: %1").arg(failureModeStr));
    _connectMockLink(MAV_AUTOPILOT_PX4, static_cast<MockConfiguration::FailureMode_t>(failureMode));
    _disconnectMockLink();
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
    QVERIFY_SIGNAL_WAIT(activeVehicleSpy, TestTimeout::mediumMs());
    auto* vehicle = mvm->activeVehicle();
    QVERIFY(vehicle);
    QSignalSpy initialConnectCompleteSpy{vehicle, &Vehicle::initialConnectComplete};
    // Both ends of mocklink (and the initial connect state machine?) operate on
    // a different thread. The initial connection may already be complete.
    QVERIFY_TRUE_WAIT(initialConnectCompleteSpy.count() > 0 || vehicle->isInitialConnectComplete(),
                      TestTimeout::mediumMs());
    QCOMPARE(vehicle->firmwareBoardVendorId(), mockVendor);
    QCOMPARE(vehicle->firmwareBoardProductId(), mockProduct);
    LinkManager::instance()->disconnectAll();
    QSignalSpy vehicleRemovedSpy{mvm, &MultiVehicleManager::activeVehicleChanged};
    QVERIFY_SIGNAL_WAIT(vehicleRemovedSpy, TestTimeout::mediumMs());
}

UT_REGISTER_TEST(InitialConnectTest, TestLabel::Integration, TestLabel::Vehicle)
